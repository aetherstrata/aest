# Selects the vcpkg toolchain; include before project().
#
# An explicit CMAKE_TOOLCHAIN_FILE wins, then a vcpkg checkout named by the
# VCPKG_ROOT environment variable, then the vcpkg submodule of this repo. The
# vcpkg toolchain bootstraps the vcpkg executable on first use.

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    return()
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _aest_vcpkg_root)
else()
    cmake_path(SET _aest_vcpkg_root NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../vcpkg")
endif()

set(_aest_vcpkg_toolchain "${_aest_vcpkg_root}/scripts/buildsystems/vcpkg.cmake")
if(NOT EXISTS "${_aest_vcpkg_toolchain}")
    message(FATAL_ERROR
        "vcpkg not found at '${_aest_vcpkg_root}'. "
        "Run `git submodule update --init` or point VCPKG_ROOT to a vcpkg checkout.")
endif()

set(CMAKE_TOOLCHAIN_FILE "${_aest_vcpkg_toolchain}" CACHE FILEPATH "vcpkg toolchain file")
unset(_aest_vcpkg_root)
unset(_aest_vcpkg_toolchain)
