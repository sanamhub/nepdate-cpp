# `nepdate` (C++): public API reference (v0.1)

The complete public surface. Behaviour is defined by the shared specification in `shared/spec/`
(`ALGORITHM.md`, `FORMATTING.md`, `PARSING.md`, `PATRO.md`, `PARITY.md`). Design decisions are in
ADR-0002 (representation), ADR-0003 (conventions and errors) and ADR-0006 (rvalue accessors).

Rules that apply to every declaration below:

- `[[nodiscard]]` is on every function that returns a value.
- Everything is `constexpr` unless it reads the clock, returns `std::string` or `std::vector`, or
  uses `std::ostream`. The `constexpr` members are proved by `static_assert` in the tests.
- `noexcept` is on everything except `result<T>::value()`, the value-returning accessors of a
  non-trivial result, and the functions that allocate or format through the standard library.
- Anything in `nepdate::detail` or `nepdate::patro::detail` is not API, even when a header makes it
  visible.
- The core is header-only and adds no dependency. `<nepdate/nepdate.hpp>` includes all of the core.
  The pieces are `<nepdate/date.hpp>`, `<nepdate/date_range.hpp>`, `<nepdate/convert.hpp>`,
  `<nepdate/fiscal_year.hpp>`, `<nepdate/format.hpp>`, `<nepdate/ostream.hpp>`,
  `<nepdate/parse.hpp>` and `<nepdate/version.hpp>`.

## Dates, identity and comparison

| C# member | C++ member | Notes |
|---|---|---|
| `NepaliDate(int yearBs, int monthBs, int dayBs)` | `make_date(int, int, int)`, `make_date(int, month, int)` | Returns `result<date>`. |
| `NepaliDate(string rawNepaliDate)` | `parse(std::string_view)` | Returns `result<date>`. |
| `NepaliDate(DateTime engDate)` | `from_gregorian(...)`, `from_sys_days(...)` | Returns `result<date>`. |
| `bool IsDefault { get; }` | not ported | A valid `date` always exists, so there is no zero value. |
| `int Year { get; }` | `date::year()` | 1901 to 2199. |
| `int Month { get; }` | `date::month()` | Returns the `month` enumerator. |
| `int Day { get; }` | `date::day()` | 1 to the month length. |
| `DateTime EnglishDate { get; }` | `date::to_year_month_day()`, `date::to_sys_days()` | The AD date. |
| `DayOfWeek DayOfWeek { get; }` | `date::weekday()` | `std::chrono::weekday`, Sunday is 0. |
| `int DayOfYear { get; }` | `date::day_of_year()` | 1-based in the BS year. |
| `int MonthEndDay { get; }` | `date::month_length()` | 29 to 32. |
| `NepaliMonths MonthName { get; }` | `date::month()` | The `month` enumerator. |
| `static NepaliDate Today { get; }` | `today(std::chrono::minutes = nepal_utc_offset)` | Reads the system clock. |
| `static NepaliDate Now { get; }` | `today(...)` | Same date as `Today`. |
| `static readonly NepaliDate MinValue` | `date::min()` | BS 1901-01-01. |
| `static readonly NepaliDate MaxValue` | `date::max()` | BS 2199-12-30. |
| `NepaliDate MonthEndDate()` | `date::last_of_month()` | The same year and month, last day. |
| `void Deconstruct(out int, out int, out int)` | `date::to_tuple()` | Works with `auto [y, m, d]`. |
| `bool IsLeapYear()` | `date::year_length()` | The BS year length, 364 to 367. |
| `bool IsToday()` | `date::is_today(minutes)` | Takes the zone explicitly. |
| `bool IsYesterday()` | `date::is_yesterday(minutes)` | Takes the zone explicitly. |
| `bool IsTomorrow()` | `date::is_tomorrow(minutes)` | Takes the zone explicitly. |
| `int CompareTo(NepaliDate)` | `operator<=>` | Standard ordering. |
| `bool Equals(NepaliDate)` | `operator==` | Standard equality. |
| `override int GetHashCode()` | `std::hash<date>` | Hashes the packed value. |

`date` is 4 bytes, trivially copyable, ordered chronologically, and has no default constructor.
`to_tuple` returns `std::tuple<int, int, int>`, which keeps the month as a number like the C#
deconstruction. Every clock member also has an injected-clock form:

```cpp
struct fixed_clock {
    static constexpr std::chrono::sys_seconds now() noexcept { return std::chrono::sys_seconds{}; }
};

const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
const int year = d.year();
const std::chrono::weekday weekday = d.weekday();
const bool today = d.is_today(nepdate::nepal_utc_offset, fixed_clock{});
auto [y, m, day] = d.to_tuple();
```

## Conversion and serialization

| C# member | C++ member | Notes |
|---|---|---|
| `DateTimeExtensions.ToNepaliDate(this DateTime)` | `from_gregorian(int, int, int)`, `from_gregorian(std::chrono::year_month_day)` | Returns `result<date>`. |
| `DateTimeExtensions.ToNepaliDate(this DateTime)` | `from_sys_days(std::chrono::sys_days)` | Days since 1970-01-01. |
| `DateTimeExtensions.ToNepaliDate(this DateTime)` | `from_local_days(std::chrono::local_days)` | A local calendar day. |
| `NepaliDate.BulkConvert.ToNepaliDates(IEnumerable<DateTime>)` | `to_nepali_dates(std::span<const std::chrono::sys_days>)` | Ordered; returns `result<std::vector<date>>`. |
| `BulkConvert.ToEnglishDates(IEnumerable<string>)` | `to_english_dates(std::span<const std::string>)` | Ordered; returns `result<std::vector<std::chrono::sys_days>>`. |
| `BulkConvert.ToEnglishDates(IEnumerable<NepaliDate>)` | `to_english_dates(std::span<const date>)` | Ordered; returns `std::vector<std::chrono::sys_days>`. |
| `NepaliDateTypeConverter` integer form | `from_ymd_int(std::int64_t)` | Reads `YYYYMMDD`. |
| `NepaliDateTypeConverter` integer form | `date::to_ymd_int()` | Writes `YYYYMMDD`. |
| `NepaliDateJsonConverter` | `to_iso_string(date)`, `parse(...)` | The canonical `YYYY-MM-DD` text. |
| `NepaliDateJsonConverter` | `operator<<`, `std::formatter<date>` | The native stream and format hooks. |

Batch conversion keeps input order. `from_ymd_int` takes a `std::int64_t` and decodes in 64 bits
before narrowing, so a value beyond `int` cannot wrap into a valid date. The `YYYYMMDD` integer is
20810415 for BS 2081-04-15.

```cpp
#include <nepdate/nepdate.hpp>

#include <chrono>
#include <vector>

const std::vector<std::chrono::sys_days> gregorian{
    std::chrono::sys_days{std::chrono::year{2024} / 7 / 30}};
const auto bs = nepdate::to_nepali_dates(gregorian);
const auto ad = nepdate::to_english_dates(*bs);
const std::string iso = nepdate::to_iso_string((*bs)[0]); // "2081-04-15"
const int ymd = (*bs)[0].to_ymd_int();                    // 20810415
const auto back = nepdate::from_ymd_int(ymd);
```

## Arithmetic

| C# member | C++ member | Notes |
|---|---|---|
| `NepaliDate AddMonths(double months, bool awayFromMonthEnd = false)` | `date::add_months(int, overflow)` | Optional `overflow::clamp` or `overflow::spill`. |
| `NepaliDate AddDays(double days)` | `date::add_days(int)` | Returns `result<date>`. |
| `NepaliDate AddYears(int years, bool awayFromMonthEnd = false)` | `date::add_years(int, overflow)` | `add_months(12 * years)`. |
| `TimeSpan Subtract(NepaliDate nepDateTo)` | `date::days_until(date)` | Returns `other - this` (D-07). |
| `TimeSpan Subtract(NepaliDate nepDateTo)` | `date::diff(date)` | Years, months and days as a `date_diff`. |
| `static TimeSpan operator -(NepaliDate d1, NepaliDate d2)` | `operator-(date, date)` | Returns a `date_diff`; its `total_days` is `d1 - d2`. |

`days_until` and `operator-` answer opposite questions and are not interchangeable. The operator
keeps the C# direction, positive when the left date is later. `diff` keeps the named direction,
with `total_days` equal to `days_until`.

```cpp
const nepdate::date d = nepdate::make_date(2081, 4, 32).value();
const auto clamped = d.add_months(2, nepdate::overflow::clamp); // 2081-06-30
const auto spilled = d.add_months(2, nepdate::overflow::spill); // 2081-07-02
const int days = d.days_until(nepdate::make_date(2081, 5, 1).value());
const int signed_days = (d - nepdate::make_date(2081, 5, 1).value()).total_days;
```

## Formatting

| C# member | C++ member | Notes |
|---|---|---|
| `override string ToString()` | `to_string(date)`, `to_chars(char*, char*, date)` | `YYYY/MM/DD`. |
| `string ToString(DateFormats, Separators, bool leadingZeros)` | `to_chars_short(...)`, `format_short(...)` | `short_format` carries order, separator and padding. |
| `string ToLongDateString(...)` | `to_chars_long(...)`, `format_long(...)` | `long_format` carries weekday, year and padding. |
| `string ToUnicodeString(...)` | `to_chars_short(..., lang::ne)` | Devanagari digits. |
| `string ToLongDateUnicodeString(...)` | `to_chars_long(..., lang::ne)` | Devanagari names and digits. |
| `string ToString(string format, IFormatProvider)` | `to_chars_pattern(...)`, `format_pattern(...)` | Standard and custom patterns. |
| `ISpanFormattable.TryFormat(...)` | `to_chars_*` | Returns `std::to_chars_result`. |
| `enum DateFormats` | `date_order` | `ymd`, `ydm`, `myd`, `mdy`, `dym`, `dmy`. |
| `enum Separators` | `separator` | Slash, backslash, dot, underscore, dash, space. |
| `enum NepaliMonths` | `month` | 1 to 12. |

Every `to_chars_*` writes into `[first, last)` without allocating and returns
`{end of output, errc{}}`, or `{last, std::errc::value_too_large}` when the buffer is too small.
`max_short_size` and `max_long_size` are bounds for the fixed forms. The `std::string` twins
allocate only the returned string.

The custom pattern language is the shared one: `yyyy` and longer runs write the four-digit year,
`y`, `yy` and `yyy` the last two digits, `MMMM` the full month name, `MMM` the short name (`Asa` and
`Aso` for Ashad and Ashoj), `MM` and `M` the month, `dd` and `d` the day, a backslash escapes the
next character and a trailing backslash writes a literal `\`, and single quotes hold literal text.

```cpp
#include <nepdate/nepdate.hpp>

#include <array>
#include <string>

const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
const std::string plain = nepdate::to_string(d);                       // "2081/04/15"
const std::string nepali = nepdate::format_short(d, {}, nepdate::lang::ne);
const std::string long_form = nepdate::format_long(d, {.weekday = true}, nepdate::lang::en);
const std::string pattern = nepdate::format_pattern(d, "yyyy-MM-dd", nepdate::lang::en);
std::array<char, 16> buffer{};
const auto written = nepdate::to_chars_short(
    buffer.data(), buffer.data() + buffer.size(), d,
    {nepdate::date_order::dmy, nepdate::separator::dash, false}, nepdate::lang::en);
```

## Parsing

| C# member | C++ member | Notes |
|---|---|---|
| `NepaliDate.Parse(string)` | `parse(std::string_view)` | Returns `result<date>`. |
| `NepaliDate.TryParse(string, out NepaliDate)` | `try_parse(std::string_view)` | Returns `std::optional<date>`. |
| `SmartDateParser.Parse(string)` | `parse_lenient(std::string_view)` | Returns `result<date>`. |
| `SmartDateParser.TryParse(string, out NepaliDate)` | `try_parse_lenient(std::string_view)` | Returns `std::optional<date>`. |
| `InvalidNepaliDateFormatException` | `errc` and `result<T>` | Error kinds are the `errc` enumerators. |

Strict parsing accepts year, month and day in that order, with 1 to 4 ASCII or Devanagari digits
per group, mixed if needed. The separators are `-`, `/`, `.`, `_`, backslash, space, `|` and the
danda U+0964. Leading, trailing and repeated separators are ignored. The lenient parser follows the
closed rules in `PARSING.md`: a month name with two numbers, or three numbers in year-month-day or
day-month-year order. A successful parse still validates the date.

The `u8` overloads accept `u8""` literals. `try_parse` and `try_parse_lenient` never fail.

```cpp
#include <nepdate/nepdate.hpp>

#include <optional>

const nepdate::result<nepdate::date> strict = nepdate::parse("2081/04/15");
const std::optional<nepdate::date> lenient = nepdate::try_parse_lenient("2081 Shrawan 15");
if (lenient.has_value()) {
    const nepdate::date d = *lenient;
}
```

## Ranges

| C# member | C++ member | Notes |
|---|---|---|
| `readonly struct NepaliDateRange` | `date_range` | Inclusive, non-empty, a random-access view. |
| `NepaliDateRange(NepaliDate, NepaliDate)` | `date_range::make(date, date)` | `std::nullopt` when `start > end`. |
| `NepaliDate Start { get; }` | `date_range::first()` | First day. |
| `NepaliDate End { get; }` | `date_range::last()` | Last day, inclusive. |
| `bool IsEmpty { get; }` | not exposed | A range is always non-empty. |
| `int Length { get; }` | `date_range::size()` | Number of days. |
| `static NepaliDateRange SingleDay(NepaliDate)` | `date_range::single(date)` | One day. |
| `static NepaliDateRange FromDayCount(NepaliDate, int)` | `date_range::from_day_count(date, int)` | Returns `result<date_range>`. |
| `static NepaliDateRange ForMonth(int, int)` | `date_range::of_month(int, month)` | Returns `result<date_range>`. |
| `static NepaliDateRange ForFiscalYear(int)` | `fiscal_year{year}.range()` | Returns `result<date_range>`. |
| `static NepaliDateRange ForCalendarYear(int)` | `date_range::of_year(int)` | Returns `result<date_range>`. |
| `static NepaliDateRange CurrentMonth()` | `current_month(minutes)` | Takes the zone; injected-clock form too. |
| `static NepaliDateRange CurrentFiscalYear()` | `current_fiscal_year(minutes)` | Takes the zone; injected-clock form too. |
| `static NepaliDateRange CurrentCalendarYear()` | `current_calendar_year(minutes)` | Takes the zone; injected-clock form too. |
| `bool Contains(NepaliDate)` | `date_range::contains(date)` | Inclusive. |
| `bool Contains(NepaliDateRange)` | `date_range::contains(date_range)` | Inclusive. |
| `bool Overlaps(NepaliDateRange)` | `date_range::overlaps(date_range)` | A shared day. |
| `bool IsAdjacentTo(NepaliDateRange)` | `date_range::is_adjacent_to(date_range)` | False at the data ends. |
| `NepaliDateRange Intersect(NepaliDateRange)` | `date_range::intersect(date_range)` | `std::nullopt` when disjoint. |
| `NepaliDateRange Union(NepaliDateRange)` | `date_range::union_with(date_range)` | Smallest covering range. |
| `NepaliDateRange[] Except(NepaliDateRange)` | `date_range::subtract(date_range)` | Returns a `range_set` of zero to two ranges. |
| `NepaliDateRange[] SplitByMonth()` | `date_range::split_by_month()` | One piece per month. |
| `NepaliDateRange[] SplitByFiscalQuarter()` | `date_range::split_by_fiscal_quarter()` | One piece per fiscal quarter. |
| `IEnumerable<NepaliDate> DatesWithInterval(int)` | `date_range::dates_with_interval(int)` | Returns `result<date_interval_view>`. |
| `IEnumerable<NepaliDate> WorkingDays(bool)` | `date_range::working_days(bool)` | Returns a `day_filter_view`. |
| `IEnumerable<NepaliDate> WeekendDays(bool)` | `date_range::weekend_days(bool)` | Returns a `day_filter_view`. |
| `IEnumerator<NepaliDate> GetEnumerator()` | `date_range::begin()`, `date_range::end()` | Random-access iteration. |
| `override string ToString()` | `to_string(date_range)` | `"start - end"`. |
| `string ToString(DateFormats, Separators)` | `to_chars_range(...)`, `format_range(...)` | Both ends with the same options. |
| `Equals`, `GetHashCode`, `==`, `!=` | `operator==`, `std::hash<date_range>` | Value equality of the bounds. |

`range_set` is a small fixed container that never allocates. `date_interval_view` is a
random-access view over every `interval`-th day; the walk stops before the step that would pass the
end. `day_filter_view` is a bidirectional view. Both views are `std::ranges` views and borrowed
ranges. The two splits return `std::vector<date_range>`, and the string functions allocate their
result. The other range operations allocate nothing. A range that reaches 2199-12-30 never throws.

```cpp
#include <nepdate/nepdate.hpp>

const nepdate::date_range month =
    nepdate::date_range::of_month(2081, nepdate::month::shrawan).value();
const nepdate::date_range week =
    nepdate::date_range::make(nepdate::make_date(2081, 4, 10).value(),
                              nepdate::make_date(2081, 4, 16).value())
        .value();

for (const nepdate::date d : week) {
    // every day, in order
}
for (const nepdate::date d : week.working_days(true)) {
    // the working days
}
const auto overlap = month.intersect(week);
const auto pieces = week.split_by_month();
const auto rest = week.subtract(nepdate::date_range::single(nepdate::make_date(2081, 4, 12).value()));
const auto every_other = week.dates_with_interval(2);
```

## Fiscal year

| C# member | C++ member | Notes |
|---|---|---|
| `enum FiscalYearQuarters` | `quarter` | `q1` to `q4`; Q1 starts in Shrawan. |
| `NepaliDate FiscalYearStartDate(int)` | `fiscal_year::of(date).start()` | Returns `result<date>`. |
| `NepaliDate FiscalYearEndDate(int)` | `fiscal_year::of(date).end()` | Returns `result<date>`. |
| `(NepaliDate, NepaliDate) FiscalYearStartAndEndDate(int)` | `fiscal_year::of(date).range()` | Returns `result<date_range>`. |
| `NepaliDate FiscalYearQuarterStartDate(...)` | `fiscal_year::quarter_range(quarter)` | The range of the quarter. |
| `NepaliDate FiscalYearQuarterEndDate(...)` | `fiscal_year::quarter_range(quarter)` | Its `last()`. |
| `(NepaliDate, NepaliDate) FiscalYearQuarterStartAndEndDate(...)` | `fiscal_year::quarter_range(quarter)` | The same range. |
| `static NepaliDate GetFiscalYearStartDate(int)` | `fiscal_year{year}.start()` | Returns `result<date>`. |
| `static NepaliDate GetFiscalYearEndDate(int)` | `fiscal_year{year}.end()` | Returns `result<date>`. |
| `static (NepaliDate, NepaliDate) GetFiscalYearStartAndEndDate(int)` | `fiscal_year{year}.range()` | Returns `result<date_range>`. |
| `static NepaliDate GetFiscalYearQuarterStartDate(int, int)` | not ported | Use the fiscal-year value and a `quarter`. |
| `static NepaliDate GetFiscalYearQuarterEndDate(int, int)` | not ported | Use the fiscal-year value and a `quarter`. |
| `static (NepaliDate, NepaliDate) GetFiscalYearQuarterStartAndEndDate(int, int)` | not ported | Use the fiscal-year value and a `quarter`. |

`fiscal_year{2082}` is FY 2082/83. `of(date)` finds the containing fiscal year. `quarter_of(date)`
and `months_of(quarter)` are free functions. `label(lang)` writes `2082/83` or `२०८२/८३`.

```cpp
#include <nepdate/nepdate.hpp>

const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
const nepdate::fiscal_year fy = nepdate::fiscal_year::of(d);
const std::string label = fy.label(nepdate::lang::en); // "2081/82"
const nepdate::result<nepdate::date_range> quarter = fy.quarter_range(nepdate::quarter::q1);
```

## Errors and result

| C# member | C++ member | Notes |
|---|---|---|
| `InvalidNepaliDateFormatException` | `errc` | Validation and parse kinds. |
| (exception value) | `result<T>` | Holds a value or an `errc`. |
| (exception value) | `bad_result_access` | Thrown by `value()` on an error. |
| (standard error machinery) | `std::error_code`, `std::error_condition` | `errc` converts to `std::error_code`; `make_error_condition` makes a condition. |

`errc` names the four validation kinds (`out_of_range`, `invalid_month`, `invalid_day`,
`invalid_gregorian`) and the five parse kinds (`invalid_character`, `wrong_group_count`,
`number_too_long`, `ambiguous`, `unrecognized`). `to_string(errc)` returns the name, and
`is_parse_error(errc)` tells the two families apart.

`result<T>` follows `std::expected`: `has_value`, `value`, `error`, `operator*`, `operator->`,
`value_or` and `explicit operator bool`. A small trivially copyable `T` uses a compact union and
`result<date>` is 4 bytes. A larger or non-trivial `T`, such as `std::vector<date>`, uses optional
storage. `value()` on an error throws `bad_result_access`; a build without exceptions aborts
instead, and a constant expression that reaches it fails to compile.

```cpp
#include <nepdate/nepdate.hpp>

#include <system_error>

const nepdate::result<nepdate::date> parsed = nepdate::parse("2081/04/15");
if (!parsed) {
    const nepdate::errc error = parsed.error();
    const std::error_code code = error;
} else {
    const nepdate::date d = parsed.value();
}
```

## Patro

The patro is a separate library, `nepdate::patro`, in `<nepdate/patro.hpp>`. It depends on the core;
the core never includes it. The bundled data covers BS 2077-01-01 to 2084-08-20.

| C# member | C++ member | Notes |
|---|---|---|
| `readonly struct CalendarInfo` | `patro::day_info` | Five members in one lookup. |
| `string TithiNp { get; }` | `patro::day_info::tithi()->name(lang::ne)` | Optional. |
| `string TithiEn { get; }` | `patro::day_info::tithi()->name(lang::en)` | Optional. |
| `bool IsPublicHoliday { get; }` | `patro::day_info::is_public_holiday()` | The gazetted flag. |
| `string[] EventsNp { get; }` | `patro::day_info::events()` plus `name(lang::ne)` | File order. |
| `string[] EventsEn { get; }` | `patro::day_info::events()` plus `name(lang::en)` | File order. |
| `CalendarInfo GetCalendarInfo()` | `patro::info(date)` | Never fails; empty outside coverage. |

```cpp
#include <nepdate/patro.hpp>

const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
const nepdate::patro::day_info info = nepdate::patro::info(d);
const bool holiday = info.is_public_holiday();
for (const nepdate::patro::event e : info.events()) {
    const std::string_view name = e.name(nepdate::lang::en);
}
```

## Notes

- The core's fixed-size paths allocate nothing: `to_chars_*`, `to_string(date)` (small-string
  buffer), parsing, arithmetic, conversion and every range set operation except the two splits.
  `to_nepali_dates` and `to_english_dates` return a vector and allocate that vector. The patro data
  is static and view-only.
- `operator<<` lives in `<nepdate/ostream.hpp>` so a translation unit that only formats into a
  buffer does not include `<ostream>`. `<nepdate/nepdate.hpp>` includes it.
- `std::formatter<date>` and `std::formatter<date_range>` are available where the standard library
  has `<format>`. `{}` prints the default form and `{:<pattern>}` the pattern form, with
  `lang::en`.
- The clock members take a fixed UTC offset, with `nepal_utc_offset` (+05:45) as the default. They
  also take an injected clock with a `now()` member, so a test can fix the instant.
