# ADR-0003: API conventions, errors and parity mapping

- Status: Accepted
- Date: 2026-10-08
- Deciders: Sanam Pakuwal

## Context

The library must work in code built without exceptions or RTTI (games, embedded, many Windows
codebases), must not allocate (ADR-0002), and is header-only C++20 (ADR-0001), so it can't use
`std::expected` (C++23). The full surface is in `docs/API.md`.

## Decision

### 1. Conventions

- Standard-library style names: `snake_case` everywhere, scoped enums (`nepdate::month::shrawan`),
  free functions for construction (`make_date`, `from_gregorian`, `parse`), members for queries.
- Every function that can fail returns `result<T>`; nothing else in the API fails. Every function is
  `noexcept` except `result<T>::value()`, the functions that return `std::string` or
  `std::vector`, and the stream operators.
- Reuse standard types where the meaning is identical: `std::chrono::weekday`,
  `std::chrono::year_month_day`, `sys_days`, `local_days`, `sys_seconds`, `std::to_chars_result`,
  `std::span`, `std::string_view`, `std::optional`, `std::ranges::view_interface`.
- Allocation is visible in the signature. The fixed-size paths return no owning container: every
  `to_chars_*` writes into a caller buffer, and `to_string` returns one small string. The batch
  conversions and the two range splits return a `std::vector`, and the range set operations return
  a value, `std::optional` or the fixed `range_set`.
- `constexpr` on everything in the core except `today()` (reads the clock), the `std::string` and
  `std::vector` functions (C++20 `constexpr` container support is uneven across standard libraries)
  and the `std::u8string_view` parse overloads (they need a `reinterpret_cast`).
- No default constructor for `date` (D-01). Containers that need a placeholder use
  `std::optional<date>`.
- `today()` takes a UTC offset defaulting to Nepal's +05:45, because a server in UTC is 5 h 45 min
  behind Nepal and C++20's time-zone database (`std::chrono::locate_zone`) isn't available in libc++
  or Apple's toolchain. Callers with a database pass the offset from it (`docs/API.md` notes). Each
  clock function also takes an injected clock with a `now()` member, so a test can fix the instant.
- `operator<<` for `date` and `date_range` lives in `<nepdate/ostream.hpp>`, separate from
  `format.hpp`, so a translation unit that only formats into a buffer does not include `<ostream>`.
  `std::formatter<date>` and `std::formatter<date_range>` cover the formatting library. `errc` maps
  to `std::error_code` through `make_error_code`, and to `std::error_condition` through
  `make_error_condition`.

### 2. Errors: `nepdate::errc` and `nepdate::result<T>`

One flat `enum class errc : std::uint8_t` with the four validation kinds of ALGORITHM §3 and the
five parse kinds of PARSING §4 (`is_parse_error(e)` tells them apart). A parse that succeeds
syntactically but fails validation returns the same `out_of_range`, `invalid_month` or
`invalid_day` as `make_date`, as PARSING §1 step 4 requires.

`result<T>` holds a `T` or an `errc`:

- Member names follow `std::expected` (`has_value`, `value`, `error`, `operator*`, `operator->`,
  `value_or`, `explicit operator bool`), so moving to `std::expected<T, errc>` later is mostly a type
  alias. No monadic members (`and_then`, `transform`) in v0.1 (YAGNI).
- A small trivially copyable `T` uses a union of `T` and `errc` plus a flag. `result<date>` is a
  4-byte specialization (ADR-0002 §1). A larger or non-trivial `T`, such as
  `std::vector<date>`, uses `std::optional<T>` and a separate error.
- `value()` on an error throws `bad_result_access`. A build without exceptions calls
  `std::abort()` instead, so the core still works with `-fno-exceptions`. In a constant expression
  the error path is ill-formed, so `constexpr auto d = make_date(2081, 4, 33).value();` fails to
  compile. The `noexcept` specification does not depend on the exception setting, so the ODR holds.
- `operator*`, `operator->` and `error()` have preconditions, checked with `assert` in debug builds.

Why not the alternatives:

| Option | Rejected because |
|---|---|
| `std::expected<T, errc>` | C++23 only; MSVC has no stable C++23 mode (ADR-0001 §1) |
| exceptions instead of `result<T>` | must work with `-fno-exceptions`; throwing costs far more than a conversion |
| `bool f(args, T& out)` or `errc f(args, T& out)` | needs a default-constructible `T` (D-01 forbids one for `date`), can't be `constexpr`-initialised in one expression |
| a `std::expected` alias when available, our type otherwise | different types in C++20 and C++23 translation units of one program: an ODR violation |
| returning `std::error_code` instead of `result<T>` | the category singleton and `std::string` messages are not `constexpr`, and a value and an error would need an out parameter for `date` (D-01); `errc` now converts to `std::error_code` for callers that want it |

### 3. Parity mapping (`shared/spec/PARITY.md`, applied unchanged)

| ID | C++ |
|---|---|
| D-01 | `date` has no default constructor and no invalid value; `result<date>` and `std::optional<date>` stand for "no date" |
| D-02, D-03 | `add_months(int, overflow)`, `add_years(int, overflow)`, `add_days(int)`: integers only |
| D-04 | `month_short_name` returns `Asa` and `Aso`; pattern `MMM` follows it |
| D-05, D-06 | `parse` (strict, ASCII and Devanagari digits, at most 4 per group), `parse_lenient` (closed rules) |
| D-07 | `date::days_until(other)` returns `int` |
| D-08 | `fiscal_year` type with `of`, `start`, `end`, `range`, `quarter_range`, `label` |
| D-09 | `date_range` with `make`, `of_month`, `of_year`, `size`, `contains`, iteration; no set operations |
| D-10 | not ported |
| D-11 | `to_chars_iso` / `to_iso_string` (`YYYY-MM-DD`) and `parse` read it back; no serializer integrations |
| D-12 | `date::year_length()` and `date::month_length()`; no `is_leap_year` |
| D-13 | separate library `nepdate::patro` (`nepdate-patro`), namespace `nepdate::patro` |
| D-14 | not ported; compare with `today(offset)` and `add_days(±1)` |
| D-15 | `lang` parameter on every formatter; `G`, `g`, `d`, `D`, `s` in `to_chars_pattern` |

A C++-only deviation is not allowed; propose it to the hub.
