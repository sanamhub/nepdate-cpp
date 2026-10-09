# ADR-0001: Layout, toolchain, packaging and shared data

- Status: Accepted
- Date: 2026-10-08
- Deciders: Sanam Pakuwal

## Context

The C++ port is one spoke of the NepDate family (hub ADR-0001). Hub ADR-0004 fixes its names: vcpkg
and Conan packages `nepdate` and `nepdate-patro`, CMake targets `nepdate::nepdate` and
`nepdate::patro`, namespaces `nepdate` and `nepdate::patro`, header `<nepdate/patro.hpp>`. The port
must be the fastest Bikram Sambat library in C++ and beat C# NepDate (ADR-0002), and must build
with the three compiler families C++ users have: MSVC, GCC and Clang (including Apple Clang).

## Decision

### 1. Language standard: C++20, not C++23

| Need | C++20 | Only in C++23 |
|---|---|---|
| `constexpr` tables and conversions, `<=>`, `std::span`, `std::string_view`, `char8_t`, ranges, `<chrono>` calendar types | yes | |
| `std::to_chars` for integers | yes (C++17), we write our own digits anyway (ADR-0002 §5) | `constexpr` `to_chars` |
| `std::expected` | | yes |

C++23 would buy only `std::expected`, and ADR-0003 replaces it with a 30-line `result<T>`. Against
it:

- **MSVC has no stable C++23 mode.** Visual Studio 2026 18.6 (MSVC Build Tools 14.51, May 2026)
  still offers only `/std:c++23preview`, documented as "may not be ABI compatible across releases",
  and `/std:c++latest`
  ([Microsoft Learn, /std](https://learn.microsoft.com/cpp/build/reference/std-specify-language-standard-version),
  [What's new for MSVC](https://learn.microsoft.com/cpp/overview/what-s-new-for-msvc)). A library
  shouldn't force users onto a preview switch.
- **The core is header-only (§2), so its standard is the user's standard.** Every translation unit
  that includes `<nepdate/nepdate.hpp>` compiles our code in its own mode. C++20 is the newest
  mode all four toolchains ship as stable today.
- `std::expected` exists in libstdc++ 12, libc++ 16 and MSVC STL (VS 2022 17.3), so the reason to
  revisit is MSVC's `/std:c++23`, not library support. Revisit then (supersede this ADR).

Minimum compilers, each one a CI job (ADR-0004): GCC 12, Clang 16 (with libstdc++ and with libc++),
Apple Clang from Xcode 16 (`macos-15` runner), MSVC 19.44 (Visual Studio 2022 17.14,
`windows-2022` runner). Newest: GCC 14, Clang 18, Xcode 26 (`macos-26`), MSVC 14.50+ (Visual Studio
2026, `windows-2025-vs2026` runner). These floors are what CI proves, not a guess at what might work.

C++20 modules (`import nepdate;`) are not offered in v0.1: CMake's module support needs 3.28 and
generator support differs between toolchains. Revisit after v0.1.

### 2. Core header-only, patro compiled

- **Core (`nepdate::nepdate`, CMake `INTERFACE` library):** the conversions must be `constexpr`
  (ADR-0002 §4), so the tables they read must be visible in the header as `inline constexpr`
  arrays. That makes the core header-only. The generated table header is about 50 KB of source
  (3589 + 3413 integers); it adds a few milliseconds per translation unit. The linker keeps one copy
  of each `inline constexpr` table.
- **Patro (`nepdate::patro`, `STATIC` or `SHARED` per `BUILD_SHARED_LIBS`):** about 81 KB of tables
  and strings (ADR-0002 §7). Compiled once into one object file instead of parsed by every
  translation unit that includes the header. Lookups are not `constexpr`; nothing in PATRO.md needs
  that. Symbols are exported with CMake's `GenerateExportHeader` (`<nepdate/patro_export.hpp>`,
  generated at configure time and installed); only functions are exported, never data.

### 3. Repository layout and CMake

```
CMakeLists.txt            project(nepdate VERSION x.y.z LANGUAGES CXX); core INTERFACE target;
                          options NEPDATE_BUILD_TESTS, NEPDATE_BUILD_PATRO, NEPDATE_BUILD_TOOLS
CMakePresets.json         dev, ci-<compiler>, asan, ubsan, tidy, coverage, fuzz, release
include/nepdate/          core public headers; detail/ for internals; detail/calendar_tables.hpp generated
patro/CMakeLists.txt      buildable on its own: find_package(nepdate ${PROJECT_VERSION} EXACT CONFIG)
                          when not inside the top-level build
patro/include/nepdate/patro.hpp
patro/src/patro.cpp, patro/src/patro_tables.hpp (generated), patro/tests/
tests/                    core tests; tests/fuzz/ libFuzzer targets and corpus
tools/codegen/            the generator (§5); tools/ci.cmake; tools/coreonly/
bench/                    separate CMake project (ADR-0004 §6)
cmake/                    nepdateConfig.cmake.in, nepdate-patroConfig.cmake.in
ports/nepdate, ports/nepdate-patro   vcpkg overlay ports (tested in CI, submitted upstream by the human)
conan/nepdate, conan/nepdate-patro   Conan 2 recipes (same)
```

- `cmake_minimum_required(VERSION 3.25...4.1)`. 3.25 gives workflow presets
  (`cmake --workflow --preset ...`), `FILE_SET HEADERS` installation (3.23) and
  `CMAKE_COMPILE_WARNING_AS_ERROR` (3.24). Ubuntu 24.04 ships 3.28; vcpkg and Conan bring their own.
- `target_compile_features(... INTERFACE cxx_std_20)` and `CMAKE_CXX_EXTENSIONS OFF`. The core adds
  no compile options to its users (no `/utf-8`, no warnings): its headers are ASCII (§6).
- Install: `nepdate` installs `include/nepdate/**` (minus `patro.hpp`) and a config package
  `nepdate` (`nepdateConfig.cmake`, version file with `SameMinorVersion`). `nepdate-patro` installs
  `patro.hpp`, `patro_export.hpp`, `patro_config.hpp`, the library, and a config package
  `nepdate-patro` whose config file calls `find_dependency(nepdate <same version> EXACT)`.
  Users write `find_package(nepdate CONFIG REQUIRED)` and `find_package(nepdate-patro CONFIG REQUIRED)`.

### 4. Packaging: vcpkg and Conan, two packages each

Checked 2026-10-08: neither `microsoft/vcpkg` (2874 ports) nor `conan-io/conan-center-index` (1951
recipes) has a port or recipe named `nepdate`, `nepdate-patro`, `nepali` or `bikram`.

| Package | vcpkg port | Conan recipe | Builds |
|---|---|---|---|
| `nepdate` | header-only (`set(VCPKG_BUILD_TYPE release)`, installs headers and config) | `package_type = "header-library"`, `package_id` clears settings | top-level `CMakeLists.txt` with `NEPDATE_BUILD_PATRO=OFF`, tests off |
| `nepdate-patro` | depends on `nepdate` with `version>=` the same version | `requires("nepdate/<same version>")` | `patro/CMakeLists.txt` against the installed core |

Both are built from the same release tarball. The repository keeps both as overlay ports and as
recipes, and CI installs them and builds a consumer (ADR-0004). Submitting them to
`microsoft/vcpkg` and `conan-io/conan-center-index` is a human task after v0.1.0 (ADR-0005).

### 5. Shared data and the generator

- `shared/` is a read-only copy of the hub's `data/` and `spec/` with `shared/VERSION` (hub
  ADR-0001 §3). CI calls the hub's reusable `shared-check`; a weekly `shared-sync` opens a PR that
  runs the regenerate command `cmake -S . -B build/codegen -DNEPDATE_BUILD_TOOLS=ON && cmake --build build/codegen --target codegen`.
- `tools/codegen` is a C++20 program built only when `NEPDATE_BUILD_TOOLS=ON`. It reads JSON with
  nlohmann-json (MIT, vcpkg feature `dev`; dev time only, never a dependency of the libraries) and
  writes `include/nepdate/detail/calendar_tables.hpp`, `patro/src/patro_tables.hpp` and
  `patro/include/nepdate/patro_config.hpp`, each starting with
  `// Generated by tools/codegen from shared/data. DO NOT EDIT.`
  `--check` generates in memory, compares bytes with the files on disk, prints each differing path
  and exits 1 without writing. Output is deterministic: sorted input, fixed formatting, LF endings,
  no timestamps.
- Generated files are committed, so users and package managers never run the generator.

### 6. Text encoding

Library sources and headers are 7-bit ASCII. Devanagari strings in generated tables are octal
escapes, three digits per byte (`"\340\244\270"`), never hex escapes: a hex escape swallows any hex
digit that follows it (`"\xe0\xa4\xb8a"` is one invalid escape), an octal escape stops after three
digits. The readable text goes in a comment beside the literal. This keeps the headers correct
under any source charset (MSVC without `/utf-8` reads source as the system code page). Test sources
may contain UTF-8; the test targets add `/utf-8` on MSVC.

### 7. Dev dependencies

vcpkg manifest mode: `vcpkg.json` at the root with no default dependencies and a feature `dev`
(catch2, nlohmann-json, benchmark), and a `builtin-baseline` commit pinned. Versions on that
baseline today: Catch2 3.16.0, nlohmann-json 3.12.0, Google Benchmark 1.9.5. CI uses the vcpkg that
GitHub runners ship (`VCPKG_INSTALLATION_ROOT`) with binary caching. The library targets never link
any of them; a consumer of the packages pulls nothing.

## Consequences

- `find_package(nepdate)` adds headers only; nothing to link, no ABI to break in the core.
- A user who also wants the patro links one small library and gets about 81 KB of read-only data.
- Users on C++17 can't use the library. C++20 is the default for new Visual Studio 2026 projects,
  and the newest stable mode everywhere, so we accept that.
- The generator is C++, so contributors need only the C++ toolchain and vcpkg, no Python.

## Alternatives considered

- **C++23 with `std::expected`:** see §1. Rejected until MSVC ships `/std:c++23`.
- **C++17:** no `<=>`, no `char8_t`, no `std::span`, no `<chrono>` calendar types, and `constexpr`
  functions can't contain `try` or uninitialised variables. Rejected.
- **Core as a compiled library:** loses `constexpr` conversions and inlining across the call (the
  call itself costs about as much as BS to AD). Rejected.
- **Patro header-only:** simplest packaging, but every translation unit would parse about 240 KB of
  escaped strings and tables. Rejected.
- **Generator in Python or CMake script:** Python adds a second toolchain; CMake's `string(JSON)`
  re-parses the document on every call, which is quadratic on 8 patro files. Rejected.
- **FetchContent for test dependencies:** works, but needs a SHA-256 per archive and builds Catch2
  on every fresh configure; vcpkg with binary caching is faster in CI and is also how we test the
  ports. Rejected.
