# Benchmarks (ADR-0004 §5). A separate project, never built by the library's CI gates.
#
#   cmake -S bench -B build/bench -G Ninja -DCMAKE_BUILD_TYPE=Release
#         -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
#         -DVCPKG_MANIFEST_DIR=<repository root> -DVCPKG_MANIFEST_FEATURES=dev
#   cmake --build build/bench
#   start /affinity 4 /wait build\bench\nepdate_bench.exe --benchmark_repetitions=10
#         --benchmark_report_aggregates_only=true
cmake_minimum_required(VERSION 3.25...4.1)

project(nepdate_bench LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(NEPDATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NEPDATE_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." nepdate EXCLUDE_FROM_ALL)

find_package(benchmark CONFIG REQUIRED)

add_executable(nepdate_bench bench_main.cpp)
target_compile_features(nepdate_bench PRIVATE cxx_std_20)
target_link_libraries(nepdate_bench PRIVATE nepdate::nepdate benchmark::benchmark_main)
