<p align="center">
  <img src="https://raw.githubusercontent.com/sanamhub/nepdate-cpp/main/docs/assets/nepdate-cpp-128.png" width="128" alt="NepDate for C++ logo">
</p>

<h1 align="center">nepdate-cpp</h1>

<p align="center">
  <a href="https://github.com/sanamhub/nepdate-cpp/actions/workflows/ci.yml"><img src="https://github.com/sanamhub/nepdate-cpp/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/sanamhub/nepdate-cpp/releases"><img src="https://img.shields.io/github/v/release/sanamhub/nepdate-cpp?include_prereleases&sort=semver&logo=github" alt="Release"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus" alt="C++20">
  <a href="https://github.com/sanamhub/nepdate-cpp/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue" alt="License: MIT"></a>
</p>

Bikram Sambat dates for C++20. `nepdate` converts between the Bikram Sambat (Nepali) and
Gregorian calendars for BS 1901-01-01 to 2199-12-30 (AD 1844-04-11 to 2143-04-15), with
arithmetic, formatting and parsing. The core is header-only with no dependencies, the
`to_chars` and parse paths allocate nothing (0 bytes, checked by the allocation tests), and
conversion is `constexpr`, so it also runs at compile time. BS to AD takes 7.09 ns on MSVC and
5.58 ns on Clang, against 13.29 ns for C# NepDate ([`docs/benchmarks.md`](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/benchmarks.md)).

```cpp
#include <nepdate/nepdate.hpp>

#include <iomanip>
#include <iostream>

int main() {
    const nepdate::date d = nepdate::make_date(2081, 4, 15).value();
    const auto ad = d.to_year_month_day();
    std::cout << nepdate::to_string(d) << '\n';
    std::cout << static_cast<int>(ad.year()) << '-' << std::setfill('0') << std::setw(2)
              << static_cast<unsigned>(ad.month()) << '-' << std::setw(2)
              << static_cast<unsigned>(ad.day()) << '\n';
    std::cout << nepdate::to_string(nepdate::from_gregorian(ad).value()) << '\n';
}
```

> **Status:** 0.1.0-beta.1, the first public pre-release. The shared vector suites pass on every
> supported compiler and the public API is listed in [`docs/API.md`](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/API.md), but it may
> still change before 0.1.0. Patro data (tithi, holidays, events) is informational, not an official
> calendar.

## Install

One repository, two packages, one version. The core `nepdate` is header-only; `nepdate-patro` is a
compiled library on top of it for tithi, public holidays and events.

With CMake `FetchContent`, pinned to the release tag:

```cmake
include(FetchContent)
FetchContent_Declare(nepdate
    GIT_REPOSITORY https://github.com/sanamhub/nepdate-cpp.git
    GIT_TAG v0.1.0-beta.1)
FetchContent_MakeAvailable(nepdate)
target_link_libraries(your_app PRIVATE nepdate::nepdate nepdate::patro)
```

Each [GitHub Release](https://github.com/sanamhub/nepdate-cpp/releases) also has a source archive
with `SHA256SUMS`, `SHA512SUMS` and a provenance attestation; its notes show the same
`FetchContent_Declare` with the archive URL and `URL_HASH`, and how to check the download with
`gh attestation verify`.

The vcpkg and Conan Center submissions are prepared but not merged upstream yet. Until they are,
the overlay ports and recipes in this tree work:

```sh
vcpkg install nepdate nepdate-patro --overlay-ports=ports
```

```sh
conan create conan/nepdate
conan create conan/nepdate-patro
```

Set `NEPDATE_BUILD_PATRO` to `OFF` before `FetchContent_MakeAvailable` if you need only the core.
Or build and install, then `find_package`:

```cmake
find_package(nepdate CONFIG REQUIRED)
find_package(nepdate-patro CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE nepdate::nepdate nepdate::patro)
```

`cmake --install` works today. `find_package` for `nepdate-patro` pulls in `nepdate` through
`find_dependency`, so linking `nepdate::patro` alone is enough.

## Usage

**Conversion.** Build a date, read its fields, convert to and from Gregorian. The example above is
`examples/quick_start.cpp`, run by the test suite.

**Arithmetic and fiscal year.** `add_days`, `add_months` and `add_years` return `result<date>`
with `overflow::clamp` or `overflow::spill`. `diff` returns whole years, months and days.
`fiscal_year` gives the start, end, range and the `2082/83` label.

**Formatting.** `to_chars`, `to_chars_iso`, `to_chars_short`, `to_chars_long` and
`to_chars_pattern` write into the caller's buffer and allocate nothing. The `to_string` twins
return `std::string`. `std::formatter<nepdate::date>` prints `{}` as `YYYY/MM/DD` and
`{:<pattern>}` through the shared pattern language, where the standard library has `<format>`.

**Patro.** `nepdate::patro::info(d)` returns the tithi, public-holiday flag and events for one day:

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

**Parsing.** `parse` reads strict `year month day` text; `parse_lenient` accepts human input such
as `2081 Shrawan 15`. Both are `constexpr` over `std::string_view` and reject rather than guess:

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

**`<chrono>` interop.** `to_year_month_day`, `to_sys_days`, `from_gregorian`, `from_sys_days` and
`from_sys_time` use the standard calendar types. `today(utc_offset)` returns the current BS date in
a fixed-offset zone, defaulting to Nepal's +05:45.

## Supported compilers and data range

CI builds and tests on every compiler below; the floors are what CI proves, not a guess at what
might work.

| Compiler | Minimum |
|---|---|
| GCC | 12 |
| Clang | 16, with libstdc++ and with libc++ |
| Apple Clang | Xcode 16 |
| MSVC | 19.44 (Visual Studio 2022 17.14) |

The core covers BS 1901-01-01 to 2199-12-30. Patro data covers BS 2077-01-01 to 2084-08-20 only;
`nepdate::patro::info` returns an empty `day_info` outside that range.

## Relationship to the C# original

This port follows the shared spec and data in the hub
[`sanamhub/nepdate`](https://github.com/sanamhub/nepdate). The original library and calendar data
are [NepDate](https://github.com/RajuPrasai/NepDate) by Raju Prasai, MIT licensed. The deliberate
differences from the original are the `D-xx` rows in
[`shared/spec/PARITY.md`](https://github.com/sanamhub/nepdate-cpp/blob/main/shared/spec/PARITY.md).

## Performance

Every row is faster than C# NepDate and allocates nothing, measured with Google Benchmark on a
laptop that was not idle. B8 still misses its own 250 ns target. Details are in
[`docs/benchmarks.md`](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/benchmarks.md).

| Operation | MSVC ns/op | Clang ns/op | C# ns/op |
|---|---:|---:|---:|
| B1 BS to AD | 7.09 | 5.58 | 13.29 |
| B2 AD to BS | 8.94 | 6.23 | 18.64 |
| B6 strict parse | 12.13 | 10.13 | 21.55 |
| B8 patro month grid | 516.64 | about 450 | 3,132.66 |

## More

| Topic | Where |
|---|---|
| Every public name, with signatures | [docs/API.md](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/API.md) |
| Benchmarks, targets and the B8 miss | [docs/benchmarks.md](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/benchmarks.md) |
| The other C and C++ calendar libraries | [docs/competitors.md](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/competitors.md) |
| What changed in each version | [CHANGELOG.md](https://github.com/sanamhub/nepdate-cpp/blob/main/CHANGELOG.md) |
| Architecture decision records | [docs/adr](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/adr) |
| Building, gates and pull requests | [CONTRIBUTING.md](https://github.com/sanamhub/nepdate-cpp/blob/main/CONTRIBUTING.md) |
| Releasing and rolling back | [docs/runbooks/release.md](https://github.com/sanamhub/nepdate-cpp/blob/main/docs/runbooks/release.md) |
| Reporting a vulnerability | [SECURITY.md](https://github.com/sanamhub/nepdate-cpp/blob/main/SECURITY.md) |
| A wrong date, holiday or tithi | the hub's [data error form](https://github.com/sanamhub/nepdate/issues/new/choose) |
| The other languages | [sanamhub/nepdate](https://github.com/sanamhub/nepdate#packages) |

## License

MIT, see [LICENSE](https://github.com/sanamhub/nepdate-cpp/blob/main/LICENSE). Calendar and patro data from NepDate by Raju Prasai and
contributors, MIT, see [THIRD-PARTY-NOTICES.txt](https://github.com/sanamhub/nepdate-cpp/blob/main/THIRD-PARTY-NOTICES.txt). Not affiliated with
the Government of Nepal.
