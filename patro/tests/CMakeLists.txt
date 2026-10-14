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

# C4-03 AC: a wrong calendar fingerprint must fail to compile, and a control build without the
# fake value must succeed. Both targets stay out of the normal build; the tests build them.
foreach(expect_error IN ITEMS 0 1)
    set(target patro_compile_fail_${expect_error})
    add_executable(${target} EXCLUDE_FROM_ALL compile_fail/fingerprint.cpp)
    target_link_libraries(${target} PRIVATE nepdate::patro)
    target_compile_definitions(${target} PRIVATE NEPDATE_EXPECT_COMPILE_ERROR=${expect_error})
    if(expect_error)
        target_compile_definitions(${target} PRIVATE NEPDATE_PATRO_CALENDAR_FINGERPRINT=0x1)
    endif()
    nepdate_set_warnings(${target})
endforeach()
add_test(NAME patro_compile_fail_control
    COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target patro_compile_fail_0)
add_test(NAME patro_compile_fail_fingerprint
    COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target patro_compile_fail_1)
set_tests_properties(patro_compile_fail_control patro_compile_fail_fingerprint
    PROPERTIES RESOURCE_LOCK nepdate_build_tree)
set_tests_properties(patro_compile_fail_fingerprint PROPERTIES WILL_FAIL TRUE)
