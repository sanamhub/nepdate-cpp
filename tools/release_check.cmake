# Exits 0 only if every version location of ADR-0005 equals VERSION and CHANGELOG.md has a dated
# section for it.
#
#   cmake -DVERSION=X.Y.Z -P tools/release_check.cmake
#   cmake -DVERSION=X.Y.Z-beta.1 -P tools/release_check.cmake
#
# CMake's project(VERSION) holds numbers only, so a pre-release is split: X.Y.Z goes in project()
# and NEPDATE_VERSION_MAJOR/MINOR/PATCH, the part after "-" in NEPDATE_VERSION_PRERELEASE ("" for
# a stable version), and the whole string in NEPDATE_VERSION_STRING, the vcpkg ports
# ("version-semver") and the Conan recipes. A stable version needs "version" in the ports and an
# empty pre-release, so a beta's files never pass for the stable tag and the other way round.
#
# Locations: project() in CMakeLists.txt and patro/CMakeLists.txt, the macros in
# include/nepdate/version.hpp, ports/*/vcpkg.json (and the patro port's nepdate dependency),
# conan/*/conanfile.py (and the patro recipe's requires), and a `## [VERSION] - YYYY-MM-DD`
# heading in CHANGELOG.md.

cmake_minimum_required(VERSION 3.25)

get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Semver 2.0 core plus an optional pre-release; build metadata is not allowed in a tag here.
set(ident "(0|[1-9][0-9]*|[0-9]*[A-Za-z-][0-9A-Za-z-]*)")
if(NOT VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-${ident}(\\.${ident})*)?$")
    message(FATAL_ERROR "release_check: pass -DVERSION=X.Y.Z or X.Y.Z-PRERELEASE (got '${VERSION}')")
endif()
set(major "${CMAKE_MATCH_1}")
set(minor "${CMAKE_MATCH_2}")
set(patch "${CMAKE_MATCH_3}")
set(core "${major}.${minor}.${patch}")
string(LENGTH "${core}" core_length)
string(SUBSTRING "${VERSION}" ${core_length} -1 suffix)
if(suffix STREQUAL "")
    set(prerelease "")
    set(port_field "version")
else()
    string(SUBSTRING "${suffix}" 1 -1 prerelease)
    set(port_field "version-semver")
endif()

set(problems "")

foreach(file IN ITEMS CMakeLists.txt patro/CMakeLists.txt)
    if(file STREQUAL "CMakeLists.txt")
        set(name nepdate)
    else()
        set(name nepdate-patro)
    endif()
    file(READ "${root}/${file}" text)
    if(NOT text MATCHES "project\\(${name} VERSION ([0-9.]+)")
        list(APPEND problems "${file}: no project(${name} VERSION ...)")
    elseif(NOT CMAKE_MATCH_1 STREQUAL core)
        list(APPEND problems "${file}: project version ${CMAKE_MATCH_1}, want ${core}")
    endif()
endforeach()

file(READ "${root}/include/nepdate/version.hpp" text)
foreach(part IN ITEMS MAJOR MINOR PATCH)
    string(TOLOWER "${part}" lower)
    if(NOT text MATCHES "#define NEPDATE_VERSION_${part} ([0-9]+)")
        list(APPEND problems "version.hpp: NEPDATE_VERSION_${part} missing")
    elseif(NOT CMAKE_MATCH_1 STREQUAL ${lower})
        list(APPEND problems "version.hpp: NEPDATE_VERSION_${part} is ${CMAKE_MATCH_1}")
    endif()
endforeach()
if(NOT text MATCHES "#define NEPDATE_VERSION_PRERELEASE \"([^\"]*)\"")
    list(APPEND problems "version.hpp: NEPDATE_VERSION_PRERELEASE missing")
elseif(NOT CMAKE_MATCH_1 STREQUAL prerelease)
    list(APPEND problems "version.hpp: NEPDATE_VERSION_PRERELEASE is \"${CMAKE_MATCH_1}\", want \"${prerelease}\"")
endif()
if(NOT text MATCHES "#define NEPDATE_VERSION_STRING \"([^\"]*)\"")
    list(APPEND problems "version.hpp: NEPDATE_VERSION_STRING missing")
elseif(NOT CMAKE_MATCH_1 STREQUAL VERSION)
    list(APPEND problems "version.hpp: NEPDATE_VERSION_STRING is ${CMAKE_MATCH_1}")
endif()

file(GLOB ports "${root}/ports/*/vcpkg.json")
foreach(port IN LISTS ports)
    file(READ "${port}" text)
    file(RELATIVE_PATH relative "${root}" "${port}")
    string(JSON port_version ERROR_VARIABLE json_error GET "${text}" "${port_field}")
    if(json_error)
        list(APPEND problems "${relative}: no \"${port_field}\"")
    elseif(NOT port_version STREQUAL VERSION)
        list(APPEND problems "${relative}: ${port_field} ${port_version}")
    endif()
    # The patro port's floor on the core must be this version: a beta is lower than its stable
    # version in semver, so a stale "0.1.0" floor would reject the 0.1.0-beta.1 core.
    string(JSON count ERROR_VARIABLE json_error LENGTH "${text}" dependencies)
    if(NOT json_error AND count GREATER 0)
        math(EXPR last "${count} - 1")
        foreach(i RANGE ${last})
            string(JSON dep_name ERROR_VARIABLE dep_error GET "${text}" dependencies ${i} name)
            if(NOT dep_error AND dep_name STREQUAL "nepdate")
                string(JSON floor ERROR_VARIABLE floor_error GET "${text}" dependencies ${i} "version>=")
                if(floor_error OR NOT floor STREQUAL VERSION)
                    list(APPEND problems "${relative}: nepdate dependency version>= '${floor}', want ${VERSION}")
                endif()
            endif()
        endforeach()
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

# Every recipe and test package that names the core pins this exact version.
file(GLOB requirers "${root}/conan/*/conanfile.py" "${root}/conan/*/test_package/conanfile.py")
foreach(recipe IN LISTS requirers)
    file(READ "${recipe}" text)
    file(RELATIVE_PATH relative "${root}" "${recipe}")
    if(text MATCHES "requires\\(\"nepdate/([^\"]*)\"" AND NOT CMAKE_MATCH_1 STREQUAL VERSION)
        list(APPEND problems "${relative}: requires nepdate/${CMAKE_MATCH_1}")
    endif()
endforeach()

# The patro's standalone build does find_package(nepdate ${PROJECT_VERSION} EXACT), which the
# project() check above keeps equal to the core's.

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
