cmake_minimum_required(VERSION 3.20)
project(regimeflow_data_source_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REGIMEFLOW_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH "RegimeFlow source checkout")

add_library(regimeflow_data_source_template SHARED
    data_source_template.cpp
)

target_include_directories(regimeflow_data_source_template
    PRIVATE
        "${REGIMEFLOW_SOURCE_DIR}/include"
)

if(WIN32)
    target_compile_definitions(regimeflow_data_source_template PRIVATE NOMINMAX)
endif()
