cmake_minimum_required(VERSION 3.20)

project(RegimeFlowVcpkgConsumer LANGUAGES CXX)

find_package(RegimeFlow CONFIG REQUIRED)

add_executable(regimeflow_vcpkg_consumer main.cpp)
target_compile_features(regimeflow_vcpkg_consumer PRIVATE cxx_std_20)
target_link_libraries(regimeflow_vcpkg_consumer PRIVATE
    RegimeFlow::regimeflow_engine
    RegimeFlow::regimeflow_strategy
)
