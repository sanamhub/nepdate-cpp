# Patro tests (C4-03 and C4-06 fill these in). Only built from the top-level build, where the
# vcpkg feature dev provides Catch2.

find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(nepdate_patro_tests patro_test.cpp)
target_link_libraries(nepdate_patro_tests PRIVATE nepdate::patro Catch2::Catch2WithMain)
target_compile_definitions(nepdate_patro_tests PRIVATE
    NEPDATE_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_set_warnings(nepdate_patro_tests)
nepdate_set_instrumentation(nepdate_patro_tests)
if(NEPDATE_NO_EXCEPTIONS)
    target_compile_definitions(nepdate_patro_tests PRIVATE CATCH_CONFIG_DISABLE_EXCEPTIONS)
endif()
catch_discover_tests(nepdate_patro_tests)
