vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        curl ENABLE_CURL
        openssl ENABLE_OPENSSL
        postgres ENABLE_POSTGRES
        websocket ENABLE_WEBSOCKET
)

if(DEFINED ENV{REGIMEFLOW_VCPKG_SOURCE_PATH} AND EXISTS "$ENV{REGIMEFLOW_VCPKG_SOURCE_PATH}/CMakeLists.txt")
    set(SOURCE_PATH "$ENV{REGIMEFLOW_VCPKG_SOURCE_PATH}")
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO gregorian-09/regime-flow
        # Release source: v1.0.14
        # Pin the source commit that contains the 1.0.14 code. The overlay port lives in
        # this repository, so pinning the preceding source commit avoids a self-referential
        # archive checksum while preserving immutable source provenance.
        REF a0772535fbaade4660fe9066a645ea1fb7732921
        SHA512 a7ab7c177f2d32f7f78d69c19ca81ce3ee21e7b90ea178f3d31f840e34be5e3a1dec7e447a49bb193a5346b9cc09abb3592ec9e50485055731da9f870b69c155
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
