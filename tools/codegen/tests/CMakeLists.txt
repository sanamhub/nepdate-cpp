find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(nepdate_codegen_tests codegen_test.cpp names_test.cpp patro_test.cpp)
target_link_libraries(nepdate_codegen_tests PRIVATE nepdate_codegen_lib Catch2::Catch2WithMain)
target_compile_definitions(nepdate_codegen_tests
    PRIVATE NEPDATE_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
nepdate_set_warnings(nepdate_codegen_tests)
nepdate_set_instrumentation(nepdate_codegen_tests)
catch_discover_tests(nepdate_codegen_tests)

# The command-line ACs of C0-03: identical bytes on two runs, LF only, --check exit codes.
add_test(NAME codegen_cli
    COMMAND "${CMAKE_COMMAND}"
        -DCODEGEN=$<TARGET_FILE:nepdate_codegen>
        -DSHARED=${PROJECT_SOURCE_DIR}/shared
        -DCOMMITTED=${PROJECT_SOURCE_DIR}/include/nepdate/detail/calendar_tables.hpp
        -DWORK=${CMAKE_CURRENT_BINARY_DIR}/codegen_cli
        -P "${CMAKE_CURRENT_SOURCE_DIR}/codegen_cli_test.cmake"
)
