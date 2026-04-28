vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        curl ENABLE_CURL
        openssl ENABLE_OPENSSL
        postgres ENABLE_POSTGRES
)

if(DEFINED ENV{REGIMEFLOW_VCPKG_SOURCE_PATH} AND EXISTS "$ENV{REGIMEFLOW_VCPKG_SOURCE_PATH}/CMakeLists.txt")
    set(SOURCE_PATH "$ENV{REGIMEFLOW_VCPKG_SOURCE_PATH}")
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO gregorian-09/regime-flow
        REF v1.0.10
        SHA512 54430cf8d89064606d3ed78f2e5917af3fbfd45c6811c6d41eec973eeeb00861ee1c68df7511b60757c348dfb8f54422e198c9fb1efd05e3121244c6e3213e35
    )
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_BENCHMARKS=OFF
        -DBUILD_PYTHON_BINDINGS=OFF
        -DENABLE_IBAPI=OFF
        -DENABLE_ZMQ=OFF
        -DENABLE_REDIS=OFF
        -DENABLE_KAFKA=OFF
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/RegimeFlow)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
