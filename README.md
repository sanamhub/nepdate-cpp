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

**Status:** v0.1.0 prepared, C5 checkpoint ready for review. Progress: [docs/plan/PROGRESS.md](docs/plan/PROGRESS.md).

## Quick start

`nepdate` is a header-only library; `nepdate-patro` is a compiled library on top of it. Both are
installed from this repository, or from the vcpkg and Conan Center packages whose ports and
recipes live in `ports/` and `conan/`.

Install with vcpkg (overlay ports from this tree, until they land upstream):

```sh
vcpkg install nepdate nepdate-patro --overlay-ports=ports
```

Install with Conan:

```sh
conan create conan/nepdate
conan create conan/nepdate-patro
```

Then use `find_package` and link the targets:

```cmake
find_package(nepdate CONFIG REQUIRED)
find_package(nepdate-patro CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE nepdate::nepdate nepdate::patro)
```

The three programs in `examples/` (built and run by CTest) show the main features.
`examples/quick_start.cpp` converts 2081-04-15 to AD and back:

```cpp
#include <nepdate/nepdate.hpp>

#include <iostream>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const auto ad = d.to_year_month_day();
    std::cout << nepdate::to_string(d) << '\n';
    std::cout << ad << '\n';
    std::cout << nepdate::to_string(nepdate::from_gregorian(ad).value()) << '\n';
}
```

`examples/patro.cpp` reads the tithi, holiday flag and events for one day:

```cpp
#include <nepdate/patro.hpp>

#include <iostream>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const nepdate::patro::day_info day = nepdate::patro::info(d);
    if (const auto t = day.tithi()) {
        std::cout << t->name(nepdate::lang::en) << '\n';
    }
    std::cout << "holiday: " << (day.is_public_holiday() ? 1 : 0) << '\n';
    for (const nepdate::patro::event e : day.events()) {
        std::cout << e.name(nepdate::lang::en) << '\n';
    }
}
```

`examples/parse.cpp` parses strict and lenient text:

```cpp
#include <nepdate/nepdate.hpp>

#include <iostream>

int main() {
    const nepdate::result<nepdate::date> strict = nepdate::parse("2081/04/15");
    const nepdate::result<nepdate::date> lenient = nepdate::parse_lenient("2081 Shrawan 15");
    std::cout << nepdate::to_string(strict.value()) << '\n';
    std::cout << nepdate::to_string(lenient.value()) << '\n';
}
```

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
