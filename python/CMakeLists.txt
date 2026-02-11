find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)
if (REGIMEFLOW_FETCH_DEPS)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        URL https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.zip
    )
    FetchContent_MakeAvailable(pybind11)
else()
    find_package(pybind11 CONFIG REQUIRED)
endif()

pybind11_add_module(_core bindings.cpp)

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
