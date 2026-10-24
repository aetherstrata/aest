# Consumer project used by CI to check that the installed packages (from
# `cmake --install` or from the vcpkg ports) are usable.
cmake_minimum_required(VERSION 3.30)

include(../../cmake/VcpkgToolchain.cmake)

project(aest_package_test LANGUAGES CXX)

find_package(aest CONFIG REQUIRED)

add_executable(aest_package_test main.cpp)
target_link_libraries(aest_package_test PRIVATE aest::aest)

enable_testing()
add_test(NAME aest_package_test COMMAND aest_package_test)
