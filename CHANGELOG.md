# Changelog

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
