// from_sys_time, today and the <chrono> interop (PORT-PLAN phase 4).

#include <nepdate/date.hpp>
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
