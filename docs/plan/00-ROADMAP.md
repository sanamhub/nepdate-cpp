# Roadmap (C++ port)

Each phase is one PR (or a short series), sized so an agent finishes it in one session and a
reviewer reads it in about an hour. Task IDs `C<phase>-<n>`; branch `feat/C1-04-serial`; commit
`feat(nepdate): C1-04 serial conversion` (scopes: `nepdate`, `patro`, `codegen`, `ci`, `bench`,
`shared`).

```
C0 repo + CI ─► C1 core + conversion ─► C2 arithmetic/fiscal/range ─┐
                         └────────────► C3 format + parse ───────────┴─► C4 patro + interop ─► C5 docs, bench, release
```

Phases map one to one onto `shared/spec/PORT-PLAN.md` phases 0 to 5. C2 and C3 may run in
parallel; C4 needs both. **Every phase ends at its Checkpoint section: the agent stops, opens the
PR, and waits for the human review before the next phase.**

## Preconditions (human tasks)

| ID | Task | Needed before |
|---|---|---|
| HC-1 | Hub task H0 merged, so `shared/VERSION` holds a hub commit SHA instead of `pending`; re-sync `shared/` | C0 |
| HC-2 | **Install a C++ toolchain on the reference machine:** the Visual Studio 2026 workload "Desktop development with C++" (MSVC v145, CMake, Ninja, clang-cl optional), or LLVM 18+ from https://github.com/llvm/llvm-project/releases plus CMake 3.25+ and Ninja. This machine has neither today | C0 (to run `tools/ci.cmake` locally), C5 (benchmarks) |
| HC-3 | vcpkg available locally (`git clone https://github.com/microsoft/vcpkg` and `bootstrap-vcpkg`), `VCPKG_ROOT` set | C0 |
| HC-4 | Hub H3 (reusable `shared-check`, `shared-sync`) merged | C0-06 |
| HC-5 | Hub H5 (`shared/spec/vectors/csharp-tests/catalog.tsv`) merged and synced | C1 (catalogue tests) |
| HC-6 | Hub H6-03 (C# baseline rerun on an idle machine) | C5-02 |
| HC-7 | Create `sanamhub/nepdate-cpp`, branch protection on `main`, environment `production` with a required reviewer | C0-06 |

| Phase | File | Output |
|---|---|---|
| C0 | `C0-repo-and-ci.md` | CMake project, presets, vcpkg manifest, generator skeleton, `tools/ci.cmake`, CI, community files |
| C1 | `C1-core-and-conversion.md` | `date`, `month`, `errc`, `result`, generated tables, BS ↔ AD, vectors, exhaustive, `constexpr` and allocation tests |
| C2 | `C2-arithmetic-fiscal-range.md` | add days/months/years, diff, fiscal year, range |
| C3 | `C3-format-and-parse.md` | names, `to_chars_*`, `std::formatter`, strict and lenient parse, fuzzing |
| C4 | `C4-patro-and-interop.md` | `nepdate::patro` library, isolation gate, `<chrono>` interop, `today` |
| C5 | `C5-docs-bench-release.md` | doc comments, README examples, benchmarks vs competitors and C#, packages, release workflow, v0.1.0 |
| C6 | (no plan file; review follow-up) | clang-tidy gate passes; instrumented patro; library-only coverage |
| C7 | `C7-hardening.md` | `result<T>` rvalue accessors (ADR-0006), `parse_lenient` split, B8 on MSVC, re-bench |

## The C# test catalogue

`shared/spec/vectors/csharp-tests/catalog.tsv` (395 `[Fact]` and 147 `[InlineData]` rows from
C# NepDate's tests at `cb05cb65`) assigns each row a phase (1 to 4), `n/a` or a deviation. Its
`phase` column is the authority. For orientation, the C# test files map roughly like this:

| Phase | C# test files (Fact + InlineData counts) |
|---|---|
| 1 | `NepaliDateConstructionTests` (6 + 23), `NepaliDatePropertiesTests` (9), `NepaliDateComparableTests` (11), `DictionaryIntegrityTests` (7 + 4), most of `OptimizationVerificationTests` (59 + 74) |
| 2 | `NepaliDateArithmeticEdgeCaseTests` (25), `NepaliDateManipulationTests` (3 + 9), `FiscalYearTests` (9 + 2), `FiscalYearInstanceMethodTests` (38 + 7), `NepaliDateRangeTests` (19), `NepaliDateRangeExtendedTests` (64, many D-09) |
| 3 | `NepaliDateFormattableTests` (20), `NepaliDateParsableTests` (5), `NepaliDateMonthNameTests` (3 + 28), `SmartDateParserTests` (14, D-06), `StringExtensionsTests` (5) |
| 4 | `CalendarDataTests` (35), `DateTimeExtensionsTests` (5), `NepaliDateIsRelativeTests` (17, D-14) |
| n/a or deviation | `NepaliDateTypeConverterTests` (18), `NepaliDateSerializationTests` (9, D-11), `BulkConvertTests` (7, D-10), `MemoryUsageTests` (4), `MemoryOptimizationBenchmark` (3) |

Each phase implements its rows as Catch2 tests named after the row `id` (ADR-0004 §3), plus the
extra tests C++ allows: exhaustive loops, `static_assert` checks, allocation counting, fuzzing,
TSan.

## Definition of Done (every task)

1. Every acceptance criterion has a test (or a line in the PR saying why it can't).
2. `cmake -P tools/ci.cmake` passes; the PR quotes its summary line.
3. No public name beyond `docs/API.md`; the libraries depend only on the standard library.
4. Allocation tests still report 0 for every path in ADR-0002 §6.
5. `CHANGELOG.md` `[Unreleased]` updated for anything a user would notice.
6. Prose follows `.claude/skills/writing-style/SKILL.md`. PR has "What to look at first".

## Milestones

| Milestone | Phases | Exit |
|---|---|---|
| M1 converts | C0 to C1 | month-boundary vectors, exhaustive round trip and phase-1 catalogue rows pass on every CI compiler |
| M2 parity | C2 to C3 | all C# golden rows pass except `PARITY.md` deviations; fuzzers clean |
| M3 v0.1.0 | C4 to C5 | isolation gate passes, benchmark table published with every ADR-0002 §9 target met or reported, tagged |
