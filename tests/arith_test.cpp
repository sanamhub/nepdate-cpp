// C2-01 to C2-03: add_days, next, prev, days_until, add_months, add_years and diff
// (ALGORITHM section 8 and 8a). The golden files are in vectors_test.cpp.

#include "random_pairs.hpp"

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <climits>
#include <cstdint>
#include <optional>

using nepdate::date;
using nepdate::errc;
using nepdate::make_date;

namespace {

constexpr int kMaxSerial = 109211;

constexpr date bs(int y, int m, int d) {
    return make_date(y, m, d).value();
}

// The date of a serial day, through add_days from min().
date from_serial(int s) {
    return date::min().add_days(s).value();
}

} // namespace

TEST_CASE("add_days at the ends of the calendar", "[arith][add_days]") {
    CHECK(date::max().add_days(1).error() == errc::out_of_range);
    CHECK(date::min().add_days(-1).error() == errc::out_of_range);
    CHECK(date::min().add_days(kMaxSerial) == date::max());
    CHECK(date::max().add_days(-kMaxSerial) == date::min());
    CHECK(date::min().add_days(kMaxSerial + 1).error() == errc::out_of_range);
    CHECK(date::max().add_days(-kMaxSerial - 1).error() == errc::out_of_range);
    // UBSan would report an int overflow here if the range check came after the addition.
    CHECK(date::max().add_days(INT_MAX).error() == errc::out_of_range);
    CHECK(date::min().add_days(INT_MIN).error() == errc::out_of_range);
    CHECK(bs(2081, 4, 15).add_days(INT_MAX).error() == errc::out_of_range);
    CHECK(bs(2081, 4, 15).add_days(INT_MIN).error() == errc::out_of_range);
}

TEST_CASE("add_days crosses months and years both ways", "[arith][add_days]") {
    STATIC_REQUIRE(bs(2080, 12, 30).add_days(5) == bs(2081, 1, 5));
    STATIC_REQUIRE(bs(2081, 1, 5).add_days(-5) == bs(2080, 12, 30));
    CHECK(bs(2081, 4, 32).add_days(1) == bs(2081, 5, 1));
    CHECK(bs(2081, 5, 1).add_days(-1) == bs(2081, 4, 32));
    CHECK(bs(2081, 4, 15).add_days(0) == bs(2081, 4, 15));
}

TEST_CASE("next and prev stop at the ends", "[arith][add_days]") {
    STATIC_REQUIRE(!date::max().next().has_value());
    STATIC_REQUIRE(!date::min().prev().has_value());
    STATIC_REQUIRE(date::min().next() == bs(1901, 1, 2));
    STATIC_REQUIRE(date::max().prev() == bs(2199, 12, 29));
    CHECK(bs(2081, 4, 32).next() == bs(2081, 5, 1));
    CHECK(bs(2081, 1, 1).prev() == bs(2080, 12, 30));
}

TEST_CASE("days_until is signed and spans the calendar", "[arith][days_until]") {
    STATIC_REQUIRE(date::min().days_until(date::max()) == kMaxSerial);
    STATIC_REQUIRE(date::max().days_until(date::min()) == -kMaxSerial);
    CHECK(bs(2081, 4, 15).days_until(bs(2081, 4, 15)) == 0);
}

TEST_CASE("add_days(-n) undoes add_days(n), and days_until measures n", "[arith][add_days]") {
    // Fixed seed, 10,000 pairs; n is drawn wider than the calendar so some sums fail.
    const auto [s, n] =
        GENERATE(nepdate_test::random_pairs(10000, 0, kMaxSerial, -2 * kMaxSerial, 2 * kMaxSerial));
    const date d = from_serial(s);
    const auto moved = d.add_days(n);
    const bool in_range = s + n >= 0 && s + n <= kMaxSerial;
    REQUIRE(moved.has_value() == in_range);
    if (moved) {
        CHECK(moved->add_days(-n) == d);
        CHECK(d.days_until(*moved) == n);
    } else {
        CHECK(moved.error() == errc::out_of_range);
    }
}
