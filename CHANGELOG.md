# Changelog

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- The full NepDate API. Dates gain `to_tuple`, `to_ymd_int`, `is_today`, `is_yesterday` and
  `is_tomorrow` with an explicit zone and an injectable clock. Conversion gains
  `from_ymd_int`, the ordered `to_nepali_dates` and `to_english_dates` batch functions, and
  `to_iso_string` with `parse`.
- Ranges gain `single`, `from_day_count`, `current_month`, `current_fiscal_year`,
  `current_calendar_year`, containment of a range, overlap, adjacency, intersection, union,
  `subtract`, the month and fiscal-quarter splits, `dates_with_interval`, `working_days`,
  `weekend_days`, `range_set`, `date_interval_view`, `day_filter_view`, `to_string`, the
  `to_chars_range` and `format_range` pair, and `std::hash<date_range>`.
- `std::formatter<date_range>`, `operator<<` for `date` and `date_range`, and a `std::error_code`
  and `std::error_condition` mapping for `errc`.
- `std::optional<date>` forms `try_parse` and `try_parse_lenient`, and `u8` overloads for them.

### Changed

- `result<T>` also works for a value that is not small and trivially copyable, such as
  `std::vector<date>`, while `result<date>` stays 4 bytes.
- A trailing backslash in a custom format pattern now writes a literal backslash instead of
  writing nothing.
- `result<T>::value()` now throws on an error instead of calling `std::abort()`. A build without
  exceptions still aborts, and a constant expression that reaches the error path still fails to
  compile.

### Breaking changes

- A format pattern that ends in a backslash now writes a backslash. Migration: remove the trailing
  `\` when the old output without it was intended.
- `result<T>::value()` on an error throws `nepdate::bad_result_access`. Migration: check
  `has_value()` or catch `nepdate::bad_result_access` before calling `value()`.

## [0.1.0-rc.1] - 2026-11-03

### Fixed

- The vcpkg ports and Conan recipes now build the release archive outside CI: the ports carry the
  archive's SHA512 instead of `0`, and the recipes download it through a new `conandata.yml`. Before,
  the README's overlay command failed without the CI-only `NEPDATE_CPP_SOURCE`.

### Added

- `verify-published.yml` also runs weekly against the newest release and daily against the release
  metadata, and reports a failure as one issue labelled `published-check`.
  `scripts/verify-published.sh` is the weekly path; the release path still checks both build forms.
- Both vcpkg ports install a `usage` file; both Conan recipes reject C++ standards before 20.
- `NEPDATE_BUILD_COREONLY` (on when top level, as before) so the port and recipe skip the gate 9
  program.

## [0.1.0-beta.1] - 2026-10-22

First public pre-release. The API may still change before 0.1.0.

Two packages from one tag: header-only `nepdate` and the compiled `nepdate-patro`. The public
surface is in `docs/API.md`.

### Added

- BS and AD conversion. `date` is a 4-byte Bikram Sambat date; `make_date`, `from_gregorian`,
  `from_sys_days` and the `to_*` members convert both ways. Conversion is `constexpr` and
  allocates nothing. All 109,212 days from BS 1901 to 2199 round-trip BS to AD to BS in the
  tests, and the shared conversion vectors pass.
- Arithmetic and fiscal year. `add_days`, `add_months`, `add_years`, `diff`, `date_range` and
  `fiscal_year`, with the shared clamp and spill rules.
- Formatting and parsing. `to_chars_*` write into the caller's buffer and allocate nothing;
  `parse` and `parse_lenient` are `constexpr` over `std::string_view` and reject rather than
  guess.
- The patro. `nepdate::patro` provides `info`, `coverage`, `public_holidays`, `tithi` and `event`
  for tithi, public holidays and events over BS 2077 to 2084.
- vcpkg overlay ports and Conan recipes for both packages.
- `NEPDATE_VERSION_PRERELEASE` in `<nepdate/version.hpp>`: the semver pre-release part (`beta.1`),
  empty for a stable version. `project()` holds numbers only, so `NEPDATE_VERSION_STRING` carries
  the full version.
- C++20, built and tested on GCC 12, Clang 16 (with libstdc++ and with libc++), Apple Clang 16 and
  MSVC 19.44, each also with exceptions and RTTI disabled.

### Notes

- Benchmarks against C# NepDate are in `docs/benchmarks.md`. BS to AD is 7.09 ns on MSVC and
  5.58 ns on Clang against 13.29 ns for C# NepDate, with zero allocation. The patro month grid
  (B8) misses its target and is reported there.
