// The patro rows of phase 4 of the C# catalogue (shared/spec/vectors/csharp-tests/
// catalog.tsv). The CalendarDataTests rows check C#'s internal tables; here they check this
// port's tables of ADR-0002 section 7 (dense per-day layout instead of sorted offsets, PATRO
// section 3), so the mapped checks are noted. The interop rows live in tests/cs_phase4.cpp. This
// file is UTF-8; the library headers are ASCII.

#include "../src/patro_tables.hpp"

#include <nepdate/detail/calendar_tables.hpp>
#include <nepdate/patro.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

// NOLINTBEGIN(bugprone-unchecked-optional-access): every std::optional is dereferenced
// after a REQUIRE(has_value()) guard, which the purely syntactic check cannot see.

using nepdate::date;
using nepdate::lang;
using nepdate::make_date;
namespace patro = nepdate::patro;
namespace detail = nepdate::patro::detail;

namespace {

date d(int y, int m, int day) {
    return make_date(y, m, day).value();
}

std::string_view name_of(std::size_t k, lang l) {
    const std::uint32_t i = static_cast<std::uint32_t>(k) * 2U + (l == lang::ne ? 0U : 1U);
    return {detail::kNameBlob.data() + detail::kNameOffsets[i],
            detail::kNameOffsets[i + 1] - detail::kNameOffsets[i]};
}

} // namespace

// ---- CalendarDataTests: the port's tables (ADR-0002 section 7) ----

TEST_CASE("CalendarDataTests.CalendarStrings_Pool_IsNonEmpty", "[cs]") {
    CHECK_FALSE(detail::kNameBlob.empty());
}

TEST_CASE("CalendarDataTests.CalendarStrings_Pool_AllEntriesHaveNonEmptyNpAndEn", "[cs]") {
    const std::size_t pairs = detail::kNameOffsets.size() / 2;
    std::size_t failures = 0;
    for (std::size_t k = 0; k < pairs; ++k) {
        failures += name_of(k, lang::ne).empty() || name_of(k, lang::en).empty() ? 1U : 0U;
    }
    CHECK(failures == 0);
}

TEST_CASE("CalendarDataTests.CalendarOffsets_MonthStart_HasExpectedLength", "[cs]") {
    // The port's calendar is 1901..2199 (ALGORITHM section 1): 299 years of 12 months, plus the
    // total as a sentinel.
    CHECK(nepdate::detail::kMonthStart.size() == 299U * 12U + 1U);
}

TEST_CASE("CalendarDataTests.CalendarOffsets_MonthStart_IsStrictlyMonotonic", "[cs]") {
    std::size_t violations = 0;
    for (std::size_t i = 0; i + 1 < nepdate::detail::kMonthStart.size(); ++i) {
        violations +=
            nepdate::detail::kMonthStart[i] < nepdate::detail::kMonthStart[i + 1] ? 0U : 1U;
    }
    CHECK(violations == 0);
}

TEST_CASE("CalendarDataTests.CalendarOffsets_TotalDays_MatchesExpectedCount", "[cs]") {
    // C# asserts 32509 for its 2001-2089 tables; the port covers 1901-2199 (ALGORITHM section 1).
    CHECK(nepdate::detail::kMonthStart.back() == 109212U);
}

TEST_CASE("CalendarDataTests.TithiData_DayOffsets_IsStrictlySorted", "[cs]") {
    // Dense layout: the tithi days form one contiguous ascending run.
    std::size_t runs = 0;
    std::uint32_t first = detail::kDays;
    std::uint32_t last = 0;
    for (std::uint32_t i = 0; i < detail::kDays; ++i) {
        const bool has = detail::kDayTithi[i] != 0xFFU;
        if (has && (i == 0 || detail::kDayTithi[i - 1] == 0xFFU)) {
            ++runs;
            first = i;
        }
        if (has) {
            last = i;
        }
    }
    CHECK(runs == 1);
    CHECK(first < last);
    CHECK(last < detail::kDays);
}

TEST_CASE("CalendarDataTests.TithiData_AllPoolIds_AreInRange", "[cs]") {
    std::size_t violations = 0;
    for (const std::uint8_t index : detail::kDayTithi) {
        violations += index != 0xFFU && index >= 32U ? 1U : 0U;
    }
    CHECK(violations == 0);
}

TEST_CASE("CalendarDataTests.TithiData_EntryCount_Matches2190", "[cs]") {
    std::size_t tithi_days = 0;
    for (const std::uint8_t index : detail::kDayTithi) {
        tithi_days += index != 0xFFU ? 1U : 0U;
    }
    CHECK(tithi_days == 2190);
}

TEST_CASE("CalendarDataTests.EventData_DayOffsets_IsStrictlySorted", "[cs]") {
    // Dense layout: the days with events come in strictly ascending order.
    std::size_t event_days = 0;
    for (std::uint32_t i = 0; i < detail::kDays; ++i) {
        event_days += detail::kDayEventStart[i] < detail::kDayEventStart[i + 1] ? 1U : 0U;
    }
    CHECK(event_days == 1671);
}

TEST_CASE("CalendarDataTests.EventData_SliceStart_IsNonDecreasing", "[cs]") {
    std::size_t violations = 0;
    for (std::size_t i = 0; i + 1 < detail::kDayEventStart.size(); ++i) {
        violations += detail::kDayEventStart[i] <= detail::kDayEventStart[i + 1] ? 0U : 1U;
    }
    CHECK(violations == 0);
}

TEST_CASE("CalendarDataTests.EventData_SliceStart_SentinelEqualsEntriesLength", "[cs]") {
    CHECK(detail::kDayEventStart.size() == detail::kDays + 1U);
    CHECK(detail::kDayEventStart.back() == detail::kEvents.size());
}

TEST_CASE("CalendarDataTests.EventData_AllEntries_AreInRange", "[cs]") {
    // Each event index resolves to non-empty names in both languages.
    std::size_t violations = 0;
    for (const patro::event e : detail::kEvents) {
        violations += e.name(lang::ne).empty() || e.name(lang::en).empty() ? 1U : 0U;
    }
    CHECK(violations == 0);
}

TEST_CASE("CalendarDataTests.EventData_DayCount_Matches1671", "[cs]") {
    std::size_t event_days = 0;
    for (std::uint32_t i = 0; i < detail::kDays; ++i) {
        event_days += detail::kDayEventStart[i] < detail::kDayEventStart[i + 1] ? 1U : 0U;
    }
    CHECK(event_days == 1671);
}

TEST_CASE("CalendarDataTests.HolidayData_DayOffsets_IsStrictlySorted", "[cs]") {
    std::size_t violations = 0;
    for (std::size_t i = 1; i < detail::kHolidays.size(); ++i) {
        violations += detail::kHolidays[i - 1] < detail::kHolidays[i] ? 0U : 1U;
    }
    CHECK(violations == 0);
}

TEST_CASE("CalendarDataTests.HolidayData_Count_Matches294", "[cs]") {
    CHECK(detail::kHolidays.size() == 294);
}

TEST_CASE("CalendarDataTests.CalendarOffsets_GetOffset_BaseYear_Month1_Day1_ReturnsZero", "[cs]") {
    // C# offsets start at 2001-01-01; the port's serials start at 1901-01-01 (ALGORITHM section 1).
    CHECK(nepdate::detail::to_serial(date::min()) == 0);
}

TEST_CASE("CalendarDataTests.CalendarOffsets_GetOffset_OutOfRange_ReturnsNegativeOne", "[cs]") {
    // C# returns -1 for a year before 2001; the port rejects anything before 1901 instead.
    CHECK(make_date(1900, 1, 1).error() == nepdate::errc::out_of_range);
}

TEST_CASE("CalendarDataTests.CalendarOffsets_GetOffset_AllOffsetsAreUniqueSentinel", "[cs]") {
    // Every month has its own serial (the C# row checks GetOffset(2001, 1, 1) >= 0).
    CHECK(nepdate::detail::to_serial(date::min()) >= 0);
    std::size_t violations = 0;
    for (std::size_t i = 0; i + 1 < nepdate::detail::kMonthStart.size(); ++i) {
        violations +=
            nepdate::detail::kMonthStart[i] < nepdate::detail::kMonthStart[i + 1] ? 0U : 1U;
    }
    CHECK(violations == 0);
}

// ---- CalendarDataTests: the PATRO section 4 anchors through the public API ----

TEST_CASE("CalendarDataTests.NepaliDate_2081_01_01_CalendarData", "[cs]") {
    REQUIRE(patro::info(d(2081, 1, 1)).tithi().has_value());
    CHECK(patro::info(d(2081, 1, 1)).tithi()->name(lang::ne) == "पञ्चमी");
}

TEST_CASE("CalendarDataTests.NepaliDate_2081_01_02_Tithi_IsShashthi", "[cs]") {
    REQUIRE(patro::info(d(2081, 1, 2)).tithi().has_value());
    CHECK(patro::info(d(2081, 1, 2)).tithi()->name(lang::ne) == "षष्ठी");
}

TEST_CASE("CalendarDataTests.NepaliDate_2081_01_05_IsPublicHoliday", "[cs]") {
    CHECK(patro::info(d(2081, 1, 5)).is_public_holiday());
}

TEST_CASE("CalendarDataTests.NepaliDate_2081_02_15_IsPublicHoliday_GanatantraDiwas", "[cs]") {
    CHECK(patro::info(d(2081, 2, 15)).is_public_holiday());
}

TEST_CASE("CalendarDataTests.NepaliDate_2080_11_01_CalendarData", "[cs]") {
    REQUIRE(patro::info(d(2080, 11, 1)).tithi().has_value());
    CHECK(patro::info(d(2080, 11, 1)).tithi()->name(lang::ne) == "चतुर्थी");
}

TEST_CASE("CalendarDataTests.NepaliDate_BeforeRange_AllPropertiesReturnDefaults", "[cs]") {
    const patro::day_info day = patro::info(d(2070, 6, 15));
    CHECK_FALSE(day.tithi().has_value());
    CHECK_FALSE(day.is_public_holiday());
    CHECK(day.events().empty());
}

TEST_CASE("CalendarDataTests.NepaliDate_AfterRange_AllPropertiesReturnDefaults", "[cs]") {
    const patro::day_info day = patro::info(d(2090, 1, 1));
    CHECK_FALSE(day.tithi().has_value());
    CHECK_FALSE(day.is_public_holiday());
    CHECK(day.events().empty());
}

TEST_CASE("CalendarDataTests.NepaliDate_OutsideTithiRange_TithiIsEmpty", "[cs]") {
    // 2077-05-01 is inside the coverage but before the tithi range (2078-01-02).
    const patro::day_info day = patro::info(d(2077, 5, 1));
    CHECK_FALSE(day.tithi().has_value());
}

TEST_CASE("CalendarDataTests.NepaliDate_EventsNp_And_EventsEn_AlwaysHaveSameLength", "[cs]") {
    const patro::day_info day = patro::info(d(2081, 1, 1));
    for (const patro::event e : day.events()) {
        CHECK_FALSE(e.name(lang::ne).empty());
        CHECK_FALSE(e.name(lang::en).empty());
    }
}

TEST_CASE("CalendarDataTests.GetCalendarInfo_TithiMatchesFlatProperties", "[cs]") {
    // The C# flat property and GetCalendarInfo agree; here the API and the dense table do.
    const date day = d(2081, 1, 1);
    const std::uint32_t offset = nepdate::detail::to_serial(day) - detail::kFirst;
    const std::string_view table_name = name_of(detail::kDayTithi[offset], lang::ne);
    REQUIRE(patro::info(day).tithi().has_value());
    CHECK(patro::info(day).tithi()->name(lang::ne) == table_name);
}

TEST_CASE("CalendarDataTests.AllDaysInScrapedRange_OffsetsAreInBounds", "[cs]") {
    // Every covered day resolves its tithi, holiday bit and event slice without leaving the
    // tables: the indices stay in range because the generator checked them.
    std::size_t failures = 0;
    for (std::uint32_t i = 0; i < detail::kDays; ++i) {
        const std::uint8_t tithi_index = detail::kDayTithi[i];
        failures += tithi_index != 0xFFU && tithi_index >= 32U ? 1U : 0U;
        const std::uint16_t first = detail::kDayEventStart[i];
        failures += detail::kDayEventStart[i + 1] < first ? 1U : 0U;
    }
    CHECK(failures == 0);
}

TEST_CASE("CalendarDataTests.AllDaysInScrapedRange_NoExceptionOnPropertyAccess", "[cs]") {
    // info and every day_info accessor run on every covered day without failing.
    std::size_t failures = 0;
    for (const date day : patro::coverage()) {
        const patro::day_info info = patro::info(day);
        failures += info.tithi().has_value() && info.tithi()->name(lang::en).empty() ? 1U : 0U;
        for (const patro::event e : info.events()) {
            failures += e.name(lang::ne).empty() ? 1U : 0U;
        }
    }
    CHECK(failures == 0);
}

TEST_CASE("CalendarDataTests.AllTithiEntries_ResolveToNonEmptyStrings", "[cs]") {
    std::size_t failures = 0;
    for (std::size_t k = 0; k < 32; ++k) {
        failures += name_of(k, lang::ne).empty() || name_of(k, lang::en).empty() ? 1U : 0U;
    }
    CHECK(failures == 0);
}

TEST_CASE("CalendarDataTests.AllHolidayOffsets_AreInBounds", "[cs]") {
    std::size_t failures = 0;
    for (const date holiday : detail::kHolidays) {
        const std::uint32_t offset = nepdate::detail::to_serial(holiday) - detail::kFirst;
        failures += offset >= detail::kDays ? 1U : 0U;
    }
    CHECK(failures == 0);
}

TEST_CASE("CalendarDataTests.AllEventOffsets_AreInBounds", "[cs]") {
    std::size_t failures = 0;
    for (std::uint32_t i = 0; i < detail::kDays; ++i) {
        failures += detail::kDayEventStart[i] > detail::kEvents.size() ? 1U : 0U;
    }
    CHECK(failures == 0);
}

TEST_CASE("CalendarDataTests.HolidayCount_ViaApi_Matches294", "[cs]") {
    CHECK(patro::public_holidays().size() == 294);
}

TEST_CASE("CalendarDataTests.CalendarInfo_EventArrays_AreNeverNull", "[cs]") {
    // The port's span is always valid; a day with events yields a non-empty one.
    CHECK_FALSE(patro::info(d(2081, 1, 1)).events().empty());
}
// NOLINTEND(bugprone-unchecked-optional-access)
