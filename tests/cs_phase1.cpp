// The phase-1 rows of C# NepDate's own tests (shared/spec/vectors/csharp-tests/catalog.tsv).
// Each row is a TEST_CASE named after its id, or a cs-deviation / cs-n/a line. The CTest test
// catalog_coverage (tools/check_catalog.cmake) checks that every phase-1 row is here.
//
// Rows that build a date from a string use parse. The row that reads the clock uses
// today().

#include <nepdate/date.hpp>
#include <nepdate/parse.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <compare>
#include <functional>
#include <optional>

using nepdate::date;
using nepdate::errc;
using nepdate::from_gregorian;
using nepdate::make_date;
using nepdate::month;
using nepdate::parse;
using nepdate::result;
namespace chr = std::chrono;

namespace {

constexpr chr::year_month_day ymd(int y, unsigned m, unsigned d) {
    return chr::year_month_day{chr::year{y}, chr::month{m}, chr::day{d}};
}

// NepaliDateComparableTests: _d1 = 2081-01-01, _d2 = 2081-06-15, _d3 = 2082-01-01.
date d1() {
    return make_date(2081, 1, 1).value();
}
date d2() {
    return make_date(2081, 6, 15).value();
}
date d3() {
    return make_date(2082, 1, 1).value();
}

// C#'s SortedSet, Array.Sort and List.Sort rows all become one std::sort.
void check_sorts() {
    std::array<date, 3> dates = {d3(), d1(), d2()};
    std::sort(dates.begin(), dates.end());
    CHECK(dates[0] == d1());
    CHECK(dates[1] == d2());
    CHECK(dates[2] == d3());
}

} // namespace

TEST_CASE("NepaliDateComparableTests.CompareTo_Object_Earlier_ReturnsNegative", "[cs]") {
    CHECK(std::is_lt(d1() <=> d2()));
}

TEST_CASE("NepaliDateComparableTests.CompareTo_Object_Same_ReturnsZero", "[cs]") {
    CHECK(std::is_eq(d1() <=> make_date(2081, 1, 1).value()));
}

TEST_CASE("NepaliDateComparableTests.CompareTo_Object_Later_ReturnsPositive", "[cs]") {
    CHECK(std::is_gt(d3() <=> d1()));
}

TEST_CASE("NepaliDateComparableTests.CompareTo_Object_Null_ReturnsPositive", "[cs]") {
    // C# null is an empty std::optional<date> here (D-01); it orders before every date.
    CHECK(std::optional<date>{d1()} > std::optional<date>{});
}

// cs-n/a: NepaliDateComparableTests.CompareTo_WrongType_ThrowsArgumentException <=> takes a date, so comparing with a string doesn't compile

TEST_CASE("NepaliDateComparableTests.CompareTo_Generic_Earlier_ReturnsNegative", "[cs]") {
    CHECK(std::is_lt(d1() <=> d2()));
}

TEST_CASE("NepaliDateComparableTests.CompareTo_Generic_Same_ReturnsZero", "[cs]") {
    CHECK(std::is_eq(d1() <=> make_date(2081, 1, 1).value()));
}

TEST_CASE("NepaliDateComparableTests.CompareTo_Generic_Later_ReturnsPositive", "[cs]") {
    CHECK(std::is_gt(d3() <=> d1()));
}

TEST_CASE("NepaliDateComparableTests.SortedSet_OrdersCorrectly", "[cs]") {
    check_sorts();
}

TEST_CASE("NepaliDateComparableTests.ArraySort_ViaIComparable_OrdersCorrectly", "[cs]") {
    check_sorts();
}

TEST_CASE("NepaliDateComparableTests.List_Sort_OrdersCorrectly", "[cs]") {
    check_sorts();
}

TEST_CASE("DictionaryIntegrityTests.AllNepaliMonths_RoundtripToEnglishAndBack_ProducesOriginalDate",
          "[cs]") {
    int failures = 0;
    for (int y = 1901; y <= 2199; ++y) {
        for (int m = 1; m <= 12; ++m) {
            const date d = make_date(y, m, 1).value();
            failures += from_gregorian(d.to_year_month_day()) == d ? 0 : 1;
        }
    }
    CHECK(failures == 0);
}

TEST_CASE("DictionaryIntegrityTests.AllNepaliDays_RoundtripToEnglishAndBack_ProducesOriginalDate",
          "[cs]") {
    int failures = 0;
    int total = 0;
    for (int y = 1901; y <= 2199; ++y) {
        for (int m = 1; m <= 12; ++m) {
            const int length = make_date(y, m, 1)->month_length();
            for (int day = 1; day <= length; ++day) {
                const date d = make_date(y, m, day).value();
                failures += from_gregorian(d.to_year_month_day()) == d ? 0 : 1;
                ++total;
            }
        }
    }
    CHECK(failures == 0);
    CHECK(total == 109212);
}

TEST_CASE("DictionaryIntegrityTests.AllEnglishDays_RoundtripToNepaliAndBack_ProducesOriginalDate",
          "[cs]") {
    int failures = 0;
    const chr::sys_days first = date::min().to_sys_days();
    const chr::sys_days last = date::max().to_sys_days();
    for (chr::sys_days ad = first; ad <= last; ad += chr::days{1}) {
        const auto bs = from_gregorian(chr::year_month_day{ad});
        failures += bs.has_value() && bs->to_sys_days() == ad ? 0 : 1;
    }
    CHECK(failures == 0);
}

TEST_CASE("DictionaryIntegrityTests.AllMonths_MonthEndDay_IsWithinValidRange", "[cs]") {
    for (int y = 1901; y <= 2199; ++y) {
        for (int m = 1; m <= 12; ++m) {
            const int length = make_date(y, m, 1)->month_length();
            REQUIRE(length >= 29);
            REQUIRE(length <= 32);
        }
    }
}

TEST_CASE("DictionaryIntegrityTests.ConsecutiveNepaliDays_ProduceConsecutiveEnglishDays", "[cs]") {
    int violations = 0;
    chr::sys_days previous = date::min().to_sys_days() - chr::days{1};
    for (int y = 1901; y <= 2199; ++y) {
        for (int m = 1; m <= 12; ++m) {
            const int length = make_date(y, m, 1)->month_length();
            for (int day = 1; day <= length; ++day) {
                const chr::sys_days ad = make_date(y, m, day)->to_sys_days();
                violations += ad == previous + chr::days{1} ? 0 : 1;
                previous = ad;
            }
        }
    }
    CHECK(violations == 0);
}

TEST_CASE("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#1", "[cs]") {
    CHECK(make_date(2080, 1, 1)->to_year_month_day() == ymd(2023, 4, 14));
}

TEST_CASE("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#2", "[cs]") {
    CHECK(make_date(2000, 1, 1)->to_year_month_day() == ymd(1943, 4, 14));
}

TEST_CASE("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#3", "[cs]") {
    CHECK(make_date(2081, 1, 1)->to_year_month_day() == ymd(2024, 4, 13));
}

TEST_CASE("DictionaryIntegrityTests.KnownReferenceDates_ConvertCorrectly#4", "[cs]") {
    CHECK(make_date(1901, 1, 1)->to_year_month_day() == ymd(1844, 4, 11));
}

TEST_CASE("DictionaryIntegrityTests.BoundaryDates_ConvertCorrectly", "[cs]") {
    CHECK(date::min().year() == 1901);
    CHECK(date::min().to_year_month_day() == ymd(1844, 4, 11));
    CHECK(date::max().year() == 2199);
    CHECK(date::max().to_year_month_day() == ymd(2143, 4, 15));
}

TEST_CASE("DictionaryIntegrityTests.TotalDaysInRange_MatchesEnglishDateSpan", "[cs]") {
    int total = 0;
    for (int y = 1901; y <= 2199; ++y) {
        total += make_date(y, 1, 1)->year_length();
    }
    const auto span = (date::max().to_sys_days() - date::min().to_sys_days()).count() + 1;
    CHECK(total == span);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidNepaliDate_CreatesInstance", "[cs]") {
    const date d = make_date(2080, 5, 15).value();
    CHECK(d.year() == 2080);
    CHECK(d.month() == month::bhadra);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_MinValue_CreatesInstance", "[cs]") {
    CHECK(date::min() == make_date(1901, 1, 1).value());
}

TEST_CASE("NepaliDateConstructionTests.Constructor_MaxValue_CreatesInstance", "[cs]") {
    CHECK(date::max().year() == 2199);
    CHECK(date::max().month() == month::chaitra);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#1", "[cs]") {
    CHECK(make_date(2080, 0, 1).error() == errc::invalid_month);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#2", "[cs]") {
    CHECK(make_date(2080, 13, 1).error() == errc::invalid_month);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#3", "[cs]") {
    CHECK(make_date(2080, 1, 0).error() == errc::invalid_day);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidNepaliDate_ThrowsException#4", "[cs]") {
    CHECK(make_date(2080, 1, 33).error() == errc::invalid_day);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#1", "[cs]") {
    const date d = parse("2080/05/15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#2", "[cs]") {
    const date d = parse("2080-05-15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#3", "[cs]") {
    const date d = parse("2080.05.15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#4", "[cs]") {
    const date d = parse("2080_05_15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#5", "[cs]") {
    const date d = parse(R"(2080\05\15)").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidStringFormats_CreatesInstance#6", "[cs]") {
    const date d = parse("2080 05 15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

// cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#1 D-06 (autoAdjust guesses the field order)

// cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#2 D-06 (autoAdjust guesses the field order)

TEST_CASE("NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#3", "[cs]") {
    const date d = parse("2080/05/15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

// cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjustedFormats_CreatesInstance#4 D-06 (2-digit years are not expanded)

// cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjust_MonthOverflowWithBoundaryDay_SwapsAsDocumented#1 D-06 (autoAdjust swaps month and day)

// cs-deviation: NepaliDateConstructionTests.Constructor_AutoAdjust_MonthOverflowWithBoundaryDay_SwapsAsDocumented#2 D-06 (autoAdjust swaps month and day)

// cs-deviation: NepaliDateConstructionTests.TryParse_AutoAdjust_MonthOverflowWithBoundaryDay_SwapsAsDocumented D-06 (autoAdjust swaps month and day)

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#1",
          "[cs]") {
    const result<date> r = parse("");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#2",
          "[cs]") {
    const result<date> r = parse("invalid");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("NepaliDateConstructionTests.Constructor_InvalidStringFormats_ThrowsException#3",
          "[cs]") {
    const result<date> r = parse("2080/5");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("NepaliDateConstructionTests.Constructor_ValidFormatInvalidDate_ThrowsException#1",
          "[cs]") {
    CHECK(parse("2080/5/40").error() == errc::invalid_day);
}

TEST_CASE("NepaliDateConstructionTests.Constructor_EnglishDate_ConvertsCorrectly", "[cs]") {
    CHECK(from_gregorian(2023, 8, 30).value() == make_date(2080, 5, 13).value());
}

TEST_CASE("NepaliDateConstructionTests.TryParse_ValidString_ReturnsTrueWithCorrectDate", "[cs]") {
    const date d = parse("2080/05/15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#1", "[cs]") {
    const result<date> r = parse("");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#2", "[cs]") {
    const result<date> r = parse("invalid");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("NepaliDateConstructionTests.TryParse_InvalidString_ReturnsFalseWithDefault#3", "[cs]") {
    const result<date> r = parse("2080/5");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("NepaliDatePropertiesTests.EnglishDate_Conversion_ReturnsCorrectDate", "[cs]") {
    CHECK(make_date(2080, 5, 15)->to_year_month_day() == ymd(2023, 9, 1));
}

TEST_CASE("NepaliDatePropertiesTests.DayOfWeek_ReturnsCorrectValue", "[cs]") {
    CHECK(make_date(2080, 5, 15)->weekday() == chr::Friday);
}

TEST_CASE("NepaliDatePropertiesTests.MonthEndDay_ReturnsCorrectValue", "[cs]") {
    CHECK(make_date(2080, 5, 15)->month_length() == 31);
}

TEST_CASE("NepaliDatePropertiesTests.MonthName_ReturnsCorrectEnum", "[cs]") {
    CHECK(make_date(2080, 5, 15)->month() == month::bhadra);
}

TEST_CASE("NepaliDatePropertiesTests.Today_ReturnsCurrentNepaliDate", "[cs]") {
    // C# compares Today with new NepaliDate(DateTime.Today); the port compares today() with the
    // current instant shifted by Nepal's offset.
    const nepdate::date t = nepdate::today().value();
    // NOLINTBEGIN(misc-include-cleaner): floor and days live in an internal libstdc++ header; <chrono> is the correct include
    const std::chrono::sys_seconds now = chr::floor<chr::seconds>(chr::system_clock::now());
    CHECK(t == nepdate::from_sys_days(chr::floor<chr::days>(now + nepdate::nepal_utc_offset)));
    // NOLINTEND(misc-include-cleaner)
}

TEST_CASE("NepaliDatePropertiesTests.DayOfYear_FirstDayOfYear_Returns1", "[cs]") {
    CHECK(make_date(2080, 1, 1)->day_of_year() == 1);
}

TEST_CASE("NepaliDatePropertiesTests.DayOfYear_FirstDayOfMonth4_EqualsSum_Of_Months1To3_Plus1",
          "[cs]") {
    const int expected = make_date(2080, 1, 1)->month_length() +
                         make_date(2080, 2, 1)->month_length() +
                         make_date(2080, 3, 1)->month_length() + 1;
    CHECK(make_date(2080, 4, 1)->day_of_year() == expected);
}

TEST_CASE("NepaliDatePropertiesTests.Equals_NullObject_ReturnsFalse", "[cs]") {
    // C# null is an empty std::optional<date> here (D-01).
    CHECK(std::optional<date>{make_date(2080, 5, 15).value()} != std::optional<date>{});
}

// cs-n/a: NepaliDatePropertiesTests.Equals_DifferentType_ReturnsFalse == takes a date, so comparing with a string doesn't compile

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#1", "[cs]") {
    const date d = parse("2080/05/15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#2", "[cs]") {
    const date d = parse("2080-05-15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#3", "[cs]") {
    const date d = parse("2080.05.15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#4", "[cs]") {
    const date d = parse("2080_05_15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#5", "[cs]") {
    const date d = parse(R"(2080\05\15)").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#6", "[cs]") {
    const date d = parse("2080 05 15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#7", "[cs]") {
    const date d = parse("2080।05।15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_AllSeparators_ParseCorrectly#8", "[cs]") {
    const date d = parse("2080|05|15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#1", "[cs]") {
    const result<date> r = parse("");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#2", "[cs]") {
    const result<date> r = parse("invalid");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#3", "[cs]") {
    const result<date> r = parse("2080/05");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#4", "[cs]") {
    const result<date> r = parse("2080");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#5", "[cs]") {
    const result<date> r = parse("2080/05/15/99");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidStrings_ThrowsException#6", "[cs]") {
    const result<date> r = parse("abc/def/ghi");
    REQUIRE_FALSE(r.has_value());
    CHECK(nepdate::is_parse_error(r.error()));
}

// cs-n/a: OptimizationVerificationTests.Constructor_NullString_ThrowsException std::string_view has no null state to pass

TEST_CASE("OptimizationVerificationTests.Constructor_WithoutLeadingZeros_ParsesCorrectly#1",
          "[cs]") {
    const date d = parse("2080/5/15").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 15);
}

TEST_CASE("OptimizationVerificationTests.Constructor_WithoutLeadingZeros_ParsesCorrectly#2",
          "[cs]") {
    const date d = parse("2080/05/5").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 5);
    CHECK(d.day() == 5);
}

TEST_CASE("OptimizationVerificationTests.Constructor_WithoutLeadingZeros_ParsesCorrectly#3",
          "[cs]") {
    const date d = parse("2080/1/1").value();
    CHECK(d.year() == 2080);
    CHECK(static_cast<int>(d.month()) == 1);
    CHECK(d.day() == 1);
}

TEST_CASE("OptimizationVerificationTests.Operator_Equality_SameDates_ReturnsTrue", "[cs]") {
    const date a = make_date(2080, 5, 15).value();
    const date b = make_date(2080, 5, 15).value();
    CHECK(a == b);
}

TEST_CASE("OptimizationVerificationTests.Operator_Inequality_DifferentDates_ReturnsTrue", "[cs]") {
    const date a = make_date(2080, 5, 15).value();
    const date b = make_date(2080, 5, 16).value();
    CHECK(a != b);
}

TEST_CASE("OptimizationVerificationTests.Operator_LessThan_EarlierDate_ReturnsTrue", "[cs]") {
    const date earlier = make_date(2080, 5, 15).value();
    const date later = make_date(2080, 5, 16).value();
    CHECK(earlier < later);
    CHECK(!(later < earlier));
    CHECK(!(earlier < earlier));
}

TEST_CASE("OptimizationVerificationTests.Operator_LessThanOrEqual_SameAndEarlier_ReturnsTrue",
          "[cs]") {
    const date earlier = make_date(2080, 5, 15).value();
    const date later = make_date(2080, 5, 16).value();
    CHECK(earlier <= later);
    CHECK(earlier <= earlier);
    CHECK(!(later <= earlier));
}

TEST_CASE("OptimizationVerificationTests.Operator_GreaterThan_LaterDate_ReturnsTrue", "[cs]") {
    const date earlier = make_date(2080, 5, 15).value();
    const date later = make_date(2080, 5, 16).value();
    CHECK(later > earlier);
    CHECK(!(earlier > later));
    CHECK(!(later > later));
}

TEST_CASE("OptimizationVerificationTests.Operator_GreaterThanOrEqual_SameAndLater_ReturnsTrue",
          "[cs]") {
    const date earlier = make_date(2080, 5, 15).value();
    const date later = make_date(2080, 5, 16).value();
    CHECK(later >= earlier);
    CHECK(later >= later);
    CHECK(!(earlier >= later));
}

TEST_CASE("OptimizationVerificationTests.Operator_MinMaxBoundary_CorrectOrdering", "[cs]") {
    CHECK(date::min() < date::max());
    CHECK(date::max() > date::min());
}

TEST_CASE("OptimizationVerificationTests.CompareTo_EarlierDate_ReturnsNegative", "[cs]") {
    const date earlier = make_date(2080, 5, 15).value();
    const date later = make_date(2080, 5, 16).value();
    CHECK(std::is_lt(earlier <=> later));
}

TEST_CASE("OptimizationVerificationTests.CompareTo_SameDate_ReturnsZero", "[cs]") {
    const date a = make_date(2080, 5, 15).value();
    CHECK(std::is_eq(a <=> a));
}

TEST_CASE("OptimizationVerificationTests.CompareTo_LaterDate_ReturnsPositive", "[cs]") {
    const date earlier = make_date(2080, 5, 15).value();
    const date later = make_date(2080, 5, 16).value();
    CHECK(std::is_gt(later <=> earlier));
}

TEST_CASE("OptimizationVerificationTests.CompareTo_DifferentYears_CorrectOrdering", "[cs]") {
    CHECK(std::is_lt(make_date(2080, 1, 1).value() <=> make_date(2081, 1, 1).value()));
}

TEST_CASE("OptimizationVerificationTests.CompareTo_DifferentMonths_CorrectOrdering", "[cs]") {
    CHECK(std::is_lt(make_date(2080, 1, 1).value() <=> make_date(2080, 2, 1).value()));
}

TEST_CASE("OptimizationVerificationTests.Equals_SameDates_ReturnsTrue", "[cs]") {
    const date a = make_date(2080, 5, 15).value();
    CHECK(a == make_date(2080, 5, 15).value());
}

TEST_CASE("OptimizationVerificationTests.Equals_DifferentDates_ReturnsFalse", "[cs]") {
    const date a = make_date(2080, 5, 15).value();
    CHECK(!(a == make_date(2080, 5, 16).value()));
}

TEST_CASE("OptimizationVerificationTests.GetHashCode_EqualDates_SameHashCode", "[cs]") {
    const std::hash<date> hash;
    CHECK(hash(make_date(2080, 5, 15).value()) == hash(make_date(2080, 5, 15).value()));
}

TEST_CASE("OptimizationVerificationTests.GetHashCode_StableAcrossMultipleCalls", "[cs]") {
    const std::hash<date> hash;
    const date a = make_date(2080, 5, 15).value();
    CHECK(hash(a) == hash(a));
}

// cs-deviation: OptimizationVerificationTests.IsLeapYear_ChecksEnglishYearCorrectly#1 D-12 (IsLeapYear is not ported; year_length() instead)

// cs-deviation: OptimizationVerificationTests.IsLeapYear_ChecksEnglishYearCorrectly#2 D-12 (IsLeapYear is not ported; year_length() instead)

// cs-deviation: OptimizationVerificationTests.IsLeapYear_ChecksEnglishYearCorrectly#3 D-12 (IsLeapYear is not ported; year_length() instead)

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#1",
          "[cs]") {
    CHECK(make_date(2080, 0, 15).error() == errc::invalid_month);
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#2",
          "[cs]") {
    CHECK(make_date(2080, 13, 15).error() == errc::invalid_month);
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#3",
          "[cs]") {
    CHECK(make_date(2080, 5, 0).error() == errc::invalid_day);
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#4",
          "[cs]") {
    CHECK(make_date(2080, 5, 33).error() == errc::invalid_day);
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#5",
          "[cs]") {
    CHECK(make_date(1900, 1, 1).error() == errc::out_of_range);
}

TEST_CASE("OptimizationVerificationTests.Constructor_InvalidYearMonthDay_ThrowsException#6",
          "[cs]") {
    CHECK(make_date(2200, 1, 1).error() == errc::out_of_range);
}

TEST_CASE("OptimizationVerificationTests.Constructor_MinValue_IsValid", "[cs]") {
    CHECK(date::min() == make_date(1901, 1, 1).value());
}

TEST_CASE("OptimizationVerificationTests.Constructor_MaxValue_IsValid", "[cs]") {
    CHECK(date::max() == make_date(2199, 12, 30).value());
}

TEST_CASE("OptimizationVerificationTests.EnglishDate_MinValue_ConvertsAndRoundtrips", "[cs]") {
    CHECK(from_gregorian(date::min().to_year_month_day()) == date::min());
}

TEST_CASE("OptimizationVerificationTests.EnglishDate_MaxValue_ConvertsAndRoundtrips", "[cs]") {
    CHECK(from_gregorian(date::max().to_year_month_day()) == date::max());
}

TEST_CASE("OptimizationVerificationTests.EnglishDate_JustBeforeMin_ThrowsException", "[cs]") {
    CHECK(from_gregorian(1844, 4, 10).error() == errc::out_of_range);
}

TEST_CASE("OptimizationVerificationTests.EnglishDate_JustAfterMax_ThrowsException", "[cs]") {
    CHECK(from_gregorian(2143, 4, 16).error() == errc::out_of_range);
}

TEST_CASE("OptimizationVerificationTests.MonthEndDay_CreatingDateWithExactEndDay_Succeeds",
          "[cs]") {
    for (int y = 2070; y <= 2090; ++y) {
        for (int m = 1; m <= 12; ++m) {
            const int length = make_date(y, m, 1)->month_length();
            CHECK(make_date(y, m, length)->day() == length);
            CHECK(make_date(y, m, length + 1).error() == errc::invalid_day);
        }
    }
}

TEST_CASE("OptimizationVerificationTests.DayOfWeek_MatchesEnglishDateDayOfWeek", "[cs]") {
    for (int y = 1910; y <= 2190; y += 30) {
        for (int m = 1; m <= 12; m += 3) {
            const date d = make_date(y, m, 15).value();
            CHECK(d.weekday() == chr::weekday{d.to_sys_days()});
        }
    }
}

// cs-deviation: OptimizationVerificationTests.IsDefault_DefaultStruct_ReturnsTrue D-01 (date has no default value; std::optional<date> stands for no date)

// cs-deviation: OptimizationVerificationTests.IsDefault_ValidDate_ReturnsFalse D-01 (date has no default value; std::optional<date> stands for no date)
