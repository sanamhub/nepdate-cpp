# C4: Patro library and `<chrono>` interop

Read first: `shared/spec/PATRO.md`, `shared/spec/PORT-PLAN.md` phase 4, hub ADR-0004 (patro in a
separate package), ADR-0001 §2 to §4, ADR-0002 §7, `docs/API.md` (patro, construction from
`<chrono>`, `today`), ADR-0004 §1 gate 9.

The patro is the CMake project `patro/` producing target `nepdate::patro`, namespace
`nepdate::patro`, header `<nepdate/patro.hpp>`. It depends on the core; nothing under `include/`
may mention it.

## Tasks

### C4-01 Patro project skeleton
`patro/CMakeLists.txt`: when not inside the top-level build, `project(nepdate-patro VERSION <same>)`
and `find_package(nepdate ${PROJECT_VERSION} EXACT CONFIG REQUIRED)`. Library `nepdate_patro`
(alias `nepdate::patro`, export name `patro`), `STATIC` or `SHARED` per `BUILD_SHARED_LIBS`,
`target_link_libraries(nepdate_patro PUBLIC nepdate::nepdate)`, `generate_export_header` writing
`<nepdate/patro_export.hpp>`, hidden visibility by default (`CXX_VISIBILITY_PRESET hidden`,
`VISIBILITY_INLINES_HIDDEN ON`). Install rules and `cmake/nepdate-patroConfig.cmake.in` with
`find_dependency(nepdate <version> EXACT)`. Top-level `CMakeLists.txt` adds `patro/` when
`NEPDATE_BUILD_PATRO` is ON.
AC:
- `cmake -S patro -B build/patro-only -DCMAKE_PREFIX_PATH=<core install prefix>` configures and
  builds against an installed core.
- With `-DBUILD_SHARED_LIBS=ON` on Windows, the DLL exports only the functions in `docs/API.md`
  (`dumpbin /exports`), no data symbols.

### C4-02 Patro tables (`patro/src/patro_tables.hpp`, `patro/include/nepdate/patro_config.hpp`)
Extend `tools/codegen` to read `shared/data/patro/*.json` (files sorted by year, days by date) and
write the tables of ADR-0002 §7: `kFirst`, `kDays`, `kDayTithi`, `kHolidayBits`, `kDayEventStart`,
`kEvents`, `kHolidays`, `kNameBlob`, `kNameOffsets`, in `namespace nepdate::patro::detail`, all
`constexpr`. The file is a private header included only by `patro.cpp`, so the tables have internal
linkage in one translation unit and are never installed. Names are deduplicated (np, en)
pairs in first-seen order, the 32 tithi names first. Serials are computed in the generator from the
calendar JSON with the ALGORITHM §4 formula (the generator never includes the library). A
`// 2081-01-01` comment per day block, readable Nepali text in a comment beside each name.
`patro_config.hpp` holds `NEPDATE_PATRO_CALENDAR_FINGERPRINT` (ADR-0002 §7). `codegen_check` covers
both files. No table, name or string from `shared/data/patro/` goes into any file under `include/`.
AC:
- Counts: tithi days T = 2190, holidays H = 294, event days E = 1671, events 3032, distinct name
  pairs 753 (PATRO §3), `kDays == 2793`, `kFirst == 64285`.
- The generator exits non-zero, naming the missing date, on an in-memory copy of the data with one
  tithi day removed (unit test in `tools/codegen/tests/`).
- `grep` for any Devanagari escape or event name under `include/` finds nothing (a CTest test does
  this).

### C4-03 Patro API (`patro/include/nepdate/patro.hpp`, `patro/src/patro.cpp`)
`tithi`, `event`, `day_info`, `info`, `coverage`, `public_holidays` exactly as `docs/API.md`.
`info` is three loads and one bit test after the range check (ADR-0002 §7). `patro.hpp` includes
`<nepdate/date.hpp>`, `<nepdate/date_range.hpp>`, `<nepdate/patro_export.hpp>`,
`<nepdate/patro_config.hpp>`, and `static_assert`s the fingerprint.
AC:
- Every row of the PATRO §4 anchors: tithi, holiday flag and events in order, both languages for
  2081-01-01.
- `coverage()` is 2077-01-01..2084-08-20; `public_holidays()` has 294 dates, strictly ascending,
  each with `info(d).is_public_holiday()`.
- `info` of 2070-06-15, 2090-01-01, `date::min()` and `date::max()` is empty.
- `std::is_trivially_copyable_v<day_info>`, `sizeof(tithi) == 1`, `sizeof(event) == 2`.
- A header compiled with a fake `NEPDATE_PATRO_CALENDAR_FINGERPRINT` fails the `static_assert`
  (compile-fail test).

### C4-04 Core isolation gate (gate 9)
`tools/coreonly/main.cpp`: includes only `<nepdate/nepdate.hpp>`, converts one date and prints it
with `std::fputs`. Gate 9 in `tools/ci.cmake` and job `isolation` in CI: build it in Release, read
the binary as bytes, fail if it contains `World Day Against Human Trafficking`; also fail if the
patro test executable does not contain it; also fail if `nepdate::nepdate` has any
`INTERFACE_LINK_LIBRARIES` or if any file under `include/` contains `#include <nepdate/patro`.
AC:
- `tools/ci.cmake` ends with all gates passed including gate 9.
- The core tests build and pass with `-DNEPDATE_BUILD_PATRO=OFF` and with the `patro/` directory
  deleted from a scratch copy.
- Adding `#include <nepdate/patro.hpp>` and one `nepdate::patro::info` call to `tools/coreonly`
  makes gate 9 fail (try locally, don't commit).

### C4-05 `<chrono>` interop and `today`
`from_sys_time(t, offset)` = `from_sys_days(floor<days>(t + offset))`; `today(offset)` calls it with
`system_clock::now()`. Out-of-range clocks return `out_of_range` (PORT-PLAN phase 4: `today` can
fail here).
AC:
- `from_sys_time` of 2024-07-30 18:30:00 UTC is BS 2081-04-15 with offset 0 and 2081-04-16 with
  `nepal_utc_offset` (PORT-PLAN phase 4 AC).
- `make_date(2081, 4, 15)->to_sys_days()` is `sys_days{2024y/July/30}`; round trip through
  `from_sys_days` for all serials.
- A clock reading in AD 1800 and one in AD 2200 return `out_of_range`.
- The doc comment of `today` states the +05:45 default and the pre-1986 caveat (`docs/API.md` notes).

### C4-06 Catalogue, allocation, threads
- Catalogue rows with `phase` 4 in `patro/tests/cs_phase4.cpp` (patro rows) and
  `tests/cs_phase4.cpp` (interop rows); `NepaliDateIsRelativeTests` rows are D-14 deviations.
  `NEPDATE_CATALOG_PHASE=4`.
- Allocation: 0 for `info`, every `day_info` accessor, `tithi::name`, `event::name`, iterating
  `events()`, `coverage`, `public_holidays`, `from_sys_time`, `today`.
- TSan test (ADR-0004 §3): 8 threads calling `info` and `add_days` on every coverage day.

AC: all pass in `dev`, `noexcept` and `tsan`; `catalog_coverage` passes at phase 4.

## Checkpoint (stop here for review)

The reviewer checks:
- [ ] Gate 9 output in the PR, and the result of the C4-04 "add the include" experiment.
- [ ] `patro.hpp` is ASCII; the readable Nepali is only in comments of generated files.
- [ ] `info` has no search: read `patro.cpp` and count the loads.
- [ ] The generated patro file sizes roughly match ADR-0002 §7 (about 82 KB of data; report the
      object file size from `size` or `dumpbin /headers`).
- [ ] Both libraries build and test on their own (C4-01, C4-04 ACs).
- [ ] B7 and B8 rough numbers from `bench/` if HC-2 is done (targets 10 and 250 ns).
- [ ] `cmake -P tools/ci.cmake` summary line in the PR.
