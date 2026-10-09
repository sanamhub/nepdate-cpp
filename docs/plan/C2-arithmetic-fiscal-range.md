# C2: Arithmetic, diff, fiscal year, range

Read first: `shared/spec/ALGORITHM.md` §8 to §8b, `docs/API.md` (date arithmetic, fiscal year,
range), ADR-0002 §3, `shared/spec/PARITY.md` D-02, D-03, D-07, D-08, D-09, D-12,
`shared/spec/PORT-PLAN.md` phase 2.

Golden files are in `shared/spec/vectors/csharp-golden/`; `ERR` in a cell means the call returns
`errc::out_of_range`. `add-days.tsv` and `add-months.tsv` include negative amounts.

## Tasks

### C2-01 `add_days`, `next`, `prev`, `days_until`
Through the serial. `add_days` returns `out_of_range` when `n < -109211` or `n > 109211` before
adding, so no `int` overflow. `next`/`prev` return `std::nullopt` at the ends.
AC:
- Golden `add-days.tsv` (9162 rows) passes.
- `date::max().add_days(1)` and `date::min().add_days(-1)` are `out_of_range`;
  `add_days(INT_MAX)` and `add_days(INT_MIN)` are `out_of_range` (UBSan clean).
- `date::min().days_until(date::max()) == 109211` and the reverse is `-109211`.
- Property (Catch2 generator, fixed seed, 10,000 pairs): if `d.add_days(n)` succeeds, adding `-n`
  gives `d` back, and `d.days_until(*d.add_days(n)) == n`.

### C2-02 `add_months`, `add_years` with `clamp` / `spill`
ALGORITHM §8 on the month index `i = (y-1901)*12 + (m-1)`, length from `kMonthStart`.
`add_years` returns `out_of_range` when `|n| > 299` before multiplying by 12; `add_months` when
`|n| > 3588`.
AC:
- Golden `add-months.tsv` (10180 rows): column `clamp` with `overflow::clamp`, column `overflow`
  with `overflow::spill`.
- The §8 examples: 2081-04-32 + 2 → clamp 2081-06-30, spill 2081-07-02; + 5 → clamp 2081-09-29,
  spill 2081-10-03.
- `add_years(INT_MAX)` and `add_months(INT_MIN)` are `out_of_range`.

### C2-03 `diff` (ALGORITHM §8a)
`years`, `months`, `days` are magnitudes; `negative` and `total_days` carry the sign. Start from
`n0 = (b.y - a.y) * 12 + (b.m - a.m)` and step down at most once (the §8a hint). No loop over days.
AC:
- The six rows of the ALGORITHM §8a table.
- These extra rows, computed with the §8a pseudocode from `shared/data/`:

  | a | b | negative | years | months | days | total_days |
  |---|---|---|---|---|---|---|
  | 2081-04-32 | 2081-06-30 | false | 0 | 2 | 0 | 61 |
  | 2081-01-01 | 2082-01-01 | false | 1 | 0 | 0 | 366 |
  | 2081-04-15 | 2083-02-10 | false | 1 | 9 | 26 | 663 |
  | 2083-02-10 | 2081-04-15 | true | 1 | 9 | 26 | -663 |

- `a.diff(b).total_days == a.days_until(b)` for 10,000 random pairs from a fixed seed.

### C2-04 `fiscal_year`, `quarter` (ALGORITHM §8b)
`fiscal_year::of`, `start`, `end`, `range`, `quarter_range`, `to_chars_label`, `label`,
`quarter_of`, `months_of`. `to_chars_label` writes digits with the C3 digit writer if C3 is merged;
otherwise a local helper that C3-02 replaces.
AC:
- Golden `dates.tsv` columns `fy_start`, `fy_end`, `quarter_start`, `quarter_end` (the quarter of the
  row's date in its own fiscal year), all 1018 rows, including the `ERR` cells.
- `fiscal_year{2082}.label(lang::en) == "2082/83"` and `label(lang::ne) == "२०८२/८३"`;
  `fiscal_year{2099}.label(lang::en) == "2099/00"`.
- `fiscal_year{1900}.start()` and `fiscal_year{2199}.end()` are `out_of_range`.
- `months_of(quarter::q4)` is Baishakh, Jestha, Ashad.

### C2-05 `date_range`
`make`, `of_month`, `of_year`, `first`, `last`, `size`, `contains`, iteration. The iterator holds a
serial (`std::uint32_t`) and converts on dereference; it models
`std::random_access_iterator`. `std::ranges::enable_borrowed_range<date_range>` is true.
AC:
- For every year 1901..2199, `of_year(y)->size()` equals the sum of its 12 month lengths and is in
  364..367 (PORT-PLAN phase 2 AC).
- `of_month(2081, month::shrawan)` has 32 dates, the first 2081-04-01 and the last 2081-04-32; a
  `for` loop that breaks at the third date visits exactly 3.
- `date_range::make(date::max(), date::min())` is `std::nullopt`; `of_year(2200)` is `out_of_range`.
- `static_assert(std::ranges::random_access_range<date_range>)` and
  `static_assert(std::ranges::view<date_range>)`.
- `std::ranges::distance(r) == r.size()` and `r.begin()[k]` equals `r.first().add_days(k)` for every
  `k` of `of_year(2081)`.

### C2-06 Catalogue and allocation
- Every catalogue row with `phase` 2 as a test in `tests/cs_phase2.cpp` (or a deviation line; most
  `NepaliDateRangeExtendedTests` rows are D-09). `NEPDATE_CATALOG_PHASE=2`.
- Allocation tests: 0 for `add_days`, `add_months`, `add_years`, `diff`, `next`, `prev`, every
  `fiscal_year` member except `label`, `date_range` construction and a full iteration of
  `of_year(2081)`.
- `STATIC_REQUIRE` for the §8 examples and one `fiscal_year{2081}.start()`.

AC: all pass in `dev` and `noexcept`; `catalog_coverage` passes at phase 2.

## Checkpoint (stop here for review)

The reviewer checks:
- [ ] Each overflow guard (C2-01, C2-02) comes before the arithmetic it protects.
- [ ] `diff` has no loop over days; at most one step down.
- [ ] Golden rows: no rows skipped (there is no deviation in these files).
- [ ] Deviation lines for D-08 and D-09 rows name the D-ID and say what the port does instead.
- [ ] `date_range` iterator: check it against `std::random_access_iterator` requirements by reading
      the `static_assert`, not by eye.
- [ ] `cmake -P tools/ci.cmake` summary line in the PR.
