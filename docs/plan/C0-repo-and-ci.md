# C0: Repository, CMake, generator skeleton, CI

Read first: `AGENTS.md`, ADR-0001, ADR-0004, `shared/spec/PORT-PLAN.md` phase 0.

Preconditions: HC-1, HC-2, HC-3 (roadmap). C0-06 also needs HC-4 and HC-7. If one is unmet, stop
and tell the human.

## Tasks

### C0-01 CMake project and presets
- `CMakeLists.txt`: `cmake_minimum_required(VERSION 3.25...4.1)`,
  `project(nepdate VERSION 0.0.0 LANGUAGES CXX)`. Target `nepdate` (`INTERFACE`) with alias
  `nepdate::nepdate`, `target_compile_features(nepdate INTERFACE cxx_std_20)`, headers as a
  `FILE_SET HEADERS` with base dir `include/`. Options `NEPDATE_BUILD_TESTS` (ON when top level),
  `NEPDATE_BUILD_PATRO` (ON), `NEPDATE_BUILD_TOOLS` (OFF). `CMAKE_CXX_EXTENSIONS OFF`.
- `include/nepdate/nepdate.hpp` (umbrella, empty for now apart from `version.hpp`) and
  `include/nepdate/version.hpp` with `NEPDATE_VERSION_*` = 0.0.0.
- `cmake/warnings.cmake`: function `nepdate_set_warnings(target)` with the flags in ADR-0004 §1,
  used by tests and tools only (never on the `INTERFACE` target).
- `CMakePresets.json` (version 6): configure presets `dev`, `noexcept`, `tidy`, `asan-ubsan`,
  `tsan`, `coverage`, `fuzz`, `release`, each with `binaryDir` `build/<name>` and the vcpkg toolchain
  file from `$env{VCPKG_ROOT}`; matching build and test presets; workflow preset `dev`
  (configure, build, test).
- Install rules and `cmake/nepdateConfig.cmake.in` with a version file (`SameMinorVersion`).
- `CHANGELOG.md` with an empty `## [Unreleased]`.

AC:
- `cmake --workflow --preset dev` succeeds on a clean tree with MSVC, GCC and Clang.
- `cmake --install build/dev --prefix <tmp>` installs `include/nepdate/version.hpp` and
  `lib/cmake/nepdate/nepdateConfig.cmake`.

### C0-02 vcpkg manifest
`vcpkg.json`: `"name": "nepdate-dev"`, no `dependencies`, feature `dev` with `catch2`,
`nlohmann-json`, `benchmark`; `builtin-baseline` set to the vcpkg commit current on the day (record
it in the PR). Presets with tests or tools set `VCPKG_MANIFEST_FEATURES=dev`.
AC: `vcpkg install --x-feature=dev` in manifest mode resolves Catch2 3.x, nlohmann-json 3.12.x and
benchmark 1.9.x; the `nepdate` target's `INTERFACE_LINK_LIBRARIES` is empty.

### C0-03 Generator skeleton (`tools/codegen/`)
`main.cpp` built when `NEPDATE_BUILD_TOOLS=ON`. Reads `shared/data/calendar/bs-calendar.json`,
checks 299 years × 12 lengths in 29..32 and `min_year` 1901, `max_year` 2199, epoch 1901-01-01 =
1844-04-11, and writes `include/nepdate/detail/calendar_tables.hpp` containing the header
comment (ADR-0001 §5), include guard, `namespace nepdate::detail`, and for now only
`inline constexpr int kMinYear = 1901;` and `inline constexpr int kMaxYear = 2199;`. Paths come
from command-line arguments (`--shared <dir> --out <dir>`). Custom targets `codegen` (writes) and
`codegen_check` (`--check`, ADR-0001 §5). Output uses LF only, also on Windows (open the output in
binary mode).
AC:
- Running `codegen` twice gives identical bytes.
- `--check` exits 0 on a fresh tree and 1 after one byte of the generated header is changed, and
  prints that path.
- A unit test in `tools/codegen/tests/` feeding a month length of 33 for 2050 month 7 gets an error
  that names `2050` and month `7`.

### C0-04 `tools/ci.cmake`
CMake script running the gates of ADR-0004 §1 that exist so far (format, ascii, codegen, build and
test, no-exceptions, tidy, sanitizers, coverage), printing each command, stopping at the first
failure, ending with `ci: <passed>/<total> gates passed (<skipped> skipped)`. A gate whose tool is
missing is skipped with a warning. Also `tools/release_check.cmake` (`-DVERSION=X.Y.Z`): exits 0
only if every version location in ADR-0005 agrees with `VERSION` and `CHANGELOG.md` has
`## [X.Y.Z] - YYYY-MM-DD`; until C5 creates ports and recipes it checks only the files that exist.
AC:
- A `.cpp` file with wrong indentation makes `tools/ci.cmake` fail at gate 1 (PORT-PLAN phase 0 AC).
- A non-ASCII byte in a file under `include/` makes it fail at gate 2.
- `cmake -DVERSION=9.9.9 -P tools/release_check.cmake` exits non-zero.

### C0-05 Lint and format configs
`.clang-format` (based on `LLVM`, `ColumnLimit: 100`, `IndentWidth: 4`,
`AllowShortFunctionsOnASingleLine: Inline`), `.clang-tidy` (`bugprone-*`, `cert-*`,
`cppcoreguidelines-*` minus `avoid-magic-numbers` and `pro-bounds-constant-array-index` (tables are
indexed after range checks; keep the check in tests off too), `modernize-*` minus
`use-trailing-return-type`, `performance-*`, `readability-*` minus `magic-numbers`,
`misc-*` minus `non-private-member-variables-in-classes`; `WarningsAsErrors: '*'`;
`HeaderFilterRegex: '(include|patro)/'`), `.editorconfig`. Each disabled check has a comment saying
why.
AC: gate 1 and gate 6 pass on the skeleton with clang-format 18 and clang-tidy 18.

### C0-06 GitHub Actions and repository files
- `.github/workflows/ci.yml`: the jobs of ADR-0004 §4 that exist so far (msvc-2026, msvc-2022, gcc,
  clang, apple, lint, sanitizers, coverage), each running the presets; job `shared-check` calling the
  hub's reusable workflow. Every action pinned by full SHA with the version in a comment;
  `permissions: contents: read`; `concurrency` with `cancel-in-progress`. vcpkg binary caching via
  `VCPKG_BINARY_SOURCES` and the Actions cache.
- `.github/workflows/shared-sync.yml`: the hub's reusable workflow pinned by SHA, `regenerate` =
  the command in ADR-0001 §5, cron `15 3 * * 2` plus `workflow_dispatch`.
- `.github/dependabot.yml` (`github-actions` weekly), `pull_request_template.md` (task IDs, what and
  why, how tested, "What to look at first", `tools/ci.cmake` line), `ISSUE_TEMPLATE/bug_report.yml`
  (input, expected, actual, library version, compiler and version, OS), `SECURITY.md` (private
  advisory, first reply within a week), `CONTRIBUTING.md` (points to `AGENTS.md` and the writing
  style).

AC: CI green on the first PR on every job; every `uses:` line is a 40-character SHA with the version
in a comment; no job uses `secrets: inherit` or `pull_request_target`.

## Checkpoint (stop here for review)

The reviewer checks:
- [ ] `cmake -P tools/ci.cmake` output pasted in the PR; the summary line counts and skips are right.
- [ ] The core target adds no compile options or link libraries to its users (look at
      `INTERFACE_*` properties in `build/dev/CMakeFiles` or `cmake --build build/dev --target help`).
- [ ] Generator output has LF endings on Windows and the "DO NOT EDIT" header.
- [ ] Every action is pinned by SHA; the hub workflows too.
- [ ] Each disabled clang-tidy check has a reason.
- [ ] No library code yet beyond the skeleton; nothing public beyond `version.hpp`.
