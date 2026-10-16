# `nepdate` (C++): public API contract (v0.1)

The complete public surface for v0.1. Implement exactly this; adding a public name needs this file
changed in the same PR, approved by the maintainer. Behaviour: `shared/spec/` (`ALGORITHM.md`,
`FORMATTING.md`, `PARSING.md`, `PATRO.md`, `PARITY.md`). Decisions: ADR-0002 (representation),
ADR-0003 (conventions and errors).

Rules that apply to every declaration below:

- Every function is `noexcept` except the ones returning `std::string` (they can only fail by
  running out of memory) and `result<T>::value()`.
- Every function marked `constexpr` must also work in a constant expression; tests prove it with
  `static_assert` (ADR-0004).
- Anything in `namespace nepdate::detail` or `nepdate::patro::detail` is not API, even when it is
  visible in a header.
- `[[nodiscard]]` on every function that returns a value.

## Core: target `nepdate::nepdate`, header-only

Headers: `<nepdate/nepdate.hpp>` includes all of the core. The pieces are `<nepdate/date.hpp>`,
`<nepdate/format.hpp>`, `<nepdate/parse.hpp>`, `<nepdate/fiscal_year.hpp>`,
`<nepdate/date_range.hpp>` and `<nepdate/version.hpp>`.

```cpp
// ---------- <nepdate/version.hpp> ----------
#define NEPDATE_VERSION_MAJOR 0
#define NEPDATE_VERSION_MINOR 1
#define NEPDATE_VERSION_PATCH 0
#define NEPDATE_VERSION_STRING "0.1.0"

namespace nepdate {

// ---------- errors and result: <nepdate/date.hpp> ----------
enum class errc : std::uint8_t {
    out_of_range = 1,      // outside BS 1901-01-01..2199-12-30 or AD 1844-04-11..2143-04-15
    invalid_month,         // month not 1..12
    invalid_day,           // day not 1..month length
    invalid_gregorian,     // not a real proleptic Gregorian date
    invalid_character,     // PARSING §4, the five parse kinds
    wrong_group_count,
    number_too_long,
    ambiguous,
    unrecognized,
};
constexpr std::string_view to_string(errc e) noexcept;   // "out_of_range", ...; "" for other values
constexpr bool is_parse_error(errc e) noexcept;          // true for the five parse kinds

// Holds a T or an errc, never both (ADR-0003). T is trivially copyable.
// Names follow std::expected so code moves to it easily later.
template <class T>
class [[nodiscard]] result {
public:
    constexpr result(T value) noexcept;                   // implicit, so `return d;` works
    constexpr result(errc error) noexcept;                // implicit, so `return errc::x;` works
    constexpr bool has_value() const noexcept;
    constexpr explicit operator bool() const noexcept;
    constexpr const T& value() const&;                    // on an error: std::abort() at run time,
                                                          // a compile error in a constant expression
    constexpr T value() &&;                               // by copy, so a range-for over a
    constexpr T value() const&&;                          // temporary's value is safe (ADR-0006)
    constexpr const T& operator*() const& noexcept;       // precondition: has_value()
    constexpr T operator*() && noexcept;                  // by copy (ADR-0006)
    constexpr const T* operator->() const noexcept;       // precondition: has_value()
    constexpr T value_or(T fallback) const noexcept;
    constexpr errc error() const noexcept;                // precondition: !has_value()
    friend constexpr bool operator==(const result&, const result&) noexcept = default;
};
// result<date> is 4 bytes (ADR-0002 §2); other result<T> are T plus one byte. T is at most 16
// bytes (ADR-0006).

// ---------- names ----------
enum class lang : std::uint8_t { en, ne };
enum class month : std::uint8_t {
    baishakh = 1, jestha, ashad, shrawan, bhadra, ashoj,
    kartik, mangsir, poush, magh, falgun, chaitra,
};

// ---------- the date ----------
// A valid Bikram Sambat date. 4 bytes, trivially copyable, no default constructor (D-01).
// Ordered chronologically by <=>.
class date {
public:
    date() = delete;
    static constexpr date min() noexcept;                 // 1901-01-01
    static constexpr date max() noexcept;                 // 2199-12-30

    constexpr int year() const noexcept;                  // 1901..2199
    constexpr nepdate::month month() const noexcept;
    constexpr int day() const noexcept;                   // 1..32
    constexpr std::chrono::weekday weekday() const noexcept;   // ALGORITHM §7
    constexpr int day_of_year() const noexcept;           // 1..367
    constexpr int month_length() const noexcept;          // 29..32
    constexpr int year_length() const noexcept;           // 364..367 (D-12)
    constexpr date first_of_month() const noexcept;
    constexpr date last_of_month() const noexcept;

    constexpr std::chrono::year_month_day to_year_month_day() const noexcept;  // AD
    constexpr std::chrono::sys_days to_sys_days() const noexcept;               // AD, midnight UTC

    constexpr result<date> add_days(int n) const noexcept;                       // out_of_range
    constexpr result<date> add_months(int n, overflow o = overflow::clamp) const noexcept;
    constexpr result<date> add_years(int n, overflow o = overflow::clamp) const noexcept;
    constexpr int days_until(date other) const noexcept;  // other - *this, signed (D-07)
    constexpr date_diff diff(date other) const noexcept;  // ALGORITHM §8a
    constexpr std::optional<date> next() const noexcept;  // nullopt at max()
    constexpr std::optional<date> prev() const noexcept;  // nullopt at min()

    friend constexpr bool operator==(date, date) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(date, date) noexcept = default;
};

enum class overflow : std::uint8_t { clamp, spill };      // ALGORITHM §8: clamp = C# default,
                                                          // spill = C# awayFromMonthEnd: true
struct date_diff {                                        // ALGORITHM §8a
    bool negative;                                        // other is before *this
    int years, months, days;                              // magnitudes
    int total_days;                                       // signed, equals days_until(other)
    friend constexpr bool operator==(const date_diff&, const date_diff&) noexcept = default;
};

// ---------- construction ----------
constexpr result<date> make_date(int year, int month, int day) noexcept;
constexpr result<date> make_date(int year, nepdate::month m, int day) noexcept;
    // out_of_range, invalid_month, invalid_day, in that order (ALGORITHM §3)
constexpr result<date> from_gregorian(int year, int month, int day) noexcept;
    // invalid_gregorian, then out_of_range (ALGORITHM §3)
constexpr result<date> from_gregorian(std::chrono::year_month_day ymd) noexcept;
constexpr result<date> from_sys_days(std::chrono::sys_days d) noexcept;      // out_of_range
constexpr result<date> from_local_days(std::chrono::local_days d) noexcept;  // out_of_range

// Today's BS date for a clock reading and a UTC offset; the default is Asia/Kathmandu, whose
// fixed offset is +05:45 (see the note at the end of this file).
inline constexpr std::chrono::minutes nepal_utc_offset{345};                 // +05:45
constexpr result<date> from_sys_time(std::chrono::sys_seconds t,
                                     std::chrono::minutes utc_offset) noexcept;
result<date> today(std::chrono::minutes utc_offset = nepal_utc_offset) noexcept;
    // from_sys_time(floor<seconds>(system_clock::now()), utc_offset); out_of_range when the clock
    // is outside AD 1844-04-11..2143-04-15

// ---------- fiscal year: <nepdate/fiscal_year.hpp> ----------
enum class quarter : std::uint8_t { q1 = 1, q2, q3, q4 };  // Q1 = Shrawan..Ashoj, Q4 = Baishakh..Ashad
constexpr quarter quarter_of(date d) noexcept;
constexpr std::array<month, 3> months_of(quarter q) noexcept;

class fiscal_year {                                       // ALGORITHM §8b, D-08
public:
    constexpr explicit fiscal_year(int start_year) noexcept;  // FY 2082/83 is fiscal_year{2082}
    static constexpr fiscal_year of(date d) noexcept;
    constexpr int start_year() const noexcept;
    constexpr result<date> start() const noexcept;        // out_of_range
    constexpr result<date> end() const noexcept;          // out_of_range
    constexpr result<date_range> range() const noexcept;
    constexpr result<date_range> quarter_range(quarter q) const noexcept;
    constexpr std::to_chars_result to_chars_label(char* first, char* last, lang l) const noexcept;
        // "2082/83" or "२०८२/८३"
    std::string label(lang l) const;
    friend constexpr bool operator==(fiscal_year, fiscal_year) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(fiscal_year, fiscal_year) noexcept = default;
};

// ---------- range: <nepdate/date_range.hpp> ----------
// Inclusive range first..last, first <= last. A random-access view of date (D-09: no set operations).
class date_range : public std::ranges::view_interface<date_range> {
public:
    class iterator;                                       // random access, value_type date,
                                                          // holds a serial, default constructible
    static constexpr std::optional<date_range> make(date first, date last) noexcept;  // nullopt if first > last
    static constexpr result<date_range> of_month(int year, month m) noexcept;         // out_of_range, invalid_month
    static constexpr result<date_range> of_year(int year) noexcept;                   // out_of_range
    constexpr date first() const noexcept;
    constexpr date last() const noexcept;
    constexpr std::size_t size() const noexcept;          // number of days, >= 1
    constexpr bool contains(date d) const noexcept;
    constexpr iterator begin() const noexcept;
    constexpr iterator end() const noexcept;
    friend constexpr bool operator==(date_range, date_range) noexcept = default;
};

// ---------- formatting: <nepdate/format.hpp> ----------
// Every to_chars_* writes into [first, last) without allocating and returns std::to_chars_result
// (<charconv>): {end of output, errc{}} on success, {last, std::errc::value_too_large} when the
// buffer is too small (buffer contents then unspecified). Nothing is null-terminated.
enum class date_order : std::uint8_t { ymd, ydm, myd, mdy, dym, dmy };
enum class separator : std::uint8_t { slash, backslash, dot, underscore, dash, space };
struct short_format { date_order order = date_order::ymd; separator sep = separator::slash; bool pad = true; };
struct long_format  { bool weekday = false; bool year = true; bool pad = true; };   // defaults = C#

inline constexpr std::size_t max_short_size = 30;         // bytes, any short_format, lang::ne
inline constexpr std::size_t max_long_size  = 80;         // bytes, any long_format, lang::ne

constexpr std::to_chars_result to_chars(char* first, char* last, date d) noexcept;          // "2081/04/15"
constexpr std::to_chars_result to_chars_iso(char* first, char* last, date d) noexcept;      // "2081-04-15" (D-11)
constexpr std::to_chars_result to_chars_short(char* first, char* last, date d,
                                              short_format f, lang l) noexcept;             // FORMATTING §3
constexpr std::to_chars_result to_chars_long(char* first, char* last, date d,
                                             long_format f, lang l) noexcept;               // FORMATTING §4
constexpr std::to_chars_result to_chars_pattern(char* first, char* last, date d,
                                                std::string_view pattern, lang l) noexcept; // FORMATTING §5
constexpr std::to_chars_result to_chars_number(char* first, char* last, unsigned value,
                                               int min_width, lang l) noexcept;             // ASCII or Devanagari digits

// The same, allocating exactly the returned string.
std::string to_string(date d);                            // "2081/04/15"
std::string to_iso_string(date d);                       // "2081-04-15"
std::string format_short(date d, short_format f, lang l);
std::string format_long(date d, long_format f, lang l);
std::string format_pattern(date d, std::string_view pattern, lang l);

constexpr std::string_view month_name(month m, lang l) noexcept;        // FORMATTING §1; "" outside 1..12
constexpr std::string_view month_short_name(month m, lang l) noexcept;  // D-04: "Asa", "Aso"
constexpr std::string_view weekday_name(std::chrono::weekday w, lang l) noexcept;
constexpr std::string_view weekday_short_name(std::chrono::weekday w, lang l) noexcept;
    // weekday names: "" when !w.ok()

// ---------- parsing: <nepdate/parse.hpp> ----------
constexpr result<date> parse(std::string_view s) noexcept;              // strict, PARSING §1
constexpr result<date> parse_lenient(std::string_view s) noexcept;      // PARSING §2
result<date> parse(std::u8string_view s) noexcept;                      // same bytes, for u8"" literals
result<date> parse_lenient(std::u8string_view s) noexcept;

} // namespace nepdate

// ---------- standard library hooks ----------
template <> struct std::hash<nepdate::date>;              // hashes the 4-byte value

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
// {} prints "YYYY/MM/DD". {:<pattern>} uses FORMATTING §5 with lang::en; the pattern can't
// contain '{' or '}'. Nepali output goes through the to_chars_* functions.
template <> struct std::formatter<nepdate::date, char>;
#endif
```

## Patro: target `nepdate::patro`, compiled library

Header `<nepdate/patro.hpp>`. It includes the core; the core never includes it.

```cpp
namespace nepdate::patro {

// A tithi (lunar day) name; 1 byte, an index into a static table.
class tithi {
public:
    std::string_view name(lang l) const noexcept;         // PATRO §2
    friend bool operator==(tithi, tithi) noexcept = default;
};

// One event name; 2 bytes, an index into a static table.
class event {
public:
    std::string_view name(lang l) const noexcept;
    friend bool operator==(event, event) noexcept = default;
};

// What the patro says about one day. Trivially copyable; views static data, never owns.
class day_info {
public:
    std::optional<patro::tithi> tithi() const noexcept;
    bool is_public_holiday() const noexcept;
    std::span<const patro::event> events() const noexcept;   // file order; empty when none
};

day_info info(date d) noexcept;                           // never fails; empty outside coverage
date_range coverage() noexcept;                           // 2077-01-01..2084-08-20 today
std::span<const date> public_holidays() noexcept;         // ascending, 294 dates today

} // namespace nepdate::patro
```

`tithi` and `event` have a constructor taking a `nepdate::patro::detail` tag, used only by the
generated tables. It is not API.

## Notes

- The listing groups names by topic. In the real headers, `overflow`, `date_diff` and a forward
  declaration of `date_range` come before `date`, because `date`'s members use them.
- `max_short_size` and `max_long_size` are upper bounds with margin: outputs are at most 26 bytes
  (padded Nepali short) and 68 bytes (Nepali long with weekday: a 24-byte weekday, a 21-byte month,
  6 bytes of day, 12 of year, 5 of punctuation). A test formats every
  date of 2081 with every option and checks the bound.
- `spill` is the neutral name for the spec's "Overflow" mode; `overflow` is the type, so the
  enumerator can't share the name (same as the Go port).
- `weekday()` reuses `std::chrono::weekday`, which counts from Sunday = 0 in `c_encoding()`, like the
  spec. AD values are `std::chrono::year_month_day` and `sys_days`, so `<chrono>` arithmetic and
  formatting work on them directly.
- `from_sys_days` treats the day count as a calendar date. `from_sys_time(t, offset)` adds the offset
  first, so 2024-07-30 18:30 UTC is BS 2081-04-15 with offset 0 and 2081-04-16 with
  `nepal_utc_offset`. Nepal has used +05:45 since 1986-01-01; for older instants or other zones, pass
  the offset (for example from `std::chrono::time_zone::get_info(t).offset` where the standard
  library has the time-zone database). The doc comment on `today` must say this.
- No `operator<<` and no `std::error_code` in v0.1 (ADR-0003). Both can be added later without
  breaking anything.
