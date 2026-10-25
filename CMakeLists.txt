cmake_minimum_required(VERSION 3.30)

# Every library in the collection, in dependency order.
# Keep in sync with aest/src/aest.ixx and ports/aest/vcpkg.json.
set(AEST_ALL_LIBS colors)

set(AEST_LIBS "${AEST_ALL_LIBS}" CACHE STRING "Libraries to build (subset of: ${AEST_ALL_LIBS})")
option(AEST_BUILD_META "Build the umbrella 'aest' module" ON)

# PROJECT_IS_TOP_LEVEL is only set by project(), but AEST_BUILD_TESTS must be
# known before it to select the vcpkg manifest features.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(aest_is_top_level ON)
else()
    set(aest_is_top_level OFF)
endif()

option(AEST_BUILD_TESTS "Build the unit tests" ${aest_is_top_level})
option(AEST_BUILD_EXAMPLES "Build the examples" OFF)
option(AEST_BUILD_DOCS "Build the Doxygen documentation" OFF)
option(AEST_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(AEST_USE_VCPKG "Use vcpkg (VCPKG_ROOT or the vcpkg submodule) for dependencies" ON)

if(AEST_USE_VCPKG)
    include(cmake/VcpkgToolchain.cmake)
endif()

if(AEST_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(aest
    VERSION 0.0.1
    DESCRIPTION "Collection of C++ utility libraries"
    HOMEPAGE_URL "https://github.com/aetherstrata/aest"
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AestLibrary)

if(AEST_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 CONFIG REQUIRED)
    include(Catch)
endif()

foreach(lib IN LISTS AEST_LIBS)
    if(NOT lib IN_LIST AEST_ALL_LIBS)
        message(FATAL_ERROR "Unknown aest library '${lib}' (known: ${AEST_ALL_LIBS})")
    endif()
    add_subdirectory(${lib})
endforeach()

if(AEST_BUILD_META)
    add_subdirectory(aest)
endif()

if(AEST_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(AEST_DOXYGEN_INPUT "\"${CMAKE_CURRENT_SOURCE_DIR}/README.md\" \"${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md\"")
    foreach(lib IN LISTS AEST_ALL_LIBS ITEMS aest)
        foreach(dir README.md src docs include)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${lib}/${dir}")
                string(APPEND AEST_DOXYGEN_INPUT " \"${CMAKE_CURRENT_SOURCE_DIR}/${lib}/${dir}\"")
            endif()
        endforeach()
    endforeach()
    if(AEST_WARNINGS_AS_ERRORS)
        set(AEST_DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS)
    else()
        set(AEST_DOXYGEN_WARN_AS_ERROR NO)
    endif()
    configure_file(Doxyfile.in "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(aest_docs
        COMMAND Doxygen::doxygen "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating Doxygen documentation"
        VERBATIM)
endif()
