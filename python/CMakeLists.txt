find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module NumPy)
if (REGIMEFLOW_FETCH_DEPS)
    find_package(pybind11 CONFIG QUIET)
    if (NOT pybind11_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            pybind11
            URL https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.zip
        )
        FetchContent_MakeAvailable(pybind11)
    endif()
else()
    find_package(pybind11 CONFIG REQUIRED)
endif()

pybind11_add_module(_core bindings.cpp)

set_target_properties(_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/python
)

if(WIN32)
    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(_REGIMEFLOW_VCPKG_TRIPLET "${VCPKG_TARGET_TRIPLET}")
    else()
        set(_REGIMEFLOW_VCPKG_TRIPLET "x64-windows")
    endif()
    set(_REGIMEFLOW_VCPKG_BIN "${CMAKE_SOURCE_DIR}/vcpkg_installed/${_REGIMEFLOW_VCPKG_TRIPLET}/bin")
    set(_REGIMEFLOW_VCPKG_DEBUG_BIN "${CMAKE_SOURCE_DIR}/vcpkg_installed/${_REGIMEFLOW_VCPKG_TRIPLET}/debug/bin")
    add_custom_command(TARGET _core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/python
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${_REGIMEFLOW_VCPKG_BIN} ${CMAKE_BINARY_DIR}/python
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${_REGIMEFLOW_VCPKG_DEBUG_BIN} ${CMAKE_BINARY_DIR}/python
    )
endif()

target_link_libraries(_core
    PRIVATE
        regimeflow_engine
        regimeflow_data
        regimeflow_strategy
        regimeflow_regime
        regimeflow_execution
        regimeflow_risk
        regimeflow_metrics
        regimeflow_walkforward
        regimeflow_common
        Python3::NumPy
)

install(TARGETS _core DESTINATION regimeflow)
