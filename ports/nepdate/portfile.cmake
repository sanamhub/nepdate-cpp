# Port for the header-only core (ADR-0001 section 4). When the port runs from this repository
# tree, NEPDATE_CPP_SOURCE points at the repository root (set by the CI packages job). Otherwise it
# downloads the release archive; the SHA512 is the GitHub archive of the tag in vcpkg.json, and
# changes with every release (docs/runbooks/release.md).

vcpkg_minimum_required(VERSION 2024-05-24)

# Header-only: one configuration, no debug variant, no library to fix up.
set(VCPKG_BUILD_TYPE release)

if(DEFINED ENV{NEPDATE_CPP_SOURCE})
    get_filename_component(SOURCE_PATH "$ENV{NEPDATE_CPP_SOURCE}" ABSOLUTE)
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO sanamhub/nepdate-cpp
        REF "v${VERSION}"
        SHA512 fc7b791006236d86be7c5517f78b83d05bfa62df160aee63fe56e818a204c3f8717016bdd34cac56a80e4d88bb3fb4de0a83d5c307d3a0b5b21e9c4725c6b150
        HEAD_REF main
    )
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DNEPDATE_BUILD_PATRO=OFF
        -DNEPDATE_BUILD_TESTS=OFF
        -DNEPDATE_BUILD_TOOLS=OFF
        -DNEPDATE_BUILD_COREONLY=OFF
    MAYBE_UNUSED_VARIABLES
        NEPDATE_BUILD_COREONLY
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nepdate)

# Header-only: no compiled artefacts, no debug tree.
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug"
    "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
