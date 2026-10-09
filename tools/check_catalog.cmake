# Fails unless every row of shared/spec/vectors/csharp-tests/catalog.tsv whose phase is a number
# no greater than PHASE has a Catch2 test named after its id, or a `// cs-deviation: <id> D-xx` or
# `// cs-n/a: <id> <reason>` line in a test source (ADR-0004 §3). Prints each missing id.
#
#   cmake -DPHASE=1 -DCATALOG=<catalog.tsv> -DTEST_EXE=<catch2 exe> -DTEST_DIR=<tests dir>
#         -P tools/check_catalog.cmake

cmake_minimum_required(VERSION 3.25)

foreach(var PHASE CATALOG TEST_EXE TEST_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "check_catalog: -D${var}=... is required")
    endif()
endforeach()

execute_process(COMMAND "${TEST_EXE}" --list-tests --verbosity quiet
    OUTPUT_VARIABLE listed RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "check_catalog: ${TEST_EXE} --list-tests exited ${rc}")
endif()
string(REPLACE "\r" "" listed "${listed}")
string(REPLACE ";" "\\;" listed "${listed}")
string(REPLACE "\n" ";" tests "${listed}")

file(GLOB_RECURSE sources "${TEST_DIR}/*.cpp" "${TEST_DIR}/*.hpp")
set(comments "")
foreach(source IN LISTS sources)
    file(STRINGS "${source}" lines REGEX "// cs-(deviation|n/a): ")
    foreach(line IN LISTS lines)
        if(line MATCHES "// cs-(deviation|n/a): ([^ ]+)")
            list(APPEND comments "${CMAKE_MATCH_2}")
        endif()
    endforeach()
endforeach()

# file(STRINGS) would split rows at the semicolons inside the asserts column, so read the whole
# file and hide them first.
file(READ "${CATALOG}" catalog)
string(REPLACE "\r" "" catalog "${catalog}")
string(REPLACE ";" "<semicolon>" catalog "${catalog}")
string(REPLACE "\n" ";" rows "${catalog}")
set(missing "")
set(checked 0)
foreach(row IN LISTS rows)
    if(row STREQUAL "" OR row MATCHES "^#" OR row MATCHES "^id\t")
        continue()
    endif()
    string(REPLACE "\t" ";" fields "${row}")
    list(GET fields 0 id)
    list(GET fields -1 phase)
    if(NOT phase MATCHES "^[0-9]+$" OR phase GREATER PHASE)
        continue()
    endif()
    math(EXPR checked "${checked} + 1")
    if(NOT id IN_LIST tests AND NOT id IN_LIST comments)
        list(APPEND missing "${id}")
    endif()
endforeach()

if(missing)
    list(LENGTH missing count)
    list(JOIN missing "\n  " text)
    message(FATAL_ERROR "check_catalog: ${count} of ${checked} rows up to phase ${PHASE} have no "
        "test and no cs-deviation or cs-n/a line:\n  ${text}")
endif()
message(STATUS "check_catalog: all ${checked} rows up to phase ${PHASE} are covered")
