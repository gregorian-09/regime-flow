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
        # Release source: v1.0.16
        # Pin the source commit that contains the 1.0.16 code. The overlay port lives in
        # this repository, so pinning the preceding source commit avoids a self-referential
        # archive checksum while preserving immutable source provenance.
        REF 5d8c5e3a3fe3a085062808084966ce15c014f288
        SHA512 27c5a96d95ac1037acb68528027cc78667a817775e350debe55c948aea49e20a5dac1bd38002c1219ad17e7bb417ad7ca9a46b51ba33a1c1376e464a2f981388
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
