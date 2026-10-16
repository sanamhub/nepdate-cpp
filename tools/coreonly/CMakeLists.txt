# The core-only program of gate 9 (ADR-0004 section 1). It links nothing but
# nepdate::nepdate; the gate builds it in Release and reads its bytes.
add_executable(coreonly main.cpp)
target_link_libraries(coreonly PRIVATE nepdate::nepdate)
nepdate_set_warnings(coreonly)
