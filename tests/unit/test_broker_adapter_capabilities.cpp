#include <gtest/gtest.h>

#include "regimeflow/live/alpaca_adapter.h"
#include "regimeflow/live/binance_adapter.h"
#if defined(REGIMEFLOW_ENABLE_IBAPI)
#include "regimeflow/live/ib_adapter.h"
#endif

namespace regimeflow::test
{
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
    };
#endif

    TEST(BrokerAdapterCapabilities, AlpacaEquitySupportsExpectedTifMatrix) {
        live::AlpacaAdapter::Config cfg;
        cfg.api_key = "key";
        cfg.secret_key = "secret";
        cfg.asset_class = "equity";
        cfg.enable_streaming = false;

        live::AlpacaAdapter adapter(std::move(cfg));

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
}  // namespace regimeflow::test
