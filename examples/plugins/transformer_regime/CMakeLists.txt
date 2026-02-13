cmake_minimum_required(VERSION 3.20)
project(transformer_regime_plugin CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED REGIMEFLOW_ROOT)
  message(FATAL_ERROR "REGIMEFLOW_ROOT not set (path to repo root)")
endif()
if(NOT DEFINED REGIMEFLOW_BUILD)
  message(FATAL_ERROR "REGIMEFLOW_BUILD not set (path to build directory)")
endif()

add_library(transformer_regime_detector SHARED
  transformer_regime_detector.cpp
)

target_include_directories(transformer_regime_detector PRIVATE ${REGIMEFLOW_ROOT}/include)

target_link_directories(transformer_regime_detector PRIVATE ${REGIMEFLOW_BUILD}/lib)

target_link_libraries(transformer_regime_detector PRIVATE
  -Wl,--start-group
  regimeflow_common
  regimeflow_regime
  regimeflow_plugins
  -Wl,--end-group
)

set_target_properties(transformer_regime_detector PROPERTIES OUTPUT_NAME transformer_regime_detector)
