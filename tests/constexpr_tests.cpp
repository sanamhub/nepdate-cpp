// C1-08: compile-time checks (ADR-0004 §3). Every STATIC_REQUIRE here is evaluated by the compiler.

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <type_traits>

using nepdate::date;
using nepdate::errc;
using nepdate::from_gregorian;
using nepdate::make_date;
using nepdate::result;
namespace chr = std::chrono;

namespace {

constexpr chr::year_month_day ymd(int y, unsigned m, unsigned d) {
    return chr::year_month_day{chr::year{y}, chr::month{m}, chr::day{d}};
}

// One make_date(...).value() in a constexpr variable.
constexpr date kNewYear2081 = make_date(2081, 1, 1).value();

} // namespace

TEST_CASE("ALGORITHM section 9 holds at compile time", "[constexpr]") {
    STATIC_REQUIRE(make_date(1901, 1, 1)->to_year_month_day() == ymd(1844, 4, 11));
    STATIC_REQUIRE(kNewYear2081.to_year_month_day() == ymd(2024, 4, 13));
    STATIC_REQUIRE(make_date(2082, 1, 1)->to_year_month_day() == ymd(2025, 4, 14));
    STATIC_REQUIRE(make_date(2083, 1, 1)->to_year_month_day() == ymd(2026, 4, 14));
    STATIC_REQUIRE(make_date(2199, 12, 30)->to_year_month_day() == ymd(2143, 4, 15));

    STATIC_REQUIRE(from_gregorian(1844, 4, 11).value() == date::min());
    STATIC_REQUIRE(from_gregorian(2024, 4, 13).value() == kNewYear2081);
    STATIC_REQUIRE(from_gregorian(2025, 4, 14).value() == make_date(2082, 1, 1).value());
    STATIC_REQUIRE(from_gregorian(2026, 4, 14).value() == make_date(2083, 1, 1).value());
    STATIC_REQUIRE(from_gregorian(2143, 4, 15).value() == date::max());

    STATIC_REQUIRE(date::min().weekday() == chr::Thursday);
    STATIC_REQUIRE(kNewYear2081.weekday() == chr::Saturday);
    STATIC_REQUIRE(make_date(2082, 1, 1)->weekday() == chr::Monday);
    STATIC_REQUIRE(make_date(2083, 1, 1)->weekday() == chr::Tuesday);
}

TEST_CASE("sizes and traits of C1-02 and C1-03", "[constexpr]") {
    STATIC_REQUIRE(sizeof(date) == 4);
    STATIC_REQUIRE(sizeof(result<date>) == 4);
    STATIC_REQUIRE(std::is_trivially_copyable_v<date>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<result<date>>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<result<int>>);
    STATIC_REQUIRE(!std::is_default_constructible_v<date>);
    STATIC_REQUIRE(make_date(2081, 4, 33).error() == errc::invalid_day);
    STATIC_REQUIRE(from_gregorian(2023, 2, 29).error() == errc::invalid_gregorian);
}
