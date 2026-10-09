find_package(nlohmann_json 3 CONFIG REQUIRED)

add_library(nepdate_codegen_lib STATIC calendar.cpp calendar.hpp)
target_include_directories(nepdate_codegen_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(nepdate_codegen_lib PUBLIC cxx_std_20)
target_link_libraries(nepdate_codegen_lib PUBLIC nlohmann_json::nlohmann_json)
nepdate_set_warnings(nepdate_codegen_lib)
nepdate_set_instrumentation(nepdate_codegen_lib)

add_executable(nepdate_codegen main.cpp)
target_link_libraries(nepdate_codegen PRIVATE nepdate_codegen_lib)
nepdate_set_warnings(nepdate_codegen)
nepdate_set_instrumentation(nepdate_codegen)

# $<TARGET_FILE> instead of the bare target name: CMake only substitutes the name when it isn't
# cross-compiling, and a toolchain file that sets CMAKE_SYSTEM_NAME counts as cross-compiling.
set(_codegen_args --shared "${PROJECT_SOURCE_DIR}/shared" --out "${PROJECT_SOURCE_DIR}")

add_custom_target(codegen
    COMMAND $<TARGET_FILE:nepdate_codegen> ${_codegen_args}
    COMMENT "Regenerating tables from shared/data"
    DEPENDS nepdate_codegen
    VERBATIM
)
add_custom_target(codegen_check
    COMMAND $<TARGET_FILE:nepdate_codegen> ${_codegen_args} --check
    COMMENT "Checking generated tables against shared/data"
    DEPENDS nepdate_codegen
    VERBATIM
)

if(NEPDATE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
