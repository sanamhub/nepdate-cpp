# Warning flags of ADR-0004 §1 for the tests and tools. Never applied to the nepdate INTERFACE
# target: the core adds no options to its users (ADR-0001 §3). Presets turn them into errors with
# CMAKE_COMPILE_WARNING_AS_ERROR.
function(nepdate_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wold-style-cast
            -Wnon-virtual-dtor
        )
        # Doc comments are checked by Clang only; GCC has no -Wdocumentation.
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE -Wdocumentation)
        endif()
    endif()
endfunction()
