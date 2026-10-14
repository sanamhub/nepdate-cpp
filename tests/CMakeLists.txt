find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

# The core must reach its users with no link libraries (ADR-0001 §3, gate 9 from C4).
get_target_property(_nepdate_links nepdate INTERFACE_LINK_LIBRARIES)
if(_nepdate_links)
    message(FATAL_ERROR "nepdate::nepdate links ${_nepdate_links}; it must link nothing")
endif()

add_executable(nepdate_tests
    arith_test.cpp
    civil_test.cpp
    constexpr_tests.cpp
    cs_phase1.cpp
    cs_phase2.cpp
    cs_phase3.cpp
    cs_phase4.cpp
    date_range_test.cpp
    date_test.cpp
    exhaustive_test.cpp
    fiscal_year_test.cpp
    format_test.cpp
    formatter_test.cpp
    fuzz/corpus_test.cpp
    names_test.cpp
    parse_test.cpp
    pattern_test.cpp
    result_test.cpp
    serial_test.cpp
    tables_test.cpp
    time_test.cpp
    vectors_test.cpp
    version_test.cpp
)
target_link_libraries(nepdate_tests PRIVATE nepdate::nepdate Catch2::Catch2WithMain)
# The vectors are read at run time from shared/ (ADR-0004 section 3).
target_compile_definitions(nepdate_tests PRIVATE NEPDATE_SHARED_DIR="${PROJECT_SOURCE_DIR}/shared")
# The fuzz seed corpus (C3-07) is read at run time from the source tree.
target_compile_definitions(
    nepdate_tests PRIVATE NEPDATE_FUZZ_CORPUS_DIR="${PROJECT_SOURCE_DIR}/tests/fuzz/corpus")
nepdate_set_warnings(nepdate_tests)
nepdate_set_instrumentation(nepdate_tests)
if(NEPDATE_NO_EXCEPTIONS)
    target_compile_definitions(nepdate_tests PRIVATE CATCH_CONFIG_DISABLE_EXCEPTIONS)
endif()
catch_discover_tests(nepdate_tests)

# C1-08: allocation counting replaces the global operator new, so it gets its own executable and
# Catch2's own allocations stay outside the measured loops (ADR-0004 section 3).
add_executable(nepdate_alloc_tests alloc_counter.cpp alloc_tests.cpp)
target_link_libraries(nepdate_alloc_tests PRIVATE nepdate::nepdate Catch2::Catch2WithMain)
nepdate_set_warnings(nepdate_alloc_tests)
nepdate_set_instrumentation(nepdate_alloc_tests)
if(NEPDATE_NO_EXCEPTIONS)
    target_compile_definitions(nepdate_alloc_tests PRIVATE CATCH_CONFIG_DISABLE_EXCEPTIONS)
endif()
catch_discover_tests(nepdate_alloc_tests)

# C3-07: libFuzzer targets for strict and lenient parsing, built only by the fuzz preset (Clang
# with -fsanitize=address,undefined,fuzzer-no-link). The seed corpus in fuzz/corpus runs through
# the same property as a normal test on every platform (fuzz/corpus_test.cpp above). libFuzzer
# provides main, so the final link adds -fsanitize=fuzzer; the preset's fuzzer-no-link keeps the
# Catch2 binaries' own main.
if(NEPDATE_SANITIZE MATCHES "fuzzer" AND NOT MSVC)
    foreach(name IN ITEMS fuzz_parse fuzz_parse_lenient)
        add_executable(${name} fuzz/${name}.cpp)
        target_link_libraries(${name} PRIVATE nepdate::nepdate)
        nepdate_set_warnings(${name})
        nepdate_set_instrumentation(${name})
        target_link_options(${name} PRIVATE -fsanitize=fuzzer)
    endforeach()
endif()

# C1-07: every C# catalogue row up to this phase has a test or a deviation line (ADR-0004 §3).
# The limit rises by one in each phase. The 35 patro rows of phase 4 live in the patro test
# binary (C4-06); a core-only build checks what its own tests can cover.
set(NEPDATE_CATALOG_PHASE 4)
if(TARGET nepdate::patro)
    set(_catalog_exes "$<TARGET_FILE:nepdate_tests>;$<TARGET_FILE:nepdate_patro_tests>")
    set(_catalog_dirs "${CMAKE_CURRENT_SOURCE_DIR};${PROJECT_SOURCE_DIR}/patro/tests")
    set(_catalog_phase ${NEPDATE_CATALOG_PHASE})
else()
    set(_catalog_exes "$<TARGET_FILE:nepdate_tests>")
    set(_catalog_dirs "${CMAKE_CURRENT_SOURCE_DIR}")
    set(_catalog_phase 3)
endif()
# Escaped semicolons keep each -D value one argument through add_test's list expansion.
string(REPLACE ";" "\\;" _catalog_exes "${_catalog_exes}")
string(REPLACE ";" "\\;" _catalog_dirs "${_catalog_dirs}")
add_test(NAME catalog_coverage
    COMMAND "${CMAKE_COMMAND}"
        -DPHASE=${_catalog_phase}
        -DCATALOG=${PROJECT_SOURCE_DIR}/shared/spec/vectors/csharp-tests/catalog.tsv
        -DTEST_EXES=${_catalog_exes}
        -DTEST_DIRS=${_catalog_dirs}
        -P "${PROJECT_SOURCE_DIR}/tools/check_catalog.cmake"
)

# C0-04 AC: a version that matches nothing must fail the release check.
add_test(NAME release_check_rejects_9.9.9
    COMMAND "${CMAKE_COMMAND}" -DVERSION=9.9.9 -P "${PROJECT_SOURCE_DIR}/tools/release_check.cmake")
set_tests_properties(release_check_rejects_9.9.9 PROPERTIES WILL_FAIL TRUE)

# C4-02 AC: no patro table, name or string goes into any file under include/.
add_test(NAME patro_data_isolation
    COMMAND "${CMAKE_COMMAND}" -DINCLUDE_DIR=${PROJECT_SOURCE_DIR}/include
        -P "${PROJECT_SOURCE_DIR}/tools/check_patro_in_include.cmake")

# C1-02 AC: `constexpr auto d = make_date(2081, 4, 33).value();` must not compile. Both targets stay
# out of the normal build; the tests build them. RESOURCE_LOCK keeps the two builds from running
# the build tool in this tree at the same time.
foreach(expect_error IN ITEMS 0 1)
    set(target compile_fail_${expect_error})
    add_executable(${target} EXCLUDE_FROM_ALL compile_fail/value_on_error.cpp)
    target_link_libraries(${target} PRIVATE nepdate::nepdate)
    target_compile_definitions(${target} PRIVATE NEPDATE_EXPECT_COMPILE_ERROR=${expect_error})
    nepdate_set_warnings(${target})
endforeach()
add_test(NAME compile_fail_control
    COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target compile_fail_0)
add_test(NAME compile_fail_value_on_error
    COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target compile_fail_1)
set_tests_properties(compile_fail_control compile_fail_value_on_error
    PROPERTIES RESOURCE_LOCK nepdate_build_tree)
set_tests_properties(compile_fail_value_on_error PROPERTIES WILL_FAIL TRUE)
