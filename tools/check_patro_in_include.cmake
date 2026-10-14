# Fails when patro data leaks into the core headers: no file under include/ may contain a patro
# tithi or event name, in ASCII or as a Devanagari octal escape (C4-02 AC).
#
#   cmake -DINCLUDE_DIR=<include dir> -P tools/check_patro_in_include.cmake

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED INCLUDE_DIR)
    message(FATAL_ERROR "check_patro_in_include: -DINCLUDE_DIR=... is required")
endif()

# Distinctive patro strings: the first two tithi names and one event name, plus the octal escape
# of the Nepali tithi name Panchami (U+092A U+091E ...). The core's own month and weekday names
# never match these.
set(needles
    "Panchami"
    "Shashthi"
    "World Day Against Human Trafficking"
    "\\340\\244\\252\\340\\244\\236")

file(GLOB_RECURSE headers "${INCLUDE_DIR}/*")
foreach(header IN LISTS headers)
    file(READ "${header}" text)
    foreach(needle IN LISTS needles)
        string(FIND "${text}" "${needle}" at)
        if(NOT at EQUAL -1)
            file(RELATIVE_PATH relative "${INCLUDE_DIR}/.." "${header}")
            message(FATAL_ERROR
                "check_patro_in_include: ${relative} contains patro data '${needle}'")
        endif()
    endforeach()
endforeach()
message(STATUS "check_patro_in_include: no patro data under include/")
