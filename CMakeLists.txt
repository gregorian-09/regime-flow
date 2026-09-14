cmake_minimum_required(VERSION 3.20)
project(RegimeFlow VERSION 1.0.15 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING
        "Minimum macOS deployment target" FORCE)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_SANITIZERS "Enable address/undefined sanitizers" OFF)
option(ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)
option(ENABLE_WERROR "Treat warnings as errors" OFF)
option(REGIMEFLOW_FETCH_DEPS "Fetch supported source dependencies when missing (explicit opt-in)" OFF)
option(ENABLE_OPENSSL "Enable OpenSSL/WSS support" ON)
option(ENABLE_WEBSOCKET "Enable Boost.Beast WebSocket support" ON)
option(ENABLE_CURL "Enable libcurl HTTP support" ON)
option(ENABLE_POSTGRES "Enable PostgreSQL data source" ON)
option(ENABLE_ZMQ "Enable ZeroMQ adapter" ON)
option(ENABLE_REDIS "Enable Redis adapter" ON)
option(ENABLE_KAFKA "Enable Kafka adapter" ON)
option(ENABLE_IBAPI "Enable Interactive Brokers adapter" ON)
set(CLANG_TIDY_CHECKS "-*,clang-analyzer-*,bugprone-use-after-move" CACHE STRING
    "Checks passed to clang-tidy when ENABLE_CLANG_TIDY is ON")

if(BUILD_PYTHON)
    set(BUILD_PYTHON_BINDINGS ON)
endif()

set(REGIMEFLOW_PACKAGE_NEEDS_BOOST_SYSTEM OFF)
set(REGIMEFLOW_PACKAGE_NEEDS_CURL OFF)
set(REGIMEFLOW_PACKAGE_NEEDS_OPENSSL OFF)
set(REGIMEFLOW_PACKAGE_NEEDS_POSTGRES OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(WIN32)
    if(BUILD_SHARED_LIBS)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

if(ENABLE_SANITIZERS AND NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "ENABLE_SANITIZERS requires GCC or Clang")
endif()

function(regimeflow_apply_sanitizers target)
    if(NOT ENABLE_SANITIZERS)
        return()
    endif()
    target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()

function(regimeflow_apply_sanitizers_to_directory directory)
    if(NOT ENABLE_SANITIZERS)
        return()
    endif()
    get_property(_regimeflow_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_regimeflow_target IN LISTS _regimeflow_targets)
        if(_regimeflow_target MATCHES "^(regimeflow_|_core)")
            regimeflow_apply_sanitizers(${_regimeflow_target})
        endif()
    endforeach()
endfunction()

# Keep project warning policy out of vendored and fetched dependencies.  Applying
# this after targets are declared is intentional: CMake permits target usage
# requirements to be added after target creation.
function(regimeflow_apply_project_options_to_directory directory)
    get_property(_regimeflow_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_regimeflow_target IN LISTS _regimeflow_targets)
        if(_regimeflow_target MATCHES "^(regimeflow_|_core)")
            target_compile_options(${_regimeflow_target} PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
                $<$<CXX_COMPILER_ID:MSVC>:/W4>
            )
            if(WIN32)
                target_compile_definitions(${_regimeflow_target} PRIVATE
                    _WIN32_WINNT=0x0601
                    _WIN32_WINDOWS=0x0601
                )
            endif()
            if(ENABLE_WERROR)
                target_compile_options(${_regimeflow_target} PRIVATE
                    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>
                    $<$<CXX_COMPILER_ID:MSVC>:/WX>
                )
            endif()
        endif()
    endforeach()
endfunction()

if(ENABLE_CLANG_TIDY)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_CLANG_TIDY requires GCC or Clang-compatible command lines")
    endif()

    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16)
    if(NOT CLANG_TIDY_EXE)
        message(FATAL_ERROR "ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()

    set(CMAKE_CXX_CLANG_TIDY
        ${CLANG_TIDY_EXE};
        -p=${CMAKE_BINARY_DIR};
        --checks=${CLANG_TIDY_CHECKS}
    )
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

regimeflow_apply_project_options_to_directory(${CMAKE_SOURCE_DIR}/src)
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests")
    regimeflow_apply_project_options_to_directory(${CMAKE_SOURCE_DIR}/tests)
endif()
if(BUILD_PYTHON_BINDINGS AND EXISTS "${CMAKE_SOURCE_DIR}/python")
    regimeflow_apply_project_options_to_directory(${CMAKE_SOURCE_DIR}/python)
endif()
if(BUILD_BENCHMARKS AND EXISTS "${CMAKE_SOURCE_DIR}/benchmarks")
    regimeflow_apply_project_options_to_directory(${CMAKE_SOURCE_DIR}/benchmarks)
endif()

regimeflow_apply_sanitizers_to_directory(${CMAKE_SOURCE_DIR}/src)
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests")
    regimeflow_apply_sanitizers_to_directory(${CMAKE_SOURCE_DIR}/tests)
endif()
if(BUILD_PYTHON_BINDINGS AND EXISTS "${CMAKE_SOURCE_DIR}/python")
    regimeflow_apply_sanitizers_to_directory(${CMAKE_SOURCE_DIR}/python)
endif()
if(BUILD_BENCHMARKS AND EXISTS "${CMAKE_SOURCE_DIR}/benchmarks")
    regimeflow_apply_sanitizers_to_directory(${CMAKE_SOURCE_DIR}/benchmarks)
endif()

if(BUILD_DOCS AND EXISTS "${CMAKE_SOURCE_DIR}/docs")
    find_package(Doxygen REQUIRED)
    add_subdirectory(docs)
endif()

install(DIRECTORY include/regimeflow DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS
    regimeflow_common
    regimeflow_data
    regimeflow_regime
    regimeflow_execution
    regimeflow_strategy
    regimeflow_risk
    regimeflow_metrics
    regimeflow_engine
    regimeflow_walkforward
    regimeflow_plugins
    EXPORT RegimeFlowTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
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
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RegimeFlow
)

install(EXPORT RegimeFlowTargets
    FILE RegimeFlowTargets.cmake
    NAMESPACE RegimeFlow::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RegimeFlow
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/RegimeFlowConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/RegimeFlowConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RegimeFlow
)
