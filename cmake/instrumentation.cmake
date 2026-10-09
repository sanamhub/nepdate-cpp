# Build modes for the presets noexcept, asan-ubsan, tsan, coverage and fuzz (ADR-0004 §1). They
# apply to tests and tools through nepdate_set_instrumentation(), never to the nepdate INTERFACE
# target, so a user of the library never inherits them.

option(NEPDATE_NO_EXCEPTIONS "Build tests without exceptions and RTTI (gate 5)" OFF)
set(NEPDATE_SANITIZE "" CACHE STRING
    "Comma-separated -fsanitize= list for tests and tools, for example address,undefined")
option(NEPDATE_COVERAGE "Build tests with --coverage for gcovr (gate 8)" OFF)

set(_nepdate_sanitize_compile "")
set(_nepdate_sanitize_link "")
if(NEPDATE_SANITIZE)
    if(MSVC)
        # MSVC has AddressSanitizer only (ADR-0004 §1); the other kinds are dropped.
        if(NOT NEPDATE_SANITIZE MATCHES "address")
            message(FATAL_ERROR
                "NEPDATE_SANITIZE=${NEPDATE_SANITIZE} is not supported by this compiler")
        endif()
        set(_nepdate_sanitize_compile /fsanitize=address)
    else()
        set(_nepdate_sanitize_compile
            -fsanitize=${NEPDATE_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
        set(_nepdate_sanitize_link -fsanitize=${NEPDATE_SANITIZE})
    endif()

    # Fail at configure time, with a message tools/ci.cmake recognises, instead of at link time.
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "${_nepdate_sanitize_compile}")
    list(JOIN CMAKE_REQUIRED_FLAGS " " CMAKE_REQUIRED_FLAGS)
    set(CMAKE_REQUIRED_LINK_OPTIONS ${_nepdate_sanitize_link})
    check_cxx_source_compiles("int main() { return 0; }" NEPDATE_SANITIZE_LINKS)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(NOT NEPDATE_SANITIZE_LINKS)
        message(FATAL_ERROR
            "NEPDATE_SANITIZE=${NEPDATE_SANITIZE} is not supported by this compiler")
    endif()
endif()

function(nepdate_set_instrumentation target)
    if(NEPDATE_NO_EXCEPTIONS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /EHs-c- /GR-)
            target_compile_definitions(${target} PRIVATE _HAS_EXCEPTIONS=0)
        else()
            target_compile_options(${target} PRIVATE -fno-exceptions -fno-rtti)
        endif()
    endif()
    if(NEPDATE_SANITIZE)
        target_compile_options(${target} PRIVATE ${_nepdate_sanitize_compile})
        target_link_options(${target} PRIVATE ${_nepdate_sanitize_link})
    endif()
    if(NEPDATE_COVERAGE)
        target_compile_options(${target} PRIVATE --coverage)
        target_link_options(${target} PRIVATE --coverage)
    endif()
endfunction()
