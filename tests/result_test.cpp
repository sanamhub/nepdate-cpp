// C1-02: errc and the generic result<T>. result<date> is tested in date_test.cpp.

#include <nepdate/detail/result.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

using nepdate::errc;
using nepdate::result;

TEST_CASE("result<int> holds a value or an error", "[result]") {
    STATIC_REQUIRE(std::is_trivially_copyable_v<result<int>>);

    constexpr result<int> ok = 7;
    STATIC_REQUIRE(ok.has_value());
    STATIC_REQUIRE(static_cast<bool>(ok));
    STATIC_REQUIRE(ok.value() == 7);
    STATIC_REQUIRE(*ok == 7);
    STATIC_REQUIRE(ok.value_or(1) == 7);

    constexpr result<int> bad = errc::invalid_day;
    STATIC_REQUIRE(!bad.has_value());
    STATIC_REQUIRE(!static_cast<bool>(bad));
    STATIC_REQUIRE(bad.error() == errc::invalid_day);
    STATIC_REQUIRE(bad.value_or(1) == 1);

    STATIC_REQUIRE(ok == result<int>{7});
    STATIC_REQUIRE(ok != result<int>{8});
    STATIC_REQUIRE(ok != bad);
    STATIC_REQUIRE(bad == result<int>{errc::invalid_day});
    STATIC_REQUIRE(bad != result<int>{errc::out_of_range});
}

namespace {

struct pair {
    int a;
    int b;
    friend constexpr bool operator==(pair, pair) noexcept = default;
};

} // namespace

TEST_CASE("operator-> reaches the value", "[result]") {
    constexpr result<pair> r = pair{1, 2};
    STATIC_REQUIRE(r->b == 2);
    STATIC_REQUIRE(r == result<pair>{pair{1, 2}});
}

TEST_CASE("to_string names every errc", "[result]") {
    STATIC_REQUIRE(to_string(errc::out_of_range) == "out_of_range");
    STATIC_REQUIRE(to_string(errc::invalid_month) == "invalid_month");
    STATIC_REQUIRE(to_string(errc::invalid_day) == "invalid_day");
    STATIC_REQUIRE(to_string(errc::invalid_gregorian) == "invalid_gregorian");
    STATIC_REQUIRE(to_string(errc::invalid_character) == "invalid_character");
    STATIC_REQUIRE(to_string(errc::wrong_group_count) == "wrong_group_count");
    STATIC_REQUIRE(to_string(errc::number_too_long) == "number_too_long");
    STATIC_REQUIRE(to_string(errc::ambiguous) == "ambiguous");
    STATIC_REQUIRE(to_string(errc::unrecognized) == "unrecognized");
    STATIC_REQUIRE(to_string(errc{}).empty());
    STATIC_REQUIRE(to_string(static_cast<errc>(10)).empty());
}

TEST_CASE("is_parse_error is true for exactly the five parse kinds", "[result]") {
    int count = 0;
    for (unsigned v = 0; v <= 255; ++v) {
        const auto e = static_cast<errc>(v);
        if (is_parse_error(e)) {
            ++count;
            CHECK(v >= 5);
            CHECK(v <= 9);
        }
    }
    CHECK(count == 5);
}
