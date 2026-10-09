# AGENTS.md: rules for AI coding agents (C++ port)

You are implementing a planned project. Follow the plan; don't redesign it. Performance is the top
priority of this port (ADR-0002), so the data layout in ADR-0002 is not optional.

## 0. Before writing any code

1. Read `README.md`, this file, every ADR in `docs/adr/`, and `docs/API.md`.
2. Open `docs/plan/00-ROADMAP.md`, find your phase, read that phase file and the spec sections it
   lists under "Read first". Check its preconditions; if one is unmet, stop and say so.
3. At the end of the phase, stop at its "Checkpoint" section and wait for the human review. Don't
   start the next phase in the same session.

## 1. Sources of truth (in priority order)

1. `shared/spec/*.md` + `shared/spec/vectors/*`: behaviour. Vectors win over prose (report the
   disagreement).
2. `docs/adr/*`: decisions. 3. `docs/plan/*`: tasks and acceptance criteria. 4. `docs/API.md`: the
   public surface.

`shared/` is a read-only copy of the hub. **Never edit it.** A data or spec problem is reported to
the human as a hub issue. Never use the C# NepDate docs as a source (`shared/spec/PARITY.md` §1).

## 2. Hard rules

- **Standard and compilers:** C++20 exactly (`cxx_std_20`, `CMAKE_CXX_EXTENSIONS OFF`). No C++23
  library or language feature, not even behind `#if`, except the one `__cpp_lib_format` guard listed
  in `docs/API.md`. Code must build warning-free on every compiler in ADR-0004.
- **Dependencies:** the libraries depend on nothing but the standard library. Catch2, Google
  Benchmark and nlohmann-json are dev-only (vcpkg manifest feature `dev`, ADR-0001 §5); competitor
  code is fetched only inside `bench/` and is never vendored or redistributed.
- **Public surface:** nothing public beyond `docs/API.md`. Everything else lives in
  `namespace nepdate::detail` (or `nepdate::patro::detail`) under `include/nepdate/detail/`.
- **Core isolation:** nothing under `include/` includes `<nepdate/patro.hpp>` or holds tithi,
  holiday or event data (hub ADR-0004). The patro may include core `detail/` headers; the reverse is
  forbidden.
- **No allocation** on any path ADR-0002 §6 lists (construction, conversion, arithmetic, comparison,
  `to_chars_*` formatting, parsing, patro lookups). Only functions returning `std::string` allocate,
  and only the returned string. The allocation tests (ADR-0004) prove it.
- **No exceptions thrown, no RTTI used.** Library code never writes `throw`, `try`, `typeid` or
  `dynamic_cast`. The libraries and tests build and pass with `-fno-exceptions -fno-rtti` (GCC,
  Clang) and `/EHs-c- /GR-` with `_HAS_EXCEPTIONS=0` (MSVC). The one abort is `result<T>::value()`
  on an error (ADR-0003).
- **No undefined behaviour:** no signed overflow (range-check before arithmetic on caller-supplied
  `int`s), no reading past a table, no `reinterpret_cast` except `char8_t` ↔ `char` views in the
  parse overloads. UBSan in CI is the check.
- **Tables:** every table comes from `tools/codegen`. Don't edit `include/nepdate/detail/calendar_tables.hpp`
  or `patro/src/patro_tables.hpp` by hand; change the generator and run
  `cmake --build build --target codegen`. No JSON is read at run time. Tables are `inline constexpr`
  (core) or `constexpr` at namespace scope (patro), so they live in read-only data and cost nothing
  at start-up. No function-local statics, no `std::map`, no `std::vector` in library code.
- **Headers are ASCII.** Every file under `include/` and `patro/include/` is 7-bit ASCII; Devanagari
  in library code is written as octal escapes (`"\340\244\270"`), with the readable text in a
  comment (ADR-0001 §6). Tests may contain UTF-8 and are compiled with `/utf-8` on MSVC.
- **Parity:** tests may skip rows **only** for deviations in `shared/spec/PARITY.md`, with a comment
  naming the D-ID. C# catalogue rows are tests named after the row `id`, or a
  `// cs-deviation: <id> D-xx` comment (ADR-0004). Don't weaken or delete a test to make CI pass.
- **Formatting and lint:** `clang-format` (config in `.clang-format`) and `clang-tidy` (config in
  `.clang-tidy`) clean. No `// NOLINT` without a reason on the same line.
- No features outside the plan. Secrets never go in code, logs or commits.

## 3. How to work

- One task ID at a time (`C1-03`). Test first, see it fail, then implement.
- Run `cmake -P tools/ci.cmake` (C0-04) before saying you're done; paste its last line in the PR.
- Never claim a benchmark number you didn't measure. Numbers from another machine say which.
- PR description: task IDs, what changed and why, how tested, "What to look at first" (2 to 3
  bullets), and the `tools/ci.cmake` summary line.

## 4. Writing

All prose follows [`.claude/skills/writing-style/SKILL.md`](.claude/skills/writing-style/SKILL.md),
for every agent. Doc comments (`///`) say what the function returns and which `errc` values it can
return. **No AI attribution** in commits or PRs.
