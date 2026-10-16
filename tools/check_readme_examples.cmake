# Extracts the ```cpp fenced blocks of README.md and compares each, in order, with the
# body of one example file. The README quick start shows exactly the example programs, character
# for character (CRLF folded to LF first). Blocks are fenced with exactly ```cpp and ```.
#
# Line handling is done with string(FIND)/string(SUBSTRING) rather than splitting on ';', because
# the C++ blocks contain semicolons (and CMake strings have no \n escape).
#
#   cmake -DREADME=<path> -DEXAMPLES=a;b;c -P tools/check_readme_examples.cmake

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED README OR NOT DEFINED EXAMPLES)
    message(FATAL_ERROR "check_readme_examples: pass -DREADME=<path> -DEXAMPLES=a;b;c")
endif()

# CMake strings have no \n or \r escapes; build both characters by code point.
string(ASCII 10 nl)
string(ASCII 13 cr)

file(READ "${README}" readme)
string(REPLACE "${cr}${nl}" "${nl}" readme "${readme}")
string(REPLACE "${cr}" "${nl}" readme "${readme}")

list(LENGTH EXAMPLES example_count)
set(block_index 0)
set(text "${readme}")
while(TRUE)
    string(FIND "${text}" "```cpp" open_pos)
    if(open_pos EQUAL -1)
        break()
    endif()
    # rest starts at the newline right after the ```cpp fence.
    math(EXPR rest_start "${open_pos} + 6")
    string(SUBSTRING "${text}" "${rest_start}" -1 rest)
    string(FIND "${rest}" "${nl}```" close_pos)
    if(close_pos EQUAL -1)
        message(FATAL_ERROR "check_readme_examples: unterminated ```cpp fence in ${README}")
    endif()
    # The block is rest[1 .. close_pos]: the first line through the newline that opens the fence.
    string(SUBSTRING "${rest}" 1 "${close_pos}" block)
    if(block_index GREATER_EQUAL example_count)
        message(FATAL_ERROR "check_readme_examples: more ```cpp blocks than ${example_count} example files")
    endif()
    list(GET EXAMPLES ${block_index} expected_file)
    file(READ "${expected_file}" expected)
    string(REPLACE "${cr}${nl}" "${nl}" expected "${expected}")
    string(REPLACE "${cr}" "${nl}" expected "${expected}")
    if(NOT block STREQUAL expected)
        message(FATAL_ERROR
            "check_readme_examples: ```cpp block ${block_index} differs from ${expected_file}\n--- block ---\n${block}\n--- file ---\n${expected}")
    endif()
    math(EXPR block_index "${block_index} + 1")
    math(EXPR next "${close_pos} + 3")
    string(SUBSTRING "${rest}" "${next}" -1 text)
endwhile()

if(NOT block_index EQUAL example_count)
    message(FATAL_ERROR "check_readme_examples: found ${block_index} ```cpp blocks, expected ${example_count}")
endif()
