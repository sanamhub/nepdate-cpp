# Port for the compiled patro library (ADR-0001 section 4). It builds patro/CMakeLists.txt
# against the installed core, exactly like the standalone build a user does.

vcpkg_minimum_required(VERSION 2024-05-24)

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
    SOURCE_PATH "${SOURCE_PATH}/patro"
    OPTIONS
        -DNEPDATE_BUILD_TESTS=OFF
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/nepdate-patro)

# Headers are shared between configurations; the debug tree keeps only its library.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
