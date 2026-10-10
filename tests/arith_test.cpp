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
using nepdate::overflow;

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

TEST_CASE("ALGORITHM section 8 examples", "[arith][add_months]") {
    constexpr date d = bs(2081, 4, 32);
    STATIC_REQUIRE(d.add_months(2, overflow::clamp) == bs(2081, 6, 30));
    STATIC_REQUIRE(d.add_months(2, overflow::spill) == bs(2081, 7, 2));
    STATIC_REQUIRE(d.add_months(5, overflow::clamp) == bs(2081, 9, 29));
    STATIC_REQUIRE(d.add_months(5, overflow::spill) == bs(2081, 10, 3));
    STATIC_REQUIRE(d.add_months(2) == d.add_months(2, overflow::clamp));
    CHECK(d.add_months(-2, overflow::clamp) == bs(2081, 2, 32));
    CHECK(d.add_months(-2, overflow::spill) == bs(2081, 2, 32));
    CHECK(d.add_months(0, overflow::spill) == d);
}

TEST_CASE("add_months and add_years across years and with negative amounts",
          "[arith][add_months]") {
    CHECK(bs(2080, 11, 15).add_months(5) == bs(2081, 4, 15));
    CHECK(bs(2080, 3, 15).add_months(-5) == bs(2079, 10, 15));
    CHECK(bs(2081, 4, 32).add_years(1, overflow::clamp) == bs(2082, 4, 31));
    CHECK(bs(2081, 4, 32).add_years(1, overflow::spill) == bs(2082, 5, 1));
    CHECK(bs(2081, 4, 32).add_years(-1) == bs(2080, 4, 32));
    CHECK(date::min().add_years(298) == bs(2199, 1, 1));
    CHECK(date::max().add_years(-298) == bs(1901, 12, 30));
}

TEST_CASE("add_months and add_years out of range", "[arith][add_months]") {
    CHECK(date::max().add_months(1).error() == errc::out_of_range);
    CHECK(date::min().add_months(-1).error() == errc::out_of_range);
    CHECK(date::min().add_months(3587) == bs(2199, 12, 1));
    CHECK(date::min().add_months(3588).error() == errc::out_of_range);
    CHECK(date::max().add_months(-3587) == bs(1901, 1, 30));
    CHECK(date::max().add_months(-3588).error() == errc::out_of_range);
    CHECK(date::min().add_years(299).error() == errc::out_of_range);
    CHECK(date::max().add_years(-299).error() == errc::out_of_range);
    // Guards before the arithmetic: UBSan would catch an overflow in 12 * n or index + n.
    CHECK(bs(2081, 4, 15).add_years(INT_MAX).error() == errc::out_of_range);
    CHECK(bs(2081, 4, 15).add_years(INT_MIN).error() == errc::out_of_range);
    CHECK(bs(2081, 4, 15).add_months(INT_MAX).error() == errc::out_of_range);
    CHECK(bs(2081, 4, 15).add_months(INT_MIN).error() == errc::out_of_range);
    CHECK(bs(2081, 4, 15).add_months(INT_MIN, overflow::spill).error() == errc::out_of_range);
}

TEST_CASE("spill out of the last month is out_of_range", "[arith][add_months]") {
    // Chaitra 2199 has 30 days, so day 31 of an earlier month spills into 2200.
    const date d = make_date(2199, 1, 31).value();
    CHECK(d.add_months(11, overflow::clamp) == bs(2199, 12, 30));
    CHECK(d.add_months(11, overflow::spill).error() == errc::out_of_range);
}
