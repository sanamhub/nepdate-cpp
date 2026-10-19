# The docs gate. Checks tracked markdown outside shared/:
#   1. every relative link resolves to a tracked file or directory;
#   2. no em dash (U+2014).
# Plain CMake plus git on PATH, no other tool.
#
#   cmake -P tools/check_docs.cmake

cmake_minimum_required(VERSION 3.25)

get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(problems "")

string(ASCII 10 nl)
string(ASCII 13 cr)
string(ASCII 226 128 148 em_dash) # U+2014 as UTF-8 bytes

# ---- tracked markdown outside shared/ ----
execute_process(COMMAND git -C "${root}" ls-files "*.md"
    RESULT_VARIABLE git_rc OUTPUT_VARIABLE md_output ERROR_VARIABLE git_err)
if(NOT git_rc EQUAL 0)
    message(FATAL_ERROR "check_docs: git ls-files failed: ${git_err}")
endif()
string(REPLACE "${nl}" ";" md_files "${md_output}")
list(FILTER md_files EXCLUDE REGEX "^shared/")
list(REMOVE_ITEM md_files "")

foreach(rel IN LISTS md_files)
    file(READ "${root}/${rel}" text)
    string(REPLACE "${cr}${nl}" "${nl}" text "${text}")
    string(REPLACE "${cr}" "${nl}" text "${text}")

    # Em dash, per line for a useful message.
    set(line_no 0)
    string(REPLACE "${nl}" ";" lines "${text}")
    foreach(line IN LISTS lines)
        math(EXPR line_no "${line_no} + 1")
        if(line MATCHES "${em_dash}")
            list(APPEND problems "${rel}:${line_no}: em dash (U+2014)")
        endif()
    endforeach()

    # Relative markdown links: ](target), also used by ![alt](target). Each is on one line and
    # contains no ')'. Angle-bracket <...> is not checked: in this repo it is C++ template
    # syntax in docs/API.md, not a link.
    get_filename_component(dir "${rel}" DIRECTORY)
    string(REPLACE "${nl}" ";" lines "${text}")
    foreach(line IN LISTS lines)
        string(REGEX MATCHALL "\\]\\([^)]*\\)" links "${line}")
        foreach(link IN LISTS links)
            string(REGEX REPLACE "^\\]\\(" "" target "${link}")
            string(REGEX REPLACE "\\)$" "" target "${target}")
            string(REGEX REPLACE "[#?].*$" "" target "${target}")
            if(target STREQUAL "")
                continue()
            endif()
            if(target MATCHES "^[A-Za-z][A-Za-z0-9+.-]*:" OR target MATCHES "^/")
                continue() # scheme or root absolute
            endif()
            if(dir)
                set(base "${root}/${dir}")
            else()
                set(base "${root}")
            endif()
            cmake_path(ABSOLUTE_PATH target BASE_DIRECTORY "${base}" NORMALIZE OUTPUT_VARIABLE abs)
            cmake_path(RELATIVE_PATH abs BASE_DIRECTORY "${root}" OUTPUT_VARIABLE rel_target)
            execute_process(COMMAND git -C "${root}" ls-files -- "${rel_target}"
                OUTPUT_VARIABLE tracked RESULT_VARIABLE tracked_rc)
            string(STRIP "${tracked}" tracked)
            if(NOT tracked_rc EQUAL 0 OR tracked STREQUAL "")
                list(APPEND problems "${rel}: broken link: ${link} -> ${rel_target}")
            endif()
        endforeach()
    endforeach()
endforeach()

if(problems)
    list(JOIN problems "\n  " lines)
    message(FATAL_ERROR "check_docs: found problems:\n  ${lines}")
endif()
message(STATUS "check_docs: ${root} clean")
