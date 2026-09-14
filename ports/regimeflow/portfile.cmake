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
        # Release source: v1.0.13
        # Pin the source commit that contains the 1.0.13 code. The overlay port lives in
        # this repository, so pinning the preceding source commit avoids a self-referential
        # archive checksum while preserving immutable source provenance.
        REF b91a5d60829ff491478d49a3a44d2ce48696aee6
        SHA512 a169818b4518880b4767e3eff032b83dd2e4d9279a894b44540d38fcba12b5a05c9eb306bbaad0cf62d19f5c5af634c7e493680dd900ac43755764d2f6a73218
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
