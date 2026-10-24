# Umbrella library: its only job is to re-export every library of the
# collection. Libraries not built in this tree (e.g. when building the `aest`
# vcpkg port) are found as installed packages instead.
set(aest_deps "")
foreach(lib IN LISTS AEST_ALL_LIBS)
    if(NOT TARGET aest::${lib})
        find_package(aest-${lib} ${PROJECT_VERSION} CONFIG REQUIRED)
    endif()
    list(APPEND aest_deps aest::${lib})
endforeach()

aest_add_library(aest
    MODULES src/aest.ixx
    DEPENDS ${aest_deps})
