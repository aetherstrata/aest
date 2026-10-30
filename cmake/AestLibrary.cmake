#[=======================================================================[.rst:
AestLibrary
-----------

Helpers shared by every library of the collection, so that each library's
``CMakeLists.txt`` only lists its sources.

Naming, for a library ``<name>``:

* target ``aest_<name>``, alias and exported name ``aest::<name>``
* CMake package and vcpkg port ``aest-<name>``
* module ``aest.<name>``, namespace ``aest::<name>``

The umbrella library is the exception: ``aest`` names the target alias,
package, port and module at the same time.
#]=======================================================================]

include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(_aest_config_template "${CMAKE_CURRENT_LIST_DIR}/aest-package-config.cmake.in")

# Returns the CMake package name for library <name>.
function(_aest_package_name name out_var)
    if(name STREQUAL "aest")
        set(${out_var} "aest" PARENT_SCOPE)
    else()
        set(${out_var} "aest-${name}" PARENT_SCOPE)
    endif()
endfunction()

#[=======================================================================[.rst:
.. command:: aest_add_library

  ::

    aest_add_library(<name>
        MODULES <file>...
        [SOURCES <file>...]
        [DEPENDS <target>...]
        [FIND_PACKAGES <package>...])

  Creates the static library ``aest::<name>`` from the module units in
  ``MODULES`` (paths relative to the library folder, e.g. ``src/<name>.cppm``) and the
  module implementation units in ``SOURCES``, then installs it as the CMake
  package ``aest-<name>``.

  ``DEPENDS`` lists ``aest::<lib>`` targets; each becomes a public link
  dependency and a ``find_dependency(aest-<lib>)`` in the package config.
  A dependency not built in this tree, as when a port builds one library,
  is found as the installed package ``aest-<lib>``.
  ``FIND_PACKAGES`` lists extra ``find_dependency()`` arguments, one package
  per entry, for third-party dependencies linked by the caller.

  If the library folder has an ``include/`` directory its headers are exposed
  as a public header file set. Only do this for unavoidable macros, and keep
  them under ``include/aest/<name>/`` so include paths stay unique.
#]=======================================================================]
function(aest_add_library name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "MODULES;SOURCES;DEPENDS;FIND_PACKAGES")
    if(NOT arg_MODULES)
        message(FATAL_ERROR "aest_add_library(${name}): MODULES is required")
    endif()

    set(target "aest_${name}")
    _aest_package_name(${name} package)

    # Sources are flat in src/; the install tree separates libraries instead.
    if(name STREQUAL "aest")
        set(module_install_dir "${CMAKE_INSTALL_INCLUDEDIR}/aest")
    else()
        set(module_install_dir "${CMAKE_INSTALL_INCLUDEDIR}/aest/${name}")
    endif()

    add_library(${target} STATIC)
    add_library(aest::${name} ALIAS ${target})
    set_target_properties(${target} PROPERTIES
        EXPORT_NAME ${name}
        OUTPUT_NAME ${package}
        VERSION ${PROJECT_VERSION})
    target_compile_features(${target} PUBLIC cxx_std_20)

    target_sources(${target}
        PUBLIC FILE_SET CXX_MODULES BASE_DIRS src FILES ${arg_MODULES}
        PRIVATE ${arg_SOURCES})

    set(header_install_args "")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
        file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
        target_sources(${target} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES ${headers})
        set(header_install_args FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    foreach(dep IN LISTS arg_DEPENDS)
        if(dep MATCHES "^aest::(.+)$" AND NOT TARGET ${dep})
            _aest_package_name(${CMAKE_MATCH_1} dep_package)
            find_package(${dep_package} ${PROJECT_VERSION} CONFIG REQUIRED)
        endif()
    endforeach()
    if(arg_DEPENDS)
        target_link_libraries(${target} PUBLIC ${arg_DEPENDS})
    endif()

    _aest_set_warnings(${target})

    # Installation and package config.
    install(TARGETS ${target}
        EXPORT ${package}-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET CXX_MODULES DESTINATION ${module_install_dir}
        ${header_install_args})

    set(config_dir "${CMAKE_INSTALL_DATADIR}/${package}")
    install(EXPORT ${package}-targets
        NAMESPACE aest::
        DESTINATION ${config_dir}
        CXX_MODULES_DIRECTORY cxx-modules)

    set(AEST_PACKAGE ${package})
    set(AEST_PACKAGE_DEPENDENCIES "")
    foreach(dep IN LISTS arg_DEPENDS)
        if(dep MATCHES "^aest::(.+)$")
            _aest_package_name(${CMAKE_MATCH_1} dep_package)
            string(APPEND AEST_PACKAGE_DEPENDENCIES
                "find_dependency(${dep_package} ${PROJECT_VERSION} CONFIG)\n")
        endif()
    endforeach()
    foreach(dep IN LISTS arg_FIND_PACKAGES)
        string(APPEND AEST_PACKAGE_DEPENDENCIES "find_dependency(${dep})\n")
    endforeach()

    set(config_build_dir "${CMAKE_CURRENT_BINARY_DIR}/${package}")
    configure_package_config_file("${_aest_config_template}"
        "${config_build_dir}/${package}-config.cmake"
        INSTALL_DESTINATION ${config_dir})
    write_basic_package_version_file("${config_build_dir}/${package}-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMinorVersion)
    install(FILES
        "${config_build_dir}/${package}-config.cmake"
        "${config_build_dir}/${package}-config-version.cmake"
        DESTINATION ${config_dir})
endfunction()

#[=======================================================================[.rst:
.. command:: aest_add_tests

  ::

    aest_add_tests(<name> SOURCES <file>...)

  Builds the Catch2 test executable ``aest_<name>_tests`` linked against
  ``aest::<name>`` and registers its test cases with CTest. Does nothing
  unless ``AEST_BUILD_TESTS`` is enabled.
#]=======================================================================]
function(aest_add_tests name)
    if(NOT AEST_BUILD_TESTS)
        return()
    endif()
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES")

    set(target "aest_${name}_tests")
    add_executable(${target} ${arg_SOURCES})
    target_link_libraries(${target} PRIVATE aest::${name} Catch2::Catch2WithMain)
    _aest_set_warnings(${target})
    catch_discover_tests(${target} TEST_PREFIX "${name}: ")
endfunction()

#[=======================================================================[.rst:
.. command:: aest_add_examples

  ::

    aest_add_examples(<name> SOURCES <file>...)

  Builds one executable per source, named ``aest_<name>_example_<stem>`` and
  linked against ``aest::<name>``. Does nothing unless
  ``AEST_BUILD_EXAMPLES`` is enabled.
#]=======================================================================]
function(aest_add_examples name)
    if(NOT AEST_BUILD_EXAMPLES)
        return()
    endif()
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES")

    foreach(source IN LISTS arg_SOURCES)
        get_filename_component(stem ${source} NAME_WE)
        set(target "aest_${name}_example_${stem}")
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE aest::${name})
        _aest_set_warnings(${target})
    endforeach()
endfunction()

function(_aest_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- $<$<BOOL:${AEST_WARNINGS_AS_ERRORS}>:/WX>)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic $<$<BOOL:${AEST_WARNINGS_AS_ERRORS}>:-Werror>)
    endif()
endfunction()
