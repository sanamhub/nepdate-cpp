// result<date> and date itself: construction, accessors, comparison and hashing.

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>

#include <climits>
#include <cstdint>
#include <functional>
#include <type_traits>

using nepdate::date;
using nepdate::errc;
using nepdate::make_date;
using nepdate::month;
using nepdate::result;

TEST_CASE("date and result<date> are 4 trivially copyable bytes", "[date]") {
    STATIC_REQUIRE(sizeof(date) == 4);
    STATIC_REQUIRE(sizeof(result<date>) == 4);
    STATIC_REQUIRE(std::is_trivially_copyable_v<date>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<result<date>>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<result<int>>);
    STATIC_REQUIRE(!std::is_default_constructible_v<date>);
}

TEST_CASE("result<date> carries the error of make_date", "[date][result]") {
    STATIC_REQUIRE(!make_date(2081, 4, 33).has_value());
    STATIC_REQUIRE(make_date(2081, 4, 33).error() == errc::invalid_day);
    STATIC_REQUIRE(make_date(2081, 4, 32).has_value());
    STATIC_REQUIRE(make_date(2081, 4, 32).value().day() == 32);
    STATIC_REQUIRE(make_date(2081, 4, 33).value_or(date::min()) == date::min());
    STATIC_REQUIRE(make_date(2081, 4, 32) == make_date(2081, 4, 32));
    STATIC_REQUIRE(make_date(2081, 4, 32) != make_date(2081, 4, 31));
    STATIC_REQUIRE(make_date(2081, 4, 33) == result<date>{errc::invalid_day});
    STATIC_REQUIRE(make_date(2081, 4, 33) != make_date(2081, 13, 1));
    STATIC_REQUIRE(make_date(2081, 4, 15)->month() == month::shrawan);
}

TEST_CASE("make_date checks year, then month, then day", "[date]") {
    CHECK(make_date(2081, 4, 32).has_value());
    CHECK(make_date(2081, 4, 33).error() == errc::invalid_day);
    CHECK(make_date(2081, 13, 1).error() == errc::invalid_month);
    CHECK(make_date(2081, 0, 1).error() == errc::invalid_month);
    CHECK(make_date(2081, 1, 0).error() == errc::invalid_day);
    CHECK(make_date(1900, 13, 40).error() == errc::out_of_range);
    CHECK(make_date(2200, 1, 1).error() == errc::out_of_range);
    CHECK(make_date(INT_MIN, 1, 1).error() == errc::out_of_range);
    CHECK(make_date(INT_MAX, 1, 1).error() == errc::out_of_range);
    CHECK(make_date(2081, INT_MIN, 1).error() == errc::invalid_month);
    CHECK(make_date(2081, 4, INT_MAX).error() == errc::invalid_day);
    CHECK(make_date(2081, 4, INT_MIN).error() == errc::invalid_day);
    CHECK(make_date(2081, month::shrawan, 32) == make_date(2081, 4, 32));
}

TEST_CASE("min and max are the ends of the calendar", "[date]") {
    STATIC_REQUIRE(date::min() == make_date(1901, 1, 1).value());
    STATIC_REQUIRE(date::max() == make_date(2199, 12, 30).value());
    STATIC_REQUIRE(date::min() < date::max());
    STATIC_REQUIRE(!make_date(2199, 12, 31).has_value());
}

TEST_CASE("accessors read the packed fields", "[date]") {
    constexpr date d = make_date(2081, 4, 15).value();
    STATIC_REQUIRE(d.year() == 2081);
    STATIC_REQUIRE(d.month() == month::shrawan);
    STATIC_REQUIRE(d.day() == 15);
    STATIC_REQUIRE(d.month_length() == 32);
    STATIC_REQUIRE(d.first_of_month() == make_date(2081, 4, 1).value());
    STATIC_REQUIRE(d.last_of_month() == make_date(2081, 4, 32).value());
    STATIC_REQUIRE(make_date(2081, 1, 1)->day_of_year() == 1);
}

TEST_CASE("every year is 364 to 367 days, the sum of its months", "[date]") {
    for (int y = 1901; y <= 2199; ++y) {
        int sum = 0;
        for (int m = 1; m <= 12; ++m) {
            sum += make_date(y, m, 1)->month_length();
        }
        const int length = make_date(y, 1, 1)->year_length();
        REQUIRE(length == sum);
        REQUIRE(length >= 364);
        REQUIRE(length <= 367);
        REQUIRE(make_date(y, 12, 1)->last_of_month().day_of_year() == length);
    }
}

TEST_CASE("comparison follows the calendar", "[date]") {
    constexpr date a = make_date(2080, 12, 30).value();
    constexpr date b = make_date(2081, 1, 1).value();
    STATIC_REQUIRE(a < b);
    STATIC_REQUIRE(b > a);
    STATIC_REQUIRE(a <= a);
    STATIC_REQUIRE(a != b);
    STATIC_REQUIRE((a <=> b) == std::strong_ordering::less);
}

TEST_CASE("std::hash hashes the packed value", "[date]") {
    const date d = make_date(2081, 4, 15).value();
    const std::uint32_t packed = 2081U << 16U | 4U << 8U | 15U;
    CHECK(std::hash<date>{}(d) == std::hash<std::uint32_t>{}(packed));
    CHECK(std::hash<date>{}(d) == std::hash<date>{}(make_date(2081, 4, 15).value()));
}
