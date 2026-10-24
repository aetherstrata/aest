# AEST_SOURCE_DIR (forwarded with VCPKG_KEEP_ENV_VARS) builds from a local
# checkout instead of the pinned commit; CI uses it to test the ports against
# the commit under test. Disable binary caching when using it: the local
# sources are not part of the package ABI hash.
if(DEFINED ENV{AEST_SOURCE_DIR})
    file(TO_CMAKE_PATH "$ENV{AEST_SOURCE_DIR}" SOURCE_PATH)
else()
    vcpkg_from_git(
        OUT_SOURCE_PATH SOURCE_PATH
        URL https://github.com/aetherstrata/aest
        REF 0000000000000000000000000000000000000000 # TODO: commit of release v${VERSION}
        HEAD_REF main)
endif()

# Modules export no dllexport annotations: static only for now.
vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DAEST_LIBS=colors
        -DAEST_BUILD_META=OFF
        -DAEST_BUILD_TESTS=OFF
        -DAEST_BUILD_EXAMPLES=OFF
        -DAEST_BUILD_DOCS=OFF)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME aest-colors)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.md")
