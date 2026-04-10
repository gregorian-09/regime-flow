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
        REF v1.0.9
        SHA512 fa56b8742dd6daea695b6f8563543b1ab87e65bc6b291cadc1b70625dce3d73de3f148a81f6f006aaf74b4e94a1be1dd3665339797c6ad1e131cac75be9a52ac
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
