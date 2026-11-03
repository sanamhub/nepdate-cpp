// from_sys_time, today and the <chrono> interop (PORT-PLAN phase 4).

#include <nepdate/date.hpp>
#include <nepdate/date_range.hpp>
#include <nepdate/detail/calendar_tables.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>

using nepdate::date;
using nepdate::errc;
using nepdate::make_date;
namespace chr = std::chrono;

namespace {

constexpr chr::sys_seconds at(chr::year_month_day ymd, int h, int m, int s) {
    return chr::sys_days{ymd} + chr::hours{h} + chr::minutes{m} + chr::seconds{s};
}

// An injected clock for the deterministic tests: 2024-07-30 18:30 UTC, fixed.
struct fixed_clock {
    [[nodiscard]] static constexpr chr::sys_seconds now() noexcept {
        return at(chr::year{2024} / 7 / 30, 18, 30, 0);
    }
};

} // namespace

TEST_CASE("from_sys_time adds the offset first", "[time]") {
    const chr::sys_seconds t = at(chr::year{2024} / 7 / 30, 18, 30, 0);
    CHECK(nepdate::from_sys_time(t, chr::minutes{0}) == make_date(2081, 4, 15));
    CHECK(nepdate::from_sys_time(t, nepdate::nepal_utc_offset) == make_date(2081, 4, 16));
    // Nepal's offset is +05:45; 18:30 + 05:45 is past midnight.
    CHECK(nepdate::nepal_utc_offset == chr::minutes{5 * 60 + 45});
}

TEST_CASE("to_sys_days and from_sys_days round-trip every serial", "[time][exhaustive]") {
    std::uint32_t failures = 0;
    for (std::uint32_t s = 0; s <= nepdate::detail::kMaxSerial; ++s) {
        const date d = nepdate::detail::from_serial(s);
        failures += nepdate::from_sys_days(d.to_sys_days()) == d ? 0U : 1U;
    }
    CHECK(failures == 0);
    CHECK(make_date(2081, 4, 15)->to_sys_days() == chr::sys_days{chr::year{2024} / 7 / 30});
}

TEST_CASE("clocks far outside the range are out_of_range", "[time]") {
    CHECK(nepdate::from_sys_time(chr::sys_days{chr::year{1800} / 1 / 1}, chr::minutes{0}).error() ==
          errc::out_of_range);
    CHECK(nepdate::from_sys_time(chr::sys_days{chr::year{2200} / 1 / 1}, chr::minutes{0}).error() ==
          errc::out_of_range);
    CHECK(nepdate::from_sys_time(chr::sys_seconds::min(), chr::minutes{0}).error() ==
          errc::out_of_range);
    CHECK(nepdate::from_sys_time(chr::sys_seconds::max(), chr::minutes{0}).error() ==
          errc::out_of_range);
}

TEST_CASE("extreme offsets and day counts never wrap into the range", "[time]") {
    const chr::sys_seconds t = at(chr::year{2024} / 7 / 30, 0, 0, 0);
    CHECK(nepdate::from_sys_time(t, chr::minutes::max()).error() == errc::out_of_range);
    CHECK(nepdate::from_sys_time(t, chr::minutes::min()).error() == errc::out_of_range);
    CHECK(nepdate::from_sys_time(chr::sys_seconds::max(), chr::minutes::min()).error() ==
          errc::out_of_range);
    // 2^32 days after 2024-07-30: a 32-bit day count would wrap back to 2024-07-30.
    const std::int64_t wrapped_day = (std::int64_t{1} << 32) + 19'934;
    const chr::sys_seconds far{chr::seconds{wrapped_day * 86'400}};
    CHECK(nepdate::from_sys_time(far, chr::minutes{0}).error() == errc::out_of_range);
}

TEST_CASE("from_sys_time floors before the epoch", "[time]") {
    // AD 1844-04-11 is the first supported day; one second before it is the day before.
    const chr::sys_seconds first = at(chr::year{1844} / 4 / 11, 0, 0, 0);
    const auto first_day = nepdate::from_sys_days(chr::sys_days{chr::year{1844} / 4 / 11});
    REQUIRE(first_day.has_value());
    CHECK(nepdate::from_sys_time(first, chr::minutes{0}) == first_day);
    CHECK(nepdate::from_sys_time(first + chr::hours{23} + chr::minutes{59} + chr::seconds{59},
                                 chr::minutes{0}) == first_day);
    CHECK(nepdate::from_sys_time(first - chr::seconds{1}, chr::minutes{0}).error() ==
          errc::out_of_range);
    CHECK(nepdate::from_sys_time(first, chr::minutes{-1}).error() == errc::out_of_range);
}

TEST_CASE("today is the current instant shifted by the offset", "[time]") {
    const nepdate::result<date> t = nepdate::today();
    REQUIRE(t.has_value());
    // NOLINTBEGIN(misc-include-cleaner): floor and days live in an internal libstdc++ header; <chrono> is the correct include
    const chr::sys_seconds now = chr::floor<chr::seconds>(chr::system_clock::now());
    CHECK(*t == nepdate::from_sys_days(chr::floor<chr::days>(now + nepdate::nepal_utc_offset)));
    // NOLINTEND(misc-include-cleaner)
    // The today-in-zone form: an explicit offset changes the day where the zones differ.
    CHECK(nepdate::today(chr::minutes{0}).has_value());
}

TEST_CASE("from_sys_time works at compile time", "[time][constexpr]") {
    STATIC_REQUIRE(nepdate::from_sys_time(at(chr::year{2024} / 7 / 30, 18, 30, 0),
                                          chr::minutes{0}) == make_date(2081, 4, 15));
    STATIC_REQUIRE(nepdate::from_sys_time(at(chr::year{2024} / 7 / 30, 18, 30, 0),
                                          nepdate::nepal_utc_offset) == make_date(2081, 4, 16));
}

TEST_CASE("today and the relative predicates take an injected clock", "[time]") {
    const auto utc = nepdate::today(chr::minutes{0}, fixed_clock{});
    REQUIRE(utc.has_value());
    CHECK(*utc == make_date(2081, 4, 15));
    const auto nepal = nepdate::today(nepdate::nepal_utc_offset, fixed_clock{});
    REQUIRE(nepal.has_value());
    CHECK(*nepal == make_date(2081, 4, 16));

    CHECK(make_date(2081, 4, 15)->is_today(chr::minutes{0}, fixed_clock{}));
    CHECK(make_date(2081, 4, 14)->is_yesterday(chr::minutes{0}, fixed_clock{}));
    CHECK(make_date(2081, 4, 16)->is_tomorrow(chr::minutes{0}, fixed_clock{}));
    CHECK(!make_date(2081, 4, 16)->is_today(chr::minutes{0}, fixed_clock{}));
    CHECK(!make_date(2081, 4, 15)->is_yesterday(chr::minutes{0}, fixed_clock{}));
    CHECK(!make_date(2081, 4, 15)->is_tomorrow(chr::minutes{0}, fixed_clock{}));
    // At the data ends the relative predicates are false, not an error.
    CHECK(!date::min().is_yesterday(chr::minutes{0}, fixed_clock{}));
    CHECK(!date::max().is_tomorrow(chr::minutes{0}, fixed_clock{}));
}

TEST_CASE("the current range factories take an injected clock", "[time][range]") {
    const auto month = nepdate::current_month(chr::minutes{0}, fixed_clock{});
    REQUIRE(month.has_value());
    CHECK(month->first() == make_date(2081, 4, 1));
    CHECK(month->last() == make_date(2081, 4, 32));

    const auto fiscal = nepdate::current_fiscal_year(chr::minutes{0}, fixed_clock{});
    REQUIRE(fiscal.has_value());
    CHECK(fiscal->first() == make_date(2081, 4, 1));
    CHECK(fiscal->last() == make_date(2082, 3, 32));

    const auto calendar = nepdate::current_calendar_year(chr::minutes{0}, fixed_clock{});
    REQUIRE(calendar.has_value());
    CHECK(calendar->first() == make_date(2081, 1, 1));
    CHECK(calendar->last() == make_date(2081, 12, 31));

    // The system-clock forms return a valid range too.
    CHECK(nepdate::current_month().has_value());
    CHECK(nepdate::current_calendar_year().has_value());
}
