# Changelog

All notable changes to `nepdate` and `nepdate-patro` are recorded here. Both libraries share one
version (ADR-0005). The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-10-15

### Added

- `nepdate::date`: a 4-byte Bikram Sambat date for BS 1901-01-01 to 2199-12-30, with `year`,
  `month`, `day`, `weekday`, `day_of_year`, `month_length`, `year_length`, `first_of_month`,
  `last_of_month`, `min`, `max`, `==`, `<=>` and `std::hash`.
- `make_date`, `from_gregorian`, `from_sys_days`, `from_local_days`, `to_year_month_day` and
  `to_sys_days`: BS to AD and back, `constexpr` and without allocation.
- `errc`, `to_string(errc)`, `is_parse_error` and `result<T>`; `result<date>` is 4 bytes.
- `month` and `lang` enumerations.
- `date::add_days`, `next`, `prev` and `days_until` (D-03, D-07).
- `date::add_months` and `add_years` with `overflow::clamp` or `overflow::spill` (D-02).
- `date::diff` and `date_diff`: years, months and days between two dates (ALGORITHM §8a).
- `date_range` (`<nepdate/date_range.hpp>`): an inclusive, random-access view of dates with
  `make`, `of_month`, `of_year`, `size` and `contains` (D-09).
- `fiscal_year`, `quarter`, `quarter_of` and `months_of` (`<nepdate/fiscal_year.hpp>`): start,
  end, range, quarter ranges and the label `2082/83` or `२०८२/८३` (D-08).
- Formatting (`<nepdate/format.hpp>`): `to_chars`, `to_chars_iso`, `to_chars_short`,
  `to_chars_long`, `to_chars_pattern` and `to_chars_number` write into the caller's buffer; the
  matching `std::string` functions are `to_string`, `to_iso_string`, `format_short`,
  `format_long` and `format_pattern`. Names come from `month_name`, `month_short_name` (D-04),
  `weekday_name` and `weekday_short_name`. `std::formatter<nepdate::date>` prints `{}` as
  `YYYY/MM/DD` and `{:<pattern>}` through FORMATTING §5, behind the one `__cpp_lib_format` guard.
- Parsing (`<nepdate/parse.hpp>`): `parse` (strict) and `parse_lenient`, both `constexpr` over
  `std::string_view`, plus the `char8_t` overloads for `u8""` literals.
- Clock and time zone interop (`<nepdate/date.hpp>`): `nepal_utc_offset`, `from_sys_time` and
  `today`, which default to Nepal's fixed +05:45 offset.
- `nepdate::patro` (`<nepdate/patro.hpp>`): `info`, `coverage`, `public_holidays`, `tithi`,
  `event` and `day_info` for the tithi, holidays and events of BS 2077-01-01 to 2084-08-20.
- vcpkg overlay ports (`ports/nepdate`, `ports/nepdate-patro`) and Conan recipes
  (`conan/nepdate`, `conan/nepdate-patro`) for the two packages.
