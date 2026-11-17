# ADR-0007: Patro tables in an installed header, lookups inline

- Status: Accepted
- Date: 2026-11-17
- Deciders: Sanam Pakuwal
- Supersedes: the patro part of ADR-0001 section 2 ("compiled once into one object file") and the
  "Patro header-only" alternative it rejected. ADR-0002 section 7 (the table layout) still holds.

## Context

ADR-0002 section 9 sets 250 ns for B8, the patro month grid. The last idle run measured 503.55 ns
on MSVC (`docs/benchmarks.md`). `patro::info()` is a range check, three table reads and one bit
test, and `tithi::name()` and `event::name()` are two reads each. All three lived in `patro.cpp`
with the tables, so a month grid made about 30 `info()` calls plus one name call per tithi and per
event, each out of line. The compiler could not inline them or move anything out of the loop.

ADR-0001 section 2 kept the tables in the compiled library because every translation unit would
otherwise parse about 240 KB of escaped strings. Most of that was the name blob written as one
char literal per byte (`'\340','\244',...`), about 60,000 tokens.

## Decision

1. The generator writes the patro tables to `patro/include/nepdate/detail/patro_tables.hpp`, an
   installed header in `nepdate::patro::detail` (not API, docs/API.md). The tables stay C++17
   `inline constexpr` variables, so each program gets one definition, in read-only data, with no
   start-up code.
2. `<nepdate/patro.hpp>` includes it after it defines `tithi`, `event` and `day_info`, and defines
   `info()`, `tithi::name()` and `event::name()` inline. The public API and the public header
   names are unchanged.
3. `coverage()` and `public_holidays()` stay in the compiled library, so the `nepdate::patro`
   target, its CMake package, the vcpkg port and the Conan recipe are unchanged. A shared build
   exports only these two functions now.
4. The header is 7-bit ASCII like every installed header (ADR-0001 section 6): the name blob is
   one string literal with octal escapes and an explicit length (`std::string_view`), and the
   comments carry the English names. A string literal is a few hundred tokens instead of 60,000.
   Events are written `{tag{}, 32}` instead of the fully qualified form.
5. The generator stays the only writer, and `codegen_check` (gate 3) still fails CI when the
   header differs from `shared/data`.

A month grid over the public API is then one inline loop: the date range iterator walks serial
days, and each day is direct indexed reads into the tables. There is no new month function; B8
measures the same user code as before.

## Measurements

Before is `main` at `ecce16c`, after is this change. Both on the reference machine with other
work running, so times are CPU time of the compiler process, the minimum of 9 interleaved runs.
Sizes are in bytes.

Include cost, one translation unit, `-O2` or `/O2`, C++20:

| Compiler | core only, before | core only, after | core + `patro.hpp`, before | core + `patro.hpp`, after |
|---|---:|---:|---:|---:|
| MSVC 19.52 | 1.45 s | 1.39 s | 1.44 s | 1.55 s |
| GCC 14.3 | 0.90 s | 0.91 s | 0.92 s | 0.96 s |
| Clang 22 | 1.25 s | 1.30 s | 1.30 s | 1.34 s |

Including `<nepdate/patro.hpp>` now costs about 40 ms more per translation unit with GCC and
Clang and about 110 ms with MSVC, on top of a core include of 0.9 to 1.5 s. The generated header
is 380 KB of source, down from 710 KB for the old private header, mostly because the name blob is
one string literal instead of one char literal per byte. In one wall-clock run on the busy machine,
including the old private header the same way cost about 0.6 s more with MSVC, which is why
the format changed with the move.

Size, Release:

`nepdate_patro` static library, and two small programs that link it: `tests/consumer` (one
`info()` call, no names) and a probe that also prints the tithi and event names and calls
`public_holidays()`. MSVC sizes are of the `.lib` and `.exe` (`/O2 /MD`); GCC and Clang sizes are
of the `.a` and the stripped executable (`-O2`, libstdc++).

| Compiler | Library before | Library after | Consumer before | Consumer after | With names before | With names after |
|---|---:|---:|---:|---:|---:|---:|
| MSVC 19.52 | 127,534 | 40,394 | 48,128 | 33,280 | 106,496 | 82,944 |
| GCC 14.3 | 101,156 | 3,126 | 116,944 | 35,024 | 108,672 | 88,192 |
| Clang 22 | 100,524 | 2,830 | 108,720 | 14,512 | 108,720 | 75,952 |

Programs get smaller because the linker now drops each table the program does not use, where
before it took the whole object file with all of them.

B8 itself is not measured here; the next idle benchmark run records it in `docs/benchmarks.md`.

## Consequences

- Every translation unit that includes `<nepdate/patro.hpp>` parses the tables (see above).
- The linker keeps only the tables a program uses. A program that never asks for a name does not
  carry the 60 KB name blob.
- A shared `nepdate-patro` built from an earlier release exported `info()` and the two `name()`
  functions. Code built against it must be rebuilt, which the lockstep versions already require.
- Each program that links the shared library and calls `info()` has its own copy of the tables it
  uses, besides the copy in the library for `public_holidays()`. That is a few KB.

## Alternatives considered

- **Only the per-day tables in the header, names in the library:** `info()` inlines, but every
  tithi and event name is still an out-of-line call, about two per day in B8. The name blob as a
  string literal costs little to parse, so splitting gains little.
- **A month function in the API:** would skip the per-day range check, but B8 is defined over the
  existing calls and the API is stable from 1.0. Rejected.
- **Link-time optimisation:** would inline across the library boundary, but only for users who
  turn it on with the same compiler. Rejected as the fix.
