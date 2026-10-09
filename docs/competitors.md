# Other C and C++ code for the Bikram Sambat calendar

Checked 2026-10-08 against GitHub (repository search for "nepali date", "nepali calendar",
"bikram sambat", "nepdate", "patro", languages C and C++), the vcpkg ports tree and the Conan Center
recipes tree, and the sources themselves. Used by task C5-02 (benchmarks).

No C or C++ **library** is published in a package manager: vcpkg (2874 ports) and Conan Center (1951
recipes) have nothing for the Nepali calendar. What exists is code inside CLI tools and desktop
widgets, all GPL or unlicensed, none covering NepDate's range (1901 to 2199), none with parsing,
fiscal year or patro data, and every one disagreeing with NepDate's data in some years.

## Native support: none

| Where | Finding | Source |
|---|---|---|
| C++20 `<chrono>` | `year_month_day` and the other calendar types are the proleptic Gregorian calendar only | [time.cal] in the working draft, https://eel.is/c++draft/time.cal |
| ICU4C 78.3 (latest release, 2026-03-17) and `main` | calendar types are gregorian, japanese, buddhist, roc, persian, islamic (5 variants), hebrew, chinese, indian (the Indian national Saka calendar), coptic, ethiopic, ethiopic-amete-alem, iso8601, dangi. No Nepali or Vikram calendar | `gCalTypes` in https://github.com/unicode-org/icu/blob/main/icu4c/source/i18n/calendar.cpp |
| CLDR (`main`) | the `ca` (calendar) key has no Nepali or Vikram value | https://github.com/unicode-org/cldr/blob/main/common/bcp47/calendar.xml |
| Windows (`Windows.Globalization`) | Gregorian, Hebrew, Hijri, Japanese, Julian, Korean, Persian, Taiwan, Thai, UmAlQura, plus reserved lunar names. No Nepali calendar | https://learn.microsoft.com/uwp/api/windows.globalization.calendaridentifiers |
| Boost.Date_Time, Howard Hinnant's `date` library | Gregorian; Hinnant's examples add Julian, Islamic and ISO week calendars, not Bikram Sambat | https://www.boost.org/libs/date_time, https://github.com/HowardHinnant/date |

Apple's `Calendar.Identifier.vikram` (iOS 26) is Foundation, not available to portable C++, and is
the Indian lunisolar Vikram Samvat, not Nepal's solar Bikram Sambat (checked for the family on 2026-10-07).

## Benchmarked against (C5-02)

"Differing years" compares each table's month lengths with `shared/data/calendar/bs-calendar.json`
over the years both cover (script run 2026-10-08).

| Code (pinned commit) | Licence | Version / last change | BS range | Approach | Differing years |
|---|---|---|---|---|---|
| `khumnath/nepdate-cli` `src/bikram.h` (`27e60035`) | GPL-3.0 (LICENSE file; GitHub shows "other") | v1.1.0, 2025-08-30 | table rows 2000 to 2099, but `BS_END_YEAR = 2089` limits lookups to 2000 to 2089; outside that, an astronomical (Surya Siddhanta) calculation in `double` | header-only class `Bikram`; BS to AD sums year totals one by one, then a floating-point Julian day; AD to BS walks years then months | 2062, 2084 to 2090, 2099 |
| `aatishnn/nepdate` `date.c` (`18e5d3f9`) | GPL (GitHub metadata says GPL-2.0, the file header says GPL-3.0 or later) | no release, 2015-03-18 | 2000 to 2090 | C structs with fixed `char[10]` names; sums years, then **walks day by day** (`while (total_eDays != 0)`) | 2062, 2081, 2082, 2083, 2087, 2090 |
| `nepali-bhasa/nepali-calendar` library `nepalical` (`3ea885a8`) | GPL-3.0 | no release, 2017-01-26 | 2000-09-17 to 2090-12-30 | classes `Bs`/`Ad` over an abstract `Date`; each date owns a heap-allocated data object (`delete data` in the destructor), so every conversion allocates | 2062, 2081, 2082, 2083, 2087, 2090 |

Calls to benchmark (confirm the month base, 0 or 1, from the source in C5-02):

| Code | BS → AD | AD → BS |
|---|---|---|
| nepdate-cli | `Bikram b; b.toGregorian(y, m, d, gy, gm, gd);` | `b.fromGregorian(y, m, d); b.getYear(); b.getMonth(); b.getDay();` |
| aatishnn | `nep_to_eng(struct nep_date, struct eng_date*)` | `eng_to_nep(struct eng_date, struct nep_date*)` |
| nepalical | construct a `Bs` from (y, m, d), convert to `Ad` (see `include/nepalical/Bs.h`) | construct an `Ad`, convert to `Bs` |

Only B1 and B2 apply to them (none has month arithmetic with clamp, Nepali long format, strict parse
or patro data); the other columns say "n/a". If one of them doesn't compile with C++20 and our
warning flags relaxed for its sources only, C5-02 records why and moves it to the next table.

The 64 benchmark dates (BS 2000-01-01 to 2090-12-30) include 2081 to 2087, where these tables
disagree with NepDate, so the "agrees" column will not be 64/64 for any of them.

## Not benchmarked

| Code | Licence | Why not |
|---|---|---|
| `khumnath/nepdate` (Qt desktop widget, v2.3.2, 2026-07-13) | GPL-3.0 | conversion code needs Qt (`QDate`, `QVariantMap`); same author and method as nepdate-cli, table 2000 to 2089 |
| `aayushlbef/Tithify` (Win32 taskbar widget, v3.6.3, 2026-09-22) | Apache-2.0 | conversion lives inside a 1500-line Win32 application file; table 1975 to 2100 differs in 17 years (1977, 1981, 1989, 1993, 1998, 2087, 2090 to 2100) |
| `pakosaan/np-date` (waybar module) | none | no licence, so we can't use it; `std::map` table 2000 to 2099 |
| `khumnath/nepaliconv` | GPL-3.0 | archived, replaced by nepdate-cli; same file layout as nepalical |
| student and demo projects: `codersantoshadhikari/English-to-Nepali-date-converter`, `vjstha20/EnglishToNepaliDateConverter`, `martianbytes/NepaliDateConverter`, `Ronstha/Nepali-Date-Converter`, `nbnbhattarai/dateconverter`, `sailendraw/english-to-nepali-date-conversion` | mostly none | programs with `main`, not libraries |

## The C# column

Every benchmark table also carries C# NepDate 2.0.7 on .NET 10, copied from the hub's
`docs/benchmarks/dotnet.md` (measured on the same reference machine). Those are the numbers this
port must beat (ADR-0002 §9).

## Sources

https://github.com/khumnath/nepdate-cli, https://github.com/khumnath/nepdate,
https://github.com/aatishnn/nepdate, https://github.com/nepali-bhasa/nepali-calendar,
https://github.com/aayushlbef/Tithify, https://github.com/pakosaan/np-date,
https://github.com/khumnath/nepaliconv, https://github.com/microsoft/vcpkg/tree/master/ports,
https://github.com/conan-io/conan-center-index/tree/master/recipes.
