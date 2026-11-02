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
        SHA512 8dd754a51dc5f0dd2db842f05209f82f439d592142eb753aae288427959f5a1dc412555aaa7ee9cf23ea8304a182599c790172e8f64bebfb908ff6b7a7c73261
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
