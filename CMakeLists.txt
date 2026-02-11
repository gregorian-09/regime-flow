cmake_minimum_required(VERSION 3.20)
project(RegimeFlow VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(ENABLE_WERROR "Treat warnings as errors" OFF)
option(REGIMEFLOW_FETCH_DEPS "Fetch lightweight deps via FetchContent when missing" OFF)
option(ENABLE_OPENSSL "Enable OpenSSL/WSS support" ON)
option(ENABLE_CURL "Enable libcurl HTTP support" ON)
option(ENABLE_POSTGRES "Enable PostgreSQL data source" ON)
option(ENABLE_ZMQ "Enable ZeroMQ adapter" ON)
option(ENABLE_REDIS "Enable Redis adapter" ON)
option(ENABLE_KAFKA "Enable Kafka adapter" ON)

if(BUILD_PYTHON)
    set(BUILD_PYTHON_BINDINGS ON)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(ENABLE_WERROR)
    add_compile_options(
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Werror>
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
    )
endif()

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(src)

if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_PYTHON_BINDINGS AND EXISTS "${CMAKE_SOURCE_DIR}/python")
    add_subdirectory(python)
endif()

if(BUILD_BENCHMARKS AND EXISTS "${CMAKE_SOURCE_DIR}/benchmarks")
    add_subdirectory(benchmarks)
endif()

if(BUILD_DOCS AND EXISTS "${CMAKE_SOURCE_DIR}/docs")
    find_package(Doxygen REQUIRED)
    add_subdirectory(docs)
endif()

install(DIRECTORY include/regimeflow DESTINATION include)
install(TARGETS
    regimeflow_common
    regimeflow_data
    regimeflow_regime
    regimeflow_execution
    regimeflow_risk
    regimeflow_metrics
    regimeflow_engine
    regimeflow_walkforward
    regimeflow_plugins
    EXPORT RegimeFlowTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/RegimeFlowConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/RegimeFlowConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/RegimeFlowConfig.cmake
    INSTALL_DESTINATION lib/cmake/RegimeFlow
)

install(EXPORT RegimeFlowTargets
    FILE RegimeFlowTargets.cmake
    NAMESPACE RegimeFlow::
    DESTINATION lib/cmake/RegimeFlow
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/RegimeFlowConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/RegimeFlowConfigVersion.cmake
    DESTINATION lib/cmake/RegimeFlow
)
