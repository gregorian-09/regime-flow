cmake_minimum_required(VERSION 3.20)
project(custom_regime_backtest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED REGIMEFLOW_ROOT)
  message(FATAL_ERROR "REGIMEFLOW_ROOT not set (path to repo root)")
endif()
if(NOT DEFINED REGIMEFLOW_BUILD)
  message(FATAL_ERROR "REGIMEFLOW_BUILD not set (path to build directory)")
endif()

add_executable(run_custom_regime_backtest run_backtest.cpp)

target_include_directories(run_custom_regime_backtest PRIVATE
  ${REGIMEFLOW_ROOT}/include
)

target_link_directories(run_custom_regime_backtest PRIVATE
  ${REGIMEFLOW_BUILD}/lib
)

target_link_libraries(run_custom_regime_backtest PRIVATE
  regimeflow_common
  regimeflow_data
  regimeflow_engine
  regimeflow_strategy
  regimeflow_execution
  regimeflow_risk
  regimeflow_regime
  regimeflow_metrics
  regimeflow_plugins
)
