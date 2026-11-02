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
        SHA512 8dd754a51dc5f0dd2db842f05209f82f439d592142eb753aae288427959f5a1dc412555aaa7ee9cf23ea8304a182599c790172e8f64bebfb908ff6b7a7c73261
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
