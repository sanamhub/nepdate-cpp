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
        SHA512 f707307a3f306a09724d1518d7d699d72182e2b291c520b7929f74157cbafa77b7bdd14483b944be6477e6b411f261eb4c27ebad3af88344d11e5ea81a1b9156
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
