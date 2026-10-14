cmake_minimum_required(VERSION 3.25...4.1)

project(nepdate VERSION 0.0.0 LANGUAGES CXX)

# The regenerate command is `cmake --build <dir> --target codegen` (ADR-0001 §5). CMake 3.31 and
# later reserve that target name for their own CODEGEN commands unless this policy is OLD.
if(POLICY CMP0171)
    cmake_policy(SET CMP0171 OLD)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
# No C++20 modules in v0.1 (ADR-0001 §1), so skip the dependency scan that needs clang-scan-deps.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(NEPDATE_BUILD_TESTS "Build the core tests (needs the vcpkg feature dev)" ${PROJECT_IS_TOP_LEVEL})
# patro/ arrives in phase C4 and is built by default; the option exists so a core-only consumer
# (vcpkg port nepdate) can turn it off (ADR-0001 section 4).
option(NEPDATE_BUILD_PATRO "Build the nepdate::patro library" ON)
option(NEPDATE_BUILD_TOOLS "Build tools/codegen (needs the vcpkg feature dev)" OFF)

include(cmake/warnings.cmake)
include(cmake/instrumentation.cmake)

# The core is header-only and adds nothing but the standard to its users (ADR-0001 §3): no compile
# options, no link libraries.
add_library(nepdate INTERFACE)
add_library(nepdate::nepdate ALIAS nepdate)
target_compile_features(nepdate INTERFACE cxx_std_20)
target_sources(nepdate
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/nepdate/detail/calendar_tables.hpp
            include/nepdate/detail/civil.hpp
            include/nepdate/detail/names.hpp
            include/nepdate/detail/names_tables.hpp
            include/nepdate/detail/result.hpp
            include/nepdate/detail/serial.hpp
            include/nepdate/date.hpp
            include/nepdate/date_range.hpp
            include/nepdate/fiscal_year.hpp
            include/nepdate/format.hpp
            include/nepdate/nepdate.hpp
            include/nepdate/parse.hpp
            include/nepdate/version.hpp
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(NEPDATE_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/nepdate")

install(TARGETS nepdate EXPORT nepdateTargets FILE_SET HEADERS)
install(EXPORT nepdateTargets
    NAMESPACE nepdate::
    DESTINATION "${NEPDATE_CONFIG_INSTALL_DIR}"
)
configure_package_config_file(cmake/nepdateConfig.cmake.in
    "${PROJECT_BINARY_DIR}/nepdateConfig.cmake"
    INSTALL_DESTINATION "${NEPDATE_CONFIG_INSTALL_DIR}"
)
write_basic_package_version_file("${PROJECT_BINARY_DIR}/nepdateConfigVersion.cmake"
    COMPATIBILITY SameMinorVersion
    ARCH_INDEPENDENT
)
install(FILES
    "${PROJECT_BINARY_DIR}/nepdateConfig.cmake"
    "${PROJECT_BINARY_DIR}/nepdateConfigVersion.cmake"
    DESTINATION "${NEPDATE_CONFIG_INSTALL_DIR}"
)

if(NEPDATE_BUILD_PATRO)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/patro/CMakeLists.txt")
        add_subdirectory(patro)
    else()
        # A core-only checkout (or tarball) may ship without patro/; the core must still build.
        message(WARNING "NEPDATE_BUILD_PATRO is ON but patro/ is missing; building the core only")
    endif()
endif()

# Gate 9 (C4-04): the core-only program the isolation gate builds in Release.
add_subdirectory(tools/coreonly)

if(NEPDATE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NEPDATE_BUILD_TOOLS)
    add_subdirectory(tools/codegen)
endif()
