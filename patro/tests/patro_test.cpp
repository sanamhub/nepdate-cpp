// C4-01: the patro skeleton builds and links against the core. C4-03 brings the API tests.
#include <nepdate/patro.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("the patro links against the core", "[patro]") {
    CHECK(nepdate::date::min().year() == 1901);
}
