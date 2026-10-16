# Runs one example program and compares its standard output with the expected-output file,
# after folding CRLF to LF (Windows text mode turns '\n' into "\r\n"). Fails and prints both
# outputs on a mismatch.
#
#   cmake -DPROGRAM=<exe> -DEXPECTED=<file> -P tools/check_example_output.cmake

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED PROGRAM OR NOT DEFINED EXPECTED)
    message(FATAL_ERROR "check_example_output: pass -DPROGRAM=<exe> -DEXPECTED=<file>")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE got
    ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "check_example_output: ${PROGRAM} exited ${rc}\n${err}")
endif()

# CMake strings have no \n or \r escapes; build both characters by code point.
string(ASCII 10 nl)
string(ASCII 13 cr)

file(READ "${EXPECTED}" want)
string(REPLACE "${cr}${nl}" "${nl}" got "${got}")
string(REPLACE "${cr}${nl}" "${nl}" want "${want}")
string(REPLACE "${cr}" "${nl}" got "${got}")
string(REPLACE "${cr}" "${nl}" want "${want}")

if(NOT got STREQUAL want)
    message(FATAL_ERROR
        "check_example_output: ${PROGRAM} output differs from ${EXPECTED}\n--- got ---\n${got}\n--- want ---\n${want}")
endif()
