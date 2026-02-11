#include "regimeflow/live/ib_adapter.h"

#include "regimeflow/common/types.h"

#include "TagValue.h"
#include "Decimal.h"
#include "Execution.h"
#include "OrderCancel.h"

#include <chrono>
#include <thread>
#include <cctype>

namespace regimeflow::live {

namespace {

engine::OrderSide map_side(const std::string& action) {
    if (action == "SELL") {
        return engine::OrderSide::Sell;
    }
    return engine::OrderSide::Buy;
}

std::string to_upper(std::string value) {
    for (auto& c : value) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return value;
}

LiveOrderStatus map_status(std::string value) {
    value = to_upper(std::move(value));
    if (value == "FILLED") {
        return LiveOrderStatus::Filled;
    }
    if (value == "PARTIALLYFILLED" || value == "PARTIALFILL") {
        return LiveOrderStatus::PartiallyFilled;
    }
    if (value == "CANCELLED" || value == "CANCELED") {
        return LiveOrderStatus::Cancelled;
    }
    if (value == "REJECTED") {
        return LiveOrderStatus::Rejected;
    }
    if (value == "SUBMITTED" || value == "PRESUBMITTED") {
        return LiveOrderStatus::New;
    }
    return LiveOrderStatus::New;
}

}  // namespace

IBAdapter::IBAdapter(Config config) : config_(std::move(config)) {
    reader_signal_ = std::make_unique<EReaderOSSignal>();
    client_ = std::make_unique<EClientSocket>(this, reader_signal_.get());
}

Result<void> IBAdapter::connect() {
    if (!client_) {
        return Error(Error::Code::BrokerError, "IB client not initialized");
    }
    if (client_->isConnected()) {
        connected_ = true;
        return Ok();
    }
    if (!client_->eConnect(config_.host.c_str(), config_.port, config_.client_id)) {
        return Error(Error::Code::BrokerError, "Failed to connect to IB TWS/Gateway");
    }
    connected_ = true;

    reader_ = std::make_unique<EReader>(client_.get(), reader_signal_.get());
    reader_->start();
    reader_thread_ = std::thread([this] {
        while (connected_) {
            reader_signal_->waitForSignal();
            if (!connected_) {
                break;
            }
            reader_->processMsgs();
        }
    });

    client_->reqAccountSummary(9001, "All", "NetLiquidation,TotalCashValue,BuyingPower");
    client_->reqPositions();
    return Ok();
}

Result<void> IBAdapter::disconnect() {
    connected_ = false;
    if (client_ && client_->isConnected()) {
        client_->eDisconnect();
    }
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
    return Ok();
}

bool IBAdapter::is_connected() const {
    return connected_.load();
}

void IBAdapter::subscribe_market_data(const std::vector<std::string>& symbols) {
    if (!client_ || !client_->isConnected()) {
        return;
    }
    for (const auto& symbol : symbols) {
        auto contract = build_stock_contract(symbol);
        auto ticker_id = static_cast<TickerId>(SymbolRegistry::instance().intern(symbol));
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            ticker_to_symbol_[ticker_id] = SymbolRegistry::instance().intern(symbol);
        }
        client_->reqMktData(ticker_id, contract, "", false, false, TagValueListSPtr());
    }
}

void IBAdapter::unsubscribe_market_data(const std::vector<std::string>& symbols) {
    if (!client_ || !client_->isConnected()) {
        return;
    }
    for (const auto& symbol : symbols) {
        auto ticker_id = static_cast<TickerId>(SymbolRegistry::instance().intern(symbol));
        client_->cancelMktData(ticker_id);
        std::lock_guard<std::mutex> lock(state_mutex_);
        ticker_to_symbol_.erase(ticker_id);
    }
}

Result<std::string> IBAdapter::submit_order(const engine::Order& order) {
    if (!client_ || !client_->isConnected()) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    int64_t order_id = -1;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (next_order_id_ < 0) {
            return Error(Error::Code::BrokerError, "Order ID not initialized");
        }
        order_id = next_order_id_++;
    }
    auto contract = build_stock_contract(SymbolRegistry::instance().lookup(order.symbol));
    auto ib_order = build_order(order, order_id);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        order_contracts_[order_id] = contract;
    }
    client_->placeOrder(static_cast<OrderId>(order_id), contract, ib_order);
    return std::to_string(order_id);
}

Result<void> IBAdapter::cancel_order(const std::string& broker_order_id) {
    if (!client_ || !client_->isConnected()) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    OrderCancel order_cancel;
    client_->cancelOrder(static_cast<OrderId>(std::stoll(broker_order_id)), order_cancel);
    return Ok();
}

Result<void> IBAdapter::modify_order(const std::string& broker_order_id,
                                     const engine::OrderModification& mod) {
    if (!client_ || !client_->isConnected()) {
        return Error(Error::Code::BrokerError, "Not connected");
    }
    int64_t order_id = std::stoll(broker_order_id);
    engine::Order order;
    order.id = order_id;
    if (mod.quantity) order.quantity = *mod.quantity;
    if (mod.limit_price) order.limit_price = *mod.limit_price;
    if (mod.stop_price) order.stop_price = *mod.stop_price;
    if (mod.tif) order.tif = *mod.tif;
    ::Contract contract;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = order_contracts_.find(order_id);
        if (it != order_contracts_.end()) {
            contract = it->second;
        }
    }
    if (contract.symbol.empty()) {
        return Error(Error::Code::InvalidState, "Order contract not found for modification");
    }
    auto ib_order = build_order(order, order_id);
    client_->placeOrder(static_cast<OrderId>(order_id), contract, ib_order);
    return Ok();
}

AccountInfo IBAdapter::get_account_info() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return account_info_;
}

std::vector<Position> IBAdapter::get_positions() {
    std::vector<Position> out;
    std::lock_guard<std::mutex> lock(state_mutex_);
    out.reserve(positions_.size());
    for (const auto& [_, pos] : positions_) {
        out.push_back(pos);
    }
    return out;
}

std::vector<ExecutionReport> IBAdapter::get_open_orders() {
    std::vector<ExecutionReport> out;
    std::lock_guard<std::mutex> lock(state_mutex_);
    out.reserve(open_orders_.size());
    for (const auto& [_, report] : open_orders_) {
        out.push_back(report);
    }
    return out;
}

void IBAdapter::on_market_data(std::function<void(const MarketDataUpdate&)> cb) {
    market_cb_ = std::move(cb);
}

void IBAdapter::on_execution_report(std::function<void(const ExecutionReport&)> cb) {
    exec_cb_ = std::move(cb);
}

void IBAdapter::on_position_update(std::function<void(const Position&)> cb) {
    position_cb_ = std::move(cb);
}

int IBAdapter::max_orders_per_second() const {
    return 50;
}

int IBAdapter::max_messages_per_second() const {
    return 100;
}

void IBAdapter::poll() {}

::Contract IBAdapter::build_stock_contract(const std::string& symbol) const {
    Contract contract;
    contract.symbol = symbol;
    contract.secType = "STK";
    contract.currency = "USD";
    contract.exchange = "SMART";
    return contract;
}

::Order IBAdapter::build_order(const engine::Order& order, int64_t order_id) const {
    ::Order ib_order;
    ib_order.orderId = static_cast<OrderId>(order_id);
    ib_order.totalQuantity = order.quantity;
    ib_order.action = order.side == engine::OrderSide::Sell ? "SELL" : "BUY";
    switch (order.type) {
        case engine::OrderType::Limit:
            ib_order.orderType = "LMT";
            ib_order.lmtPrice = order.limit_price;
            break;
        case engine::OrderType::Stop:
            ib_order.orderType = "STP";
            ib_order.auxPrice = order.stop_price;
            break;
        case engine::OrderType::StopLimit:
            ib_order.orderType = "STP LMT";
            ib_order.auxPrice = order.stop_price;
            ib_order.lmtPrice = order.limit_price;
            break;
        default:
            ib_order.orderType = "MKT";
            break;
    }
    return ib_order;
}

void IBAdapter::nextValidId(OrderId orderId) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    next_order_id_ = orderId;
}

void IBAdapter::tickPrice(TickerId tickerId, TickType field, double price,
                          const TickAttrib&) {
    if (!market_cb_) {
        return;
    }
    if (field != TickType::LAST && field != TickType::DELAYED_LAST &&
        field != TickType::BID && field != TickType::ASK) {
        return;
    }
    SymbolId symbol = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = ticker_to_symbol_.find(tickerId);
        if (it == ticker_to_symbol_.end()) {
            return;
        }
        symbol = it->second;
        last_prices_[symbol] = price;
    }
    data::Tick tick;
    tick.symbol = symbol;
    tick.timestamp = Timestamp::now();
    tick.price = price;
    tick.quantity = 0.0;
    MarketDataUpdate update;
    update.data = tick;
    market_cb_(update);
}

void IBAdapter::tickSize(TickerId tickerId, TickType field, Decimal size) {
    if (!market_cb_) {
        return;
    }
    if (field != TickType::LAST_SIZE && field != TickType::DELAYED_LAST_SIZE) {
        return;
    }
    SymbolId symbol = 0;
    double last_price = 0.0;
    double size_value = DecimalFunctions::decimalToDouble(size);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto it = ticker_to_symbol_.find(tickerId);
        if (it == ticker_to_symbol_.end()) {
            return;
        }
        symbol = it->second;
        last_sizes_[symbol] = size_value;
        auto price_it = last_prices_.find(symbol);
        if (price_it != last_prices_.end()) {
            last_price = price_it->second;
        }
    }
    data::Tick tick;
    tick.symbol = symbol;
    tick.timestamp = Timestamp::now();
    tick.price = last_price;
    tick.quantity = size_value;
    MarketDataUpdate update;
    update.data = tick;
    market_cb_(update);
}

void IBAdapter::orderStatus(OrderId orderId, const std::string& status, Decimal filled,
                            Decimal remaining, double avgFillPrice, long long, int, double,
                            int, const std::string&, double) {
    ExecutionReport report;
    report.broker_order_id = std::to_string(orderId);
    report.quantity = DecimalFunctions::decimalToDouble(filled);
    report.price = avgFillPrice;
    report.status = status == "Filled" ? LiveOrderStatus::Filled
                                       : (status == "PartiallyFilled"
                                              ? LiveOrderStatus::PartiallyFilled
                                              : LiveOrderStatus::New);
    report.timestamp = Timestamp::now();
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        open_orders_[orderId] = report;
    }
    if (exec_cb_) {
        exec_cb_(report);
    }
    (void)remaining;
}

void IBAdapter::openOrder(OrderId orderId, const ::Contract& contract, const ::Order& order,
                          const ::OrderState& orderState) {
    ExecutionReport report;
    report.broker_order_id = std::to_string(orderId);
    report.symbol = contract.symbol;
    report.side = map_side(to_upper(order.action));
    report.quantity = order.totalQuantity;
    report.price = order.lmtPrice > 0 ? order.lmtPrice : order.auxPrice;
    report.status = map_status(orderState.status);
    report.timestamp = Timestamp::now();
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        open_orders_[orderId] = report;
    }
    if (exec_cb_) {
        exec_cb_(report);
    }
}

void IBAdapter::openOrderEnd() {}

void IBAdapter::execDetails(int, const ::Contract& contract, const ::Execution& execution) {
    ExecutionReport report;
    report.broker_order_id = std::to_string(execution.orderId);
    report.broker_exec_id = execution.execId;
    report.symbol = contract.symbol;
    report.side = map_side(to_upper(execution.side));
    report.quantity = execution.shares;
    report.price = execution.price;
    report.status = LiveOrderStatus::Filled;
    report.timestamp = Timestamp::now();
    if (exec_cb_) {
        exec_cb_(report);
    }
}

void IBAdapter::position(const std::string&, const ::Contract& contract,
                         Decimal position, double avgCost) {
    Position pos;
    pos.symbol = contract.symbol;
    pos.quantity = DecimalFunctions::decimalToDouble(position);
    pos.average_price = avgCost;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        positions_[SymbolRegistry::instance().intern(contract.symbol)] = pos;
    }
    if (position_cb_) {
        position_cb_(pos);
    }
}

void IBAdapter::positionEnd() {}

void IBAdapter::accountSummary(int, const std::string&, const std::string& tag,
                               const std::string& value, const std::string&) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (tag == "NetLiquidation") {
        account_info_.equity = std::stod(value);
    } else if (tag == "TotalCashValue") {
        account_info_.cash = std::stod(value);
    } else if (tag == "BuyingPower") {
        account_info_.buying_power = std::stod(value);
    }
}

void IBAdapter::accountSummaryEnd(int) {}

void IBAdapter::error(int, time_t, int, const std::string& errorString,
                      const std::string&) {
    if (exec_cb_) {
        ExecutionReport report;
        report.text = errorString;
        report.status = LiveOrderStatus::Error;
        report.timestamp = Timestamp::now();
        exec_cb_(report);
    }
}

}  // namespace regimeflow::live
