# Exits 0 only if every version location of ADR-0005 equals VERSION and CHANGELOG.md has a dated
# section for it.
#
#   cmake -DVERSION=X.Y.Z -P tools/release_check.cmake
#
# Locations: project() in CMakeLists.txt, NEPDATE_VERSION_* in include/nepdate/version.hpp,
# ports/*/vcpkg.json and conan/*/conanfile.py (checked once C5 creates them), and a
# `## [X.Y.Z] - YYYY-MM-DD` heading in CHANGELOG.md.

cmake_minimum_required(VERSION 3.25)

get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
    message(FATAL_ERROR "release_check: pass -DVERSION=X.Y.Z (got '${VERSION}')")
endif()
set(major "${CMAKE_MATCH_1}")
set(minor "${CMAKE_MATCH_2}")
set(patch "${CMAKE_MATCH_3}")

set(problems "")

file(READ "${root}/CMakeLists.txt" text)
if(NOT text MATCHES "project\\(nepdate VERSION ([0-9.]+)")
    list(APPEND problems "CMakeLists.txt: no project(nepdate VERSION ...)")
elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
    list(APPEND problems "CMakeLists.txt: project version ${CMAKE_MATCH_1}")
endif()

file(READ "${root}/include/nepdate/version.hpp" text)
foreach(part IN ITEMS MAJOR MINOR PATCH)
    string(TOLOWER "${part}" lower)
    if(NOT text MATCHES "#define NEPDATE_VERSION_${part} ([0-9]+)")
        list(APPEND problems "version.hpp: NEPDATE_VERSION_${part} missing")
    elseif(NOT CMAKE_MATCH_1 STREQUAL ${lower})
        list(APPEND problems "version.hpp: NEPDATE_VERSION_${part} is ${CMAKE_MATCH_1}")
    endif()
endforeach()
if(NOT text MATCHES "#define NEPDATE_VERSION_STRING \"([^\"]*)\"")
    list(APPEND problems "version.hpp: NEPDATE_VERSION_STRING missing")
elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
    list(APPEND problems "version.hpp: NEPDATE_VERSION_STRING is ${CMAKE_MATCH_1}")
endif()

file(GLOB ports "${root}/ports/*/vcpkg.json")
foreach(port IN LISTS ports)
    file(READ "${port}" text)
    file(RELATIVE_PATH relative "${root}" "${port}")
    string(JSON port_version ERROR_VARIABLE json_error GET "${text}" version)
    if(json_error)
        list(APPEND problems "${relative}: no \"version\"")
    elseif(NOT port_version STREQUAL VERSION)
        list(APPEND problems "${relative}: version ${port_version}")
    endif()
endforeach()

file(GLOB recipes "${root}/conan/*/conanfile.py")
foreach(recipe IN LISTS recipes)
    file(READ "${recipe}" text)
    file(RELATIVE_PATH relative "${root}" "${recipe}")
    if(NOT text MATCHES "version = \"([^\"]*)\"")
        list(APPEND problems "${relative}: no version = \"...\"")
    elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
        list(APPEND problems "${relative}: version ${CMAKE_MATCH_1}")
    endif()
endforeach()

# The patro's standalone project version must match too (patro/CMakeLists.txt): the standalone
# build does find_package(nepdate ${PROJECT_VERSION} EXACT).
file(READ "${root}/patro/CMakeLists.txt" text)
if(NOT text MATCHES "project\\(nepdate-patro VERSION ([0-9.]+)")
    list(APPEND problems "patro/CMakeLists.txt: no project(nepdate-patro VERSION ...)")
elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
    list(APPEND problems "patro/CMakeLists.txt: project version ${CMAKE_MATCH_1}")
endif()

file(READ "${root}/CHANGELOG.md" text)
string(REPLACE "." "\\." escaped "${VERSION}")
if(NOT text MATCHES "\n## \\[${escaped}\\] - [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]\n")
    list(APPEND problems "CHANGELOG.md: no '## [${VERSION}] - YYYY-MM-DD' section")
endif()

if(problems)
    list(JOIN problems "\n  " lines)
    message(FATAL_ERROR "release_check: ${VERSION} doesn't match:\n  ${lines}")
endif()
message(STATUS "release_check: every version location says ${VERSION}")
