# C1: Core types and BS ↔ AD conversion

Read first: ADR-0002 (all of it), ADR-0003, `docs/API.md` (errors and result, names, the date,
construction), `shared/spec/ALGORITHM.md` §1 to §7, `shared/spec/PARITY.md`,
`shared/spec/PORT-PLAN.md` phase 1.

Precondition: HC-5 (the catalogue is in `shared/`). Without it, do every task except C1-07, and say
in the PR that C1-07 waits for hub H5.

## Tasks

### C1-01 Generated calendar tables
Extend `tools/codegen` to write, into `include/nepdate/detail/calendar_tables.hpp`, the tables of
ADR-0002 §2: `kMonthStart` (`std::uint32_t[3589]`), `kBlockMonth` (`std::uint16_t[3413]`),
`kCalendarFingerprint` (FNV-1a 64 over the 3588 month lengths as bytes, offset basis
`0xcbf29ce484222325`, prime `0x100000001b3`), plus `kEpochUnixDays = -45920`, `kMaxSerial = 109211`.
Twelve values per line with a `// 1901` comment per BS year in `kMonthStart`; 16 per line in
`kBlockMonth`. All `inline constexpr` in `namespace nepdate::detail`.
The generator fails, naming the block, if any block of `kBlockMonth` needs more than two forward
steps.
AC:
- `kMonthStart[3588] == 109212`, `kMonthStart[0] == 0`; `kMonthStart` strictly increasing.
- For every serial `s` in 0..109211, starting at `kBlockMonth[s >> 5]`, at most two steps reach the
  month that contains `s` (test loops all serials).
- `kCalendarFingerprint` equals the value a test recomputes from `kMonthStart`.
- `codegen_check` passes after regeneration.

### C1-02 `errc` and `result<T>` (`include/nepdate/date.hpp`, `detail/result.hpp`)
Per `docs/API.md` and ADR-0003 §2. Generic `result<T>`: `static_assert(std::is_trivially_copyable_v<T>)`,
storage `union { T value; }` plus `errc` (`errc{}` = has value). Partial specialisation for `date`
storing one `std::uint32_t` (ADR-0002 §1). `value()` on an error calls
`detail::bad_result_access()` (declared non-`constexpr`, `[[noreturn]]`, calls `std::abort()`).
AC:
- `sizeof(result<date>) == 4`; `result<date>` and `result<int>` are trivially copyable.
- `static_assert(!make_date(2081, 4, 33).has_value())` and
  `static_assert(make_date(2081, 4, 33).error() == errc::invalid_day)` compile.
- A file under `tests/compile_fail/` with `constexpr auto d = make_date(2081, 4, 33).value();` fails to
  compile; a CTest test with `WILL_FAIL` builds it (`try_compile` style).
- `to_string(errc::number_too_long) == "number_too_long"`; `is_parse_error` is true for exactly five
  values.

### C1-03 `month`, `lang`, `date`, `make_date`, accessors
`make_date` validates per ALGORITHM §3 with unsigned-range checks (ADR-0002 §3), in the order year,
month, day. Accessors per `docs/API.md`; `weekday()` returns `std::chrono::weekday{(serial + 4) % 7}`.
`std::hash<nepdate::date>` returns `std::hash<std::uint32_t>{}` of the packed value.
AC:
- `make_date(2081, 4, 32)` is valid; `make_date(2081, 4, 33)` is `invalid_day`;
  `make_date(2081, 13, 1)` is `invalid_month`; `make_date(1900, 13, 40)` and `make_date(2200, 1, 1)`
  are `out_of_range`; `make_date(INT_MIN, 1, 1)` and `make_date(2081, 4, INT_MAX)` don't overflow
  (UBSan clean) and return `out_of_range` and `invalid_day`.
- `date::min()` is 1901-01-01, `date::max()` is 2199-12-30; `min() < max()`.
- `year_length()` is in 364..367 for every year and equals the sum of its 12 month lengths.
- `sizeof(date) == 4`, `std::is_trivially_copyable_v<date>`, `!std::is_default_constructible_v<date>`.

### C1-04 Serial conversion (`detail/serial.hpp`, not public)
`constexpr std::uint32_t to_serial(date)` and `constexpr date from_serial(std::uint32_t)`
(precondition `s <= kMaxSerial`) exactly as ADR-0002 §3, the two block steps written as
`i += kMonthStart[i + 1] <= s;`.
AC: `to_serial(date::min()) == 0`, `to_serial(date::max()) == 109211`; for all serials,
`to_serial(from_serial(s)) == s`.

### C1-05 Gregorian (`detail/civil.hpp`) and the AD side of `date`
`days_from_civil` and `civil_from_days` with the Neri–Schneider algorithms (ADR-0002 §3; constants
`s = 82`, `K = 719468 + 146097 * 82`, `L = 400 * 82`, 32-bit unsigned arithmetic, one
`std::uint64_t` product). `from_gregorian(int, int, int)`: Gregorian validity first
(`invalid_gregorian`, ALGORITHM §5), then reject a year outside 1844..2143 before any arithmetic
(no overflow for huge `int`s), then the serial range (`out_of_range`). Also
`from_gregorian(year_month_day)` (`!ymd.ok()` is `invalid_gregorian`), `from_sys_days`,
`from_local_days`, `to_year_month_day`, `to_sys_days`.
AC:
- AD 1844-04-11 is BS 1901-01-01 and AD 2143-04-15 is BS 2199-12-30.
- AD 1844-04-10 and 2143-04-16 are `out_of_range`; AD 2023-02-29 and 2023-02-30 are
  `invalid_gregorian`; AD 2024-02-29 converts.
- The five rows of ALGORITHM §9 match, including weekdays.
- `from_gregorian(INT_MAX, 1, 1)` is `out_of_range` without UB.

### C1-06 Vector and exhaustive tests (`tests/`)
- `tests/vectors.hpp`: a small TSV/CSV reader (`std::ifstream`, test code may allocate) taking the
  path from `NEPDATE_SHARED_DIR`.
- `month-boundaries.csv` (7176 rows: `bs`, `ad`, `weekday`, `month_length`) and
  `csharp-golden/dates.tsv` (1018 rows) columns `ad`, `weekday`, `day_of_year`, `month_length`.
  Each test fails if the file has fewer rows than stated.
- Exhaustive: every serial 0..109211 round-trips BS → serial → AD → BS, and `civil_from_days`
  agrees with a Hinnant reference typed into `tests/hinnant_reference.hpp` line by line from
  ALGORITHM §5.
- Spec cross-check: build `LEN[y][m]` from the `month_length` column of `month-boundaries.csv`
  (independent of the generated tables), compute `PACKED` and `YEAR_START` from it as ALGORITHM §2
  defines them, and check every `date::month_length()`, every year start
  (`kMonthStart[12 * (y - 1901)]`) and `YEAR_START[299] == 109212`.
AC: all of the above pass on every CI job; the exhaustive test takes under 1 s in Release (report the
Debug time in the PR).

### C1-07 C# catalogue, phase 1
`tools/check_catalog.cmake` (ADR-0004 §3) and the CTest test `catalog_coverage` with
`NEPDATE_CATALOG_PHASE=1`. Implement every catalogue row with `phase` 1 as a `TEST_CASE("<id>",
"[cs]")` in `tests/cs_phase1.cpp`, or a `// cs-deviation:` / `// cs-n/a:` line.
AC: `catalog_coverage` passes; removing one phase-1 test makes it fail and print that `id`.

### C1-08 Compile-time and allocation tests
- `tests/constexpr_tests.cpp`: `STATIC_REQUIRE` for ALGORITHM §9 (BS ↔ AD both ways, weekday), the
  C1-02 and C1-03 size and trait checks, and one `make_date(...).value()` in a `constexpr` variable.
- `tests/alloc_tests.cpp` + `tests/alloc_counter.cpp` (separate executable, ADR-0004 §3): 0
  allocations over 1000 runs each of `make_date`, `from_gregorian`, `to_year_month_day`, `weekday`,
  `day_of_year`, comparison and `std::hash`.
AC: both pass in the `dev` and `noexcept` presets.

### C1-09 First benchmark of B1 and B2
Create `bench/` as a separate CMake project (ADR-0004 §5) with Google Benchmark, the 64 dates of
BENCHMARK §2 generated in code from serials, and B1 and B2 for this library only, each consuming
year, month and day of the result with `benchmark::DoNotOptimize`. Add a bench-only copy of the
"AD date of each BS month start" variant (ADR-0002 §8) for B1. Run on the reference machine (HC-2)
with MSVC and Clang, pinned to CPU 2 (BENCHMARK §1).
AC: the PR has a table of B1, B2 (both compilers) and the B1 variant, with mean and standard
deviation, next to C# 13.29 and 18.64 ns. If the variant is at least 10 % faster on B1 with both
compilers, the PR says so and the human decides whether to adopt it (new task). If HC-2 isn't done,
the PR says "not measured" instead.

## Checkpoint (stop here for review)

The reviewer checks:
- [ ] Put ADR-0002 §3 next to `detail/serial.hpp`: does the code match line for line, including the
      branch-free block steps?
- [ ] Put `tests/hinnant_reference.hpp` next to ALGORITHM §5: one to one?
- [ ] `sizeof(date) == 4` and `sizeof(result<date>) == 4` are `static_assert`ed.
- [ ] Every `int` parameter is range-checked before arithmetic (search for `y - 1901` and check
      the cast to unsigned comes first).
- [ ] `catalog_coverage` passes and its phase limit is 1.
- [ ] Allocation test passes with exceptions on and off.
- [ ] C1-09 numbers: measured on which machine and compiler, and how they compare with ADR-0002 §9.
- [ ] Milestone M1 reached on every CI job.
