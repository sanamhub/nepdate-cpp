# Runs nepdate_codegen as a user would and checks the C0-03 acceptance criteria.
# Inputs: -DCODEGEN=<exe> -DSHARED=<shared dir> -DCOMMITTED=<committed header> -DWORK=<scratch dir>

set(relative "include/nepdate/detail/calendar_tables.hpp")
file(REMOVE_RECURSE "${WORK}")

foreach(run IN ITEMS a b)
    execute_process(COMMAND "${CODEGEN}" --shared "${SHARED}" --out "${WORK}/${run}"
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "codegen run ${run} exited ${rc}")
    endif()
endforeach()

execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
    "${WORK}/a/${relative}" "${WORK}/b/${relative}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "two runs gave different bytes")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files
    "${WORK}/a/${relative}" "${COMMITTED}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "${COMMITTED} is out of date: build the codegen target")
endif()

# C3-01: the names header sits next to the calendar header and must match it too.
set(names "include/nepdate/detail/names_tables.hpp")
get_filename_component(committed_dir "${COMMITTED}" DIRECTORY)
foreach(other IN ITEMS "${WORK}/b/${names}" "${committed_dir}/names_tables.hpp")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${WORK}/a/${names}" "${other}"
        RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "${other} differs from a fresh run: build the codegen target")
    endif()
endforeach()

file(READ "${WORK}/a/${relative}" content)
string(FIND "${content}" "\r" cr)
if(NOT cr EQUAL -1)
    message(FATAL_ERROR "generated header contains a CR byte")
endif()

execute_process(COMMAND "${CODEGEN}" --shared "${SHARED}" --out "${WORK}/a" --check
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "--check on a fresh tree exited ${rc}, expected 0")
endif()

# One byte changed: 2199 becomes 2198.
string(REPLACE "kMaxYear = 2199" "kMaxYear = 2198" changed "${content}")
file(WRITE "${WORK}/a/${relative}" "${changed}")
execute_process(COMMAND "${CODEGEN}" --shared "${SHARED}" --out "${WORK}/a" --check
    RESULT_VARIABLE rc ERROR_VARIABLE err)
if(NOT rc EQUAL 1)
    message(FATAL_ERROR "--check after a byte change exited ${rc}, expected 1")
endif()
string(FIND "${err}" "${WORK}/a/${relative}" named)
if(named EQUAL -1)
    message(FATAL_ERROR "--check did not print the changed path; it printed: ${err}")
endif()
file(READ "${WORK}/a/${relative}" after)
if(NOT after STREQUAL changed)
    message(FATAL_ERROR "--check wrote to the file")
endif()
