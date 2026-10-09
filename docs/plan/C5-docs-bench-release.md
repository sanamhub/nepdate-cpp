# C5: Docs, benchmarks, packages, release v0.1.0

Read first: ADR-0002 §9, ADR-0004 §5, ADR-0005, `docs/competitors.md`,
`shared/spec/BENCHMARK.md`, `shared/spec/PORT-PLAN.md` phase 5.

Preconditions: HC-2 (toolchain on the reference machine) and HC-6 (final C# baseline) for C5-02.

## Tasks

### C5-01 Doc comments and README examples
- A `///` comment on every public name in `docs/API.md`: what it returns, every `errc` it can
  return, and whether it allocates. `-Wdocumentation` (Clang) clean.
- `examples/quick_start.cpp`, `examples/patro.cpp`, `examples/parse.cpp`: small programs, built and
  run by CTest, each comparing its output with an expected-output file.
- README gets a "Quick start" section: install with vcpkg or Conan, `find_package`, and the three
  examples. Each C++ block in the README is the body of one example file, character for character
  (a CTest test extracts and compares).
AC: CTest runs the examples and the README comparison; Clang `-Wdocumentation` reports nothing.

### C5-02 Benchmarks against competitors and C# (`bench/`)
- Complete `bench/` (started in C1-09): B1 to B8 of BENCHMARK §3 for this library on the 64 dates
  (§2) and the patro inputs (§3); one operation consumes every part of its result.
- Competitors from `docs/competitors.md` ("Benchmarked against"), each fetched by `FetchContent` at
  its pinned commit and compiled only here; B1 and B2 only, other cells "n/a".
- Allocations per operation through `benchmark::RegisterMemoryManager` and the counting
  `operator new` (BENCHMARK §4).
- The "agrees" check: for each competitor, how many of the 64 dates match `shared/data`, printed
  before timing.
- Run on the reference machine, pinned to CPU 2, once built with MSVC and once with Clang
  (ADR-0004 §5). Write `docs/benchmarks.md` per BENCHMARK §5: machine table, toolchain and library
  versions with commits, the exact commands, and one table per compiler with ns/op, bytes/op, each
  competitor's ns/op and agrees, and the C# NepDate column copied from the hub's
  `docs/benchmarks/dotnet.md` (.NET 10).
AC:
- Every ADR-0002 §9 target is met with both compilers, or the PR reports each miss to the human with
  the numbers and a profile (Visual Studio profiler or `perf`), without dropping features.
- B1, B2 and B7 are faster than C# and allocate 0 bytes (BENCHMARK §6 for C++).
- A competitor that disagrees on any date is listed with its count, never ranked first.

### C5-03 Packages: vcpkg overlay ports and Conan recipes
- `ports/nepdate/` (`vcpkg.json`, `portfile.cmake`: header-only, `vcpkg_from_github` with
  `REF v${VERSION}`, configure with `NEPDATE_BUILD_PATRO=OFF`, `NEPDATE_BUILD_TESTS=OFF`,
  `vcpkg_cmake_config_fixup`, licence install) and `ports/nepdate-patro/` (depends on `nepdate`,
  builds `patro/`).
- `conan/nepdate/conanfile.py` (`package_type = "header-library"`) and
  `conan/nepdate-patro/conanfile.py` (`requires("nepdate/<version>")`, options `shared`, `fPIC`),
  each with a `test_package/`.
- CI job `packages` (ADR-0004 §4): install both overlay ports from the working tree
  (`--overlay-ports=ports` with a local source override) and build `tests/consumer/`; run
  `conan create conan/nepdate` and `conan create conan/nepdate-patro`.
AC: the `packages` job passes on Linux and Windows; `tools/release_check.cmake` now checks the port
and recipe versions too.

### C5-04 Release workflow and runbook
`.github/workflows/release.yml` per ADR-0005 (preflight, full CI, `production` approval, GitHub
Release with notes from the changelog, source archive with SHA-256 and SHA-512, build provenance
attestation), plus `workflow_dispatch` with input `version` that runs preflight and CI only.
`docs/runbooks/release.md` stays in sync with it.
AC (checked after C5-05, in the same PR): dispatched with `version: 0.1.0`, preflight and CI pass and
the release job shows as skipped.

### C5-05 v0.1.0
Set the version to 0.1.0 in every place ADR-0005 lists; move `[Unreleased]` to
`## [0.1.0] - <date>`. **Agent stops here.** The human tags, approves the release, and submits the
vcpkg and Conan Center PRs (runbook).

## Checkpoint (stop here for review)

The reviewer checks:
- [ ] `docs/benchmarks.md` has both compilers, the exact commands, the affinity, the C# column and
      its source, and every target met or reported.
- [ ] No competitor source is in the repository; `bench/` fetches them by commit.
- [ ] `cmake -DVERSION=0.1.0 -P tools/release_check.cmake` passes.
- [ ] The README quick start compiles (CTest result) and shows both packages.
- [ ] The dispatched release run: preflight and CI green, release job skipped.
- [ ] Milestone M3 conditions, except the tag, which is the human's.
