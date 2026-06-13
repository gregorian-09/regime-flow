#include <gtest/gtest.h>

#include "regimeflow/live/alpaca_adapter.h"
#include "regimeflow/live/binance_adapter.h"
#if defined(REGIMEFLOW_ENABLE_IBAPI)
#include "regimeflow/live/ib_adapter.h"
#endif

namespace regimeflow::test
{
    class BrokerAdapterTestAccess {
    public:
        static Result<std::string> parse_alpaca_submit_order_id(const std::string& payload) {
            return live::AlpacaAdapter::parse_submitted_order_id(payload);
        }

        static Result<std::string> parse_binance_submit_order_id(const std::string& payload) {
            return live::BinanceAdapter::parse_submitted_order_id(payload);
        }
    };

#if defined(REGIMEFLOW_ENABLE_IBAPI)
    class IBAdapterTestAccess {
    public:
        static void set_transport_connected(live::IBAdapter& adapter, const bool value) {
            adapter.connected_ = value;
        }

        static void set_trading_ready(live::IBAdapter& adapter, const bool value) {
            adapter.trading_ready_ = value;
        }

        static void deliver_next_valid_id(live::IBAdapter& adapter, const OrderId order_id) {
            adapter.nextValidId(order_id);
        }

        static live::ExecutionReport emit_order_status(live::IBAdapter& adapter,
                                                       const std::string& status) {
            live::ExecutionReport captured;
            adapter.on_execution_report([&captured](const live::ExecutionReport& report) {
                captured = report;
            });
            const Decimal zero = DecimalFunctions::doubleToDecimal(0.0);
            adapter.orderStatus(7, status, zero, zero, 101.25, 0, 0, 0.0, 0, "", 0.0);
            return captured;
        }

        static void register_ticker(live::IBAdapter& adapter,
                                    const TickerId ticker_id,
                                    const SymbolId symbol) {
            std::lock_guard<std::mutex> lock(adapter.state_mutex_);
            adapter.ticker_to_symbol_[ticker_id] = symbol;
        }

        static live::MarketDataUpdate emit_tick_price(live::IBAdapter& adapter,
                                                      const TickerId ticker_id,
                                                      const TickType field,
                                                      const double price) {
            live::MarketDataUpdate captured;
            adapter.on_market_data([&captured](const live::MarketDataUpdate& update) {
                captured = update;
            });
            adapter.tickPrice(ticker_id, field, price, TickAttrib{});
            return captured;
        }

        static live::MarketDataUpdate emit_tick_size(live::IBAdapter& adapter,
                                                     const TickerId ticker_id,
                                                     const TickType field,
                                                     const double size) {
            live::MarketDataUpdate captured;
            adapter.on_market_data([&captured](const live::MarketDataUpdate& update) {
                captured = update;
            });
            adapter.tickSize(ticker_id, field, DecimalFunctions::doubleToDecimal(size));
            return captured;
        }

        static ::Contract build_contract(const live::IBAdapter& adapter, const std::string& symbol) {
            return adapter.build_contract(symbol);
        }

        static ::Order build_order(const live::IBAdapter& adapter,
                                   const engine::Order& order,
                                   const int64_t order_id) {
            return adapter.build_order(order, order_id);
        }
    };
#endif

    TEST(BrokerAdapterCapabilities, AlpacaEquitySupportsExpectedTifMatrix) {
        live::AlpacaAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.asset_class = "equity";
        cfg.enable_streaming = false;

        live::AlpacaAdapter adapter(std::move(cfg));
        const auto capabilities = adapter.capabilities();

        EXPECT_EQ(capabilities.broker, "alpaca");
        ASSERT_EQ(capabilities.asset_classes.size(), 1u);
        EXPECT_EQ(capabilities.asset_classes.front(), AssetClass::Equity);
        EXPECT_TRUE(capabilities.supports_fractional_quantity);
        EXPECT_TRUE(capabilities.supports_short_selling);
        EXPECT_FALSE(capabilities.supports_crypto);
        EXPECT_TRUE(capabilities.supports_bracket_orders);
        EXPECT_EQ(capabilities.max_orders_per_second, adapter.max_orders_per_second());

        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::Day));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::GTC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::IOC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::FOK));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::GTD));

        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::Day));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::GTC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::IOC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::FOK));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::MarketOnOpen, engine::TimeInForce::Day));
    }

    TEST(BrokerAdapterCapabilities, AlpacaCryptoSupportsOnlyGtcAndIoc) {
        live::AlpacaAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.asset_class = "crypto";
        cfg.enable_streaming = false;

        live::AlpacaAdapter adapter(std::move(cfg));
        const auto capabilities = adapter.capabilities();

        ASSERT_EQ(capabilities.asset_classes.size(), 1u);
        EXPECT_EQ(capabilities.asset_classes.front(), AssetClass::Crypto);
        EXPECT_TRUE(capabilities.supports_fractional_quantity);
        EXPECT_FALSE(capabilities.supports_short_selling);
        EXPECT_TRUE(capabilities.supports_crypto);
        EXPECT_FALSE(capabilities.supports_bracket_orders);
        EXPECT_TRUE(capabilities.supports(engine::OrderType::Limit, engine::TimeInForce::GTC));
        EXPECT_FALSE(capabilities.supports(engine::OrderType::Limit, engine::TimeInForce::Day));

        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::GTC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::IOC));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::Day));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::FOK));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::StopLimit, engine::TimeInForce::Day));
    }

    TEST(BrokerAdapterCapabilities, BinanceSupportsSpotTifMatrix) {
        live::BinanceAdapter::Config cfg;
        cfg.enable_streaming = false;

        live::BinanceAdapter adapter(std::move(cfg));
        const auto capabilities = adapter.capabilities();

        EXPECT_EQ(capabilities.broker, "binance");
        ASSERT_EQ(capabilities.asset_classes.size(), 1u);
        EXPECT_EQ(capabilities.asset_classes.front(), AssetClass::Crypto);
        EXPECT_TRUE(capabilities.supports_fractional_quantity);
        EXPECT_FALSE(capabilities.supports_short_selling);
        EXPECT_TRUE(capabilities.supports_crypto);
        EXPECT_FALSE(capabilities.supports_bracket_orders);
        EXPECT_EQ(capabilities.max_messages_per_second, adapter.max_messages_per_second());

        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::GTC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::IOC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::FOK));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::Day));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::GTD));

        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::Day));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::IOC));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::GTC));
        EXPECT_FALSE(adapter.supports_tif(engine::OrderType::Stop, engine::TimeInForce::IOC));
    }

#if defined(REGIMEFLOW_ENABLE_IBAPI)
    TEST(BrokerAdapterCapabilities, IbSupportsCoreTimeInForceSet) {
        live::IBAdapter adapter(live::IBAdapter::Config{});
        const auto capabilities = adapter.capabilities();

        EXPECT_EQ(capabilities.broker, "interactive-brokers");
        EXPECT_GE(capabilities.asset_classes.size(), 4u);
        EXPECT_TRUE(capabilities.supports_fractional_quantity);
        EXPECT_TRUE(capabilities.supports_short_selling);
        EXPECT_FALSE(capabilities.supports_crypto);
        EXPECT_FALSE(capabilities.supports_bracket_orders);
        EXPECT_TRUE(capabilities.supports(engine::OrderType::StopLimit, engine::TimeInForce::GTD));

        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Market, engine::TimeInForce::Day));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::GTC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::IOC));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::FOK));
        EXPECT_TRUE(adapter.supports_tif(engine::OrderType::Limit, engine::TimeInForce::GTD));
    }

    TEST(BrokerAdapterCapabilities, IbSeparatesTransportAndTradingReadyState) {
        live::IBAdapter adapter(live::IBAdapter::Config{});

        EXPECT_FALSE(adapter.is_transport_connected());
        EXPECT_FALSE(adapter.is_ready_for_orders());
        EXPECT_FALSE(adapter.is_connected());

        IBAdapterTestAccess::set_transport_connected(adapter, true);
        EXPECT_TRUE(adapter.is_transport_connected());
        EXPECT_FALSE(adapter.is_ready_for_orders());
        EXPECT_FALSE(adapter.is_connected());

        IBAdapterTestAccess::deliver_next_valid_id(adapter, 42);
        EXPECT_TRUE(adapter.is_transport_connected());
        EXPECT_TRUE(adapter.is_ready_for_orders());
        EXPECT_TRUE(adapter.is_connected());

        IBAdapterTestAccess::set_trading_ready(adapter, false);
        EXPECT_TRUE(adapter.is_transport_connected());
        EXPECT_FALSE(adapter.is_ready_for_orders());
        EXPECT_FALSE(adapter.is_connected());
    }

    TEST(BrokerAdapterCapabilities, IbRejectsRemotePlaintextHostByDefault) {
        live::IBAdapter::Config cfg;
        cfg.host = "192.0.2.10";
        live::IBAdapter adapter(std::move(cfg));

        const auto result = adapter.connect();

        ASSERT_TRUE(result.is_err());
        EXPECT_EQ(result.error().code, Error::Code::BrokerError);
        EXPECT_NE(result.error().message.find("plaintext TCP"), std::string::npos);
    }

    TEST(BrokerAdapterCapabilities, IbSupportsFxFuturesAndOptionsContractOverrides) {
        live::IBAdapter::Config cfg;
        cfg.default_contract.exchange = "SMART";
        cfg.default_contract.currency = "USD";

        live::IBAdapter::ContractConfig fx_contract;
        fx_contract.security_type = "CASH";
        fx_contract.exchange = "IDEALPRO";
        cfg.contracts.emplace("EURUSD", fx_contract);

        live::IBAdapter::ContractConfig future_contract;
        future_contract.security_type = "FUT";
        future_contract.exchange = "EUREX";
        future_contract.currency = "EUR";
        future_contract.local_symbol = "FGBL MAR 27";
        future_contract.include_expired = true;
        cfg.contracts.emplace("FGBL", future_contract);

        live::IBAdapter::ContractConfig option_contract;
        option_contract.security_type = "OPT";
        option_contract.exchange = "EUREX";
        option_contract.currency = "EUR";
        option_contract.symbol_override = "BMW";
        option_contract.last_trade_date_or_contract_month = "20251219";
        option_contract.right = "C";
        option_contract.strike = 72.0;
        option_contract.multiplier = "100";
        option_contract.trading_class = "BMW";
        cfg.contracts.emplace("BMW_C72_DEC25", option_contract);

        live::IBAdapter adapter(std::move(cfg));

        const auto fx = adapter.contract_config_for_symbol("EURUSD");
        EXPECT_EQ(fx.security_type, "CASH");
        EXPECT_EQ(fx.exchange, "IDEALPRO");
        EXPECT_EQ(fx.symbol_override, "EUR");
        EXPECT_EQ(fx.currency, "USD");

        const auto future = adapter.contract_config_for_symbol("FGBL");
        EXPECT_EQ(future.security_type, "FUT");
        EXPECT_EQ(future.exchange, "EUREX");
        EXPECT_EQ(future.currency, "EUR");
        EXPECT_EQ(future.local_symbol, "FGBL MAR 27");
        EXPECT_TRUE(future.include_expired);

        const auto option = adapter.contract_config_for_symbol("BMW_C72_DEC25");
        EXPECT_EQ(option.security_type, "OPT");
        EXPECT_EQ(option.symbol_override, "BMW");
        EXPECT_EQ(option.right, "C");
        ASSERT_TRUE(option.strike.has_value());
        EXPECT_DOUBLE_EQ(*option.strike, 72.0);
        EXPECT_EQ(option.multiplier, "100");
    }


    TEST(BrokerAdapterCapabilities, IbBuildsConcreteContractsFromOverrides) {
        live::IBAdapter::Config cfg;
        live::IBAdapter::ContractConfig option_contract;
        option_contract.security_type = "OPT";
        option_contract.exchange = "SMART";
        option_contract.currency = "USD";
        option_contract.symbol_override = "SPY";
        option_contract.last_trade_date_or_contract_month = "20260116";
        option_contract.right = "P";
        option_contract.strike = 450.0;
        option_contract.multiplier = "100";
        option_contract.primary_exchange = "ARCA";
        cfg.contracts.emplace("SPY_P450_JAN26", option_contract);

        live::IBAdapter adapter(std::move(cfg));
        const auto contract = IBAdapterTestAccess::build_contract(adapter, "SPY_P450_JAN26");

        EXPECT_EQ(contract.symbol, "SPY");
        EXPECT_EQ(contract.secType, "OPT");
        EXPECT_EQ(contract.exchange, "SMART");
        EXPECT_EQ(contract.currency, "USD");
        EXPECT_EQ(contract.primaryExchange, "ARCA");
        EXPECT_EQ(contract.lastTradeDateOrContractMonth, "20260116");
        EXPECT_EQ(contract.right, "P");
        EXPECT_DOUBLE_EQ(contract.strike, 450.0);
        EXPECT_EQ(contract.multiplier, "100");
    }

    TEST(BrokerAdapterCapabilities, IbBuildsConcreteOrderTypesAndTif) {
        live::IBAdapter adapter(live::IBAdapter::Config{});
        const auto symbol = SymbolRegistry::instance().intern("AAPL");

        auto stop_limit = engine::Order::market(symbol, engine::OrderSide::Sell, 3.5);
        stop_limit.type = engine::OrderType::StopLimit;
        stop_limit.stop_price = 100.0;
        stop_limit.limit_price = 99.5;
        stop_limit.tif = engine::TimeInForce::GTD;
        stop_limit.expire_at = Timestamp::from_date(2026, 1, 2);
        const auto ib_stop_limit = IBAdapterTestAccess::build_order(adapter, stop_limit, 77);
        EXPECT_EQ(ib_stop_limit.orderId, 77);
        EXPECT_EQ(ib_stop_limit.action, "SELL");
        EXPECT_EQ(ib_stop_limit.orderType, "STP LMT");
        EXPECT_EQ(ib_stop_limit.tif, "GTD");
        EXPECT_DOUBLE_EQ(ib_stop_limit.auxPrice, 100.0);
        EXPECT_DOUBLE_EQ(ib_stop_limit.lmtPrice, 99.5);
        EXPECT_DOUBLE_EQ(DecimalFunctions::decimalToDouble(ib_stop_limit.totalQuantity), 3.5);
        EXPECT_FALSE(ib_stop_limit.goodTillDate.empty());

        auto moc = engine::Order::market(symbol, engine::OrderSide::Buy, 2.0);
        moc.type = engine::OrderType::MarketOnClose;
        const auto ib_moc = IBAdapterTestAccess::build_order(adapter, moc, 78);
        EXPECT_EQ(ib_moc.orderType, "MOC");
        EXPECT_EQ(ib_moc.tif, "DAY");

        auto moo = engine::Order::market(symbol, engine::OrderSide::Buy, 2.0);
        moo.type = engine::OrderType::MarketOnOpen;
        const auto ib_moo = IBAdapterTestAccess::build_order(adapter, moo, 79);
        EXPECT_EQ(ib_moo.orderType, "MKT");
        EXPECT_EQ(ib_moo.tif, "OPG");
    }

    TEST(BrokerAdapterCapabilities, IbMapsLifecycleStatusesExplicitly) {
        live::IBAdapter adapter(live::IBAdapter::Config{});

        const auto pending_cancel = IBAdapterTestAccess::emit_order_status(adapter, "PendingCancel");
        EXPECT_EQ(pending_cancel.status, live::LiveOrderStatus::PendingCancel);
        EXPECT_EQ(pending_cancel.text, "PendingCancel");

        const auto expired = IBAdapterTestAccess::emit_order_status(adapter, "Expired");
        EXPECT_EQ(expired.status, live::LiveOrderStatus::Expired);

        const auto inactive = IBAdapterTestAccess::emit_order_status(adapter, "Inactive");
        EXPECT_EQ(inactive.status, live::LiveOrderStatus::Inactive);

        const auto cancelled = IBAdapterTestAccess::emit_order_status(adapter, "ApiCancelled");
        EXPECT_EQ(cancelled.status, live::LiveOrderStatus::Cancelled);

        const auto rejected = IBAdapterTestAccess::emit_order_status(adapter, "Rejected");
        EXPECT_EQ(rejected.status, live::LiveOrderStatus::Rejected);
    }

    TEST(BrokerAdapterCapabilities, IbEmitsQuotesForBidAskUpdates) {
        live::IBAdapter adapter(live::IBAdapter::Config{});
        const auto symbol = SymbolRegistry::instance().intern("IBM");
        IBAdapterTestAccess::register_ticker(adapter, 101, symbol);

        const auto no_emit = IBAdapterTestAccess::emit_tick_price(adapter, 101, TickType::BID, 101.0);
        EXPECT_FALSE(std::holds_alternative<data::Quote>(no_emit.data));

        const auto quote_update = IBAdapterTestAccess::emit_tick_price(adapter, 101, TickType::ASK, 101.5);
        ASSERT_TRUE(std::holds_alternative<data::Quote>(quote_update.data));
        const auto& quote = std::get<data::Quote>(quote_update.data);
        EXPECT_EQ(quote.symbol, symbol);
        EXPECT_DOUBLE_EQ(quote.bid, 101.0);
        EXPECT_DOUBLE_EQ(quote.ask, 101.5);

        const auto sized_quote = IBAdapterTestAccess::emit_tick_size(adapter, 101, TickType::BID_SIZE, 12.0);
        ASSERT_TRUE(std::holds_alternative<data::Quote>(sized_quote.data));
        const auto& sized = std::get<data::Quote>(sized_quote.data);
        EXPECT_DOUBLE_EQ(sized.bid_size, 12.0);
        EXPECT_DOUBLE_EQ(sized.ask, 101.5);
    }
#endif

    TEST(BrokerAdapterFailures, AlpacaRequiresCredentialsBeforeConnect) {
        live::AlpacaAdapter adapter(live::AlpacaAdapter::Config{});
        const auto result = adapter.connect();

        EXPECT_TRUE(result.is_err());
        EXPECT_FALSE(adapter.is_connected());
    }

    TEST(BrokerAdapterFailures, AlpacaConnectsWithoutNetworkWhenStreamingDisabled) {
        live::AlpacaAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.enable_streaming = false;

        live::AlpacaAdapter adapter(std::move(cfg));
        const auto connect = adapter.connect();
        ASSERT_TRUE(connect.is_ok());
        EXPECT_TRUE(adapter.is_connected());

        const auto disconnect = adapter.disconnect();
        EXPECT_TRUE(disconnect.is_ok());
        EXPECT_FALSE(adapter.is_connected());
    }

    TEST(BrokerAdapterFailures, AlpacaRejectsUnsupportedMarketOnOpenCloseOrdersBeforeRestCall) {
        live::AlpacaAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.enable_streaming = false;

        live::AlpacaAdapter adapter(std::move(cfg));
        ASSERT_TRUE(adapter.connect().is_ok());

        const auto symbol = SymbolRegistry::instance().intern("AAPL");

        auto moo = engine::Order::market(symbol, engine::OrderSide::Buy, 1.0);
        moo.type = engine::OrderType::MarketOnOpen;
        EXPECT_TRUE(adapter.submit_order(moo).is_err());

        auto moc = engine::Order::market(symbol, engine::OrderSide::Buy, 1.0);
        moc.type = engine::OrderType::MarketOnClose;
        EXPECT_TRUE(adapter.submit_order(moc).is_err());

        EXPECT_TRUE(adapter.disconnect().is_ok());
    }

    TEST(BrokerAdapterFailures, AlpacaRejectsMissingLimitAndStopPricesBeforeRestCall) {
        live::AlpacaAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.enable_streaming = false;

        live::AlpacaAdapter adapter(std::move(cfg));
        ASSERT_TRUE(adapter.connect().is_ok());

        const auto symbol = SymbolRegistry::instance().intern("AAPL");

        engine::Order limit = engine::Order::market(symbol, engine::OrderSide::Buy, 1.0);
        limit.type = engine::OrderType::Limit;
        EXPECT_TRUE(adapter.submit_order(limit).is_err());

        engine::Order stop = engine::Order::market(symbol, engine::OrderSide::Buy, 1.0);
        stop.type = engine::OrderType::Stop;
        EXPECT_TRUE(adapter.submit_order(stop).is_err());

        engine::Order stop_limit = engine::Order::market(symbol, engine::OrderSide::Buy, 1.0);
        stop_limit.type = engine::OrderType::StopLimit;
        stop_limit.stop_price = 100.0;
        EXPECT_TRUE(adapter.submit_order(stop_limit).is_err());

        stop_limit.limit_price = 101.0;
        stop_limit.stop_price = 0.0;
        EXPECT_TRUE(adapter.submit_order(stop_limit).is_err());

        EXPECT_TRUE(adapter.disconnect().is_ok());
    }

    TEST(BrokerAdapterFailures, AlpacaRequiresBrokerIssuedOrderIdInSubmitResponse) {
        const auto invalid_json = BrokerAdapterTestAccess::parse_alpaca_submit_order_id("not-json");
        ASSERT_TRUE(invalid_json.is_err());
        EXPECT_EQ(invalid_json.error().code, Error::Code::ParseError);
        ASSERT_TRUE(invalid_json.error().details.has_value());
        EXPECT_NE(invalid_json.error().details->find("category=schema"), std::string::npos);

        const auto missing_id = BrokerAdapterTestAccess::parse_alpaca_submit_order_id(R"({"status":"accepted"})");
        ASSERT_TRUE(missing_id.is_err());
        EXPECT_EQ(missing_id.error().code, Error::Code::ParseError);
        EXPECT_NE(missing_id.error().message.find("broker order id"), std::string::npos);
        ASSERT_TRUE(missing_id.error().details.has_value());
        EXPECT_NE(missing_id.error().details->find("category=schema"), std::string::npos);
    }

    TEST(BrokerAdapterFailures, BinanceRejectsSubmitWithoutConnection) {
        live::BinanceAdapter::Config cfg;
        cfg.enable_streaming = false;
        live::BinanceAdapter adapter(std::move(cfg));

        const auto symbol = SymbolRegistry::instance().intern("BTCUSDT");
        const auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 0.01);
        const auto result = adapter.submit_order(order);

        EXPECT_TRUE(result.is_err());
    }

    TEST(BrokerAdapterFailures, BinanceRequiresCredentialsAfterConnect) {
        live::BinanceAdapter::Config cfg;
        cfg.enable_streaming = false;
        live::BinanceAdapter adapter(std::move(cfg));

        ASSERT_TRUE(adapter.connect().is_ok());
        const auto symbol = SymbolRegistry::instance().intern("BTCUSDT");
        const auto order = engine::Order::market(symbol, engine::OrderSide::Buy, 0.01);
        const auto result = adapter.submit_order(order);

        EXPECT_TRUE(result.is_err());
        EXPECT_TRUE(adapter.disconnect().is_ok());
    }

    TEST(BrokerAdapterFailures, BinanceCancelRequiresKnownOrderSymbolMapping) {
        live::BinanceAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.enable_streaming = false;
        live::BinanceAdapter adapter(std::move(cfg));

        ASSERT_TRUE(adapter.connect().is_ok());
        adapter.subscribe_market_data({"BTCUSDT", "ETHUSDT"});

        const auto result = adapter.cancel_order("12345");

        ASSERT_TRUE(result.is_err());
        EXPECT_NE(result.error().message.find("order-symbol mapping"), std::string::npos);
        EXPECT_TRUE(adapter.disconnect().is_ok());
    }

    TEST(BrokerAdapterFailures, BinanceRequiresBrokerIssuedOrderIdInSubmitResponse) {
        const auto invalid_json = BrokerAdapterTestAccess::parse_binance_submit_order_id("not-json");
        ASSERT_TRUE(invalid_json.is_err());
        EXPECT_EQ(invalid_json.error().code, Error::Code::ParseError);
        ASSERT_TRUE(invalid_json.error().details.has_value());
        EXPECT_NE(invalid_json.error().details->find("category=schema"), std::string::npos);

        const auto missing_id = BrokerAdapterTestAccess::parse_binance_submit_order_id(R"({"status":"NEW"})");
        ASSERT_TRUE(missing_id.is_err());
        EXPECT_EQ(missing_id.error().code, Error::Code::ParseError);
        EXPECT_NE(missing_id.error().message.find("broker order id"), std::string::npos);
        ASSERT_TRUE(missing_id.error().details.has_value());
        EXPECT_NE(missing_id.error().details->find("category=schema"), std::string::npos);
    }
}  // namespace regimeflow::test
