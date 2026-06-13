add_library(regimeflow_common
    common/config.cpp
    common/json.cpp
    common/time.cpp
    common/types.cpp
    common/yaml_config.cpp
    common/sha256.cpp
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(regimeflow_common PRIVATE -Wno-free-nonheap-object)
endif()

set(IBAPI_ROOT "${CMAKE_SOURCE_DIR}/third_party/ibapi/IBJts/source/cppclient/client")
if(ENABLE_IBAPI AND EXISTS "${IBAPI_ROOT}")
    set(IBAPI_PROTO_DIR "${IBAPI_ROOT}/protobufUnix")
    set(IBAPI_SOURCES
        "${IBAPI_ROOT}/ContractCondition.cpp"
        "${IBAPI_ROOT}/Decimal.cpp"
        "${IBAPI_ROOT}/DefaultEWrapper.cpp"
        "${IBAPI_ROOT}/EClient.cpp"
        "${IBAPI_ROOT}/EClientSocket.cpp"
        "${IBAPI_ROOT}/EClientUtils.cpp"
        "${IBAPI_ROOT}/EDecoder.cpp"
        "${IBAPI_ROOT}/EDecoderUtils.cpp"
        "${IBAPI_ROOT}/EMessage.cpp"
        "${IBAPI_ROOT}/EMutex.cpp"
        "${IBAPI_ROOT}/EOrderDecoder.cpp"
        "${IBAPI_ROOT}/EReader.cpp"
        "${IBAPI_ROOT}/EReaderOSSignal.cpp"
        "${IBAPI_ROOT}/ESocket.cpp"
        "${IBAPI_ROOT}/ExecutionCondition.cpp"
        "${IBAPI_ROOT}/MarginCondition.cpp"
        "${IBAPI_ROOT}/OperatorCondition.cpp"
        "${IBAPI_ROOT}/OrderCondition.cpp"
        "${IBAPI_ROOT}/PercentChangeCondition.cpp"
        "${IBAPI_ROOT}/PriceCondition.cpp"
        "${IBAPI_ROOT}/SoftDollarTier.cpp"
        "${IBAPI_ROOT}/StdAfx.cpp"
        "${IBAPI_ROOT}/TimeCondition.cpp"
        "${IBAPI_ROOT}/Utils.cpp"
        "${IBAPI_ROOT}/VolumeCondition.cpp"
    )
    set(IBAPI_PROTO_SOURCES
        "${IBAPI_PROTO_DIR}/AccountDataEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountSummary.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountSummaryEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountSummaryRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountUpdateMulti.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountUpdateMultiEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountUpdateTime.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountUpdatesMultiRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/AccountValue.pb.cc"
        "${IBAPI_PROTO_DIR}/AllOpenOrdersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/AttachedOrders.pb.cc"
        "${IBAPI_PROTO_DIR}/AutoOpenOrdersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/CalculateImpliedVolatilityRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/CalculateOptionPriceRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelAccountSummary.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelAccountUpdatesMulti.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelCalculateImpliedVolatility.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelCalculateOptionPrice.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelContractData.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelFundamentalsData.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelHeadTimestamp.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelHistogramData.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelHistoricalData.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelHistoricalTicks.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelMarketData.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelMarketDepth.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelNewsBulletins.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelOrderRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelPnL.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelPnLSingle.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelPositions.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelPositionsMulti.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelRealTimeBars.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelScannerSubscription.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelTickByTick.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelWshEventData.pb.cc"
        "${IBAPI_PROTO_DIR}/CancelWshMetaData.pb.cc"
        "${IBAPI_PROTO_DIR}/ComboLeg.pb.cc"
        "${IBAPI_PROTO_DIR}/CommissionAndFeesReport.pb.cc"
        "${IBAPI_PROTO_DIR}/CompletedOrder.pb.cc"
        "${IBAPI_PROTO_DIR}/CompletedOrdersEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/CompletedOrdersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/Contract.pb.cc"
        "${IBAPI_PROTO_DIR}/ContractData.pb.cc"
        "${IBAPI_PROTO_DIR}/ContractDataEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/ContractDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/ContractDescription.pb.cc"
        "${IBAPI_PROTO_DIR}/ContractDetails.pb.cc"
        "${IBAPI_PROTO_DIR}/CurrentTime.pb.cc"
        "${IBAPI_PROTO_DIR}/CurrentTimeInMillis.pb.cc"
        "${IBAPI_PROTO_DIR}/CurrentTimeInMillisRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/CurrentTimeRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/DeltaNeutralContract.pb.cc"
        "${IBAPI_PROTO_DIR}/DepthMarketDataDescription.pb.cc"
        "${IBAPI_PROTO_DIR}/DisplayGroupList.pb.cc"
        "${IBAPI_PROTO_DIR}/DisplayGroupUpdated.pb.cc"
        "${IBAPI_PROTO_DIR}/ErrorMessage.pb.cc"
        "${IBAPI_PROTO_DIR}/Execution.pb.cc"
        "${IBAPI_PROTO_DIR}/ExecutionDetails.pb.cc"
        "${IBAPI_PROTO_DIR}/ExecutionDetailsEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/ExecutionFilter.pb.cc"
        "${IBAPI_PROTO_DIR}/ExecutionRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/ExerciseOptionsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/FAReplace.pb.cc"
        "${IBAPI_PROTO_DIR}/FARequest.pb.cc"
        "${IBAPI_PROTO_DIR}/FamilyCode.pb.cc"
        "${IBAPI_PROTO_DIR}/FamilyCodes.pb.cc"
        "${IBAPI_PROTO_DIR}/FamilyCodesRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/FundamentalsData.pb.cc"
        "${IBAPI_PROTO_DIR}/FundamentalsDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/GlobalCancelRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/HeadTimestamp.pb.cc"
        "${IBAPI_PROTO_DIR}/HeadTimestampRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/HistogramData.pb.cc"
        "${IBAPI_PROTO_DIR}/HistogramDataEntry.pb.cc"
        "${IBAPI_PROTO_DIR}/HistogramDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalData.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalDataBar.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalDataEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalDataUpdate.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalNews.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalNewsEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalNewsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalSchedule.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalSession.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTick.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTickBidAsk.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTickLast.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTicks.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTicksBidAsk.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTicksLast.pb.cc"
        "${IBAPI_PROTO_DIR}/HistoricalTicksRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/IdsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/IneligibilityReason.pb.cc"
        "${IBAPI_PROTO_DIR}/ManagedAccounts.pb.cc"
        "${IBAPI_PROTO_DIR}/ManagedAccountsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDataType.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDataTypeRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDepth.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDepthData.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDepthExchanges.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDepthExchangesRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDepthL2.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketDepthRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketRule.pb.cc"
        "${IBAPI_PROTO_DIR}/MarketRuleRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/MatchingSymbolsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsArticle.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsArticleRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsBulletin.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsBulletinsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsProvider.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsProviders.pb.cc"
        "${IBAPI_PROTO_DIR}/NewsProvidersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/NextValidId.pb.cc"
        "${IBAPI_PROTO_DIR}/OpenOrder.pb.cc"
        "${IBAPI_PROTO_DIR}/OpenOrdersEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/OpenOrdersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/Order.pb.cc"
        "${IBAPI_PROTO_DIR}/OrderAllocation.pb.cc"
        "${IBAPI_PROTO_DIR}/OrderBound.pb.cc"
        "${IBAPI_PROTO_DIR}/OrderCancel.pb.cc"
        "${IBAPI_PROTO_DIR}/OrderCondition.pb.cc"
        "${IBAPI_PROTO_DIR}/OrderState.pb.cc"
        "${IBAPI_PROTO_DIR}/OrderStatus.pb.cc"
        "${IBAPI_PROTO_DIR}/PlaceOrderRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/PnL.pb.cc"
        "${IBAPI_PROTO_DIR}/PnLRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/PnLSingle.pb.cc"
        "${IBAPI_PROTO_DIR}/PnLSingleRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/PortfolioValue.pb.cc"
        "${IBAPI_PROTO_DIR}/Position.pb.cc"
        "${IBAPI_PROTO_DIR}/PositionEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/PositionMulti.pb.cc"
        "${IBAPI_PROTO_DIR}/PositionMultiEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/PositionsMultiRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/PositionsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/PriceIncrement.pb.cc"
        "${IBAPI_PROTO_DIR}/QueryDisplayGroupsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/RealTimeBarTick.pb.cc"
        "${IBAPI_PROTO_DIR}/RealTimeBarsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/ReceiveFA.pb.cc"
        "${IBAPI_PROTO_DIR}/ReplaceFAEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/RerouteMarketDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/RerouteMarketDepthRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/ScannerData.pb.cc"
        "${IBAPI_PROTO_DIR}/ScannerDataElement.pb.cc"
        "${IBAPI_PROTO_DIR}/ScannerParameters.pb.cc"
        "${IBAPI_PROTO_DIR}/ScannerParametersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/ScannerSubscription.pb.cc"
        "${IBAPI_PROTO_DIR}/ScannerSubscriptionRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/SecDefOptParameter.pb.cc"
        "${IBAPI_PROTO_DIR}/SecDefOptParameterEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/SecDefOptParamsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/SetServerLogLevelRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/SmartComponent.pb.cc"
        "${IBAPI_PROTO_DIR}/SmartComponents.pb.cc"
        "${IBAPI_PROTO_DIR}/SmartComponentsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/SoftDollarTier.pb.cc"
        "${IBAPI_PROTO_DIR}/SoftDollarTiers.pb.cc"
        "${IBAPI_PROTO_DIR}/SoftDollarTiersRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/StartApiRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/SubscribeToGroupEventsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/SymbolSamples.pb.cc"
        "${IBAPI_PROTO_DIR}/TickAttribBidAsk.pb.cc"
        "${IBAPI_PROTO_DIR}/TickAttribLast.pb.cc"
        "${IBAPI_PROTO_DIR}/TickByTickData.pb.cc"
        "${IBAPI_PROTO_DIR}/TickByTickRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/TickGeneric.pb.cc"
        "${IBAPI_PROTO_DIR}/TickNews.pb.cc"
        "${IBAPI_PROTO_DIR}/TickOptionComputation.pb.cc"
        "${IBAPI_PROTO_DIR}/TickPrice.pb.cc"
        "${IBAPI_PROTO_DIR}/TickReqParams.pb.cc"
        "${IBAPI_PROTO_DIR}/TickSize.pb.cc"
        "${IBAPI_PROTO_DIR}/TickSnapshotEnd.pb.cc"
        "${IBAPI_PROTO_DIR}/TickString.pb.cc"
        "${IBAPI_PROTO_DIR}/UnsubscribeFromGroupEventsRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/UpdateDisplayGroupRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/UserInfo.pb.cc"
        "${IBAPI_PROTO_DIR}/UserInfoRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/VerifyCompleted.pb.cc"
        "${IBAPI_PROTO_DIR}/VerifyMessageApi.pb.cc"
        "${IBAPI_PROTO_DIR}/VerifyMessageRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/VerifyRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/WshEventData.pb.cc"
        "${IBAPI_PROTO_DIR}/WshEventDataRequest.pb.cc"
        "${IBAPI_PROTO_DIR}/WshMetaData.pb.cc"
        "${IBAPI_PROTO_DIR}/WshMetaDataRequest.pb.cc"
    )
    add_library(ibapi STATIC ${IBAPI_SOURCES} ${IBAPI_PROTO_SOURCES})
    target_include_directories(ibapi PUBLIC ${IBAPI_ROOT} ${IBAPI_PROTO_DIR})
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(ibapi PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-parameter
        )
    elseif(MSVC)
        target_compile_options(ibapi PRIVATE /wd4996 /wd4100)
    endif()
    set(_regimeflow_ibapi_protobuf_version 3.21.12)
    set(_regimeflow_ibapi_protobuf_version_macro 3021012)
    file(READ "${IBAPI_PROTO_DIR}/ExecutionRequest.pb.h" _regimeflow_ibapi_proto_probe)
    if(NOT _regimeflow_ibapi_proto_probe MATCHES "${_regimeflow_ibapi_protobuf_version_macro}[ \t]*< PROTOBUF_MIN_PROTOC_VERSION")
        message(FATAL_ERROR
            "Bundled IB API protobuf stubs are not generated for protobuf "
            "${_regimeflow_ibapi_protobuf_version}; regenerate third_party/ibapi "
            "stubs or update the pinned protobuf version together.")
    endif()
    set(_regimeflow_required_protobuf_version ${_regimeflow_ibapi_protobuf_version})
    set(_regimeflow_protobuf_target "")
    find_package(Protobuf ${_regimeflow_required_protobuf_version} QUIET)
    set(_protobuf_has_required_headers OFF)
    if(Protobuf_FOUND)
        foreach(_proto_inc IN LISTS Protobuf_INCLUDE_DIRS)
            if(EXISTS "${_proto_inc}/google/protobuf/port_def.inc"
               AND EXISTS "${_proto_inc}/google/protobuf/map_field_inl.h")
                set(_protobuf_has_required_headers ON)
                break()
            endif()
        endforeach()
        if(_protobuf_has_required_headers)
            if(TARGET protobuf::libprotobuf)
                set(_regimeflow_protobuf_target protobuf::libprotobuf)
            elseif(TARGET protobuf::protobuf)
                set(_regimeflow_protobuf_target protobuf::protobuf)
            endif()
        endif()
    endif()
    if(NOT Protobuf_FOUND OR NOT _protobuf_has_required_headers)
        if(REGIMEFLOW_FETCH_DEPS)
            include(FetchContent)
            FetchContent_Declare(
                protobuf
                URL https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.21.12.zip
            )
            set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
            set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(protobuf)
            set(Protobuf_FOUND TRUE)
            set(Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src")
            set(Protobuf_VERSION "${_regimeflow_required_protobuf_version}")
            if(TARGET libprotobuf)
                set(_regimeflow_protobuf_target libprotobuf)
            elseif(TARGET protobuf::libprotobuf)
                set(_regimeflow_protobuf_target protobuf::libprotobuf)
            endif()
        else()
            message(FATAL_ERROR "Protobuf >= ${_regimeflow_required_protobuf_version} is required for the bundled IB API generated sources.")
        endif()
    endif()
    target_include_directories(ibapi PUBLIC ${Protobuf_INCLUDE_DIRS})
    find_package(Threads REQUIRED)
    target_link_libraries(ibapi PUBLIC Threads::Threads)
    if(_regimeflow_protobuf_target)
        target_link_libraries(ibapi PUBLIC ${_regimeflow_protobuf_target})
    else()
        target_link_libraries(ibapi PUBLIC ${Protobuf_LIBRARIES})
    endif()
    set(IBAPI_NEEDS_BID_COMPAT OFF)
    find_library(IBAPI_BID_LIBRARY NAMES
        bid
        bidgcc000
        bidgcc001
        bidgcc010
        bidgcc011
        bidgcc100
        bidgcc101
        bidgcc110
        bidgcc111
        bidvc
        bidvc141
        bidvc142
        bidvc143
        bidvc144
        HINTS /opt/homebrew/lib /usr/local/lib ${_vcpkg_lib_hints}
    )
    if(IBAPI_BID_LIBRARY)
        target_link_libraries(ibapi PUBLIC ${IBAPI_BID_LIBRARY})
    endif()
    if((APPLE OR WIN32) AND NOT IBAPI_BID_LIBRARY)
        set(IBAPI_NEEDS_BID_COMPAT ON)
    endif()
    if(IBAPI_NEEDS_BID_COMPAT)
        target_sources(ibapi PRIVATE ${CMAKE_SOURCE_DIR}/src/live/ib_bid_compat.cpp)
        target_compile_definitions(ibapi PUBLIC REGIMEFLOW_IBAPI_SOFTBID)
    endif()
endif()

set(REGIMEFLOW_PUBLIC_INCLUDE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

function(regimeflow_target_public_include target)
    target_include_directories(${target} PUBLIC ${REGIMEFLOW_PUBLIC_INCLUDE})
endfunction()

function(regimeflow_enable_windows_exports target)
    if(WIN32 AND BUILD_SHARED_LIBS)
        set_target_properties(${target} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endfunction()

regimeflow_target_public_include(regimeflow_common)
regimeflow_enable_windows_exports(regimeflow_common)

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
regimeflow_enable_windows_exports(regimeflow_data)

target_link_libraries(regimeflow_data
    PUBLIC
        regimeflow_common
        regimeflow_plugins
)

if(ENABLE_POSTGRES)
find_package(PostgreSQL QUIET)
if(PostgreSQL_FOUND)
    target_compile_definitions(regimeflow_data PRIVATE REGIMEFLOW_USE_LIBPQ)
    if(TARGET PostgreSQL::PostgreSQL)
        target_link_libraries(regimeflow_data PRIVATE PostgreSQL::PostgreSQL)
    else()
        target_include_directories(regimeflow_data PRIVATE ${PostgreSQL_INCLUDE_DIRS})
        target_link_libraries(regimeflow_data PRIVATE ${PostgreSQL_LIBRARIES})
    endif()
    set(REGIMEFLOW_PACKAGE_NEEDS_POSTGRES ON PARENT_SCOPE)
endif()
endif()

if(ENABLE_CURL)
find_package(CURL QUIET)
if(CURL_FOUND)
    target_compile_definitions(regimeflow_data PRIVATE REGIMEFLOW_USE_CURL)
    if(TARGET CURL::libcurl)
        target_link_libraries(regimeflow_data PRIVATE CURL::libcurl)
    else()
        target_include_directories(regimeflow_data PRIVATE ${CURL_INCLUDE_DIRS})
        target_link_libraries(regimeflow_data PRIVATE ${CURL_LIBRARIES})
    endif()
    set(REGIMEFLOW_PACKAGE_NEEDS_CURL ON PARENT_SCOPE)
endif()
endif()

if(ENABLE_WEBSOCKET)
    find_package(Boost QUIET COMPONENTS system)
    set(REGIMEFLOW_BOOST_WEBSOCKET_HEADERS_FOUND OFF)
    if(Boost_FOUND)
        foreach(_boost_include IN LISTS Boost_INCLUDE_DIRS)
            if(EXISTS "${_boost_include}/boost/asio/io_context.hpp"
               AND EXISTS "${_boost_include}/boost/beast/core.hpp")
                set(REGIMEFLOW_BOOST_WEBSOCKET_HEADERS_FOUND ON)
                break()
            endif()
        endforeach()
    endif()
    if(Boost_FOUND AND REGIMEFLOW_BOOST_WEBSOCKET_HEADERS_FOUND)
        target_compile_definitions(regimeflow_data PUBLIC REGIMEFLOW_USE_BOOST_BEAST)
        if(TARGET Boost::headers)
            target_link_libraries(regimeflow_data PUBLIC Boost::headers Boost::system)
        elseif(TARGET Boost::boost)
            target_link_libraries(regimeflow_data PUBLIC Boost::boost Boost::system)
        else()
            target_include_directories(regimeflow_data
                PUBLIC
                    $<BUILD_INTERFACE:${Boost_INCLUDE_DIRS}>
            )
            target_link_libraries(regimeflow_data PUBLIC Boost::system)
        endif()
        set(REGIMEFLOW_PACKAGE_NEEDS_BOOST_SYSTEM ON PARENT_SCOPE)
    elseif(REGIMEFLOW_FETCH_DEPS)
        message(FATAL_ERROR "ENABLE_WEBSOCKET=ON requires Boost.Asio, Boost.Beast, and Boost.System; install Boost or configure with -DENABLE_WEBSOCKET=OFF.")
    endif()
endif()

if(ENABLE_OPENSSL)
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_compile_definitions(regimeflow_data PUBLIC REGIMEFLOW_USE_OPENSSL)
    target_link_libraries(regimeflow_data PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    set(REGIMEFLOW_PACKAGE_NEEDS_OPENSSL ON PARENT_SCOPE)
endif()
endif()

add_executable(regimeflow_mmap_builder tools/mmap_builder.cpp)
target_link_libraries(regimeflow_mmap_builder PRIVATE regimeflow_data)

add_executable(regimeflow_alpaca_fetch tools/alpaca_fetch_main.cpp)
target_link_libraries(regimeflow_alpaca_fetch PRIVATE regimeflow_data)

add_library(regimeflow_engine
    engine/audit_log.cpp
    engine/backtest_engine.cpp
    engine/backtest_results.cpp
    engine/backtest_runner.cpp
    engine/dashboard_snapshot.cpp
    engine/engine_factory.cpp
    engine/execution_pipeline.cpp
    engine/event_generator.cpp
    engine/event_loop.cpp
    engine/market_data_cache.cpp
    engine/order_book_cache.cpp
    engine/order.cpp
    engine/order_manager.cpp
    engine/order_routing.cpp
    engine/portfolio.cpp
    engine/regime_tracker.cpp
    engine/parity_checker.cpp
    engine/replay_journal.cpp
    engine/timer_service.cpp
)

regimeflow_target_public_include(regimeflow_engine)
regimeflow_enable_windows_exports(regimeflow_engine)

target_link_libraries(regimeflow_engine
    PUBLIC
        regimeflow_common
        regimeflow_data
        regimeflow_execution
        regimeflow_risk
        regimeflow_metrics
        regimeflow_regime
        regimeflow_plugins
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
regimeflow_enable_windows_exports(regimeflow_execution)

target_link_libraries(regimeflow_execution
    PUBLIC
        regimeflow_common
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
regimeflow_enable_windows_exports(regimeflow_strategy)

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
regimeflow_enable_windows_exports(regimeflow_risk)

target_link_libraries(regimeflow_risk
    PUBLIC
        regimeflow_common
        regimeflow_plugins
)

add_library(regimeflow_metrics
    metrics/attribution.cpp
    metrics/drawdown.cpp
    metrics/live_performance.cpp
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
regimeflow_enable_windows_exports(regimeflow_metrics)

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
regimeflow_enable_windows_exports(regimeflow_regime)

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
regimeflow_enable_windows_exports(regimeflow_plugins)

if(UNIX AND NOT APPLE)
    target_link_libraries(regimeflow_plugins PUBLIC ${CMAKE_DL_LIBS})
endif()

add_library(regimeflow_walkforward
    walkforward/optimizer.cpp
)

regimeflow_target_public_include(regimeflow_walkforward)
regimeflow_enable_windows_exports(regimeflow_walkforward)

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
    live/execution_quality.cpp
    live/live_engine.cpp
    live/live_order_manager.cpp
    live/mq_adapter.cpp
    live/mq_codec.cpp
    live/prometheus_exporter.cpp
    live/secret_hygiene.cpp
    live/types.cpp
)

if(ENABLE_IBAPI)
    list(APPEND REGIMEFLOW_LIVE_SOURCES live/ib_adapter.cpp)
endif()

add_library(regimeflow_live ${REGIMEFLOW_LIVE_SOURCES})

regimeflow_target_public_include(regimeflow_live)
regimeflow_enable_windows_exports(regimeflow_live)

target_link_libraries(regimeflow_live
    PUBLIC
        regimeflow_common
        regimeflow_engine
        regimeflow_metrics
        regimeflow_strategy
        regimeflow_risk
        regimeflow_regime
        regimeflow_data
)

if(WIN32)
    target_link_libraries(regimeflow_live PRIVATE Psapi ws2_32)
    target_compile_definitions(regimeflow_live PRIVATE _WIN32_WINNT=0x0A00 NOMINMAX)
endif()

if(ENABLE_IBAPI)
    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_ENABLE_IBAPI)
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
    if(APPLE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
        if(PKG_CONFIG_FOUND)
            list(APPEND PKG_CONFIG_PATH "/opt/homebrew/lib/pkgconfig" "/opt/homebrew/share/pkgconfig")
        endif()
    endif()
    if(WIN32)
        set(_vcpkg_lib_suffix "")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_vcpkg_lib_suffix "/debug")
        endif()
        if(DEFINED VCPKG_INSTALLED_DIR)
            list(APPEND _vcpkg_lib_hints "${VCPKG_INSTALLED_DIR}/x64-windows${_vcpkg_lib_suffix}/lib")
            list(APPEND _vcpkg_inc_hints "${VCPKG_INSTALLED_DIR}/x64-windows/include")
        endif()
    endif()
    if(ENABLE_ZMQ)
        if(WIN32)
            # On Windows, prefer vcpkg CMake config targets to avoid bare library names.
            find_package(cppzmq CONFIG REQUIRED)
            target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_ZMQ)
            target_link_libraries(regimeflow_live PRIVATE cppzmq)
        else()
            if(PKG_CONFIG_FOUND)
                pkg_check_modules(ZMQ libzmq)
            endif()
            if(ZMQ_FOUND)
                target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_ZMQ)
                target_include_directories(regimeflow_live PRIVATE ${ZMQ_INCLUDE_DIRS})
                target_link_libraries(regimeflow_live PRIVATE ${ZMQ_LIBRARIES})
            else()
                find_library(ZMQ_LIBRARY NAMES libzmq zmq
                    HINTS ${_vcpkg_lib_hints}
                )
                if(ZMQ_LIBRARY)
                    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_ZMQ)
                    target_link_libraries(regimeflow_live PRIVATE ${ZMQ_LIBRARY})
                    if(_vcpkg_inc_hints)
                        target_include_directories(regimeflow_live PRIVATE ${_vcpkg_inc_hints})
                    endif()
                else()
                    message(FATAL_ERROR "ENABLE_ZMQ=ON requires libzmq/cppzmq; install ZeroMQ or configure with -DENABLE_ZMQ=OFF.")
                endif()
            endif()
        endif()
    endif()

    if(ENABLE_REDIS)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(REDIS IMPORTED_TARGET hiredis)
        endif()
        if(REDIS_FOUND)
            target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_REDIS)
            if(TARGET PkgConfig::REDIS)
                target_link_libraries(regimeflow_live PRIVATE PkgConfig::REDIS)
            else()
                target_include_directories(regimeflow_live PRIVATE ${REDIS_INCLUDE_DIRS})
                target_link_directories(regimeflow_live PRIVATE ${REDIS_LIBRARY_DIRS})
                target_link_libraries(regimeflow_live PRIVATE ${REDIS_LIBRARIES})
            endif()
        else()
            find_library(REDIS_LIBRARY hiredis
                HINTS /opt/homebrew/lib /usr/local/lib
            )
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
                    set_target_properties(hiredis PROPERTIES SYSTEM ON)
                    # hiredis headers are in the repo root; create a shim include dir
                    # so <hiredis/hiredis.h> resolves when building from source.
                    set(HIREDIS_SHIM_DIR "${hiredis_BINARY_DIR}/include")
                    file(MAKE_DIRECTORY "${HIREDIS_SHIM_DIR}/hiredis")
                    file(GLOB HIREDIS_HEADERS "${hiredis_SOURCE_DIR}/*.h")
                    file(COPY ${HIREDIS_HEADERS} DESTINATION "${HIREDIS_SHIM_DIR}/hiredis")
                    if(EXISTS "${hiredis_SOURCE_DIR}/adapters")
                        file(COPY "${hiredis_SOURCE_DIR}/adapters" DESTINATION "${HIREDIS_SHIM_DIR}/hiredis")
                    endif()
                    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_REDIS)
                    target_link_libraries(regimeflow_live PRIVATE hiredis)
                    target_include_directories(regimeflow_live SYSTEM PRIVATE "${HIREDIS_SHIM_DIR}")
                else()
                    message(FATAL_ERROR "REGIMEFLOW_FETCH_DEPS=ON fetched hiredis, but no hiredis target was exported.")
                endif()
            else()
                message(FATAL_ERROR "ENABLE_REDIS=ON requires hiredis; install hiredis, enable REGIMEFLOW_FETCH_DEPS, or configure with -DENABLE_REDIS=OFF.")
            endif()
        endif()
    endif()

    if(ENABLE_KAFKA)
        if(WIN32)
            # On Windows, use the vcpkg CMake target to get the correct lib path.
            find_package(RdKafka CONFIG REQUIRED)
            target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_KAFKA)
            target_link_libraries(regimeflow_live PRIVATE RdKafka::rdkafka)
        else()
            if(PKG_CONFIG_FOUND)
                pkg_check_modules(KAFKA rdkafka)
            endif()
            if(KAFKA_FOUND)
                target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_KAFKA)
                target_include_directories(regimeflow_live PRIVATE ${KAFKA_INCLUDE_DIRS})
                target_link_libraries(regimeflow_live PRIVATE ${KAFKA_LIBRARIES})
            else()
                find_library(KAFKA_LIBRARY NAMES rdkafka
                    HINTS ${_vcpkg_lib_hints}
                )
                if(KAFKA_LIBRARY)
                    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_KAFKA)
                    target_link_libraries(regimeflow_live PRIVATE ${KAFKA_LIBRARY})
                    if(_vcpkg_inc_hints)
                        target_include_directories(regimeflow_live PRIVATE ${_vcpkg_inc_hints})
                    endif()
                else()
                    message(FATAL_ERROR "ENABLE_KAFKA=ON requires librdkafka; install librdkafka or configure with -DENABLE_KAFKA=OFF.")
                endif()
            endif()
        endif()
    endif()
endif()

if(ENABLE_IBAPI AND TARGET ibapi)
    target_link_libraries(regimeflow_live PUBLIC ibapi)
    target_include_directories(regimeflow_live PRIVATE ${IBAPI_ROOT})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/live/mq_adapter.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-c99-extensions"
    )
endif()

if(CURL_FOUND)
    target_compile_definitions(regimeflow_live PRIVATE REGIMEFLOW_USE_CURL)
    if(TARGET CURL::libcurl)
        target_link_libraries(regimeflow_live PRIVATE CURL::libcurl)
    else()
        target_include_directories(regimeflow_live PRIVATE ${CURL_INCLUDE_DIRS})
        target_link_libraries(regimeflow_live PRIVATE ${CURL_LIBRARIES})
    endif()
endif()

add_executable(regimeflow_live_cli tools/live_main.cpp)
target_link_libraries(regimeflow_live_cli PRIVATE regimeflow_live)
set_target_properties(regimeflow_live_cli PROPERTIES OUTPUT_NAME regimeflow_live)

add_executable(regimeflow_live_validate tools/live_validation_main.cpp)
target_link_libraries(regimeflow_live_validate PRIVATE regimeflow_live)

add_executable(regimeflow_parity_check tools/parity_check_main.cpp)
target_link_libraries(regimeflow_parity_check PRIVATE regimeflow_engine regimeflow_common)

add_executable(regimeflow_backtest_controls_demo tools/backtest_controls_demo.cpp)
target_link_libraries(regimeflow_backtest_controls_demo PRIVATE
    regimeflow_engine
    regimeflow_strategy
    regimeflow_execution
    regimeflow_data
    regimeflow_risk
    regimeflow_metrics
    regimeflow_regime
    regimeflow_plugins
    regimeflow_common
)

add_executable(regimeflow_strategy_tester tools/strategy_tester_main.cpp)
target_link_libraries(regimeflow_strategy_tester PRIVATE
    regimeflow_engine
    regimeflow_strategy
    regimeflow_execution
    regimeflow_data
    regimeflow_risk
    regimeflow_metrics
    regimeflow_regime
    regimeflow_plugins
    regimeflow_common
)
