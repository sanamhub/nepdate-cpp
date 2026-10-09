#include <nepdate/nepdate.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

TEST_CASE("version macros agree with each other", "[version]") {
    constexpr std::string_view expected = "0.0.0";
    STATIC_REQUIRE(NEPDATE_VERSION_MAJOR == 0);
    STATIC_REQUIRE(NEPDATE_VERSION_MINOR == 0);
    STATIC_REQUIRE(NEPDATE_VERSION_PATCH == 0);
    STATIC_REQUIRE(std::string_view{NEPDATE_VERSION_STRING} == expected);
}
