# C7: Hardening after the C6 review

Read first: ADR-0003 §2, ADR-0006, ADR-0002 §9, `shared/spec/PARSING.md` §2,
`docs/benchmarks.md`, the C6 rows of `PROGRESS.md`.

No behaviour change anywhere except C7-01 (an API addition). Every task keeps zig, MSVC dev and
noexcept, and the WSL gate set green (format, ascii, codegen, build-and-test, no-exceptions, tidy,
sanitizers, isolation, coverage with gcc-14, TSan, 60 s fuzz per target).

## Tasks

### C7-01 `result<T>` rvalue accessors (ADR-0006)
- Add `value() &&`, `value() const&&`, `operator*() &&` returning `T`, and qualify the existing ones
  `const&`, in `include/nepdate/detail/result.hpp` and the `result<date>` specialisation in
  `date.hpp`. `static_assert(sizeof(T) <= 16)` with a message naming ADR-0006.
- Update `docs/API.md` §result.
- Revert the two C6 workarounds in `tests/format_test.cpp` and `tests/alloc_tests.cpp` back to
  `for (const date d : date_range::of_year(2081).value())`, so the sanitizer gate proves the fix.
- Tests: the range-for form for `value()` and `operator*`, a `STATIC_REQUIRE` that
  `decltype(std::declval<result<int>>().value())` is `int` and the lvalue form is `const int&`.
AC: the ASan gate passes with the reverted loops; checking out only the test change (no header
change) fails ASan with stack-use-after-scope. Report both runs.

### C7-02 Split `parse_lenient` (cognitive complexity 88)
- Rewrite it as the PARSING §2 steps, each its own `constexpr` function in `detail`: split into
  tokens, classify each token (number, month name, ignored word, other), apply the error order
  (`number_too_long`, `ambiguous`, `unrecognized`), assemble Y M D, validate through `make_date`.
  Each function below the clang-tidy threshold (25); remove the C6 NOLINTNEXTLINE.
- Keep the old function, renamed, in `tests/reference/parse_lenient_c6.hpp` (test-only).
- Equivalence: a test runs both on every row of `parse.tsv` (67), every file of
  `tests/fuzz/corpus`, and 200,000 generated inputs (seeded; mixes digits, ASCII and Devanagari
  month names, ignored words, separators and junk bytes), comparing value and error. A libFuzzer
  target `fuzz_parse_lenient_equiv` does the same, 60 s in WSL.
- Allocation test still 0 bytes; `constexpr` tests unchanged; B6/B-lenient numbers before and after
  (MSVC), no slower than 5 %.
AC: equivalence on all inputs; negative control: change one error-order branch in the new code and
show the equivalence test fail, then revert.

### C7-03 B8 on MSVC (487 ns against 250)
- Profile the month grid with the Visual Studio profiler (or `perf` in WSL with clang) and say where
  the time goes.
- The grid walks consecutive days, so it should step `date`/weekday/serial incrementally from the
  first cell instead of calling `from_serial` and `info` per day from scratch; patro `info` for
  consecutive days can walk the event-start table forward. Keep zero allocation and the exact output
  (the existing B8 consumer check and the patro vectors).
- Report MSVC and Clang B8 before and after on the same run conditions (pinned CPU 2, 10
  repetitions). If MSVC still misses 250 ns, report the profile; don't drop features.

### C7-04 Clang re-bench and docs
- Re-run the full B1 to B8 bench with Clang (zig) after C7-02 and C7-03, and MSVC again, and update
  `docs/benchmarks.md` (both tables, targets table, misses). The B6 Clang row must be measured after
  the C5 fast path, not copied.
- PROGRESS rows for C7-01 to C7-04, then `| C7 checkpoint | ready for review |`.
