find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

# The core must reach its users with no link libraries (ADR-0001 §3, gate 9 from C4).
get_target_property(_nepdate_links nepdate INTERFACE_LINK_LIBRARIES)
if(_nepdate_links)
    message(FATAL_ERROR "nepdate::nepdate links ${_nepdate_links}; it must link nothing")
endif()

add_executable(nepdate_tests
    version_test.cpp
)
target_link_libraries(nepdate_tests PRIVATE nepdate::nepdate Catch2::Catch2WithMain)
nepdate_set_warnings(nepdate_tests)
nepdate_set_instrumentation(nepdate_tests)
if(NEPDATE_NO_EXCEPTIONS)
    target_compile_definitions(nepdate_tests PRIVATE CATCH_CONFIG_DISABLE_EXCEPTIONS)
endif()
catch_discover_tests(nepdate_tests)
