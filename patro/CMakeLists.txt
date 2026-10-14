# The patro (PATRO.md, hub ADR-0004): tithi, holidays and events, in its own package on top of the
# core. Built from the top level, or on its own against an installed core (C4-01 AC):
#
#   cmake --install <core build> --prefix <prefix>
#   cmake -S patro -B <dir> -DCMAKE_PREFIX_PATH=<prefix>
#   cmake --build <dir>
#
# Export name patro, so the installed target is nepdate::patro (hub ADR-0004).

cmake_minimum_required(VERSION 3.25...4.1)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Standalone: the same version as the core it builds against.
    project(nepdate-patro VERSION 0.1.0 LANGUAGES CXX)
    find_package(nepdate ${PROJECT_VERSION} EXACT CONFIG REQUIRED)
    include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/warnings.cmake")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
# No C++20 modules in v0.1 (ADR-0001 section 1).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# The patro tests need Catch2 from the vcpkg feature dev. The top-level build turns them on
# with its own NEPDATE_BUILD_TESTS option (an existing cache variable is never overridden); a
# standalone patro build leaves them off.
option(NEPDATE_BUILD_TESTS "Build the patro tests" OFF)

add_library(nepdate_patro src/patro.cpp)
add_library(nepdate::patro ALIAS nepdate_patro)
set_target_properties(nepdate_patro PROPERTIES
    EXPORT_NAME patro
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
target_compile_features(nepdate_patro PUBLIC cxx_std_20)
target_include_directories(nepdate_patro PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_link_libraries(nepdate_patro PUBLIC nepdate::nepdate)
nepdate_set_warnings(nepdate_patro)
if(MSVC)
    # The generated patro_tables.hpp carries readable UTF-8 comments (C4-02); the public headers
    # stay 7-bit ASCII, so /utf-8 is private to this target and never propagates to users.
    target_compile_options(nepdate_patro PRIVATE /utf-8)
endif()

# <nepdate/patro_export.hpp>, generated at configure time. Only functions are exported, never data
# (ADR-0001 section 2; the C4-01 dumpbin AC checks the shared build).
include(GenerateExportHeader)
generate_export_header(nepdate_patro
    BASE_NAME patro
    EXPORT_FILE_NAME include/nepdate/patro_export.hpp)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(NEPDATE_PATRO_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/nepdate-patro")

install(TARGETS nepdate_patro EXPORT nepdatePatroTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES
    include/nepdate/patro.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/include/nepdate/patro_export.hpp"
    include/nepdate/patro_config.hpp
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nepdate")
install(EXPORT nepdatePatroTargets
    NAMESPACE nepdate::
    DESTINATION "${NEPDATE_PATRO_CONFIG_INSTALL_DIR}")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/nepdate-patroConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/nepdate-patroConfig.cmake"
    INSTALL_DESTINATION "${NEPDATE_PATRO_CONFIG_INSTALL_DIR}")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/nepdate-patroConfigVersion.cmake"
    COMPATIBILITY SameMinorVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/nepdate-patroConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/nepdate-patroConfigVersion.cmake"
    DESTINATION "${NEPDATE_PATRO_CONFIG_INSTALL_DIR}")

if(NEPDATE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
