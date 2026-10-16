// Gregorian conversion and the AD side of date.

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <chrono>
#include <climits>

using nepdate::date;
using nepdate::errc;
using nepdate::from_gregorian;
using nepdate::make_date;
namespace chr = std::chrono;

namespace {

constexpr chr::year_month_day ymd(int y, unsigned m, unsigned d) {
    return chr::year_month_day{chr::year{y}, chr::month{m}, chr::day{d}};
}

} // namespace

TEST_CASE("the ends of the calendar convert both ways", "[civil]") {
    STATIC_REQUIRE(from_gregorian(1844, 4, 11).value() == date::min());
    STATIC_REQUIRE(from_gregorian(2143, 4, 15).value() == date::max());
    STATIC_REQUIRE(date::min().to_year_month_day() == ymd(1844, 4, 11));
    STATIC_REQUIRE(date::max().to_year_month_day() == ymd(2143, 4, 15));
}

TEST_CASE("AD errors: out of range and not a real date", "[civil]") {
    CHECK(from_gregorian(1844, 4, 10).error() == errc::out_of_range);
    CHECK(from_gregorian(2143, 4, 16).error() == errc::out_of_range);
    CHECK(from_gregorian(2023, 2, 29).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(2023, 2, 30).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(2024, 2, 29).has_value());
    CHECK(from_gregorian(2023, 13, 1).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(2023, 4, 31).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(1900, 2, 29).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(2000, 2, 29).has_value());
    CHECK(from_gregorian(1800, 2, 29).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(1600, 2, 29).error() == errc::out_of_range);
    CHECK(from_gregorian(INT_MAX, 1, 1).error() == errc::out_of_range);
    CHECK(from_gregorian(INT_MIN, 1, 1).error() == errc::out_of_range);
    CHECK(from_gregorian(INT_MAX, 2, 29).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(2024, INT_MIN, 1).error() == errc::invalid_gregorian);
    CHECK(from_gregorian(2024, 1, INT_MAX).error() == errc::invalid_gregorian);
}

TEST_CASE("ALGORITHM section 9 reference values", "[civil]") {
    struct row {
        int bs_y, bs_m, bs_d;
        int ad_y;
        unsigned ad_m, ad_d;
        unsigned weekday; // 0 = Sunday
    };
    constexpr std::array<row, 5> rows = {{
        {1901, 1, 1, 1844, 4, 11, 4},
        {2081, 1, 1, 2024, 4, 13, 6},
        {2082, 1, 1, 2025, 4, 14, 1},
        {2083, 1, 1, 2026, 4, 14, 2},
        {2199, 12, 30, 2143, 4, 15, 9},
    }};
    for (const row& r : rows) {
        const date bs = make_date(r.bs_y, r.bs_m, r.bs_d).value();
        CHECK(bs.to_year_month_day() == ymd(r.ad_y, r.ad_m, r.ad_d));
        CHECK(from_gregorian(r.ad_y, static_cast<int>(r.ad_m), static_cast<int>(r.ad_d)).value() ==
              bs);
        if (r.weekday < 7) { // the spec gives no weekday for the last day
            CHECK(bs.weekday() == chr::weekday{r.weekday});
            CHECK(bs.weekday() == chr::weekday{bs.to_sys_days()});
        }
    }
}

TEST_CASE("year_month_day, sys_days and local_days overloads", "[civil]") {
    constexpr date d = make_date(2081, 4, 15).value();
    STATIC_REQUIRE(d.to_year_month_day() == ymd(2024, 7, 30));
    STATIC_REQUIRE(nepdate::from_gregorian(ymd(2024, 7, 30)).value() == d);
    STATIC_REQUIRE(nepdate::from_gregorian(ymd(2023, 2, 29)).error() == errc::invalid_gregorian);
    STATIC_REQUIRE(nepdate::from_sys_days(d.to_sys_days()).value() == d);
    STATIC_REQUIRE(
        nepdate::from_local_days(chr::local_days{d.to_sys_days().time_since_epoch()}).value() == d);
    STATIC_REQUIRE(nepdate::from_sys_days(chr::sys_days{chr::days{-45921}}).error() ==
                   errc::out_of_range);
    STATIC_REQUIRE(d.to_sys_days() == chr::sys_days{ymd(2024, 7, 30)});
}
