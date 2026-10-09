# ADR-0004: Testing and quality gates

- Status: Accepted
- Date: 2026-10-08
- Deciders: Sanam Pakuwal

## Decision

### 1. One command runs every gate

`cmake -P tools/ci.cmake` (CMake script mode, so it works the same on Windows, Linux and macOS
without bash or Python) runs, in order, stopping at the first failure, and ends with
`ci: <passed>/<total> gates passed (<skipped> skipped)`:

1. **format:** `clang-format --dry-run --Werror` on every `*.hpp`/`*.cpp` outside `shared/`, `build*/`
   and generated files. Skipped with a warning when `clang-format` 18 is not found; CI always runs it.
2. **ascii:** every file under `include/` and `patro/include/` is 7-bit ASCII (ADR-0001 §6).
3. **codegen:** build `tools/codegen`, run `--check` (ADR-0001 §5).
4. **build and test:** `cmake --workflow --preset dev` (configure, build with warnings as errors,
   `ctest --output-on-failure`).
5. **no-exceptions:** configure preset `noexcept` (`-fno-exceptions -fno-rtti`, or `/EHs-c- /GR-
   /D_HAS_EXCEPTIONS=0` on MSVC), build and run the tests.
6. **tidy:** preset `tidy` (`CMAKE_CXX_CLANG_TIDY`, `.clang-tidy` with `WarningsAsErrors: '*'`).
   Skipped with a warning when `clang-tidy` is not found.
7. **sanitizers:** preset `asan-ubsan` (`-fsanitize=address,undefined -fno-sanitize-recover=all`);
   on MSVC `/fsanitize=address` only. Skipped where the toolchain can't do it.
8. **coverage:** preset `coverage` (GCC `--coverage`, `gcovr --fail-under-line 90
   --fail-under-branch 80`, filters `include/` and `patro/`). Skipped with a warning when `gcovr` is
   not found. Org target is 80 % lines; this port asks for 90.
9. **isolation** (added in C4): the core-only program `tools/coreonly` is built in Release; its
   binary must not contain the bytes `World Day Against Human Trafficking`, and a CMake check
   asserts that `nepdate::nepdate`'s `INTERFACE_LINK_LIBRARIES` is empty. The patro test binary
   must contain the string, so the check can't pass by accident (hub ADR-0004).

Warnings: GCC and Clang `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
-Wold-style-cast -Wnon-virtual-dtor`; MSVC `/W4 /permissive- /utf-8` (tests) and
`/Zc:__cplusplus`. All as errors in CI.

### 2. Test framework: Catch2 v3

Catch2 3.x (3.16.0, released 2026-08-25, on the pinned vcpkg baseline) over doctest (2.5.3,
2026-07-06): both are maintained, but Catch2 has `STATIC_REQUIRE` for compile-time checks, generators with a fixed seed for
property-style tests (`GENERATE(take(n, random(a, b)))`), and `catch_discover_tests` for CTest.
Compile time is higher than doctest, which matters little for about ten test files. Catch2 builds
with `CATCH_CONFIG_DISABLE_EXCEPTIONS` for the gate 5 run.

RapidCheck is not used: it has never published a release or tag (only commits), so it can only be
pinned by commit, and the date domain is small enough to test
exhaustively (109,212 serials), which beats random sampling. Catch2 generators cover the rest.

### 3. What the tests cover

- **Vectors:** every file in `shared/spec/vectors/`, read at run time from a path CMake passes as a
  compile definition (`NEPDATE_SHARED_DIR`). A test fails if a file has fewer rows than the spec
  states, so a missing file can't pass silently.
- **C# catalogue:** every row of `shared/spec/vectors/csharp-tests/catalog.tsv` (hub H5) tagged with
  the current phase or earlier has a `TEST_CASE` whose name is the row's `id` exactly, tagged
  `[cs]`, or a line `// cs-deviation: <id> D-xx` (or `// cs-n/a: <id> <reason>` for rows tagged
  `n/a`) in a test file. The CTest test `catalog_coverage` runs `tools/check_catalog.cmake`, which
  reads the TSV, lists test names with `--list-tests`, greps the comments, and fails on any missing
  row. Its phase limit `NEPDATE_CATALOG_PHASE` rises by one in each phase.
- **Exhaustive:** all 109,212 serials round-trip BS → serial → AD → BS, and both Gregorian
  algorithms (Neri–Schneider in the library, Hinnant in the test) agree on every one.
- **Compile time:** `STATIC_REQUIRE` / `static_assert` for the ALGORITHM §9 reference values, the
  FORMATTING examples, a parse, `sizeof(date) == 4`, `sizeof(result<date>) == 4`,
  `std::is_trivially_copyable_v` for `date`, `result<date>`, `day_info`, `tithi`, `event`, and
  `std::ranges::random_access_range<date_range>`.
- **Allocation:** `tests/alloc_counter.cpp` replaces every global `operator new`/`delete` form with
  counting versions (a separate test executable, so Catch2's own allocations happen outside the
  measured block). Each path in ADR-0002 §6 runs 1000 times between two reads of the counter; the
  difference must be 0.
- **Fuzzing:** libFuzzer targets `fuzz_parse` and `fuzz_parse_lenient` (Clang only, preset `fuzz`,
  with ASan and UBSan). Property: never crash; an error is one of the eight kinds PARSING can
  produce; a success formats with `to_chars` and parses back to the same date. Seed corpus: every
  `input` of `shared/spec/vectors/parse.tsv`, in `tests/fuzz/corpus/`. CI runs each target for
  60 seconds on Linux; the corpus inputs also run as a normal CTest test on every platform.
- **Thread safety:** a test reads `patro::info` and converts dates from 8 threads at once under
  ThreadSanitizer (preset `tsan`, Linux Clang). All data is immutable, so it must be clean.
- **Packaging:** CTest test `install_consumer` installs both libraries to a temporary prefix and
  builds `tests/consumer/` (a separate project using `find_package(nepdate)` and
  `find_package(nepdate-patro)`) once with `-std=c++20` and once with `-std=c++23`.

### 4. CI (GitHub Actions)

| Job | Runner | Compiler | Runs |
|---|---|---|---|
| msvc-2026 | `windows-2025-vs2026` | MSVC 14.5x, x64, Debug and Release | gates 3 to 5, ASan |
| msvc-2022 | `windows-2022` | MSVC 19.44 (floor) | gates 4 and 5 |
| gcc | `ubuntu-24.04` | GCC 12 (floor) and GCC 14 | gates 4 and 5 |
| clang | `ubuntu-24.04` | Clang 16 (floor, libstdc++) and Clang 18 with libc++ | gates 4 and 5 |
| apple | `macos-15`, `macos-26` | Apple Clang (Xcode 16, Xcode 26) | gates 4 and 5, ASan and UBSan |
| lint | `ubuntu-24.04` | Clang 18 | gates 1, 2, 3, 6 |
| sanitizers | `ubuntu-24.04` | Clang 18 | gate 7, TSan test |
| coverage | `ubuntu-24.04` | GCC 14 + gcovr | gate 8 |
| fuzz | `ubuntu-24.04` | Clang 18 | 60 s per target |
| packages | `ubuntu-24.04`, `windows-2025-vs2026` | default | overlay ports via vcpkg, recipes via `conan create` (C5) |
| isolation | `ubuntu-24.04`, `windows-2025-vs2026` | default | gate 9 (C4) |
| shared-check | hub reusable workflow | | `shared/` matches `shared/VERSION` |

Every `uses:` is pinned to a 40-character commit SHA with the version in a comment, including the
hub's `shared-check` and `shared-sync` workflows (hub H3). `permissions: contents: read`,
`concurrency` cancels superseded runs. vcpkg binary cache through the GitHub Actions cache.
Dependabot updates the action SHAs weekly.

### 5. Benchmarks: Google Benchmark

Google Benchmark 1.9.5 over nanobench: it reports mean, median and standard deviation over
`--benchmark_repetitions=10` (BENCHMARK §4 asks for mean and standard deviation), writes JSON for the
results table, has `benchmark::DoNotOptimize`, and its `benchmark::MemoryManager` hook reports bytes
allocated per run from our counting `operator new` (BENCHMARK §4 allocation column). nanobench is
excellent for nanosecond timing but reports median and error percent, not mean and standard
deviation.

`bench/` is a separate CMake project, never built by the library's CI gates:

- B1 to B8 over the 64 dates of BENCHMARK §2 (and the patro inputs of §3), `state.SetItemsProcessed`
  so the report is per operation, one benchmark per operation per library.
- Competitors from `docs/competitors.md`, fetched at configure time with `FetchContent` pinned to a
  commit SHA, compiled only inside `bench/`. GPL code is never vendored, linked into a released
  binary or redistributed.
- An "agrees" check before timing: each competitor's result for the 64 dates is compared with ours;
  disagreeing rows are counted and printed.
- Run on the reference machine with `start /affinity 4 /wait build\bench\nepdate_bench.exe
  --benchmark_repetitions=10 --benchmark_report_aggregates_only=true --benchmark_out=...json`, once
  built with MSVC and once with Clang (both Release, `/O2` or `-O2`, no `-march`).
- Not run per PR. Results go to `docs/benchmarks.md` (C5).

## Consequences

- `cmake -P tools/ci.cmake` is the same check locally and in CI; local runs skip what isn't
  installed and say so.
- Tests need the shared vectors on disk, so a source tarball without `shared/` can build but not test.
