# C3: Formatting and parsing

Read first: `shared/spec/FORMATTING.md`, `shared/spec/PARSING.md`, `docs/API.md` (formatting,
parsing, standard library hooks), ADR-0001 §6 (ASCII headers), ADR-0002 §5 and §6, PARITY D-04,
D-05, D-06, D-11, D-15, `shared/spec/PORT-PLAN.md` phase 3.

Golden files are in `shared/spec/vectors/csharp-golden/`; the parser vectors are
`shared/spec/vectors/parse.tsv` (59 rows; columns `input`, `strict`, `lenient`).

## Tasks

### C3-01 Names and digits (`include/nepdate/format.hpp`, `detail/names.hpp`)
Month and weekday names (FORMATTING §1) as `inline constexpr std::string_view` arrays. Devanagari
strings as octal escapes with the readable text in a comment (ADR-0001 §6), generated once by
`tools/codegen` into `detail/names_tables.hpp` so nobody types escapes by hand. `to_chars_number`
writes ASCII digits through a 200-byte `"00".."99"` pair table, or Devanagari digit `k` as the bytes
`0xE0 0xA5 0xA6+k`; `min_width` counts digits, not bytes.
AC:
- `month_name(month::shrawan, lang::ne) == "साउन"` (compared in a UTF-8 test file);
  `month_short_name(month::ashad, lang::en) == "Asa"`, `month_short_name(month::ashoj, lang::en) == "Aso"` (D-04);
  `month_short_name(m, lang::ne) == month_name(m, lang::ne)` for all 12.
- `month_name(static_cast<month>(13), lang::en)` and `weekday_name(std::chrono::weekday{9}, lang::en)`
  are empty.
- `to_chars_number` of 2081 with `lang::ne` gives `२०८१`; 5 with width 2 gives `०५` and `05`.
- Gate 2 (ascii) passes: no UTF-8 byte under `include/`.

### C3-02 `to_chars`, `to_chars_iso`, `to_chars_short`, `to_chars_long` and the string functions
One pass into `[first, last)`; return `{last, std::errc::value_too_large}` when the output doesn't
fit. The `std::string` functions format into `char buf[max_long_size]` and construct the string
once (ADR-0002 §5).
AC:
- Golden `dates.tsv` columns `to_string`, `unicode`, `long_en`, `long_en_weekday`, `long_ne`,
  `long_ne_weekday`, `dmy_dash_nopad` (all 1018 rows).
- The FORMATTING §3 and §4 example tables.
- Every date of 2081 in every `short_format` and `long_format` combination and both languages fits
  `max_short_size` / `max_long_size`; a buffer one byte shorter than the output returns
  `value_too_large` and never writes past `last` (ASan run).

### C3-03 `to_chars_pattern` and `format_pattern` (FORMATTING §5)
Standard patterns `""`, `G`, `g`, `d`, `D`, `s` when the whole pattern equals one of them; otherwise
the left-to-right token scan of §5. Digits from tokens follow `lang`; literal characters are copied
unchanged (D-15).
AC:
- Golden `format-pattern.tsv` (451 rows), skipping only rows whose pattern contains a run of exactly
  three `M` and whose date's month is 3 or 6, with a comment naming D-04. The test counts the
  skipped rows and fails if more than the 82 rows containing `MMM` are skipped.
- The six FORMATTING §5 examples.
- An unterminated quote (`yyyy'BS`) runs to the end of the pattern.

### C3-04 `std::formatter<nepdate::date>`
Only when `__cpp_lib_format >= 201907L` (the one feature guard AGENTS.md allows). `parse` takes the
spec up to `}` as a pattern (empty means `YYYY/MM/DD`); `format` calls `to_chars_pattern` into a
stack buffer of 256 bytes and copies it to the output iterator; a longer result is formatted in
pieces, never truncated.
AC: `std::format("{}", d) == "2081/04/15"`; `std::format("{:yyyy-MM-dd}", d) == "2081-04-15"`;
`std::format("{:MMMM d, yyyy}", d) == "Shrawan 15, 2081"`. Where the standard library lacks
`<format>`, the test is compiled out and the PR lists which CI jobs skipped it.

### C3-05 Strict `parse` (PARSING §1)
One pass over the bytes. A Devanagari digit is the 3-byte sequence `E0 A5 A6..AF`; `।` is
`E0 A5 A4`. Any other byte ≥ 0x80 is `invalid_character`. At most 4 digits per group
(`number_too_long`), a 4th group is `wrong_group_count`, fewer than 3 at the end is
`wrong_group_count`, then `make_date` validation. `constexpr`. The `std::u8string_view` overloads
forward to the `std::string_view` ones.
AC:
- The `strict` column of every row of `parse.tsv`. `ERR:<Kind>` maps to the `errc` of the same
  name in snake case (`ERR:WrongGroupCount` → `errc::wrong_group_count`).
- `STATIC_REQUIRE(parse("2080/05/15").value() == make_date(2080, 5, 15).value())`.
- `parse(u8"२०८०/०५/१५")` gives 2080-05-15.
- A truncated Devanagari sequence at the end of input (`"2080/05/1\xE0\xA5"`) is
  `invalid_character` and reads no byte past the end (ASan).

### C3-06 `parse_lenient` (PARSING §2)
Strict first; then normalise into at most 4 kept tokens on the stack (ADR-0002 §6), dropping era
markers and `गते`, `मिति`. The closed month-name list of PARSING §3 is a `constexpr` table generated
by `tools/codegen` from a list in the generator source that a test compares with the spec table
(octal escapes for Devanagari). Latin names compare ASCII case-insensitively; Devanagari compares
bytes. `constexpr`.
AC:
- The `lenient` column of every row of `parse.tsv`, same error mapping as C3-05.
- `parse_lenient("")` and `parse_lenient("   ")` are `unrecognized`; `parse("")` is
  `wrong_group_count`.
- `parse_lenient("12345/01/01")` is `number_too_long`; `parse_lenient("15 Shrawan 2080")`,
  `"Shrawan 15, 2080"` and `"2080 Shrawan 15"` all give 2080-04-15.

### C3-07 Fuzzing
`tests/fuzz/fuzz_parse.cpp` and `fuzz_parse_lenient.cpp` (`LLVMFuzzerTestOneInput`), preset `fuzz`
(Clang, `-fsanitize=fuzzer,address,undefined`). Property per ADR-0004 §3: no crash; on error, the
`errc` is one of the eight PARSING kinds; on success, `to_chars` then `parse` gives the same date.
Seed corpus in `tests/fuzz/corpus/` from every `input` of `parse.tsv`. A CTest test runs every
corpus file through the same property on all platforms (no libFuzzer needed).
AC: `fuzz_parse -max_total_time=600` and the same for `fuzz_parse_lenient` find nothing (run
locally or in CI, paste the last line); CI runs 60 s per target.

### C3-08 Catalogue, compile-time and allocation tests
- Catalogue rows with `phase` 3 in `tests/cs_phase3.cpp`; `SmartDateParserTests` rows are D-06
  deviations unless their result matches `parse_lenient`. `NEPDATE_CATALOG_PHASE=3`.
- Allocation: 0 for every `to_chars_*`, every name function, `parse` and `parse_lenient` (success
  and failure inputs), and `to_string(d)` (fits the small-string buffer, ADR-0002 §5).
- `STATIC_REQUIRE` for one `to_chars_long` with `lang::ne` into a `std::array<char, 80>`.

AC: all pass in `dev` and `noexcept`; `catalog_coverage` passes at phase 3; milestone M2.

## Checkpoint (stop here for review)

The reviewer checks:
- [ ] No hand-typed octal escapes: names and month lists come from `tools/codegen`.
- [ ] The D-04 skip filter is as narrow as C3-03 says, with the count check.
- [ ] `parse` never reads past the input (fuzz and ASan output in the PR).
- [ ] The `std::formatter` guard is the only feature-test `#if` in the library.
- [ ] B5 and B6 rough numbers from `bench/` if HC-2 is done (not a gate; ADR-0002 §9 targets 40 and
      15 ns).
- [ ] `cmake -P tools/ci.cmake` summary line in the PR.
