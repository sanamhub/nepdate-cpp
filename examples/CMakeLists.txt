# C5-01: the three README examples, built and run by CTest. Built only with NEPDATE_BUILD_TESTS,
# like the tests, so a library consumer never builds them. The patro example needs the patro
# target, which a core-only build doesn't have.

add_executable(example_quick_start quick_start.cpp)
target_link_libraries(example_quick_start PRIVATE nepdate::nepdate)

add_executable(example_parse parse.cpp)
target_link_libraries(example_parse PRIVATE nepdate::nepdate)

set(examples quick_start parse)
if(TARGET nepdate::patro)
    add_executable(example_patro patro.cpp)
    target_link_libraries(example_patro PRIVATE nepdate::patro)
    list(APPEND examples patro)
endif()

foreach(name IN LISTS examples)
    nepdate_set_warnings(example_${name})
    add_test(NAME example_${name}
        COMMAND "${CMAKE_COMMAND}"
            -DPROGRAM=$<TARGET_FILE:example_${name}>
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/${name}.out
            -P "${PROJECT_SOURCE_DIR}/tools/check_example_output.cmake")
endforeach()

# The README quick start shows exactly these example files, character for character, in this
# order. Escaped semicolons keep -DEXAMPLES one argument (tests/CMakeLists.txt does the same).
set(_readme_examples
    ${CMAKE_CURRENT_SOURCE_DIR}/quick_start.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/patro.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parse.cpp)
string(REPLACE ";" "\\;" _readme_examples "${_readme_examples}")
add_test(NAME readme_examples
    COMMAND "${CMAKE_COMMAND}"
        -DREADME=${PROJECT_SOURCE_DIR}/README.md
        -DEXAMPLES=${_readme_examples}
        -P "${PROJECT_SOURCE_DIR}/tools/check_readme_examples.cmake")
