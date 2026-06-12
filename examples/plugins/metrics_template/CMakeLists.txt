cmake_minimum_required(VERSION 3.20)
project(regimeflow_metrics_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REGIMEFLOW_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH "RegimeFlow source checkout")

add_library(regimeflow_metrics_template SHARED
    metrics_template.cpp
)

target_include_directories(regimeflow_metrics_template
    PRIVATE
        "${REGIMEFLOW_SOURCE_DIR}/include"
)

if(WIN32)
    target_compile_definitions(regimeflow_metrics_template PRIVATE NOMINMAX)
endif()
