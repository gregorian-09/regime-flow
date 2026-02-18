if (NOT BUILD_TESTS)
    return()
endif()

find_package(GTest QUIET)
if (NOT GTest_FOUND)
    if (REGIMEFLOW_FETCH_DEPS)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    else()
        message(WARNING "GTest not found; skipping tests")
        return()
    endif()
endif()

add_executable(regimeflow_tests
    unit/test_hmm_features.cpp
    unit/test_hmm_baum_welch.cpp
    unit/test_hmm_persistence.cpp
    unit/test_hmm_normalization.cpp
    unit/test_ensemble_detector.cpp
    unit/test_regime_ensemble.cpp
    unit/test_backtest_parallel.cpp
    unit/test_backtest_hooks.cpp
    unit/test_db_csv_adapter.cpp
    unit/test_market_impact.cpp
    unit/test_yaml_config.cpp
    unit/test_yaml_nested.cpp
    unit/test_engine_factory_smoke.cpp
    unit/test_corporate_actions.cpp
    unit/test_report_writer.cpp
    unit/test_corporate_actions_csv.cpp
    unit/test_corporate_actions_symbol_change_csv.cpp
    unit/test_corporate_actions_symbol_change_memory.cpp
    unit/test_corporate_actions_dividend_csv.cpp
    unit/test_order_book_execution.cpp
    unit/test_event_generator_ordering.cpp
    unit/test_event_generator_system_events.cpp
    unit/test_event_queue.cpp
    unit/test_bar_builder.cpp
    unit/test_csv_normalization.cpp
    unit/test_data_validation.cpp
    unit/test_snapshot_access.cpp
    unit/test_merged_iterator.cpp
    unit/test_regime_metrics.cpp
    unit/test_regime_history.cpp
    unit/test_regime_state_manager.cpp
    unit/test_walkforward_optimizer.cpp
    unit/test_position_sizers.cpp
    unit/test_risk_limits.cpp
    unit/test_stop_loss.cpp
    unit/test_slippage_models.cpp
    unit/test_transaction_costs.cpp
    unit/test_performance_metrics.cpp
    unit/test_performance_calculator.cpp
    unit/test_live_order_reconcile.cpp
    unit/test_live_engine_integration.cpp
    unit/test_event_bus.cpp
    unit/test_plugin_lifecycle.cpp
    unit/test_dynamic_plugin_loading.cpp
    unit/test_websocket_tls_config.cpp
    unit/test_websocket_reconnect.cpp
)

add_library(regimeflow_test_plugin SHARED
    plugins/test_dynamic_plugin.cpp
)
set_target_properties(regimeflow_test_plugin PROPERTIES
    LIBRARY_OUTPUT_NAME "regimeflow_test_plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/plugins"
)
target_link_libraries(regimeflow_test_plugin PRIVATE regimeflow_plugins regimeflow_common)

target_link_libraries(regimeflow_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        regimeflow_engine
        regimeflow_data
        regimeflow_regime
        regimeflow_common
        regimeflow_execution
        regimeflow_risk
        regimeflow_metrics
        regimeflow_strategy
        regimeflow_plugins
        regimeflow_walkforward
        regimeflow_live
)

if(EXISTS "/usr/local/lib/libbid.a")
    if(TARGET ibapi)
        target_link_libraries(regimeflow_tests PRIVATE ibapi /usr/local/lib/libbid.a)
    else()
        target_link_libraries(regimeflow_tests PRIVATE /usr/local/lib/libbid.a)
    endif()
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
        target_link_libraries(regimeflow_tests PRIVATE ${BID_LIBRARY})
    endif()
endif()

target_compile_definitions(regimeflow_tests PRIVATE
    REGIMEFLOW_TEST_ROOT="${CMAKE_SOURCE_DIR}"
)

enable_testing()
add_test(NAME regimeflow_tests COMMAND regimeflow_tests)

find_package(Python3 QUIET COMPONENTS Interpreter)
set(PYTHON_BINDINGS_SMOKE_PYTHON "")
if(EXISTS "/opt/venv/bin/python")
    set(PYTHON_BINDINGS_SMOKE_PYTHON "/opt/venv/bin/python")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/.venv/bin/python")
    set(PYTHON_BINDINGS_SMOKE_PYTHON "${CMAKE_SOURCE_DIR}/.venv/bin/python")
elseif(Python3_Interpreter_FOUND)
    set(PYTHON_BINDINGS_SMOKE_PYTHON "${Python3_EXECUTABLE}")
endif()

if(PYTHON_BINDINGS_SMOKE_PYTHON)
    if(WIN32)
        set(PYTHONPATH_SEP ";")
    else()
        set(PYTHONPATH_SEP ":")
    endif()
    set(PYTHONPATH_VALUE "${CMAKE_BINARY_DIR}/lib${PYTHONPATH_SEP}${CMAKE_BINARY_DIR}/python${PYTHONPATH_SEP}${CMAKE_SOURCE_DIR}/python")
    if(WIN32)
        string(REPLACE ";" "\\;" PYTHONPATH_VALUE "${PYTHONPATH_VALUE}")
    endif()
    add_test(NAME python_numpy_check
        COMMAND ${CMAKE_COMMAND} -E env
            PYTHONPATH=${PYTHONPATH_VALUE}
            ${PYTHON_BINDINGS_SMOKE_PYTHON} ${CMAKE_SOURCE_DIR}/tests/python/check_numpy.py
    )
    add_test(NAME python_bindings_smoke
        COMMAND ${CMAKE_COMMAND} -E env
            REGIMEFLOW_TEST_ROOT=${CMAKE_SOURCE_DIR}
            PYTHONPATH=${PYTHONPATH_VALUE}
            ${PYTHON_BINDINGS_SMOKE_PYTHON} ${CMAKE_SOURCE_DIR}/tests/python/test_bindings_smoke.py
    )
    set_tests_properties(python_bindings_smoke PROPERTIES DEPENDS python_numpy_check)

    add_test(NAME python_pytest_check
        COMMAND ${CMAKE_COMMAND} -E env
            PYTHONPATH=${PYTHONPATH_VALUE}
            ${PYTHON_BINDINGS_SMOKE_PYTHON} ${CMAKE_SOURCE_DIR}/tests/python/check_pytest.py
    )
    add_test(NAME python_native_tests
        COMMAND ${CMAKE_COMMAND} -E env
            REGIMEFLOW_TEST_ROOT=${CMAKE_SOURCE_DIR}
            PYTHONPATH=${PYTHONPATH_VALUE}
            ${PYTHON_BINDINGS_SMOKE_PYTHON} -m pytest -q -m python_native ${CMAKE_SOURCE_DIR}/python/tests
    )
    set_tests_properties(python_native_tests PROPERTIES DEPENDS python_pytest_check)
endif()

add_executable(regimeflow_bench_event_processing
    performance/bench_event_processing.cpp
)

target_link_libraries(regimeflow_bench_event_processing
    PRIVATE
        regimeflow_engine
        regimeflow_data
        regimeflow_common
)

add_executable(regimeflow_bench_data_loading
    performance/bench_data_loading.cpp
)

target_link_libraries(regimeflow_bench_data_loading
    PRIVATE
        regimeflow_engine
        regimeflow_data
        regimeflow_common
)

add_executable(regimeflow_bench_allocator_rollout
    performance/bench_allocator_rollout.cpp
)

target_link_libraries(regimeflow_bench_allocator_rollout
    PRIVATE
        regimeflow_common
)
