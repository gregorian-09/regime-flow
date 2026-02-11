#include "regimeflow/live/mq_codec.h"

#include "regimeflow/common/types.h"

#include <sstream>
#include <vector>

namespace regimeflow::live {

namespace {

std::vector<std::string> split(const std::string& input, char delim) {
    std::vector<std::string> out;
    std::string token;
    std::istringstream stream(input);
    while (std::getline(stream, token, delim)) {
        out.push_back(token);
    }
    return out;
}

std::string topic_name(LiveTopic topic) {
    switch (topic) {
        case LiveTopic::MarketData: return "MD";
        case LiveTopic::ExecutionReport: return "EXEC";
        case LiveTopic::PositionUpdate: return "POS";
        case LiveTopic::AccountUpdate: return "ACCT";
        case LiveTopic::System: return "SYS";
    }
    return "SYS";
}

std::optional<LiveTopic> parse_topic(const std::string& token) {
    if (token == "MD") return LiveTopic::MarketData;
    if (token == "EXEC") return LiveTopic::ExecutionReport;
    if (token == "POS") return LiveTopic::PositionUpdate;
    if (token == "ACCT") return LiveTopic::AccountUpdate;
    if (token == "SYS") return LiveTopic::System;
    return std::nullopt;
}

std::string side_name(engine::OrderSide side) {
    return side == engine::OrderSide::Buy ? "BUY" : "SELL";
}

engine::OrderSide parse_side(const std::string& token) {
    return token == "SELL" ? engine::OrderSide::Sell : engine::OrderSide::Buy;
}

std::string status_name(LiveOrderStatus status) {
    switch (status) {
        case LiveOrderStatus::PendingNew: return "PENDING_NEW";
        case LiveOrderStatus::New: return "NEW";
        case LiveOrderStatus::PartiallyFilled: return "PARTIAL";
        case LiveOrderStatus::Filled: return "FILLED";
        case LiveOrderStatus::PendingCancel: return "PENDING_CANCEL";
        case LiveOrderStatus::Cancelled: return "CANCELLED";
        case LiveOrderStatus::Rejected: return "REJECTED";
        case LiveOrderStatus::Expired: return "EXPIRED";
        case LiveOrderStatus::Error: return "ERROR";
    }
    return "NEW";
}

LiveOrderStatus parse_status(const std::string& token) {
    if (token == "PENDING_NEW") return LiveOrderStatus::PendingNew;
    if (token == "NEW") return LiveOrderStatus::New;
    if (token == "PARTIAL") return LiveOrderStatus::PartiallyFilled;
    if (token == "FILLED") return LiveOrderStatus::Filled;
    if (token == "PENDING_CANCEL") return LiveOrderStatus::PendingCancel;
    if (token == "CANCELLED") return LiveOrderStatus::Cancelled;
    if (token == "REJECTED") return LiveOrderStatus::Rejected;
    if (token == "EXPIRED") return LiveOrderStatus::Expired;
    if (token == "ERROR") return LiveOrderStatus::Error;
    return LiveOrderStatus::New;
}

}  // namespace

std::string LiveMessageCodec::encode(const LiveMessage& message) {
    std::ostringstream out;
    out << topic_name(message.topic);
    std::visit([&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, MarketDataUpdate>) {
            std::visit([&](const auto& data) {
                using D = std::decay_t<decltype(data)>;
                if constexpr (std::is_same_v<D, data::Bar>) {
                    out << "|BAR|" << SymbolRegistry::instance().lookup(data.symbol)
                        << "|" << data.timestamp.microseconds()
                        << "|" << data.open << "|" << data.high << "|" << data.low
                        << "|" << data.close << "|" << data.volume;
                } else if constexpr (std::is_same_v<D, data::Tick>) {
                    out << "|TICK|" << SymbolRegistry::instance().lookup(data.symbol)
                        << "|" << data.timestamp.microseconds()
                        << "|" << data.price << "|" << data.quantity;
                } else if constexpr (std::is_same_v<D, data::Quote>) {
                    out << "|QUOTE|" << SymbolRegistry::instance().lookup(data.symbol)
                        << "|" << data.timestamp.microseconds()
                        << "|" << data.bid << "|" << data.ask
                        << "|" << data.bid_size << "|" << data.ask_size;
                } else if constexpr (std::is_same_v<D, data::OrderBook>) {
                    double bid = 0.0, bid_size = 0.0, ask = 0.0, ask_size = 0.0;
                    bid = data.bids[0].price;
                    bid_size = data.bids[0].quantity;
                    ask = data.asks[0].price;
                    ask_size = data.asks[0].quantity;
                    out << "|BOOK|" << SymbolRegistry::instance().lookup(data.symbol)
                        << "|" << data.timestamp.microseconds()
                        << "|" << bid << "|" << bid_size
                        << "|" << ask << "|" << ask_size;
                }
            }, payload.data);
        } else if constexpr (std::is_same_v<T, ExecutionReport>) {
            out << "|EXEC|" << payload.broker_order_id
                << "|" << payload.broker_exec_id
                << "|" << payload.symbol
                << "|" << side_name(payload.side)
                << "|" << payload.quantity
                << "|" << payload.price
                << "|" << payload.commission
                << "|" << status_name(payload.status)
                << "|" << payload.text
                << "|" << payload.timestamp.microseconds();
        } else if constexpr (std::is_same_v<T, Position>) {
            out << "|POS|" << payload.symbol
                << "|" << payload.quantity
                << "|" << payload.average_price
                << "|" << payload.market_value;
        } else if constexpr (std::is_same_v<T, AccountInfo>) {
            out << "|ACCT|" << payload.equity
                << "|" << payload.cash
                << "|" << payload.buying_power;
        } else if constexpr (std::is_same_v<T, std::string>) {
            out << "|SYS|" << payload;
        } else {
            out << "|SYS|";
        }
    }, message.payload);
    return out.str();
}

std::optional<LiveMessage> LiveMessageCodec::decode(const std::string& payload) {
    auto parts = split(payload, '|');
    if (parts.empty()) {
        return std::nullopt;
    }
    auto topic = parse_topic(parts[0]);
    if (!topic) {
        return std::nullopt;
    }

    if (*topic == LiveTopic::MarketData && parts.size() >= 3) {
        const std::string& kind = parts[1];
        const std::string& symbol = parts[2];
        SymbolId symbol_id = SymbolRegistry::instance().intern(symbol);
        if (kind == "BAR" && parts.size() >= 9) {
            data::Bar bar{};
            bar.symbol = symbol_id;
            bar.timestamp = Timestamp(std::stoll(parts[3]));
            bar.open = std::stod(parts[4]);
            bar.high = std::stod(parts[5]);
            bar.low = std::stod(parts[6]);
            bar.close = std::stod(parts[7]);
            bar.volume = static_cast<Volume>(std::stoll(parts[8]));
            MarketDataUpdate update;
            update.data = bar;
            LiveMessage msg;
            msg.topic = LiveTopic::MarketData;
            msg.payload = update;
            return msg;
        }
        if (kind == "TICK" && parts.size() >= 6) {
            data::Tick tick{};
            tick.symbol = symbol_id;
            tick.timestamp = Timestamp(std::stoll(parts[3]));
            tick.price = std::stod(parts[4]);
            tick.quantity = static_cast<Quantity>(std::stod(parts[5]));
            MarketDataUpdate update;
            update.data = tick;
            LiveMessage msg;
            msg.topic = LiveTopic::MarketData;
            msg.payload = update;
            return msg;
        }
        if (kind == "QUOTE" && parts.size() >= 8) {
            data::Quote quote{};
            quote.symbol = symbol_id;
            quote.timestamp = Timestamp(std::stoll(parts[3]));
            quote.bid = std::stod(parts[4]);
            quote.ask = std::stod(parts[5]);
            quote.bid_size = static_cast<Quantity>(std::stod(parts[6]));
            quote.ask_size = static_cast<Quantity>(std::stod(parts[7]));
            MarketDataUpdate update;
            update.data = quote;
            LiveMessage msg;
            msg.topic = LiveTopic::MarketData;
            msg.payload = update;
            return msg;
        }
        if (kind == "BOOK" && parts.size() >= 8) {
            data::OrderBook book{};
            book.symbol = symbol_id;
            book.timestamp = Timestamp(std::stoll(parts[3]));
            book.bids[0].price = std::stod(parts[4]);
            book.bids[0].quantity = std::stod(parts[5]);
            book.asks[0].price = std::stod(parts[6]);
            book.asks[0].quantity = std::stod(parts[7]);
            MarketDataUpdate update;
            update.data = book;
            LiveMessage msg;
            msg.topic = LiveTopic::MarketData;
            msg.payload = update;
            return msg;
        }
        return std::nullopt;
    }

    if (*topic == LiveTopic::ExecutionReport && parts.size() >= 12 && parts[1] == "EXEC") {
        ExecutionReport report;
        report.broker_order_id = parts[2];
        report.broker_exec_id = parts[3];
        report.symbol = parts[4];
        report.side = parse_side(parts[5]);
        report.quantity = std::stod(parts[6]);
        report.price = std::stod(parts[7]);
        report.commission = std::stod(parts[8]);
        report.status = parse_status(parts[9]);
        report.text = parts[10];
        report.timestamp = Timestamp(std::stoll(parts[11]));
        LiveMessage msg;
        msg.topic = LiveTopic::ExecutionReport;
        msg.payload = report;
        return msg;
    }

    if (*topic == LiveTopic::PositionUpdate && parts.size() >= 6 && parts[1] == "POS") {
        Position pos;
        pos.symbol = parts[2];
        pos.quantity = std::stod(parts[3]);
        pos.average_price = std::stod(parts[4]);
        pos.market_value = std::stod(parts[5]);
        LiveMessage msg;
        msg.topic = LiveTopic::PositionUpdate;
        msg.payload = pos;
        return msg;
    }

    if (*topic == LiveTopic::AccountUpdate && parts.size() >= 5 && parts[1] == "ACCT") {
        AccountInfo info;
        info.equity = std::stod(parts[2]);
        info.cash = std::stod(parts[3]);
        info.buying_power = std::stod(parts[4]);
        LiveMessage msg;
        msg.topic = LiveTopic::AccountUpdate;
        msg.payload = info;
        return msg;
    }

    if (*topic == LiveTopic::System && parts.size() >= 3) {
        LiveMessage msg;
        msg.topic = LiveTopic::System;
        msg.payload = parts[2];
        return msg;
    }

    return std::nullopt;
}

}  // namespace regimeflow::live
