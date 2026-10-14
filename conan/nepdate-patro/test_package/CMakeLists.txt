cmake_minimum_required(VERSION 3.15)
project(package_test LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nepdate-patro CONFIG REQUIRED)

# Only the patro target: the nepdate headers it includes must arrive through it.
add_executable(example example.cpp)
target_link_libraries(example PRIVATE nepdate::patro)
target_compile_features(example PRIVATE cxx_std_20)
