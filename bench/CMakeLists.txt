# Benchmarks (ADR-0004 section 5). A separate project, never built by the library's CI gates.
#
#   cmake -S bench -B build/bench -G Ninja -DCMAKE_BUILD_TYPE=Release
#         -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
#         -DVCPKG_MANIFEST_DIR=<repository root> -DVCPKG_MANIFEST_FEATURES=dev
#   cmake --build build/bench
#   start /affinity 4 /wait build\bench\nepdate_bench.exe --benchmark_repetitions=10
#         --benchmark_report_aggregates_only=true --benchmark_out=bench-msvc.json
#
# The competitors of docs/competitors.md are fetched at their pinned commits and compiled only
# here. GPL code is never vendored, linked into a released binary or redistributed.
cmake_minimum_required(VERSION 3.25...4.1)

project(nepdate_bench LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(FetchContent)

# FetchContent_Populate is deprecated in CMake 4 (CMP0169) in favour of
# FetchContent_MakeAvailable, which add_subdirectory()s the fetched project. Two of the three
# competitors have no CMakeLists.txt, and the third builds its own CLI, so download-only is the
# right tool; keep the old policy for it.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(nepdate_cli
    GIT_REPOSITORY https://github.com/khumnath/nepdate-cli.git
    GIT_TAG 27e60035)
FetchContent_Declare(aatishnn_nepdate
    GIT_REPOSITORY https://github.com/aatishnn/nepdate.git
    GIT_TAG 18e5d3f9)
FetchContent_Declare(nepalical
    GIT_REPOSITORY https://github.com/nepali-bhasa/nepali-calendar.git
    GIT_TAG 3ea885a8)
foreach(name IN ITEMS nepdate_cli aatishnn_nepdate nepalical)
    FetchContent_GetProperties(${name})
    if(NOT ${name}_POPULATED)
        # Download only; these are not CMake projects (or build their own apps), so we compile
        # the files we need by hand below instead of FetchContent_MakeAvailable.
        FetchContent_Populate(${name})
    endif()
endforeach()

# nepalical has a bug in its fetched copy: BsData::get(y) and AdData::get(y) call get(y, 13)
# without returning it (undefined behaviour). Clang 22 turns that into a crash at -O3; fix the
# two one-liners in the fetched tree only, nothing is vendored.
foreach(header IN ITEMS BsData.h AdData.h)
    set(_path "${nepalical_SOURCE_DIR}/include/nepalical/${header}")
    file(READ "${_path}" _text)
    string(REPLACE "    get(y,13);" "    return get(y,13);" _text "${_text}")
    file(WRITE "${_path}" "${_text}")
endforeach()

set(NEPDATE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NEPDATE_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." nepdate EXCLUDE_FROM_ALL)

find_package(benchmark CONFIG REQUIRED)

# Competitor sources, with their include directories. Their own code compiles with the default
# flags: the benchmark must not relax anything for our library, and must not fail on theirs.
add_library(nepdate_bench_competitors OBJECT
    competitors.cpp
    ${aatishnn_nepdate_SOURCE_DIR}/date.c
    ${nepalical_SOURCE_DIR}/src/nepalical/Ad.cpp
    ${nepalical_SOURCE_DIR}/src/nepalical/AdData.cpp
    ${nepalical_SOURCE_DIR}/src/nepalical/Bs.cpp
    ${nepalical_SOURCE_DIR}/src/nepalical/BsData.cpp
    ${nepalical_SOURCE_DIR}/src/nepalical/Date.cpp)
target_compile_features(nepdate_bench_competitors PRIVATE cxx_std_20)
if(MSVC)
    # VS 2026 (cl 19.5x) turns C4716 (a non-void function with no return, which nepalical has in
    # AdData::get/BsData::get) into an error by default. Suppress it for their code only.
    target_compile_options(nepdate_bench_competitors PRIVATE /wd4716)
endif()
target_include_directories(nepdate_bench_competitors PRIVATE
    ${nepdate_cli_SOURCE_DIR}/src
    ${aatishnn_nepdate_SOURCE_DIR}
    ${nepalical_SOURCE_DIR}/include
    ${nepalical_SOURCE_DIR}/src)

add_executable(nepdate_bench bench_main.cpp alloc_counter.cpp)
target_compile_features(nepdate_bench PRIVATE cxx_std_20)
target_link_libraries(nepdate_bench PRIVATE
    nepdate::nepdate
    nepdate::patro
    nepdate_bench_competitors
    benchmark::benchmark)
