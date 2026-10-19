# Overlay port for the header-only core (ADR-0001 section 4). When the port runs from this
# repository tree, NEPDATE_CPP_SOURCE points at the repository root (set by the CI packages job);
# upstream submission uses vcpkg_from_github against the release archive, whose SHA512 the maintainer
# fills after the tag exists (ADR-0005).

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
        SHA512 0
        HEAD_REF main
    )
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DNEPDATE_BUILD_PATRO=OFF
        -DNEPDATE_BUILD_TESTS=OFF
        -DNEPDATE_BUILD_TOOLS=OFF
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nepdate)

# Header-only: no compiled artefacts, no debug tree.
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug"
    "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
