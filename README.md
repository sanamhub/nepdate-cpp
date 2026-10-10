# nepdate-cpp

Bikram Sambat (Nepali calendar) for C++20. Part of the NepDate family: shared data and spec live in
the hub [`sanamhub/nepdate`](https://github.com/sanamhub/nepdate); the C# original is
[NepDate](https://github.com/RajuPrasai/NepDate).

- Two libraries from one repository and one version (hub ADR-0004):

  | Library | vcpkg / Conan | CMake target | Header | Namespace |
  |---|---|---|---|---|
  | core: dates, BS ↔ AD, arithmetic, fiscal year, formatting, parsing | `nepdate` | `nepdate::nepdate` (header-only) | `<nepdate/nepdate.hpp>` | `nepdate` |
  | patro (पात्रो, the Nepali almanac): tithi, public holidays, events | `nepdate-patro` | `nepdate::patro` (compiled) | `<nepdate/patro.hpp>` | `nepdate::patro` |

  The core never includes the patro, so a program that only converts dates links no tithi, holiday
  or event strings.
- No dependencies. No heap allocation, no exceptions and no RTTI needed on any path except the
  functions that return `std::string`. Conversions are `constexpr`, so they also run at compile time.
- Built to be faster than C# NepDate on every benchmark operation (ADR-0002, `shared/spec/BENCHMARK.md`).
- BS 1901-01-01 to 2199-12-30 (AD 1844-04-11 to 2143-04-15). Same results as C# NepDate and every
  other port, except the deviations in [shared/spec/PARITY.md](shared/spec/PARITY.md), proved by the
  shared vectors.
- Patro data covers BS 2077-01-01 to 2084-08-20 only ([shared/spec/PATRO.md](shared/spec/PATRO.md)).
- Needs a C++20 compiler: GCC 12, Clang 16, Apple Clang 16 (Xcode 16), MSVC 19.44 (Visual Studio
  2022 17.14) or newer (ADR-0001).

**Status:** phase C1 (date type, BS ↔ AD conversion, tables, tests) ready for review. Progress: [docs/plan/PROGRESS.md](docs/plan/PROGRESS.md).

## Where to start

| You are | Read |
|---|---|
| an AI agent implementing a phase | [AGENTS.md](AGENTS.md) → [docs/plan/00-ROADMAP.md](docs/plan/00-ROADMAP.md) |
| reviewing the API | [docs/API.md](docs/API.md) |
| comparing with other C and C++ libraries | [docs/competitors.md](docs/competitors.md) |
| wondering "why is it like this?" | [docs/adr/README.md](docs/adr/README.md) |
| looking for the algorithm or data | [shared/spec/README.md](shared/spec/README.md) (changes go to the hub) |
| releasing | [docs/runbooks/release.md](docs/runbooks/release.md) |

## Layout

```
shared/                 hub copy: data/, spec/, VERSION (hub commit). Read-only.
include/nepdate/        core headers; detail/calendar_tables.hpp is generated   (phase C0, C1)
patro/                  the patro library: include/, src/ (patro_tables.hpp generated), tests/  (C4)
tests/                  core tests (Catch2), fuzz/ (libFuzzer)                   (C1 to C3)
tools/codegen/          table generator (C++, dev only)                         (C0)
tools/ci.cmake          runs every gate: cmake -P tools/ci.cmake                (C0)
tools/coreonly/         core-only program for the isolation gate                (C4)
bench/                  separate CMake project: B1 to B8, competitors            (C5)
ports/, conan/          vcpkg overlay ports and Conan recipes, tested in CI      (C5)
docs/                   API.md, competitors.md, adr/, plan/, runbooks/
```

## License

MIT ([LICENSE](LICENSE)). Calendar and patro data from NepDate by Raju Prasai and contributors, MIT
([shared/data/NOTICE](shared/data/NOTICE)). Not affiliated with the Government of Nepal.
