// C3-08: the phase-3 rows of C# NepDate's own tests (shared/spec/vectors/csharp-tests/catalog.tsv).
// Each row is a TEST_CASE named after its id, or a cs-deviation / cs-n/a line. The CTest test
// catalog_coverage (tools/check_catalog.cmake) checks that every row up to phase 3 is here.
//
// C# ToString(string) is format_pattern(d, pattern, lang::en), ToUnicodeString() is
// format_short(d, {}, lang::ne), ToLongDateString is format_long, MonthName is date::month(), and
// Parse/TryParse is parse / parse_lenient. SmartDateParser rows that rely on C#'s field-order
// guessing or 2-digit-year expansion are D-06 deviations; the null-input rows are cs-n/a because
// std::string_view has no null state.

#include <nepdate/date.hpp>
#include <nepdate/format.hpp>
#include <nepdate/parse.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <version> // IWYU pragma: keep: __cpp_lib_format comes from here

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#include <format>
#define NEPDATE_TEST_HAS_FORMAT 1
#else
#define NEPDATE_TEST_HAS_FORMAT 0
#endif

using nepdate::date;
using nepdate::date_order;
using nepdate::lang;
using nepdate::make_date;
using nepdate::month;
using nepdate::separator;

namespace {

date d(int y, int m, int day) {
    return make_date(y, m, day).value();
}

// The date NepaliDateFormattableTests and most OptimizationVerificationTests use.
date formattable_date() {
    return d(2081, 4, 15);
}

// The number of UTF-8 code points: the bytes that aren't a continuation byte (0b10xxxxxx).
std::size_t code_points(std::string_view s) {
    std::size_t n = 0;
    for (const char ch : s) {
        if ((static_cast<unsigned char>(ch) & 0xC0U) != 0x80U) {
            ++n;
        }
    }
    return n;
}

std::string unicode(date dt) {
    return nepdate::format_short(dt, {}, lang::ne);
}

// parse_lenient must produce the given year, month and day.
void check_parse_lenient(const char* input, int y, int m, int day) {
    INFO(input);
    CHECK(nepdate::parse_lenient(input) == make_date(y, m, day));
}

} // namespace

// ---- NepaliDateFormattableTests ----

TEST_CASE("NepaliDateFormattableTests.ToString_NullFormat_ReturnsDefault", "[cs]") {
    CHECK(nepdate::to_string(formattable_date()) == "2081/04/15");
}

TEST_CASE("NepaliDateFormattableTests.ToString_G_SameAsDefault", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "G", lang::en) ==
          nepdate::to_string(formattable_date()));
}

TEST_CASE("NepaliDateFormattableTests.ToString_g_SameAsDefault", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "g", lang::en) ==
          nepdate::to_string(formattable_date()));
}

TEST_CASE("NepaliDateFormattableTests.ToString_d_SameAsDefault", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "d", lang::en) == "2081/04/15");
}

TEST_CASE("NepaliDateFormattableTests.ToString_D_ReturnsLongDateString", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "D", lang::en).find("2081") !=
          std::string::npos);
}

TEST_CASE("NepaliDateFormattableTests.ToString_s_ReturnsSortableIsoFormat", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "s", lang::en) == "2081-04-15");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_yyyy_MM_dd_WithDash", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "yyyy-MM-dd", lang::en) == "2081-04-15");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_dd_MM_yyyy_WithDot", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "dd.MM.yyyy", lang::en) == "15.04.2081");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_MM_slash_dd_slash_yyyy", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "MM/dd/yyyy", lang::en) == "04/15/2081");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_MMMxxx_ReturnsThreeCharMonthAbbreviation",
          "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "MMMxxx", lang::en).starts_with("Shr"));
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_MMMM_ReturnsFullMonthName", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "MMMM d, yyyy", lang::en)
              .starts_with("Shrawan"));
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_yy_ReturnsTwoDigitYear", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "yy/MM/dd", lang::en).starts_with("81/"));
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_M_WithoutLeadingZero_ReturnsUnpadded", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "M/d/yyyy", lang::en) == "4/15/2081");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_SingleDigitMonth_WithoutLeadingZero", "[cs]") {
    CHECK(nepdate::format_pattern(d(2081, 1, 5), "M/d/yyyy", lang::en) == "1/5/2081");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_LiteralInSingleQuotes", "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), "yyyy'BS'", lang::en) == "2081BS");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_BackslashEscapedTokens_ProduceLiterals",
          "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), R"(yyyy\M\M-\d\d)", lang::en) == "2081MM-dd");
}

TEST_CASE("NepaliDateFormattableTests.CustomFormat_BackslashBeforeLiteralChar_IsPassedThrough",
          "[cs]") {
    CHECK(nepdate::format_pattern(formattable_date(), R"(yyyy\-MM\-dd)", lang::en) == "2081-04-15");
}

TEST_CASE("NepaliDateFormattableTests.StringInterpolation_WithFormatSpecifier_UsesIFormattable",
          "[cs]") {
#if NEPDATE_TEST_HAS_FORMAT
    CHECK(std::format("{:yyyy-MM-dd}", formattable_date()) == "2081-04-15");
#else
    SKIP("this standard library has no <format>");
#endif
}

TEST_CASE("NepaliDateFormattableTests.StringFormatMethod_WithSpecifier", "[cs]") {
#if NEPDATE_TEST_HAS_FORMAT
    CHECK(std::format("{:yyyy-MM-dd}", formattable_date()) == "2081-04-15");
#else
    SKIP("this standard library has no <format>");
#endif
}

TEST_CASE("NepaliDateFormattableTests.SortableFormat_LexicographicOrderEqualsChronologicalOrder",
          "[cs]") {
    const std::string earlier = nepdate::format_pattern(d(2080, 5, 15), "s", lang::en);
    const std::string later = nepdate::format_pattern(d(2081, 4, 15), "s", lang::en);
    CHECK(earlier < later);
}

// ---- NepaliDateParsableTests ----

TEST_CASE("NepaliDateParsableTests.Parse_Via_GenericConstraint_Returns_CorrectDate", "[cs]") {
    CHECK(nepdate::parse("2080/05/15") == d(2080, 5, 15));
}

TEST_CASE("NepaliDateParsableTests.TryParse_Via_GenericConstraint_ValidDate_Returns_True", "[cs]") {
    CHECK(nepdate::parse("2080/05/15").has_value());
}

TEST_CASE("NepaliDateParsableTests.TryParse_Via_GenericConstraint_InvalidDate_Returns_False",
          "[cs]") {
    CHECK_FALSE(nepdate::parse("invalid").has_value());
}

TEST_CASE("NepaliDateParsableTests.SpanParse_Via_GenericConstraint_Returns_CorrectDate", "[cs]") {
    CHECK(nepdate::parse("2080/05/15") == d(2080, 5, 15));
}

TEST_CASE("NepaliDateParsableTests.SpanTryParse_Via_GenericConstraint_ValidDate_Returns_True",
          "[cs]") {
    CHECK(nepdate::parse("2080/05/15").has_value());
}

// ---- NepaliDateMonthNameTests ----

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#1", "[cs]") {
    CHECK(d(2080, 1, 1).month() == month::baishakh);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#2", "[cs]") {
    CHECK(d(2080, 2, 1).month() == month::jestha);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#3", "[cs]") {
    CHECK(d(2080, 3, 1).month() == month::ashad);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#4", "[cs]") {
    CHECK(d(2080, 4, 1).month() == month::shrawan);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#5", "[cs]") {
    CHECK(d(2080, 5, 1).month() == month::bhadra);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#6", "[cs]") {
    CHECK(d(2080, 6, 1).month() == month::ashoj);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#7", "[cs]") {
    CHECK(d(2080, 7, 1).month() == month::kartik);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#8", "[cs]") {
    CHECK(d(2080, 8, 1).month() == month::mangsir);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#9", "[cs]") {
    CHECK(d(2080, 9, 1).month() == month::poush);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#10", "[cs]") {
    CHECK(d(2080, 10, 1).month() == month::magh);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#11", "[cs]") {
    CHECK(d(2080, 11, 1).month() == month::falgun);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AllMonths_ReturnCorrectEnum#12", "[cs]") {
    CHECK(d(2080, 12, 1).month() == month::chaitra);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_SameMonthDifferentDays_AllReturnSameEnum", "[cs]") {
    CHECK(d(2080, 4, 1).month() == month::shrawan);
    CHECK(d(2080, 4, 15).month() == month::shrawan);
    CHECK(d(2080, 4, 31).month() == month::shrawan);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#1",
          "[cs]") {
    CHECK(d(2070, 9, 15).month() == month::poush);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#2",
          "[cs]") {
    CHECK(d(2080, 9, 15).month() == month::poush);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#3",
          "[cs]") {
    CHECK(d(2090, 9, 15).month() == month::poush);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_SameMonthDifferentYears_AllReturnSameEnum#4",
          "[cs]") {
    CHECK(d(2100, 9, 15).month() == month::poush);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_AddingOneMonth_AdvancesEnum", "[cs]") {
    CHECK(d(2080, 10, 15).add_months(1).value().month() == month::falgun);
}

TEST_CASE("NepaliDateMonthNameTests.MonthName_YearBoundary_WrapsFromChaitraToBasikhakh", "[cs]") {
    CHECK(d(2080, 12, 15).month() == month::chaitra);
    CHECK(d(2080, 12, 15).add_months(1).value().month() == month::baishakh);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#1", "[cs]") {
    CHECK(static_cast<int>(month::baishakh) == 1);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#2", "[cs]") {
    CHECK(static_cast<int>(month::jestha) == 2);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#3", "[cs]") {
    CHECK(static_cast<int>(month::ashad) == 3);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#4", "[cs]") {
    CHECK(static_cast<int>(month::shrawan) == 4);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#5", "[cs]") {
    CHECK(static_cast<int>(month::bhadra) == 5);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#6", "[cs]") {
    CHECK(static_cast<int>(month::ashoj) == 6);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#7", "[cs]") {
    CHECK(static_cast<int>(month::kartik) == 7);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#8", "[cs]") {
    CHECK(static_cast<int>(month::mangsir) == 8);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#9", "[cs]") {
    CHECK(static_cast<int>(month::poush) == 9);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#10", "[cs]") {
    CHECK(static_cast<int>(month::magh) == 10);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#11", "[cs]") {
    CHECK(static_cast<int>(month::falgun) == 11);
}

TEST_CASE("NepaliDateMonthNameTests.NepaliMonthsEnum_IntegerValueMatchesMonthNumber#12", "[cs]") {
    CHECK(static_cast<int>(month::chaitra) == 12);
}

// ---- OptimizationVerificationTests: ToString ----

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#1", "[cs]") {
    CHECK(nepdate::to_string(d(2080, 5, 15)) == "2080/05/15");
}

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#2", "[cs]") {
    CHECK(nepdate::to_string(d(2080, 1, 1)) == "2080/01/01");
}

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#3", "[cs]") {
    CHECK(nepdate::to_string(d(2080, 12, 30)) == "2080/12/30");
}

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#4", "[cs]") {
    CHECK(nepdate::to_string(d(1901, 1, 1)) == "1901/01/01");
}

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#5", "[cs]") {
    CHECK(nepdate::to_string(d(2199, 12, 30)) == "2199/12/30");
}

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#6", "[cs]") {
    CHECK(nepdate::to_string(d(2000, 6, 9)) == "2000/06/09");
}

TEST_CASE("OptimizationVerificationTests.ToString_ProducesExactFormat#7", "[cs]") {
    CHECK(nepdate::to_string(d(2100, 10, 29)) == "2100/10/29");
}

TEST_CASE("OptimizationVerificationTests.ToString_OutputIsExactly10Characters", "[cs]") {
    CHECK(nepdate::to_string(d(2080, 5, 15)).size() == 10);
}

TEST_CASE("OptimizationVerificationTests.ToString_AllMonthsZeroPadded", "[cs]") {
    for (int m = 1; m <= 12; ++m) {
        CAPTURE(m);
        const std::string s = nepdate::to_string(d(2080, m, 1));
        CHECK(s.substr(5, 2) == (m < 10 ? "0" + std::to_string(m) : std::to_string(m)));
    }
}

TEST_CASE("OptimizationVerificationTests.ToString_AllDaysZeroPadded", "[cs]") {
    for (int day = 1; day <= 31; ++day) {
        CAPTURE(day);
        const std::string s = nepdate::to_string(d(2080, 5, day));
        CHECK(s.substr(8, 2) == (day < 10 ? "0" + std::to_string(day) : std::to_string(day)));
    }
}

TEST_CASE("OptimizationVerificationTests.ToString_RoundtripsWithConstructor", "[cs]") {
    CHECK(nepdate::parse(nepdate::to_string(d(2080, 5, 15))) == d(2080, 5, 15));
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#1",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::slash, true},
                                lang::en) == "2080/05/15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#2",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::dash, true},
                                lang::en) == "2080-05-15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#3",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::dot, true},
                                lang::en) == "2080.05.15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#4",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::underscore, true},
                                lang::en) == "2080_05_15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#5",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::space, true},
                                lang::en) == "2080 05 15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#6",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::backslash, true},
                                lang::en) == "2080\\05\\15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#7",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::dmy, separator::slash, true},
                                lang::en) == "15/05/2080");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#8",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::dmy, separator::dash, true},
                                lang::en) == "15-05-2080");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#9",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::mdy, separator::slash, true},
                                lang::en) == "05/15/2080");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#10",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ydm, separator::slash, true},
                                lang::en) == "2080/15/05");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#11",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::myd, separator::slash, true},
                                lang::en) == "05/2080/15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithFormatAndSeparator_ProducesCorrectOutput#12",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::dym, separator::slash, true},
                                lang::en) == "15/2080/05");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithoutLeadingZeros_ProducesCorrectOutput#1",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::slash, false},
                                lang::en) == "2080/5/15");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithoutLeadingZeros_ProducesCorrectOutput#2",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::dmy, separator::dash, false},
                                lang::en) == "15-5-2080");
}

TEST_CASE("OptimizationVerificationTests.ToString_WithoutLeadingZeros_ProducesCorrectOutput#3",
          "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::mdy, separator::dot, false},
                                lang::en) == "5.15.2080");
}

// ---- OptimizationVerificationTests: ToUnicodeString ----

TEST_CASE("OptimizationVerificationTests.ToUnicodeString_DefaultFormat_ConvertsAllDigits", "[cs]") {
    CHECK(unicode(d(2080, 5, 15)) == "२०८०/०५/१५");
}

TEST_CASE("OptimizationVerificationTests.ToUnicodeString_EachDigitConvertsCorrectly", "[cs]") {
    CHECK(unicode(d(2080, 5, 15)).find("१") != std::string::npos);
}

TEST_CASE("OptimizationVerificationTests.ToUnicodeString_SeparatorsPreserved", "[cs]") {
    CHECK(nepdate::format_short(d(2080, 5, 15), {date_order::ymd, separator::dash, true},
                                lang::ne) == "२०८०-०५-१५");
}

TEST_CASE("OptimizationVerificationTests.ToUnicodeString_SameLengthAsToString", "[cs]") {
    CHECK(code_points(unicode(d(2080, 5, 15))) == nepdate::to_string(d(2080, 5, 15)).size());
}

// ---- OptimizationVerificationTests: SmartDateParser ----

TEST_CASE("OptimizationVerificationTests.SmartDateParser_NepaliDigits_ParseCorrectly#1", "[cs]") {
    check_parse_lenient("२०८०/०५/१५", 2080, 5, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_NepaliDigits_ParseCorrectly#2", "[cs]") {
    check_parse_lenient("२०८०-०५-१५", 2080, 5, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_PureEnglishDigits_NoConversionNeeded",
          "[cs]") {
    CHECK(nepdate::parse_lenient("2080/05/15").value().year() == 2080);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_MixedNepaliEnglishDigits_ParseCorrectly",
          "[cs]") {
    check_parse_lenient("२०80/0५/१5", 2080, 5, 15);
}

TEST_CASE(
    "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#1",
    "[cs]") {
    check_parse_lenient("2080/05/15 B.S.", 2080, 5, 15);
}

TEST_CASE(
    "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#2",
    "[cs]") {
    check_parse_lenient("2080/05/15 BS", 2080, 5, 15);
}

TEST_CASE(
    "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#3",
    "[cs]") {
    check_parse_lenient("2080/05/15 V.S.", 2080, 5, 15);
}

TEST_CASE(
    "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#4",
    "[cs]") {
    check_parse_lenient("2080/05/15 VS", 2080, 5, 15);
}

TEST_CASE(
    "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#5",
    "[cs]") {
    check_parse_lenient("2080/05/15 bs", 2080, 5, 15);
}

TEST_CASE(
    "OptimizationVerificationTests.SmartDateParser_WithIndicators_RemovesAndParsesCorrectly#6",
    "[cs]") {
    check_parse_lenient("2080/05/15 v.s.", 2080, 5, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_WithNepaliKeywords_RemovesAndParses",
          "[cs]") {
    check_parse_lenient("2080/05/15 गते", 2080, 5, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_WithExtraSpaces_ParsesCorrectly", "[cs]") {
    check_parse_lenient("   2080   /   05   /   15   ", 2080, 5, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#1", "[cs]") {
    check_parse_lenient("15 Baisakh 2080", 2080, 1, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#2", "[cs]") {
    check_parse_lenient("15 Jestha 2080", 2080, 2, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#3", "[cs]") {
    check_parse_lenient("15 Asar 2080", 2080, 3, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#4", "[cs]") {
    check_parse_lenient("15 Shrawan 2080", 2080, 4, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#5", "[cs]") {
    check_parse_lenient("15 Bhadra 2080", 2080, 5, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#6", "[cs]") {
    check_parse_lenient("15 Ashwin 2080", 2080, 6, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#7", "[cs]") {
    check_parse_lenient("15 Kartik 2080", 2080, 7, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#8", "[cs]") {
    check_parse_lenient("15 Mangsir 2080", 2080, 8, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#9", "[cs]") {
    check_parse_lenient("15 Poush 2080", 2080, 9, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#10", "[cs]") {
    check_parse_lenient("15 Magh 2080", 2080, 10, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#11", "[cs]") {
    check_parse_lenient("15 Falgun 2080", 2080, 11, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_AllMonthNames_ParseCorrectly#12", "[cs]") {
    check_parse_lenient("15 Chaitra 2080", 2080, 12, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_MonthNameDifferentPositions_"
          "ParsesCorrectly#1",
          "[cs]") {
    check_parse_lenient("Baisakh 15, 2080", 2080, 1, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_MonthNameDifferentPositions_"
          "ParsesCorrectly#2",
          "[cs]") {
    check_parse_lenient("2080 Baisakh 15", 2080, 1, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#1",
          "[cs]") {
    check_parse_lenient("15 वैशाख 2080", 2080, 1, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#2",
          "[cs]") {
    check_parse_lenient("15 जेष्ठ 2080", 2080, 2, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#3",
          "[cs]") {
    check_parse_lenient("15 असार 2080", 2080, 3, 15);
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_NepaliUnicodeMonthNames_ParseCorrectly#4",
          "[cs]") {
    check_parse_lenient("15 श्रावण 2080", 2080, 4, 15);
}

// cs-deviation: OptimizationVerificationTests.SmartDateParser_TwoDigitYear_AdjustedTo2000s D-06 (2-digit years are not expanded)

TEST_CASE("OptimizationVerificationTests.SmartDateParser_InvalidInput_ThrowsFormatException",
          "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("not a date").has_value());
}

// cs-n/a: OptimizationVerificationTests.SmartDateParser_NullInput_ThrowsException std::string_view has no null state

TEST_CASE("OptimizationVerificationTests.SmartDateParser_EmptyInput_ThrowsException", "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("").has_value());
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_WhitespaceInput_ThrowsException", "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient(" ").has_value());
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_TryParse_ValidInput_ReturnsTrue", "[cs]") {
    CHECK(nepdate::parse_lenient("2080/05/15").has_value());
}

TEST_CASE("OptimizationVerificationTests.SmartDateParser_TryParse_InvalidInput_ReturnsFalse",
          "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("garbage").has_value());
}

// cs-n/a: OptimizationVerificationTests.SmartDateParser_TryParse_NullInput_ReturnsFalse std::string_view has no null state

// ---- OptimizationVerificationTests: round-trips and long format ----

TEST_CASE("OptimizationVerificationTests.ToString_EveryMonthFirstDay_RoundtripsViaConstructor",
          "[cs]") {
    for (int m = 1; m <= 12; ++m) {
        CAPTURE(m);
        const date first = d(2080, m, 1);
        CHECK(nepdate::parse(nepdate::to_string(first)) == first);
    }
}

TEST_CASE("OptimizationVerificationTests.UnicodeConversion_AllDigits_RoundtripCorrectly", "[cs]") {
    for (int m = 1; m <= 12; ++m) {
        CAPTURE(m);
        const date first = d(2080, m, 1);
        CHECK(nepdate::parse(unicode(first)) == first);
    }
}

TEST_CASE("OptimizationVerificationTests.ToLongDateString_Default_ContainsMonthNameAndDay",
          "[cs]") {
    CHECK(nepdate::format_long(d(2080, 5, 15), {}, lang::en).find("15") != std::string::npos);
}

TEST_CASE("OptimizationVerificationTests.ToLongDateString_WithDayName_ContainsDayOfWeek", "[cs]") {
    const std::string s = nepdate::format_long(d(2080, 5, 15), {.weekday = true}, lang::en);
    CHECK(s.find("Friday") != std::string::npos);
}

TEST_CASE("OptimizationVerificationTests.ToLongDateString_WithoutYear_DoesNotContainYear", "[cs]") {
    const std::string s = nepdate::format_long(d(2080, 5, 15), {.year = false}, lang::en);
    CHECK(s.find("2080") == std::string::npos);
}

// ---- SmartDateParserTests ----

TEST_CASE("SmartDateParserTests.Parse_StandardFormat_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("2080/04/15", 2080, 4, 15);
}

TEST_CASE("SmartDateParserTests.Parse_InvertedFormat_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("15/04/2080", 2080, 4, 15);
}

// cs-deviation: SmartDateParserTests.Parse_MonthDayYearFormat_ReturnsCorrectDate D-06 (lenient parse reads 04/15/2080 as day 04 month 15, not month 04 day 15)

TEST_CASE("SmartDateParserTests.Parse_WithMonthNames_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("15 Shrawan 2080", 2080, 4, 15);
}

TEST_CASE("SmartDateParserTests.Parse_WithNepaliUnicode_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("२०८०/०४/१५", 2080, 4, 15);
}

TEST_CASE("SmartDateParserTests.Parse_MixedFormats_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("15 साउन 2080", 2080, 4, 15);
}

TEST_CASE("SmartDateParserTests.Parse_WithSuffixes_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("15 Shrawan 2080 B.S.", 2080, 4, 15);
}

TEST_CASE("SmartDateParserTests.Parse_WithTypos_ReturnsCorrectDate", "[cs]") {
    check_parse_lenient("15 Srawan 2080", 2080, 4, 15);
}

// cs-deviation: SmartDateParserTests.Parse_ShorterYearFormats_ReturnsCorrectDate D-06 (2-digit years are not expanded)

TEST_CASE("SmartDateParserTests.Parse_InvalidFormat_ThrowsFormatException", "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("not a date").has_value());
}

TEST_CASE("SmartDateParserTests.TryParse_ValidFormat_ReturnsTrue", "[cs]") {
    CHECK(nepdate::parse_lenient("2080/04/15").has_value());
}

TEST_CASE("SmartDateParserTests.TryParse_InvalidFormat_ReturnsFalse", "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("garbage").has_value());
}

TEST_CASE("SmartDateParserTests.ExtensionMethod_ToNepaliDate_ParsesCorrectly", "[cs]") {
    check_parse_lenient("2080/04/15", 2080, 4, 15);
}

TEST_CASE("SmartDateParserTests.ExtensionMethod_TryToNepaliDate_ParsesCorrectly", "[cs]") {
    CHECK(nepdate::parse_lenient("2080/04/15").has_value());
}

// ---- StringExtensionsTests ----

TEST_CASE("StringExtensionsTests.ToNepaliDate_ValidDateString_ReturnsNepaliDate", "[cs]") {
    check_parse_lenient("2080/05/15", 2080, 5, 15);
}

TEST_CASE("StringExtensionsTests.ToNepaliDate_NepaliDigits_ReturnsNepaliDate", "[cs]") {
    check_parse_lenient("२०८०/०५/१५", 2080, 5, 15);
}

TEST_CASE("StringExtensionsTests.ToNepaliDate_InvalidDateString_ThrowsFormatException", "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("invalid").has_value());
}

TEST_CASE("StringExtensionsTests.TryToNepaliDate_ValidDateString_ReturnsTrue", "[cs]") {
    CHECK(nepdate::parse_lenient("2080/05/15").has_value());
}

TEST_CASE("StringExtensionsTests.TryToNepaliDate_InvalidDateString_ReturnsFalse", "[cs]") {
    CHECK_FALSE(nepdate::parse_lenient("invalid").has_value());
}
