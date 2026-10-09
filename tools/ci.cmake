# Runs every quality gate of ADR-0004 §1 that exists so far, in order, and stops at the first
# failure. The last line is always `ci: <passed>/<total> gates passed (<skipped> skipped)`.
#
#   cmake -P tools/ci.cmake [-DPRESET_PREFIX=<prefix>]
#
# Gates use the presets dev, noexcept, tidy, asan-ubsan and coverage. PRESET_PREFIX (or the
# environment variable NEPDATE_PRESET_PREFIX) selects presets named <prefix><name> instead, for a
# machine whose compiler needs a local CMakeUserPresets.json (CONTRIBUTING.md). When neither is set
# and CMakeUserPresets.json defines a configure preset `local-dev`, the prefix is `local-`.
#
# A gate whose tool is missing is skipped with a warning; CI installs every tool.

cmake_minimum_required(VERSION 3.25)

get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT DEFINED PRESET_PREFIX)
    if(DEFINED ENV{NEPDATE_PRESET_PREFIX})
        set(PRESET_PREFIX "$ENV{NEPDATE_PRESET_PREFIX}")
    elseif(EXISTS "${root}/CMakeUserPresets.json")
        file(READ "${root}/CMakeUserPresets.json" user_presets)
        string(JSON count ERROR_VARIABLE json_error LENGTH "${user_presets}" configurePresets)
        set(PRESET_PREFIX "")
        if(NOT json_error)
            math(EXPR last "${count} - 1")
            foreach(i RANGE ${last})
                string(JSON name GET "${user_presets}" configurePresets ${i} name)
                if(name STREQUAL "local-dev")
                    set(PRESET_PREFIX "local-")
                endif()
            endforeach()
        endif()
    else()
        set(PRESET_PREFIX "")
    endif()
endif()
if(PRESET_PREFIX)
    message(STATUS "ci: using presets ${PRESET_PREFIX}<name>")
endif()

set(gates format ascii codegen build-and-test no-exceptions tidy sanitizers coverage)
list(LENGTH gates total)
set(passed 0)
set(skipped 0)

function(ci_summary)
    message("ci: ${passed}/${total} gates passed (${skipped} skipped)")
endfunction()

# Prints the summary line and exits 1.
macro(ci_fail gate why)
    message("ci: gate ${gate} failed: ${why}")
    ci_summary()
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
        cmake_language(EXIT 1)
    else()
        message(FATAL_ERROR "ci: failed")
    endif()
endmacro()

macro(ci_pass)
    math(EXPR passed "${passed} + 1")
endmacro()

macro(ci_skip gate why)
    message(WARNING "ci: gate ${gate} skipped: ${why}")
    math(EXPR skipped "${skipped} + 1")
endmacro()

# Runs one command from the repository root, printing it first. Sets ci_rc and ci_output.
function(ci_run)
    list(JOIN ARGN " " printable)
    message(STATUS "ci> ${printable}")
    execute_process(COMMAND ${ARGN}
        WORKING_DIRECTORY "${root}"
        RESULT_VARIABLE rc
        OUTPUT_VARIABLE out
        ERROR_VARIABLE out
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
    set(ci_rc "${rc}" PARENT_SCOPE)
    set(ci_output "${out}" PARENT_SCOPE)
endfunction()

# Configure, build and (unless NO_TEST) test one preset. Sets ci_rc, ci_output and ci_step.
function(ci_preset name)
    cmake_parse_arguments(arg "NO_TEST" "" "" ${ARGN})
    set(preset "${PRESET_PREFIX}${name}")
    set(steps configure build)
    if(NOT arg_NO_TEST)
        list(APPEND steps test)
    endif()
    foreach(step IN LISTS steps)
        # foreach restores its loop variable afterwards, so keep the step in another one.
        set(current "${step}")
        if(step STREQUAL "configure")
            ci_run("${CMAKE_COMMAND}" --preset "${preset}")
        elseif(step STREQUAL "build")
            ci_run("${CMAKE_COMMAND}" --build --preset "${preset}")
        else()
            ci_run("${CMAKE_CTEST_COMMAND}" --preset "${preset}")
        endif()
        if(NOT ci_rc EQUAL 0)
            break()
        endif()
    endforeach()
    set(ci_rc "${ci_rc}" PARENT_SCOPE)
    set(ci_output "${ci_output}" PARENT_SCOPE)
    set(ci_step "${current}" PARENT_SCOPE)
endfunction()

# Every C++ source that clang-format checks: outside shared/, build trees and generated files.
file(GLOB_RECURSE sources RELATIVE "${root}"
    "${root}/*.hpp" "${root}/*.cpp" "${root}/*.h" "${root}/*.cc")
list(FILTER sources EXCLUDE REGEX "^(shared|build[^/]*|out|vcpkg_installed|\\.git)/")
list(FILTER sources EXCLUDE REGEX "^include/nepdate/detail/calendar_tables\\.hpp$")
list(FILTER sources EXCLUDE REGEX "^patro/src/patro_tables\\.hpp$")
list(FILTER sources EXCLUDE REGEX "^patro/include/nepdate/patro_config\\.hpp$")
list(SORT sources)

# ---- gate 1: format ----
find_program(clang_format NAMES clang-format-18 clang-format)
set(format_version "")
if(clang_format)
    execute_process(COMMAND "${clang_format}" --version OUTPUT_VARIABLE format_version)
endif()
if(NOT format_version MATCHES "version 18\\.")
    ci_skip(format "clang-format 18 not found")
else()
    # Batches keep the command line under the Windows limit.
    set(batch "")
    set(format_rc 0)
    foreach(source IN LISTS sources)
        list(APPEND batch "${source}")
        list(LENGTH batch n)
        if(n EQUAL 40)
            ci_run("${clang_format}" --dry-run --Werror ${batch})
            if(NOT ci_rc EQUAL 0)
                set(format_rc 1)
            endif()
            set(batch "")
        endif()
    endforeach()
    if(batch)
        ci_run("${clang_format}" --dry-run --Werror ${batch})
        if(NOT ci_rc EQUAL 0)
            set(format_rc 1)
        endif()
    endif()
    if(format_rc)
        ci_fail(format "run clang-format -i on the files above")
    endif()
    ci_pass()
endif()

# ---- gate 2: ascii ----
message(STATUS "ci> 7-bit ASCII check of include/ and patro/include/")
file(GLOB_RECURSE headers "${root}/include/*" "${root}/patro/include/*")
list(SORT headers)
foreach(header IN LISTS headers)
    file(READ "${header}" hex HEX)
    if(hex STREQUAL "")
        continue()
    endif()
    string(REGEX REPLACE "(..)" "\\1;" bytes "${hex}")
    set(high "${bytes}")
    list(FILTER high INCLUDE REGEX "^[89a-f]")
    if(high)
        list(GET high 0 first)
        list(FIND bytes "${first}" offset)
        file(RELATIVE_PATH relative "${root}" "${header}")
        ci_fail(ascii "${relative}: byte 0x${first} at offset ${offset}; write Devanagari as octal escapes (ADR-0001 section 6)")
    endif()
endforeach()
ci_pass()

# ---- gate 3: codegen ----
ci_run("${CMAKE_COMMAND}" --preset "${PRESET_PREFIX}dev")
if(ci_rc EQUAL 0)
    ci_run("${CMAKE_COMMAND}" --build --preset "${PRESET_PREFIX}dev" --target codegen_check)
endif()
if(NOT ci_rc EQUAL 0)
    ci_fail(codegen "generated files differ from shared/data: build the codegen target")
endif()
ci_pass()

# ---- gate 4: build and test ----
ci_run("${CMAKE_COMMAND}" --workflow --preset "${PRESET_PREFIX}dev")
if(NOT ci_rc EQUAL 0)
    ci_fail(build-and-test "cmake --workflow --preset ${PRESET_PREFIX}dev")
endif()
ci_pass()

# ---- gate 5: no exceptions, no RTTI ----
ci_preset(noexcept)
if(NOT ci_rc EQUAL 0)
    ci_fail(no-exceptions "preset ${PRESET_PREFIX}noexcept, step ${ci_step}")
endif()
ci_pass()

# ---- gate 6: clang-tidy ----
find_program(clang_tidy NAMES clang-tidy-18 clang-tidy)
if(NOT clang_tidy)
    ci_skip(tidy "clang-tidy not found")
else()
    ci_preset(tidy NO_TEST)
    if(NOT ci_rc EQUAL 0)
        # Gate 4 already compiled the same sources, so a compiler error from clang-tidy means it
        # can't parse this toolchain's standard library, not that the code is wrong.
        if(ci_step STREQUAL "build" AND ci_output MATCHES "\\[clang-diagnostic-error\\]")
            ci_skip(tidy "clang-tidy can't parse this toolchain's standard library headers")
        else()
            ci_fail(tidy "preset ${PRESET_PREFIX}tidy, step ${ci_step}")
        endif()
    else()
        ci_pass()
    endif()
endif()

# ---- gate 7: sanitizers ----
ci_preset(asan-ubsan)
if(NOT ci_rc EQUAL 0)
    # cmake/instrumentation.cmake stops the configure with this text when the flags don't link.
    if(ci_step STREQUAL "configure" AND ci_output MATCHES "is not supported by this compiler")
        ci_skip(sanitizers "the toolchain can't build with NEPDATE_SANITIZE=address,undefined")
    else()
        ci_fail(sanitizers "preset ${PRESET_PREFIX}asan-ubsan, step ${ci_step}")
    endif()
else()
    ci_pass()
endif()

# ---- gate 8: coverage ----
find_program(gcovr NAMES gcovr)
if(NOT gcovr)
    ci_skip(coverage "gcovr not found")
else()
    ci_preset(coverage)
    if(ci_rc EQUAL 0)
        ci_run("${gcovr}" --root "${root}"
            --filter "${root}/include/" --filter "${root}/patro/"
            --fail-under-line 90 --fail-under-branch 80
            "${root}/build/${PRESET_PREFIX}coverage")
        set(ci_step gcovr)
    endif()
    if(NOT ci_rc EQUAL 0)
        ci_fail(coverage "preset ${PRESET_PREFIX}coverage, step ${ci_step}")
    endif()
    ci_pass()
endif()

ci_summary()
