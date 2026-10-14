// C4-06: the interop rows of phase 4 of the C# catalogue (shared/spec/vectors/csharp-tests/
// catalog.tsv), plus the NepaliDateIsRelativeTests rows as D-14 deviations (PARITY.md D-14: the
// relative predicates are not ported; compare with today() and add_days(+-1)). The patro rows
// live in patro/tests/cs_phase4.cpp.

#include <nepdate/date.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>

using nepdate::date;
using nepdate::errc;
namespace chr = std::chrono;

namespace {

chr::sys_seconds at(chr::year_month_day ymd, int h, int m, int s) {
    return chr::sys_days{ymd} + chr::hours{h} + chr::minutes{m} + chr::seconds{s};
}

} // namespace

// ---- DateTimeExtensionsTests (C#'s DateTime.ToNepaliDate is from_sys_time(t, 0min)) ----

TEST_CASE("DateTimeExtensionsTests.ToNepaliDate_ValidDateTime_ReturnsCorrectNepaliDate", "[cs]") {
    const nepdate::date d =
        nepdate::from_sys_time(at(chr::year{2023} / 9 / 1, 12, 0, 0), chr::minutes{0}).value();
    CHECK(d.year() == 2080);
}

TEST_CASE("DateTimeExtensionsTests.ToNepaliDate_MinValue_ThrowsArgumentOutOfRangeException",
          "[cs]") {
    CHECK(nepdate::from_sys_time(chr::sys_days{chr::year{1} / 1 / 1}, chr::minutes{0}).error() ==
          errc::out_of_range);
}

TEST_CASE("DateTimeExtensionsTests.ToNepaliDate_MaxValue_ThrowsArgumentOutOfRangeException",
          "[cs]") {
    CHECK(
        nepdate::from_sys_time(chr::sys_days{chr::year{9999} / 12 / 31}, chr::minutes{0}).error() ==
        errc::out_of_range);
}

TEST_CASE("DateTimeExtensionsTests.ToNepaliDate_DateBeforeMinSupported_"
          "ThrowsArgumentOutOfRangeException",
          "[cs]") {
    CHECK(
        nepdate::from_sys_time(chr::sys_days{chr::year{1844} / 4 / 10}, chr::minutes{0}).error() ==
        errc::out_of_range);
}

TEST_CASE("DateTimeExtensionsTests.ToNepaliDate_DateAfterMaxSupported_"
          "ThrowsArgumentOutOfRangeException",
          "[cs]") {
    CHECK(
        nepdate::from_sys_time(chr::sys_days{chr::year{2143} / 4 / 16}, chr::minutes{0}).error() ==
        errc::out_of_range);
}

// ---- NepaliDateIsRelativeTests: D-14 (PARITY.md D-14) ----

// cs-deviation: NepaliDateIsRelativeTests.IsToday_TodaysDate_ReturnsTrue D-14 (IsToday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsToday_YesterdaysDate_ReturnsFalse D-14 (IsToday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsToday_TomorrowsDate_ReturnsFalse D-14 (IsToday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsToday_ArbitraryPastDate_ReturnsFalse D-14 (IsToday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsYesterday_YesterdaysDate_ReturnsTrue D-14 (IsYesterday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsYesterday_TodaysDate_ReturnsFalse D-14 (IsYesterday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsYesterday_TomorrowsDate_ReturnsFalse D-14 (IsYesterday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsYesterday_ArbitraryPastDate_ReturnsFalse D-14 (IsYesterday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsTomorrow_TomorrowsDate_ReturnsTrue D-14 (IsTomorrow is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsTomorrow_TodaysDate_ReturnsFalse D-14 (IsTomorrow is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsTomorrow_YesterdaysDate_ReturnsFalse D-14 (IsTomorrow is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.IsTomorrow_ArbitraryPastDate_ReturnsFalse D-14 (IsTomorrow is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.Today_OnlyIsToday_IsTrue D-14 (IsToday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.Yesterday_OnlyIsYesterday_IsTrue D-14 (IsYesterday is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.Tomorrow_OnlyIsTomorrow_IsTrue D-14 (IsTomorrow is not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.TwoDaysAhead_NoneAreTrue D-14 (the relative predicates are not ported; compare with today() and add_days(+-1))
// cs-deviation: NepaliDateIsRelativeTests.TwoDaysBehind_NoneAreTrue D-14 (the relative predicates are not ported; compare with today() and add_days(+-1))
