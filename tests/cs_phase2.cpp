// C2-06: the phase-2 rows of C# NepDate's own tests (shared/spec/vectors/csharp-tests/catalog.tsv).
// Each row is a TEST_CASE named after its id, or a cs-deviation / cs-n/a line. The CTest test
// catalog_coverage (tools/check_catalog.cmake) checks that every row up to phase 2 is here.
//
// D-08: C# fiscal-year methods take a yearOffset or a month; here the fiscal year is a value
// (fiscal_year::of(d), plus or minus whole years) and a quarter is a quarter enumerator, so those
// rows build the same dates through fiscal_year and quarter_range.
//
// Following the hub PORT-PLAN rule, rows that need a later task's API carry a pending marker: the
// DateTime rows convert with from_sys_days(floor<days>(t)) until from_sys_time (C4-05), and the
// Current* rows use a fixed date until today() (C4-05).

#include <nepdate/date.hpp>
#include <nepdate/date_range.hpp>
#include <nepdate/fiscal_year.hpp>
#include <nepdate/parse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <optional>
#include <vector>

using nepdate::date;
using nepdate::date_range;
using nepdate::errc;
using nepdate::fiscal_year;
using nepdate::make_date;
using nepdate::month;
using nepdate::overflow;
using nepdate::quarter;
namespace chr = std::chrono;

namespace {

date bs(int y, int m, int d) {
    return make_date(y, m, d).value();
}

// The last day of BS month y-m.
date last_of(int y, int m) {
    return bs(y, m, 1).last_of_month();
}

// fiscal_year::of(bs(y, m, d)) shifted by offset years: C#'s yearOffset (D-08).
fiscal_year fy_of(int y, int m, int d, int offset = 0) {
    return fiscal_year{fiscal_year::of(bs(y, m, d)).start_year() + offset};
}

// Checks start() and end() of fy; an empty expectation is not checked.
void check_fy(fiscal_year fy, std::optional<date> start, std::optional<date> end) {
    if (start) {
        CHECK(fy.start() == *start);
    }
    if (end) {
        CHECK(fy.end() == *end);
    }
}

// Checks the first and last day of quarter q of fy; an empty expectation is not checked.
void check_quarter(fiscal_year fy, quarter q, std::optional<date> start, std::optional<date> end) {
    const auto r = fy.quarter_range(q);
    REQUIRE(r.has_value());
    if (start) {
        CHECK(r->first() == *start);
    }
    if (end) {
        CHECK(r->last() == *end);
    }
}

void check_fy_out_of_range(fiscal_year fy) {
    CHECK(fy.start().error() == errc::out_of_range);
    CHECK(fy.end().error() == errc::out_of_range);
}

void check_add_days(date d, int n, date want) {
    CHECK(d.add_days(n) == want);
}

void check_add_months(date d, int n, overflow o, date want) {
    CHECK(d.add_months(n, o) == want);
}

void check_contains(date first, date last, date d, bool want) {
    const auto r = date_range::make(first, last);
    REQUIRE(r.has_value());
    CHECK(r->contains(d) == want);
}

// C#'s empty range (end before start) is std::nullopt here.
void check_ranges_equal(date f1, date l1, date f2, date l2, bool want) {
    CHECK((date_range::make(f1, l1) == date_range::make(f2, l2)) == want);
}

std::vector<date> dates_of(const date_range& r) {
    return {r.begin(), r.end()};
}

// pending C4-05: C#'s new NepaliDate(DateTime) becomes from_sys_time(t, 0min); until then the
// date part goes through from_sys_days(floor<days>(t)).
nepdate::result<date> from_date_time(chr::sys_seconds t) {
    return nepdate::from_sys_days(chr::floor<chr::days>(t));
}

chr::sys_seconds at(chr::year_month_day ymd, int h, int m, int s) {
    return chr::sys_days{ymd} + chr::hours{h} + chr::minutes{m} + chr::seconds{s};
}

// pending C4-05: C#'s Current* rows read the clock; today() replaces this fixed date.
date fixed_today() {
    return bs(2081, 4, 15);
}

} // namespace

// ---- FiscalYearTests ----

TEST_CASE("FiscalYearTests.GetFiscalYearStartAndEndDate_ValidYear_ReturnsCorrectDates", "[cs]") {
    check_fy(fiscal_year{2080}, bs(2080, 4, 1), last_of(2081, 3));
}

TEST_CASE("FiscalYearTests.GetFiscalYearStartAndEndDate_InvalidYear_ThrowsException#1", "[cs]") {
    check_fy_out_of_range(fiscal_year{1843});
}

TEST_CASE("FiscalYearTests.GetFiscalYearStartAndEndDate_InvalidYear_ThrowsException#2", "[cs]") {
    check_fy_out_of_range(fiscal_year{2200});
}

TEST_CASE("FiscalYearTests.GetFiscalYearStartDate_ValidYear_ReturnsCorrectDate", "[cs]") {
    check_fy(fiscal_year{2080}, bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearTests.GetFiscalYearEndDate_ValidYear_ReturnsCorrectDate", "[cs]") {
    check_fy(fiscal_year{2080}, std::nullopt, last_of(2081, 3));
}

TEST_CASE("FiscalYearTests.FiscalYearStartAndEndDate_OnNepaliDateInstance_ReturnsCorrectDates",
          "[cs]") {
    check_fy(fy_of(2080, 6, 15), bs(2080, 4, 1), last_of(2081, 3));
}

TEST_CASE("FiscalYearTests.FiscalYearStartDate_OnNepaliDateInstance_ReturnsCorrectDate", "[cs]") {
    check_fy(fy_of(2080, 6, 15), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearTests.FiscalYearEndDate_OnNepaliDateInstance_ReturnsCorrectDate", "[cs]") {
    check_fy(fy_of(2080, 6, 15), std::nullopt, last_of(2081, 3));
}

// C# passes a month; its quarter is quarter_of a date in that month (D-08).
TEST_CASE(
    "FiscalYearTests.GetFiscalYearQuarterStartAndEndDate_ValidYearAndMonth_ReturnsCorrectDates",
    "[cs]") {
    check_quarter(fiscal_year{2080}, nepdate::quarter_of(bs(2080, 5, 1)), bs(2080, 4, 1),
                  last_of(2080, 6));
}

TEST_CASE("FiscalYearTests.GetFiscalYearQuarterStartDate_ValidYearAndMonth_ReturnsCorrectDate",
          "[cs]") {
    check_quarter(fiscal_year{2080}, nepdate::quarter_of(bs(2080, 8, 1)), bs(2080, 7, 1),
                  std::nullopt);
}

TEST_CASE("FiscalYearTests.GetFiscalYearQuarterEndDate_ValidYearAndMonth_ReturnsCorrectDate",
          "[cs]") {
    check_quarter(fiscal_year{2080}, nepdate::quarter_of(bs(2080, 11, 1)), std::nullopt,
                  last_of(2080, 12));
}

// ---- FiscalYearInstanceMethodTests (D-08: yearOffset is fiscal_year arithmetic, and
// FiscalYearQuarters.Current is quarter_of the date) ----

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInShrawan_ReturnsSameyearShrawan1",
          "[cs]") {
    check_fy(fy_of(2080, 4, 15), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInChaitra_ReturnsSameYearShrawan1",
          "[cs]") {
    check_fy(fy_of(2080, 12, 15), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInBasikhakh_"
          "ReturnsPreviousYearShrawan1",
          "[cs]") {
    check_fy(fy_of(2080, 1, 15), bs(2079, 4, 1), std::nullopt);
}

TEST_CASE(
    "FiscalYearInstanceMethodTests.FiscalYearStartDate_DateInAshadh_ReturnsPreviousYearShrawan1",
    "[cs]") {
    check_fy(fy_of(2080, 3, 10), bs(2079, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_WithPositiveYearOffset_ShiftsForward",
          "[cs]") {
    check_fy(fy_of(2080, 4, 1, 1), bs(2081, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_WithNegativeYearOffset_ShiftsBackward",
          "[cs]") {
    check_fy(fy_of(2080, 4, 1, -1), bs(2079, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearEndDate_DateInShrawan_ReturnsNextYearAshad",
          "[cs]") {
    check_fy(fy_of(2080, 4, 15), std::nullopt, last_of(2081, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearEndDate_DateInChaitra_ReturnsNextYearAshadh",
          "[cs]") {
    check_fy(fy_of(2080, 12, 15), std::nullopt, last_of(2081, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearEndDate_DateInBasikhakh_ReturnsSameYearAshadh",
          "[cs]") {
    check_fy(fy_of(2080, 1, 15), std::nullopt, last_of(2080, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearEndDate_WithPositiveYearOffset_ShiftsForward",
          "[cs]") {
    check_fy(fy_of(2080, 4, 1, 1), std::nullopt, last_of(2082, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartAndEndDate_DateInQ2_ReturnsBothBoundaries",
          "[cs]") {
    check_fy(fy_of(2080, 8, 15), bs(2080, 4, 1), last_of(2081, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartAndEndDate_DateInQ4_"
          "ReturnsPreviousFiscalYearBoundaries",
          "[cs]") {
    check_fy(fy_of(2081, 2, 10), bs(2080, 4, 1), last_of(2081, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitFirst_ReturnsShrawan1",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q1, bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitSecond_ReturnsKartik1",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q2, bs(2080, 7, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitThird_ReturnsMagh1",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q3, bs(2080, 10, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitFourth_"
          "ReturnsNextYearBasikhakh1",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q4, bs(2081, 1, 1), std::nullopt);
}

TEST_CASE(
    "FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitFirst_ReturnsLastDayOfAshoj",
    "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q1, std::nullopt, last_of(2080, 6));
}

TEST_CASE(
    "FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitSecond_ReturnsLastDayOfPoush",
    "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q2, std::nullopt, last_of(2080, 9));
}

TEST_CASE(
    "FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitThird_ReturnsLastDayOfChaitra",
    "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q3, std::nullopt, last_of(2080, 12));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_ExplicitFourth_"
          "ReturnsNextYearLastDayOfAshadh",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q4, std::nullopt, last_of(2081, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartAndEndDate_ExplicitSecond_"
          "ReturnsBothBoundaries",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q2, bs(2080, 7, 1), last_of(2080, 9));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartAndEndDate_ExplicitThird_"
          "ReturnsBothBoundaries",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), quarter::q3, bs(2080, 10, 1), last_of(2080, 12));
}

TEST_CASE(
    "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_Current_DateInQ1_ReturnsShrawan1",
    "[cs]") {
    check_quarter(fy_of(2080, 5, 15), nepdate::quarter_of(bs(2080, 5, 15)), bs(2080, 4, 1),
                  std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_Current_DateInQ1_"
          "ReturnsLastDayOfAshoj",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15), nepdate::quarter_of(bs(2080, 5, 15)), std::nullopt,
                  last_of(2080, 6));
}

TEST_CASE(
    "FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_Current_DateInQ2_ReturnsKartik1",
    "[cs]") {
    check_quarter(fy_of(2080, 8, 15), nepdate::quarter_of(bs(2080, 8, 15)), bs(2080, 7, 1),
                  std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_Current_DateInQ3_ReturnsMagh1",
          "[cs]") {
    check_quarter(fy_of(2080, 11, 15), nepdate::quarter_of(bs(2080, 11, 15)), bs(2080, 10, 1),
                  std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_Current_DateInQ3_"
          "ReturnsLastDayOfChaitra",
          "[cs]") {
    check_quarter(fy_of(2080, 11, 15), nepdate::quarter_of(bs(2080, 11, 15)), std::nullopt,
                  last_of(2080, 12));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_ExplicitFirstWithOffset1_"
          "ReturnsNextYearShrawan1",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15, 1), quarter::q1, bs(2081, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_"
          "ExplicitFirstWithNegativeOffset_ReturnsPreviousYearShrawan1",
          "[cs]") {
    check_quarter(fy_of(2080, 5, 15, -1), quarter::q1, bs(2079, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#1",
          "[cs]") {
    check_fy(fy_of(2080, 4, 1), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#2",
          "[cs]") {
    check_fy(fy_of(2080, 6, 1), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#3",
          "[cs]") {
    check_fy(fy_of(2080, 7, 1), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_AllFiscalMonths_Return2080Shrawan1#4",
          "[cs]") {
    check_fy(fy_of(2080, 12, 1), bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_Q4CalendarMonths_"
          "ReturnPreviousYearShrawan1#1",
          "[cs]") {
    check_fy(fy_of(2080, 1, 1), bs(2079, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_Q4CalendarMonths_"
          "ReturnPreviousYearShrawan1#2",
          "[cs]") {
    check_fy(fy_of(2080, 2, 1), bs(2079, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearStartDate_Q4CalendarMonths_"
          "ReturnPreviousYearShrawan1#3",
          "[cs]") {
    check_fy(fy_of(2080, 3, 1), bs(2079, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_ExplicitFirst_"
          "ReturnsFY2080Q1Start",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q1, bs(2080, 4, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_ExplicitSecond_"
          "ReturnsFY2080Q2Start",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q2, bs(2080, 7, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_ExplicitThird_"
          "ReturnsFY2080Q3Start",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q3, bs(2080, 10, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_ExplicitFourth_"
          "ReturnsFY2080Q4Start",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q4, bs(2081, 1, 1), std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterStartDate_FromQ4Date_Current_"
          "ReturnsFY2080Q4Start",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), nepdate::quarter_of(bs(2081, 2, 10)), bs(2081, 1, 1),
                  std::nullopt);
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_ExplicitFirst_"
          "ReturnsFY2080Q1End",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q1, std::nullopt, last_of(2080, 6));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_ExplicitThird_"
          "ReturnsFY2080Q3End",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q3, std::nullopt, last_of(2080, 12));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_ExplicitFourth_"
          "ReturnsFY2080Q4End",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), quarter::q4, std::nullopt, last_of(2081, 3));
}

TEST_CASE("FiscalYearInstanceMethodTests.FiscalYearQuarterEndDate_FromQ4Date_Current_"
          "ReturnsFY2080Q4End",
          "[cs]") {
    check_quarter(fy_of(2081, 2, 10), nepdate::quarter_of(bs(2081, 2, 10)), std::nullopt,
                  last_of(2081, 3));
}

// ---- NepaliDateArithmeticEdgeCaseTests ----

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddDays_Zero_ReturnsSameDate", "[cs]") {
    check_add_days(bs(2080, 5, 15), 0, bs(2080, 5, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddDays_PositiveAcrossMonthBoundary_IsCorrect",
          "[cs]") {
    check_add_days(bs(2080, 5, 31), 1, bs(2080, 6, 1));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddDays_NegativeAcrossMonthBoundary_IsCorrect",
          "[cs]") {
    check_add_days(bs(2080, 6, 1), -1, bs(2080, 5, 31));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddDays_PositiveAcrossYearBoundary_IsCorrect",
          "[cs]") {
    check_add_days(last_of(2080, 12), 1, bs(2081, 1, 1));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddDays_NegativeAcrossYearBoundary_IsCorrect",
          "[cs]") {
    check_add_days(bs(2081, 1, 1), -1, last_of(2080, 12));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddDays_LargePositiveValue_IsCorrect", "[cs]") {
    const date start = bs(2080, 1, 1);
    const auto later = start.add_days(365);
    REQUIRE(later.has_value());
    CHECK((later->year() == 2080 || later->year() == 2081));
    CHECK(start <= *later);
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddMonths_PositiveCrossesYearBoundary_IsCorrect",
          "[cs]") {
    check_add_months(bs(2080, 11, 15), 5, overflow::clamp, bs(2081, 4, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddMonths_NegativeCrossesYearBoundary_IsCorrect",
          "[cs]") {
    check_add_months(bs(2080, 3, 15), -5, overflow::clamp, bs(2079, 10, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddMonths_PositiveMoreThan12_CrossesMultipleYears",
          "[cs]") {
    check_add_months(bs(2080, 5, 15), 14, overflow::clamp, bs(2081, 7, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddMonths_Exactly12_SameMonthNextYear", "[cs]") {
    check_add_months(bs(2080, 6, 15), 12, overflow::clamp, bs(2081, 6, 15));
}

TEST_CASE(
    "NepaliDateArithmeticEdgeCaseTests.AddMonths_DayCappedAtMonthEnd_WhenResultMonthIsShorter",
    "[cs]") {
    check_add_months(bs(2081, 4, 32), 5, overflow::clamp, last_of(2081, 9));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.AddMonths_Negative_SameDayPreservedWhenFits", "[cs]") {
    check_add_months(bs(2080, 8, 10), -1, overflow::clamp, bs(2080, 7, 10));
}

// cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_HalfMonth_AddsApproximately15Days D-02 (add_months takes whole months; use add_days for part of a month)
// cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_NegativeHalfMonth_SubtractsApproximately15Days D-02 (add_months takes whole months; use add_days for part of a month)
// cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_OneAndHalfMonths_AddsApproximately46Days D-02 (add_months takes whole months; use add_days for part of a month)
// cs-deviation: NepaliDateArithmeticEdgeCaseTests.AddMonths_FractionalValue_ReturnsDateFurtherThanZeroMonths D-02 (add_months takes whole months; use add_days for part of a month)

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.Constructor_DateTimeWithMidnightTime_IgnoresTime",
          "[cs]") {
    // pending C4-05: from_sys_time(t, 0min) once it exists.
    CHECK(from_date_time(at(chr::year{2023} / 9 / 1, 0, 0, 0)) == bs(2080, 5, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.Constructor_DateTimeWithNoonTime_IgnoresTime",
          "[cs]") {
    // pending C4-05: from_sys_time(t, 0min) once it exists.
    CHECK(from_date_time(at(chr::year{2023} / 9 / 1, 12, 30, 45)) == bs(2080, 5, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.Constructor_DateTimeWithEndOfDayTime_IgnoresTime",
          "[cs]") {
    // pending C4-05: from_sys_time(t, 0min) once it exists.
    CHECK(from_date_time(at(chr::year{2023} / 9 / 1, 23, 59, 59)) == bs(2080, 5, 15));
}

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.Constructor_"
          "TwoDateTimesOnSameEnglishDateDifferentTimes_ProduceSameNepaliDate",
          "[cs]") {
    // pending C4-05: from_sys_time(t, 0min) once it exists.
    const auto morning = from_date_time(at(chr::year{2023} / 1 / 1, 6, 0, 0));
    const auto evening = from_date_time(at(chr::year{2023} / 1 / 1, 22, 0, 0));
    REQUIRE(morning.has_value());
    CHECK(morning == evening);
}

// cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_DayYearMonthFormat_ReturnsCorrectDate D-06 (autoAdjust guesses the field order)
// cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_MonthDayYearInvertedOrder_ReturnsCorrectDate D-06 (autoAdjust guesses the field order)
// cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_TwoDigitYear_ExpandsToCurrentMillennium D-06 (2-digit years are not expanded)
// cs-deviation: NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_InvalidDateAfterAdjustment_ReturnsFalse D-06 (autoAdjust guesses the field order)

TEST_CASE("NepaliDateArithmeticEdgeCaseTests.TryParse_AutoAdjust_ValidStandardFormat_"
          "ReturnsCorrectDate",
          "[cs]") {
    CHECK(nepdate::parse("2080/05/15") == bs(2080, 5, 15));
}

// ---- NepaliDateManipulationTests ----

TEST_CASE("NepaliDateManipulationTests.MonthEndDate_ReturnsCorrectDate", "[cs]") {
    CHECK(bs(2080, 5, 15).last_of_month() == bs(2080, 5, 31));
}

TEST_CASE("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#1", "[cs]") {
    check_add_months(bs(2080, 5, 15), 1, overflow::clamp, bs(2080, 6, 15));
}

TEST_CASE("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#2", "[cs]") {
    check_add_months(bs(2080, 5, 15), -1, overflow::clamp, bs(2080, 4, 15));
}

TEST_CASE("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#3", "[cs]") {
    check_add_months(bs(2080, 5, 15), 12, overflow::clamp, bs(2081, 5, 15));
}

TEST_CASE("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#4", "[cs]") {
    check_add_months(bs(2081, 4, 32), 5, overflow::clamp, bs(2081, 9, 29));
}

TEST_CASE("NepaliDateManipulationTests.AddMonths_ReturnsCorrectDate#5", "[cs]") {
    // awayFromMonthEnd: true is overflow::spill.
    check_add_months(bs(2081, 4, 32), 5, overflow::spill, bs(2081, 10, 3));
}

TEST_CASE("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#1", "[cs]") {
    check_add_days(bs(2080, 5, 15), 5, bs(2080, 5, 20));
}

TEST_CASE("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#2", "[cs]") {
    check_add_days(bs(2080, 5, 15), -5, bs(2080, 5, 10));
}

TEST_CASE("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#3", "[cs]") {
    check_add_days(bs(2080, 5, 31), 1, bs(2080, 6, 1));
}

TEST_CASE("NepaliDateManipulationTests.AddDays_ReturnsCorrectDate#4", "[cs]") {
    check_add_days(bs(2080, 12, 30), 5, bs(2081, 1, 5));
}

// C#'s TimeSpan subtraction is days_until here (D-07).
TEST_CASE("NepaliDateManipulationTests.Subtract_TwoDates_ReturnsCorrectTimeSpan", "[cs]") {
    CHECK(bs(2080, 5, 1).days_until(bs(2080, 5, 16)) == 15);
    CHECK(bs(2080, 5, 16).days_until(bs(2080, 5, 1)) == -15);
}

TEST_CASE("NepaliDateManipulationTests.Subtract_SameDate_ReturnsZero", "[cs]") {
    CHECK(bs(2080, 5, 15).days_until(bs(2080, 5, 15)) == 0);
}

// ---- OptimizationVerificationTests (D-07: days_until) ----

TEST_CASE("OptimizationVerificationTests.Subtraction_SameDate_ReturnsZeroDays", "[cs]") {
    CHECK(bs(2080, 5, 15).days_until(bs(2080, 5, 15)) == 0);
}

TEST_CASE("OptimizationVerificationTests.Subtraction_AdjacentDays_ReturnsOneDay", "[cs]") {
    CHECK(bs(2080, 5, 15).days_until(bs(2080, 5, 16)) == 1);
    CHECK(bs(2080, 5, 16).days_until(bs(2080, 5, 15)) == -1);
}

TEST_CASE("OptimizationVerificationTests.Subtraction_CrossMonth_CorrectDays", "[cs]") {
    CHECK(bs(2080, 4, 32).days_until(bs(2080, 5, 1)) == 1);
}

TEST_CASE("OptimizationVerificationTests.Subtraction_CrossYear_CorrectDays", "[cs]") {
    CHECK(last_of(2080, 12).days_until(bs(2081, 1, 1)) == 1);
}

// ---- NepaliDateRangeTests ----

TEST_CASE("NepaliDateRangeTests.Constructor_ValidDates_CreatesRange", "[cs]") {
    const auto r = date_range::make(bs(2080, 1, 1), bs(2080, 1, 15));
    REQUIRE(r.has_value());
    CHECK(r->first() == bs(2080, 1, 1));
    CHECK(r->last() == bs(2080, 1, 15));
    CHECK(r->size() == 15);
}

TEST_CASE("NepaliDateRangeTests.Constructor_EndBeforeStart_CreatesEmptyRange", "[cs]") {
    // C# returns an empty range; make returns std::nullopt (D-09).
    CHECK(!date_range::make(bs(2080, 1, 15), bs(2080, 1, 1)).has_value());
}

TEST_CASE("NepaliDateRangeTests.SingleDay_CreatesRangeWithOneDay", "[cs]") {
    const auto r = date_range::make(bs(2080, 1, 1), bs(2080, 1, 1));
    REQUIRE(r.has_value());
    CHECK(r->first() == r->last());
    CHECK(r->size() == 1);
}

// cs-deviation: NepaliDateRangeTests.FromDayCount_CreatesCorrectRange D-09 (no FromDayCount; make(d, *d.add_days(n - 1)) instead)

TEST_CASE("NepaliDateRangeTests.ForMonth_CreatesCorrectRange", "[cs]") {
    const auto r = date_range::of_month(2080, month::baishakh);
    REQUIRE(r.has_value());
    CHECK(r->first() == bs(2080, 1, 1));
    CHECK(r->last() == last_of(2080, 1));
}

TEST_CASE("NepaliDateRangeTests.ForFiscalYear_CreatesCorrectRange", "[cs]") {
    // ForFiscalYear is fiscal_year::range (D-08).
    const auto r = fiscal_year{2080}.range();
    REQUIRE(r.has_value());
    CHECK(r->first() == bs(2080, 4, 1));
    CHECK(r->last() == last_of(2081, 3));
}

TEST_CASE("NepaliDateRangeTests.Contains_DateInRange_ReturnsTrue", "[cs]") {
    check_contains(bs(2080, 1, 1), bs(2080, 1, 15), bs(2080, 1, 10), true);
}

TEST_CASE("NepaliDateRangeTests.Contains_DateOutsideRange_ReturnsFalse", "[cs]") {
    check_contains(bs(2080, 1, 1), bs(2080, 1, 15), bs(2080, 1, 20), false);
}

// cs-deviation: NepaliDateRangeTests.Contains_RangeFullyContained_ReturnsTrue D-09 (contains takes a date only; check both ends of the inner range)
// cs-deviation: NepaliDateRangeTests.Overlaps_RangesOverlap_ReturnsTrue D-09 (no set operations; compare first() and last())
// cs-deviation: NepaliDateRangeTests.Overlaps_RangesDontOverlap_ReturnsFalse D-09 (no set operations; compare first() and last())
// cs-deviation: NepaliDateRangeTests.IsAdjacentTo_AdjacentRanges_ReturnsTrue D-09 (no set operations; compare first() and last())
// cs-deviation: NepaliDateRangeTests.Intersect_OverlappingRanges_ReturnsIntersection D-09 (no set operations; make(max of firsts, min of lasts))
// cs-deviation: NepaliDateRangeTests.Union_OverlappingRanges_ReturnsUnion D-09 (no set operations; make(min of firsts, max of lasts))
// cs-deviation: NepaliDateRangeTests.Except_RemovingRangeFromMiddle_ReturnsTwoRanges D-09 (no set operations)
// cs-deviation: NepaliDateRangeTests.SplitByMonth_RangeAcrossMonths_SplitsCorrectly D-09 (no splitting; of_month for each month)
// cs-deviation: NepaliDateRangeTests.WorkingDays_ReturnsCorrectDays D-09 (no working days; iterate and filter on weekday())

TEST_CASE("NepaliDateRangeTests.Enumeration_EnumeratesAllDaysInRange", "[cs]") {
    const auto r = date_range::make(bs(2080, 1, 1), bs(2080, 1, 10));
    REQUIRE(r.has_value());
    std::vector<date> want;
    for (int d = 1; d <= 10; ++d) {
        want.push_back(bs(2080, 1, d));
    }
    CHECK(dates_of(*r) == want);
}

// cs-deviation: NepaliDateRangeTests.Demo_RangeOperations D-09 (exercises set operations, splitting and working days)

// ---- NepaliDateRangeExtendedTests ----

TEST_CASE("NepaliDateRangeExtendedTests.ForCalendarYear_StartsAtBaishakhOne_EndsAtChaitraEnd",
          "[cs]") {
    const auto r = date_range::of_year(2080);
    REQUIRE(r.has_value());
    CHECK(r->first() == bs(2080, 1, 1));
    CHECK(r->last() == last_of(2080, 12));
}

TEST_CASE("NepaliDateRangeExtendedTests.ForCalendarYear_LengthIsReasonable", "[cs]") {
    const auto r = date_range::of_year(2080);
    REQUIRE(r.has_value());
    CHECK(r->size() >= 365);
    CHECK(r->size() <= 366);
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentMonth_StartIsFirstDayOfCurrentMonth", "[cs]") {
    const auto r = date_range::of_month(fixed_today().year(), fixed_today().month());
    REQUIRE(r.has_value());
    CHECK(r->first() == fixed_today().first_of_month());
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentMonth_EndIsLastDayOfCurrentMonth", "[cs]") {
    const auto r = date_range::of_month(fixed_today().year(), fixed_today().month());
    REQUIRE(r.has_value());
    CHECK(r->last() == fixed_today().last_of_month());
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentMonth_ContainsToday", "[cs]") {
    const auto r = date_range::of_month(fixed_today().year(), fixed_today().month());
    REQUIRE(r.has_value());
    CHECK(r->contains(fixed_today()));
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentFiscalYear_ContainsToday", "[cs]") {
    const auto r = fiscal_year::of(fixed_today()).range();
    REQUIRE(r.has_value());
    CHECK(r->contains(fixed_today()));
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentFiscalYear_StartIsShrawan", "[cs]") {
    const auto r = fiscal_year::of(fixed_today()).range();
    REQUIRE(r.has_value());
    CHECK(r->first().month() == month::shrawan);
    CHECK(r->first().day() == 1);
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentFiscalYear_EndIsAshadh", "[cs]") {
    const auto r = fiscal_year::of(fixed_today()).range();
    REQUIRE(r.has_value());
    CHECK(r->last().month() == month::ashad);
    CHECK(r->last() == r->last().last_of_month());
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentCalendarYear_ContainsToday", "[cs]") {
    const auto r = date_range::of_year(fixed_today().year());
    REQUIRE(r.has_value());
    CHECK(r->contains(fixed_today()));
}

TEST_CASE("NepaliDateRangeExtendedTests.CurrentCalendarYear_StartIsBaishakh1", "[cs]") {
    const auto r = date_range::of_year(fixed_today().year());
    REQUIRE(r.has_value());
    CHECK(r->first().month() == month::baishakh);
    CHECK(r->first().day() == 1);
}

// cs-deviation: NepaliDateRangeExtendedTests.SplitByFiscalQuarter_EmptyRange_ReturnsEmptyArray D-09 (no splitting; fiscal_year::quarter_range instead)
// cs-deviation: NepaliDateRangeExtendedTests.SplitByFiscalQuarter_RangeWithinOneQuarter_ReturnsSingleRange D-09 (no splitting; fiscal_year::quarter_range instead)
// cs-deviation: NepaliDateRangeExtendedTests.SplitByFiscalQuarter_RangeSpanningTwoQuarters_ReturnsTwoRanges D-09 (no splitting; fiscal_year::quarter_range instead)
// cs-deviation: NepaliDateRangeExtendedTests.SplitByFiscalQuarter_FullFiscalYear_ReturnsFourRanges D-09 (no splitting; fiscal_year::quarter_range instead)
// cs-deviation: NepaliDateRangeExtendedTests.SplitByFiscalQuarter_RangeInQ4Months_SingleRange D-09 (no splitting; fiscal_year::quarter_range instead)
// cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_Zero_ThrowsArgumentOutOfRangeException D-09 (no DatesWithInterval; step the iterator by n instead)
// cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_NegativeInterval_ThrowsArgumentOutOfRangeException D-09 (no DatesWithInterval; step the iterator by n instead)
// cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_IntervalOfOne_YieldsEveryDay D-09 (no DatesWithInterval; step the iterator by n instead)
// cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_IntervalOfTwo_YieldsOddDays D-09 (no DatesWithInterval; step the iterator by n instead)
// cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_IntervalLargerThanRange_YieldsOnlyStartDate D-09 (no DatesWithInterval; step the iterator by n instead)
// cs-deviation: NepaliDateRangeExtendedTests.DatesWithInterval_EmptyRange_YieldsNothing D-09 (no DatesWithInterval; step the iterator by n instead)
// cs-deviation: NepaliDateRangeExtendedTests.WeekendDays_IncludeSundayTrue_ReturnsBothSaturdayAndSunday D-09 (no weekend days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.WeekendDays_IncludeSundayFalse_ReturnsOnlySaturday D-09 (no weekend days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.WeekendDays_RangeWithNoWeekend_ReturnsEmpty D-09 (no weekend days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.WeekendDays_EmptyRange_ReturnsEmpty D-09 (no weekend days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.WorkingDays_ExcludeSundayFalse_ExcludesOnlySaturday D-09 (no working days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.WorkingDays_ExcludeSundayTrue_ExcludesBothSatAndSun D-09 (no working days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.WorkingDays_EmptyRange_ReturnsEmpty D-09 (no working days; iterate and filter on weekday())
// cs-deviation: NepaliDateRangeExtendedTests.FromDayCount_ZeroDays_ThrowsArgumentOutOfRangeException D-09 (no FromDayCount; make(d, *d.add_days(n - 1)) instead)
// cs-deviation: NepaliDateRangeExtendedTests.FromDayCount_NegativeDays_ThrowsArgumentOutOfRangeException D-09 (no FromDayCount; make(d, *d.add_days(n - 1)) instead)
// cs-deviation: NepaliDateRangeExtendedTests.Intersect_NonOverlappingRanges_ReturnsEmptyRange D-09 (no set operations; make(max of firsts, min of lasts))
// cs-deviation: NepaliDateRangeExtendedTests.Intersect_AdjacentRanges_ReturnsEmptyRange D-09 (no set operations; make(max of firsts, min of lasts))
// cs-deviation: NepaliDateRangeExtendedTests.Intersect_OneRangeFullyInsideOther_ReturnsInnerRange D-09 (no set operations; make(max of firsts, min of lasts))
// cs-deviation: NepaliDateRangeExtendedTests.Union_LeftRangeIsEmpty_ReturnsRightRange D-09 (no set operations; make(min of firsts, max of lasts))
// cs-deviation: NepaliDateRangeExtendedTests.Union_RightRangeIsEmpty_ReturnsLeftRange D-09 (no set operations; make(min of firsts, max of lasts))
// cs-deviation: NepaliDateRangeExtendedTests.Union_NonOverlappingRanges_SpansEntireGap D-09 (no set operations; make(min of firsts, max of lasts))

TEST_CASE("NepaliDateRangeExtendedTests.Contains_ExactStartDate_ReturnsTrue", "[cs]") {
    check_contains(bs(2080, 1, 5), bs(2080, 1, 20), bs(2080, 1, 5), true);
}

TEST_CASE("NepaliDateRangeExtendedTests.Contains_ExactEndDate_ReturnsTrue", "[cs]") {
    check_contains(bs(2080, 1, 5), bs(2080, 1, 20), bs(2080, 1, 20), true);
}

TEST_CASE("NepaliDateRangeExtendedTests.Contains_OneDayBeforeStart_ReturnsFalse", "[cs]") {
    check_contains(bs(2080, 1, 5), bs(2080, 1, 20), bs(2080, 1, 4), false);
}

TEST_CASE("NepaliDateRangeExtendedTests.Contains_OneDayAfterEnd_ReturnsFalse", "[cs]") {
    check_contains(bs(2080, 1, 5), bs(2080, 1, 20), bs(2080, 1, 21), false);
}

TEST_CASE("NepaliDateRangeExtendedTests.Contains_EmptyRange_ContainsNothing", "[cs]") {
    // C#'s empty range is std::nullopt, so there is no range to contain anything (D-09).
    const auto r = date_range::make(bs(2080, 5, 10), bs(2080, 5, 1));
    CHECK(!(r.has_value() && r->contains(bs(2080, 5, 5))));
}

// cs-deviation: NepaliDateRangeExtendedTests.IsAdjacentTo_GapBetweenRanges_ReturnsFalse D-09 (no set operations; compare first() and last())
// cs-deviation: NepaliDateRangeExtendedTests.IsAdjacentTo_OverlappingRanges_ReturnsFalse D-09 (no set operations; compare first() and last())
// cs-deviation: NepaliDateRangeExtendedTests.IsAdjacentTo_EmptyRange_ReturnsFalse D-09 (no set operations; compare first() and last())
// cs-deviation: NepaliDateRangeExtendedTests.Except_NonOverlappingExcludeRange_ReturnsOriginalRange D-09 (no set operations)
// cs-deviation: NepaliDateRangeExtendedTests.Except_ExcludeRangeFullyContainsThis_ReturnsEmptyArray D-09 (no set operations)
// cs-deviation: NepaliDateRangeExtendedTests.Except_ExcludeOverlapsStart_ReturnsRightPart D-09 (no set operations)
// cs-deviation: NepaliDateRangeExtendedTests.Except_ExcludeOverlapsEnd_ReturnsLeftPart D-09 (no set operations)

TEST_CASE("NepaliDateRangeExtendedTests.GetEnumerator_ThreeDayRange_YieldsExactlyThreeDates",
          "[cs]") {
    const auto r = date_range::make(bs(2080, 5, 13), bs(2080, 5, 15));
    REQUIRE(r.has_value());
    CHECK(dates_of(*r) == std::vector<date>{bs(2080, 5, 13), bs(2080, 5, 14), bs(2080, 5, 15)});
}

TEST_CASE("NepaliDateRangeExtendedTests.GetEnumerator_EmptyRange_YieldsNoDates", "[cs]") {
    // C#'s empty range is std::nullopt: nothing to iterate (D-09).
    const auto r = date_range::make(bs(2080, 5, 10), bs(2080, 5, 1));
    CHECK((r ? dates_of(*r) : std::vector<date>{}).empty());
}

TEST_CASE("NepaliDateRangeExtendedTests.GetEnumerator_SingleDayRange_YieldsOneDate", "[cs]") {
    const auto r = date_range::make(bs(2080, 5, 15), bs(2080, 5, 15));
    REQUIRE(r.has_value());
    CHECK(dates_of(*r) == std::vector<date>{bs(2080, 5, 15)});
}

// cs-deviation: NepaliDateRangeExtendedTests.ToString_DefaultFormat_IsStartDashEnd D-09 (date_range has no string form; format first() and last())
// cs-deviation: NepaliDateRangeExtendedTests.ToString_EmptyRange_ReturnsEmptyRangeString D-09 (date_range has no string form; format first() and last())
// cs-deviation: NepaliDateRangeExtendedTests.ToString_WithDateFormat_UsesSpecifiedFormat D-09 (date_range has no string form; format first() and last())
// cs-deviation: NepaliDateRangeExtendedTests.ToString_WithDateFormat_EmptyRange_ReturnsEmptyRangeString D-09 (date_range has no string form; format first() and last())

TEST_CASE("NepaliDateRangeExtendedTests.Equals_IdenticalRanges_ReturnsTrue", "[cs]") {
    check_ranges_equal(bs(2080, 1, 1), bs(2080, 1, 15), bs(2080, 1, 1), bs(2080, 1, 15), true);
}

TEST_CASE("NepaliDateRangeExtendedTests.Equals_DifferentStart_ReturnsFalse", "[cs]") {
    check_ranges_equal(bs(2080, 1, 1), bs(2080, 1, 15), bs(2080, 1, 2), bs(2080, 1, 15), false);
}

TEST_CASE("NepaliDateRangeExtendedTests.Equals_DifferentEnd_ReturnsFalse", "[cs]") {
    check_ranges_equal(bs(2080, 1, 1), bs(2080, 1, 15), bs(2080, 1, 1), bs(2080, 1, 16), false);
}

TEST_CASE("NepaliDateRangeExtendedTests.Equals_BothEmpty_ReturnsTrue", "[cs]") {
    // Two empty std::optional<date_range> compare equal.
    check_ranges_equal(bs(2080, 5, 10), bs(2080, 5, 1), bs(2079, 3, 10), bs(2079, 3, 1), true);
}

TEST_CASE("NepaliDateRangeExtendedTests.Equals_ObjectOverload_SameRange_ReturnsTrue", "[cs]") {
    check_ranges_equal(bs(2080, 1, 1), bs(2080, 1, 15), bs(2080, 1, 1), bs(2080, 1, 15), true);
}

// cs-n/a: NepaliDateRangeExtendedTests.Equals_ObjectOverload_WrongType_ReturnsFalse == takes a date_range, so comparing with a string doesn't compile
// cs-deviation: NepaliDateRangeExtendedTests.GetHashCode_EqualRanges_SameHash D-09 (date_range has no std::hash; hash first() and last() with std::hash<date>)

TEST_CASE("NepaliDateRangeExtendedTests.OperatorEquals_IdenticalRanges_ReturnsTrue", "[cs]") {
    const auto a = date_range::make(bs(2080, 1, 1), bs(2080, 1, 15)).value();
    const auto b = date_range::make(bs(2080, 1, 1), bs(2080, 1, 15)).value();
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("NepaliDateRangeExtendedTests.OperatorNotEquals_DifferentRanges_ReturnsTrue", "[cs]") {
    const auto a = date_range::make(bs(2080, 1, 1), bs(2080, 1, 15)).value();
    const auto b = date_range::make(bs(2080, 1, 1), bs(2080, 1, 16)).value();
    CHECK(a != b);
    CHECK_FALSE(a == b);
}
