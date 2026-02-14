if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

add_library(regimeflow_common
    common/config.cpp
    common/json.cpp
    common/time.cpp
    common/types.cpp
    common/yaml_config.cpp
    common/sha256.cpp
)

set(IBAPI_ROOT "${CMAKE_SOURCE_DIR}/third_party/ibapi/IBJts/source/cppclient/client")
if(ENABLE_IBAPI AND EXISTS "${IBAPI_ROOT}")
    file(GLOB IBAPI_SOURCES "${IBAPI_ROOT}/*.cpp")
    set(IBAPI_PROTO_DIR "${IBAPI_ROOT}/protobufUnix")
    file(GLOB IBAPI_PROTO_SOURCES "${IBAPI_PROTO_DIR}/*.pb.cc")
    add_library(ibapi STATIC ${IBAPI_SOURCES} ${IBAPI_PROTO_SOURCES})
    target_include_directories(ibapi PUBLIC ${IBAPI_ROOT} ${IBAPI_PROTO_DIR})
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(ibapi PRIVATE -w)
    elseif(MSVC)
        target_compile_options(ibapi PRIVATE /w)
    endif()
    find_package(Protobuf REQUIRED)
    target_include_directories(ibapi PUBLIC ${Protobuf_INCLUDE_DIRS})
    find_package(Threads REQUIRED)
    target_link_libraries(ibapi PUBLIC Threads::Threads)
    if(TARGET protobuf::libprotobuf)
        target_link_libraries(ibapi PUBLIC protobuf::libprotobuf)
    else()
        target_link_libraries(ibapi PUBLIC ${Protobuf_LIBRARIES})
    endif()
endif()

set(REGIMEFLOW_PUBLIC_INCLUDE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

function(regimeflow_target_public_include target)
    target_include_directories(${target} PUBLIC ${REGIMEFLOW_PUBLIC_INCLUDE})
endfunction()

regimeflow_target_public_include(regimeflow_common)

add_library(regimeflow_data
    data/alpaca_data_client.cpp
    data/alpaca_data_source.cpp
    data/api_data_source.cpp
    data/bar_builder.cpp
    data/corporate_actions.cpp
    data/csv_reader.cpp
    data/data_validation.cpp
    data/data_source_factory.cpp
    data/db_csv_adapter.cpp
    data/db_client.cpp
    data/db_source.cpp
    data/live_feed.cpp
    data/memory_data_source.cpp
    data/merged_iterator.cpp
    data/metadata_data_source.cpp
    data/mmap_data_source.cpp
    data/mmap_reader.cpp
    data/mmap_storage.cpp
    data/mmap_writer.cpp
    data/order_book_mmap.cpp
    data/order_book_mmap_data_source.cpp
    data/postgres_client.cpp
    data/snapshot_access.cpp
    data/symbol_metadata.cpp
    data/tick_mmap.cpp
    data/tick_mmap_data_source.cpp
    data/time_series_query.cpp
    data/tick_csv_reader.cpp
    data/validation_utils.cpp
    data/websocket_feed.cpp
)

regimeflow_target_public_include(regimeflow_data)

target_link_libraries(regimeflow_data
    PUBLIC
        regimeflow_common
        regimeflow_plugins
)

if(ENABLE_POSTGRES)
find_package(PostgreSQL QUIET)
if(PostgreSQL_FOUND)
    target_compile_definitions(regimeflow_data PRIVATE REGIMEFLOW_USE_LIBPQ)
    target_include_directories(regimeflow_data PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(regimeflow_data PRIVATE ${PostgreSQL_LIBRARIES})
endif()
endif()

if(ENABLE_CURL)
find_package(CURL QUIET)
if(CURL_FOUND)
    target_compile_definitions(regimeflow_data PRIVATE REGIMEFLOW_USE_CURL)
    target_include_directories(regimeflow_data PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(regimeflow_data PRIVATE ${CURL_LIBRARIES})
endif()
endif()

find_package(Boost QUIET COMPONENTS system)
if(Boost_FOUND)
    target_compile_definitions(regimeflow_data PUBLIC REGIMEFLOW_USE_BOOST_BEAST)
    target_include_directories(regimeflow_data PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(regimeflow_data PRIVATE Boost::system)
endif()

if(ENABLE_OPENSSL)
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_compile_definitions(regimeflow_data PUBLIC REGIMEFLOW_USE_OPENSSL)
    target_link_libraries(regimeflow_data PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
endif()

add_executable(regimeflow_mmap_builder tools/mmap_builder.cpp)
target_link_libraries(regimeflow_mmap_builder PRIVATE regimeflow_data)

add_executable(regimeflow_alpaca_fetch tools/alpaca_fetch_main.cpp)
target_link_libraries(regimeflow_alpaca_fetch PRIVATE regimeflow_data)

add_library(regimeflow_engine
    engine/audit_log.cpp
    engine/backtest_engine.cpp
    engine/backtest_runner.cpp
    engine/engine_factory.cpp
    engine/execution_pipeline.cpp
    engine/event_generator.cpp
    engine/event_loop.cpp
    engine/market_data_cache.cpp
    engine/order_book_cache.cpp
    engine/order.cpp
    engine/order_manager.cpp
    engine/portfolio.cpp
    engine/regime_tracker.cpp
    engine/timer_service.cpp
)

regimeflow_target_public_include(regimeflow_engine)

target_link_libraries(regimeflow_engine
    PUBLIC
        regimeflow_common
        regimeflow_risk
        regimeflow_metrics
        regimeflow_regime
)

add_library(regimeflow_execution
    execution/basic_execution_model.cpp
    execution/commission.cpp
    execution/execution_factory.cpp
    execution/fill_simulator.cpp
    execution/latency_model.cpp
    execution/market_impact.cpp
    execution/order_book_execution_model.cpp
    execution/slippage.cpp
    execution/transaction_cost.cpp
)

regimeflow_target_public_include(regimeflow_execution)

target_link_libraries(regimeflow_execution
    PUBLIC
        regimeflow_common
        regimeflow_engine
        regimeflow_plugins
)

add_library(regimeflow_strategy
    strategy/context.cpp
    strategy/strategy_factory.cpp
    strategy/strategy_manager.cpp
    strategy/strategies/buy_and_hold.cpp
    strategy/strategies/harmonic_pattern.cpp
    strategy/strategies/moving_average_cross.cpp
    strategy/strategies/pairs_trading.cpp
    strategy/strategies/register_builtin.cpp
)

regimeflow_target_public_include(regimeflow_strategy)

target_link_libraries(regimeflow_strategy
    PUBLIC
        regimeflow_common
        regimeflow_engine
        regimeflow_plugins
)

add_library(regimeflow_risk
    risk/risk_factory.cpp
    risk/risk_limits.cpp
    risk/position_sizer.cpp
    risk/stop_loss.cpp
)

regimeflow_target_public_include(regimeflow_risk)

target_link_libraries(regimeflow_risk
    PUBLIC
        regimeflow_common
        regimeflow_plugins
)

add_library(regimeflow_metrics
    metrics/attribution.cpp
    metrics/drawdown.cpp
    metrics/metrics_tracker.cpp
    metrics/performance.cpp
    metrics/performance_calculator.cpp
    metrics/performance_metrics.cpp
    metrics/regime_attribution.cpp
    metrics/transition_metrics.cpp
    metrics/report.cpp
    metrics/report_writer.cpp
)

regimeflow_target_public_include(regimeflow_metrics)

target_link_libraries(regimeflow_metrics
    PUBLIC
        regimeflow_common
)

add_library(regimeflow_regime
    regime/constant_detector.cpp
    regime/ensemble.cpp
    regime/features.cpp
    regime/hmm.cpp
    regime/regime_factory.cpp
    regime/state_manager.cpp
)

regimeflow_target_public_include(regimeflow_regime)

target_link_libraries(regimeflow_regime
    PUBLIC
        regimeflow_common
        regimeflow_plugins
)

add_library(regimeflow_plugins
    plugins/hooks.cpp
    plugins/registry.cpp
)

regimeflow_target_public_include(regimeflow_plugins)

target_link_libraries(regimeflow_plugins
    PUBLIC
        regimeflow_common
)

add_library(regimeflow_walkforward
    walkforward/optimizer.cpp
)

regimeflow_target_public_include(regimeflow_walkforward)

target_link_libraries(regimeflow_walkforward
    PUBLIC
        regimeflow_common
        regimeflow_metrics
        regimeflow_engine
)

set(REGIMEFLOW_LIVE_SOURCES
    live/alpaca_adapter.cpp
    live/audit_log.cpp
    live/binance_adapter.cpp
    live/event_bus.cpp
    live/live_engine.cpp
    live/live_order_manager.cpp
    live/mq_adapter.cpp
    live/mq_codec.cpp
    live/types.cpp
)

if(ENABLE_IBAPI)
    list(APPEND REGIMEFLOW_LIVE_SOURCES live/ib_adapter.cpp)
endif()

add_library(regimeflow_live ${REGIMEFLOW_LIVE_SOURCES})

regimeflow_target_public_include(regimeflow_live)

target_link_libraries(regimeflow_live
    PUBLIC
        regimeflow_common
        regimeflow_engine
        regimeflow_strategy
        regimeflow_risk
        regimeflow_regime
        regimeflow_data
)

if(WIN32)
    target_link_libraries(regimeflow_live PRIVATE Psapi)
endif()

if(ENABLE_OPENSSL)
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_OPENSSL)
    target_link_libraries(regimeflow_live PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
endif()

if(ENABLE_ZMQ OR ENABLE_REDIS OR ENABLE_KAFKA)
    find_package(PkgConfig QUIET)
    if(ENABLE_ZMQ)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(ZMQ libzmq)
        endif()
        if(ZMQ_FOUND)
            target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_ZMQ)
            target_include_directories(regimeflow_live PRIVATE ${ZMQ_INCLUDE_DIRS})
            target_link_libraries(regimeflow_live PRIVATE ${ZMQ_LIBRARIES})
        endif()
    endif()

    if(ENABLE_REDIS)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(REDIS hiredis)
        endif()
        if(REDIS_FOUND)
            target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_REDIS)
            target_include_directories(regimeflow_live PRIVATE ${REDIS_INCLUDE_DIRS})
            target_link_libraries(regimeflow_live PRIVATE ${REDIS_LIBRARIES})
        else()
            find_library(REDIS_LIBRARY hiredis)
            if(REDIS_LIBRARY)
                target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_REDIS)
                target_link_libraries(regimeflow_live PRIVATE ${REDIS_LIBRARY})
            elseif(REGIMEFLOW_FETCH_DEPS)
                include(FetchContent)
                FetchContent_Declare(
                    hiredis
                    GIT_REPOSITORY https://github.com/redis/hiredis.git
                    GIT_TAG v1.2.0
                )
                FetchContent_MakeAvailable(hiredis)
                if(TARGET hiredis)
                    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_REDIS)
                    target_link_libraries(regimeflow_live PRIVATE hiredis)
                endif()
            endif()
        endif()
    endif()

    if(ENABLE_KAFKA)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(KAFKA rdkafka)
        endif()
        if(KAFKA_FOUND)
            target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_KAFKA)
            target_include_directories(regimeflow_live PRIVATE ${KAFKA_INCLUDE_DIRS})
            target_link_libraries(regimeflow_live PRIVATE ${KAFKA_LIBRARIES})
        endif()
    endif()
endif()

if(ENABLE_IBAPI AND TARGET ibapi)
    target_link_libraries(regimeflow_live PUBLIC ibapi)
    target_include_directories(regimeflow_live PRIVATE ${IBAPI_ROOT})
    if(EXISTS "/usr/local/lib/libbid.a")
        target_link_libraries(regimeflow_live PRIVATE /usr/local/lib/libbid.a)
    else()
        find_library(BID_LIBRARY NAMES
            bid
            bidgcc000
            bidgcc001
            bidgcc010
            bidgcc011
            bidgcc100
            bidgcc101
            bidgcc110
            bidgcc111
        )
        if(BID_LIBRARY)
            target_link_libraries(regimeflow_live PRIVATE ${BID_LIBRARY})
        endif()
    endif()
endif()

if(CURL_FOUND)
    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_CURL)
    target_include_directories(regimeflow_live PRIVATE ${CURL_INCLUDE_DIRS})
    target_link_libraries(regimeflow_live PRIVATE ${CURL_LIBRARIES})
endif()

add_executable(regimeflow_live_cli tools/live_main.cpp)
target_link_libraries(regimeflow_live_cli PRIVATE regimeflow_live)
set_target_properties(regimeflow_live_cli PROPERTIES OUTPUT_NAME regimeflow_live)
