cmake_minimum_required(VERSION 3.25)
project(nepdate_consumer LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

# A separate project, built by the CI packages job against the vcpkg overlay ports and by the
# install_consumer CTest test against an installed prefix (ADR-0004 section 3). It links both
# packages, once with -std=c++20 and once with -std=c++23 (the caller passes the flag).
find_package(nepdate CONFIG REQUIRED)
find_package(nepdate-patro CONFIG REQUIRED)

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE nepdate::nepdate nepdate::patro)
target_compile_features(consumer PRIVATE cxx_std_20)
