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
        # /RTC1 (CMake's Debug default) can't be combined with /fsanitize=address. The vcpkg
        # libraries are built without ASan, so the STL container annotations must be off on our
        # side too, or the linker reports an annotation mismatch (LNK2038).
        string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        set(_nepdate_sanitize_compile /fsanitize=address /D_DISABLE_VECTOR_ANNOTATION
            /D_DISABLE_STRING_ANNOTATION /D_DISABLE_OPTIONAL_ANNOTATION)
    else()
        set(_nepdate_sanitize_compile
            -fsanitize=${NEPDATE_SANITIZE} -fno-sanitize-recover=all -fno-omit-frame-pointer)
        set(_nepdate_sanitize_link -fsanitize=${NEPDATE_SANITIZE})
    endif()

    # Fail at configure time, with a message tools/ci.cmake recognises, instead of at link time.
    # Some toolchains (zig 0.17 for x86_64-windows-gnu) accept -fsanitize=address and silently
    # build without it, so the probe also asks the compiler whether ASan or TSan is really on.
    set(_nepdate_probe "int main() { return 0; }\n")
    foreach(_kind IN ITEMS address thread)
        if(NEPDATE_SANITIZE MATCHES "${_kind}")
            string(TOUPPER "${_kind}" _upper)
            string(PREPEND _nepdate_probe
                "#if defined(__SANITIZE_${_upper}__)\n"
                "#elif defined(__has_feature)\n"
                "#if !__has_feature(${_kind}_sanitizer)\n#error ${_kind} sanitizer is off\n#endif\n"
                "#else\n#error ${_kind} sanitizer is off\n#endif\n")
        endif()
    endforeach()
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "${_nepdate_sanitize_compile}")
    list(JOIN CMAKE_REQUIRED_FLAGS " " CMAKE_REQUIRED_FLAGS)
    set(CMAKE_REQUIRED_LINK_OPTIONS ${_nepdate_sanitize_link})
    check_cxx_source_compiles("${_nepdate_probe}" NEPDATE_SANITIZE_LINKS)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(NOT NEPDATE_SANITIZE_LINKS)
        message(FATAL_ERROR
            "NEPDATE_SANITIZE=${NEPDATE_SANITIZE} is not supported by this compiler")
    endif()
endif()

function(nepdate_set_instrumentation target)
    # A static library isn't linked itself; whatever links it needs the runtime flags instead.
    get_target_property(type ${target} TYPE)
    if(type STREQUAL "STATIC_LIBRARY")
        set(link_scope INTERFACE)
    else()
        set(link_scope PRIVATE)
    endif()
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
        target_link_options(${target} ${link_scope} ${_nepdate_sanitize_link})
    endif()
    if(NEPDATE_COVERAGE)
        # Atomic counters: the thread-safety tests update the same counters from several threads,
        # and plain increments race, which gcov then reports as negative hit counts.
        target_compile_options(${target} PRIVATE --coverage -fprofile-update=atomic)
        target_link_options(${target} ${link_scope} --coverage)
    endif()
endfunction()
