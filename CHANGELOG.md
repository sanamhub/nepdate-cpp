# Changelog

All notable changes to `nepdate` and `nepdate-patro` are recorded here. Both libraries share one
version (ADR-0005). The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
