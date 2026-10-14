// C3-04: std::formatter<nepdate::date>. Compiled out where the standard library has no <format>;
// the test named below then reports itself as skipped.

#include <nepdate/format.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <version>

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#include <format>

using nepdate::date;
using nepdate::make_date;

TEST_CASE("std::format prints a date", "[format]") {
    const date d = make_date(2081, 4, 15).value();
    CHECK(std::format("{}", d) == "2081/04/15");
    CHECK(std::format("{:}", d) == "2081/04/15");
    CHECK(std::format("{:yyyy-MM-dd}", d) == "2081-04-15");
    CHECK(std::format("{:MMMM d, yyyy}", d) == "Shrawan 15, 2081");
    CHECK(std::format("{:s}", d) == "2081-04-15");
    CHECK(std::format("{:D}", d) == "Shrawan 15, 2081");
    CHECK(std::format("[{}] [{:dd}]", d, d) == "[2081/04/15] [15]");
}

TEST_CASE("std::format writes a result longer than its buffer in pieces", "[format]") {
    const date d = make_date(2081, 4, 15).value();
    std::string pattern;
    std::string expected;
    for (int i = 0; i < 60; ++i) {
        pattern += "MMMM yyyy ";
        expected += "Shrawan 2081 ";
    }
    CHECK(std::vformat("{:" + pattern + "}", std::make_format_args(d)) == expected);
}

#else

TEST_CASE("std::format prints a date", "[format]") {
    SKIP("this standard library has no <format> (__cpp_lib_format < 201907L)");
}

#endif
