cmake_minimum_required(VERSION 3.20)
project(custom_regime_plugins CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED REGIMEFLOW_ROOT)
  message(FATAL_ERROR "REGIMEFLOW_ROOT not set (path to repo root)")
endif()
if(NOT DEFINED REGIMEFLOW_BUILD)
  message(FATAL_ERROR "REGIMEFLOW_BUILD not set (path to build directory)")
endif()

add_library(custom_regime_detector SHARED
  custom_regime_detector.cpp
)
add_library(custom_regime_strategy SHARED
  custom_regime_strategy.cpp
)

target_include_directories(custom_regime_detector PRIVATE ${REGIMEFLOW_ROOT}/include)
target_include_directories(custom_regime_strategy PRIVATE ${REGIMEFLOW_ROOT}/include)

target_link_directories(custom_regime_detector PRIVATE ${REGIMEFLOW_BUILD}/lib)
target_link_directories(custom_regime_strategy PRIVATE ${REGIMEFLOW_BUILD}/lib)

target_link_libraries(custom_regime_detector PRIVATE
  regimeflow_common
  regimeflow_regime
  regimeflow_plugins
)
target_link_libraries(custom_regime_strategy PRIVATE
  regimeflow_common
  regimeflow_engine
  regimeflow_strategy
  regimeflow_plugins
)

set_target_properties(custom_regime_detector PROPERTIES OUTPUT_NAME custom_regime_detector)
set_target_properties(custom_regime_strategy PROPERTIES OUTPUT_NAME custom_regime_strategy)
