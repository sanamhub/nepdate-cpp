# ADR-0002: Date representation, tables and performance

- Status: Accepted
- Date: 2026-10-08
- Deciders: Sanam Pakuwal

## Context

Performance is the top priority of the family, and the C++ port must beat C# NepDate on B1 to B8
with zero bytes allocated (`shared/spec/BENCHMARK.md` §6). The baseline, measured on the reference
machine (hub `docs/benchmarks/dotnet.md`, NepDate 2.0.7, .NET 10, 2026-10-07):

| ID | Operation | C# ns/op | C# bytes/op |
|---|---|---:|---:|
| B1 | BS to AD | 13.29 | 0 |
| B2 | AD to BS | 18.64 | 0 |
| B3 | add 1000 days | 17.50 | 0 |
| B4 | add 13 months, clamp | 18.85 | 0 |
| B5 | long format, Nepali, weekday | 153.02 | 435 |
| B6 | strict parse | 21.55 | 0 |
| B7 | patro info | 47.93 | 51 |
| B8 | patro month grid | 3132.66 | 1429 |

How C# gets there (hub `docs/research/dotnet-internals.md`, confirmed in the source at `cb05cb65`):
BS to AD is one read of a 3588-entry table indexed by month (`NepaliToEnglish.data[(y-1901)*12+m-1]`)
plus `DateTime.AddDays`; AD to BS is one read of a 3589-entry table indexed by AD month plus a walk
back of at most one or two BS months. Both are O(1). The costs C# pays: a 32-byte date that caches
the AD date, two validations per construction, 115 KB of tuple tables built by 188 KB of IL at first
use, a binary search for tithi although tithi data is contiguous, and new `string[]` arrays for event
names and seven strings for one long format.

The spec's serial model (ALGORITHM §2 and §4: 2-bit packed lengths, `YEAR_START`, a month loop and a
binary search) is compact (2.4 KB) but not O(1). This ADR keeps the spec's results and replaces its
storage with direct lookup tables, which the spec allows ("How a port stores the tables is its own
decision").

## Decision

### 1. The date is one `std::uint32_t`

```cpp
class date { std::uint32_t v_; };   // v_ = year << 16 | month << 8 | day
static_assert(sizeof(date) == 4 && std::is_trivially_copyable_v<date>);
```

- Chronological order is integer order, so the defaulted `<=>` is one compare and `std::hash` is the
  value itself. A `struct { uint16 y; uint8 m, d; }` is also 4 bytes, but its defaulted `<=>`
  compares three members, and its bytes aren't chronological on little-endian machines.
- Accessors are a shift and a mask. Formatting, the most common use after conversion, reads
  `year()`, `month()` and `day()` for free.
- Every valid value is at least `1901 << 16`, so `result<date>` stores an error as
  `errc` in the low byte with the year bits zero and stays 4 bytes (ADR-0003). A 4-byte trivially
  copyable value travels in a register on the Windows x64, System V x86-64 and AArch64 ABIs.
- No cached AD date (C# caches it and pays 28 extra bytes plus a conversion on every construction).

Rejected: storing the serial day. B2 and B3 would get cheaper, but every accessor would need the
serial-to-month lookup (§3) and formatting calls three of them. Rejected: a debugger visualiser
(`.natvis`); the packed value is readable in hex (`0x0821040f` is 2081-04-15).

### 2. Tables: generated, `inline constexpr`, read-only data

`tools/codegen` writes `include/nepdate/detail/calendar_tables.hpp` from
`shared/data/calendar/bs-calendar.json`:

| Name | Type | Bytes | Meaning |
|---|---|---:|---|
| `kMonthStart` | `std::uint32_t[3589]` | 14,356 | serial of the first day of BS month index `i = (y-1901)*12 + (m-1)`; entry 3588 = 109212 |
| `kBlockMonth` | `std::uint16_t[3413]` | 6,826 | for block `b` (serials `32b..32b+31`), the month index that contains serial `32b` |
| `kCalendarFingerprint` | `std::uint64_t` | 8 | FNV-1a 64 of the 3588 month lengths, checked by the patro (§7) |

Month length is `kMonthStart[i+1] - kMonthStart[i]`; the year start is `kMonthStart[12*(y-1901)]`.
The spec's `PACKED` and `YEAR_START` aren't stored; a test computes them from the JSON per
ALGORITHM §2 and checks every derived value against these tables.

About 21 KB in total, inside the 48 KB L1 data cache of the reference CPU's performance cores, and
one fifth of C#'s two tables. `inline constexpr` arrays of integers are emitted once into `.rodata`
(`.rdata` on Windows): no static initialisers, no relocations, nothing runs at start-up. Each
month is at least 29 days, so a 32-day block holds at most two month starts; the generator checks
that no block needs more than two steps (today's data needs two at most).

### 3. Algorithms (all O(1), branch-light)

```
to_serial(y, m, d)  = kMonthStart[(y-1901)*12 + m-1] + d - 1                 # one load
from_serial(s):       i  = kBlockMonth[s >> 5]
                      i += kMonthStart[i+1] <= s                             # at most two steps,
                      i += kMonthStart[i+1] <= s                             # written without branches
                      y = 1901 + i / 12;  m = i % 12 + 1;  d = s - kMonthStart[i] + 1
validate(y, m, d):    unsigned(y-1901) < 299, unsigned(m-1) < 12, unsigned(d-1) < month length
weekday               = (s + 4) % 7                                          # ALGORITHM §7
add_days(n)           = from_serial(to_serial + n) after the range check
add_months(n, o)      = month-index arithmetic, ALGORITHM §8, length from kMonthStart
```

Gregorian conversion uses the Neri–Schneider algorithms (C. Neri and L. Schneider, "Euclidean
affine functions and their application to calendar algorithms", Software: Practice and Experience
53(4), 2023, arXiv:2102.06959): days ↔ (y, m, d) with 32-bit multiplications and shifts instead of
Hinnant's divisions. The results are identical to ALGORITHM §5; the experiment below checked every
day from −50,000 to +70,000 Unix days, and the C1 tests check all 109,212 serials against a
line-by-line Hinnant reference written in the test file. We don't call the standard library's
`year_month_day` ↔ `sys_days` conversions in the hot path, because their speed differs between
standard libraries; `to_year_month_day()` only wraps our numbers in the standard types.

Validation happens once, in the constructor functions. Nothing else re-validates a `date`.

### 4. `constexpr` everywhere in the core

Construction, conversion, arithmetic, formatting into a buffer, parsing from `std::string_view`,
names, `fiscal_year` and `date_range` are `constexpr` (ADR-0003 lists the exceptions: `today()`, the
`std::string` functions, the `char8_t` parse overloads). So
`constexpr auto d = nepdate::make_date(2081, 4, 15).value();` is checked at compile time, and an
invalid literal date is a compile error. No function-local statics, no lazy initialisation, no
`std::call_once`.

### 5. Formatting into the caller's buffer

`to_chars_*` write into `[first, last)` and return `std::to_chars_result`. One pass, no temporary
strings:

- Digits: a 200-byte `constexpr` table of the pairs `"00".."99"`; a 4-digit year is two pair copies.
  Devanagari digit `k` is the three bytes `E0 A5 (A6+k)`.
- Names are `constexpr std::string_view` arrays (FORMATTING §1) copied with `std::copy_n`, which is
  `constexpr` and becomes `memcpy` at run time.
- The `std::string` functions format into a stack buffer of `max_long_size` bytes and build the
  string once. Short results fit the small-string buffer (15 bytes in libstdc++ and MSVC, 22 in
  libc++), so `to_string(d)` doesn't allocate at all.

### 6. Paths that never allocate

Construction, conversion both ways, arithmetic, `diff`, comparison, hashing, names, every
`to_chars_*`, `parse`, `parse_lenient`, `fiscal_year` and `date_range` operations and iteration,
`patro::info`, `day_info` accessors, `tithi::name`, `event::name`, `patro::coverage`,
`patro::public_holidays`. The allocation tests (ADR-0004 §3) replace the global `operator new` and
`operator delete` and assert zero calls across every one of them.

Parsing tokenises into a fixed `std::array<std::string_view, 4>` on the stack: lenient parsing
accepts at most three kept tokens (PARSING §2 drops era markers and `गते`/`मिति` before counting), so
a fourth kept token already means `unrecognized`. Month-name matching compares bytes: ASCII letters
case-insensitively, Devanagari exactly.

### 7. Patro layout: dense per-day tables over the coverage

PATRO §3 suggests binary search for the sparse holiday and event days. Dense per-day tables over
the coverage are O(1) for every query and smaller, because the coverage is only 2793 days
(serials 64285 to 67077, BS 2077-01-01 to 2084-08-20):

| Name | Type | Bytes | Meaning |
|---|---|---:|---|
| `kFirst`, `kDays` | constants | | 64285 and 2793 |
| `kDayTithi` | `std::uint8_t[2793]` | 2,793 | index into the 32 tithi names; `0xFF` = no tithi |
| `kHolidayBits` | `std::uint64_t[44]` | 352 | bit `i` set = day `kFirst + i` is a public holiday |
| `kDayEventStart` | `std::uint16_t[2794]` | 5,588 | events of day `i` are `kEvents[start[i] .. start[i+1])` |
| `kEvents` | `patro::event[3032]` | 6,064 | index into the name table, file order per day |
| `kHolidays` | `date[294]` | 1,176 | ascending, the span `public_holidays()` returns |
| `kNameBlob` | `char[59800]` | 59,800 | every distinct name, Nepali and English, concatenated without terminators |
| `kNameOffsets` | `std::uint32_t[1507]` | 6,028 | name `k` in language `l` is `blob[off[2k+l] .. off[2k+l+1])`; 753 pairs (32 tithi first) |

About 81.8 KB, mostly the UTF-8 strings. Sorted serial arrays would need 7,860 bytes for the
holiday and event days plus 3,344 for event offsets, against 8,733 bytes for the dense tithi,
holiday and event-start tables, and would cost up to 11 comparisons per query.

- `info(d)`: `i = to_serial(d) - kFirst`; if `i >= kDays` return the empty `day_info`; otherwise three
  loads and one bit test. `day_info` is `{const event* first; std::uint16_t count; std::uint8_t tithi;
  bool holiday}`, 16 bytes, trivially copyable; `events()` returns `std::span<const event>{first, count}`
  pointing into `kEvents`.
- Names come from one blob with integer offsets, not an array of `std::string_view`: an array of
  pointers needs one relocation per entry (1,506) and lands in `.data.rel.ro` of a shared library or
  PIE, while offsets keep the whole table in `.rodata`.
- The generator fails on a gap inside the tithi range (PATRO §3), on more than 65,535 events, and on
  a coverage longer than `uint16` offsets allow.
- `patro_config.hpp` (generated) holds `NEPDATE_PATRO_CALENDAR_FINGERPRINT`; `patro.hpp` has
  `static_assert(nepdate::detail::kCalendarFingerprint == NEPDATE_PATRO_CALENDAR_FINGERPRINT)`, so
  core headers whose month lengths differ from the ones the patro tables were built with fail to
  compile instead of returning the wrong day's events.

### 8. What was measured (experiment, not the reference benchmark)

A throwaway prototype of §1 to §3 (not kept in this repository), built with zig 0.17.0
(`zig c++`, Clang 22.1.8, libc++, `-std=c++20 -O2`, baseline x86-64), run on the reference machine
pinned to logical CPU 2. **Other agents' jobs were running, including a C# BenchmarkDotNet run on the
same CPU,** so means were 2 to 5 times higher than the minimums and unstable. The table shows the
best of 60 repetitions per run, range over three runs; read it as "how fast it can go", not as a
published number. The prototype allocated 0 bytes, and every `static_assert` on the `constexpr`
tables and conversions compiled.

| Operation | §3 design | Spec storage (packed + `YEAR_START` + loops) | C# (§Context) |
|---|---:|---:|---:|
| B1 BS to AD, Hinnant civil | 6.9 to 9.2 ns | 8.8 to 11.6 ns | 13.29 |
| B1 BS to AD, Neri–Schneider civil | 4.6 to 4.8 ns | | |
| B2 AD to BS, block index + Hinnant | 4.7 to 6.1 ns | 9.8 to 14.6 ns | 18.64 |
| B2 AD to BS, block index + Neri–Schneider | 5.1 to 5.3 ns | | |
| B3 add 1000 days | 3.6 to 4.6 ns | 10.3 to 15.0 ns | 17.50 |

Not measured yet: B4 to B8, MSVC, and a variant that stores the AD date of each BS month start
(C#'s idea, a further 14 KB) for B1. Task C1-09 measures that variant against Neri–Schneider and
keeps it only if it is at least 10 % faster on B1 with both compilers.

### 9. Targets

On the reference machine, Release build, with MSVC (the default Windows compiler) **and** Clang, as
measured by `bench/` (ADR-0004 §6):

| ID | Target ns/op | bytes/op | C# ns/op |
|---|---:|---:|---:|
| B1 | ≤ 6 | 0 | 13.29 |
| B2 | ≤ 8 | 0 | 18.64 |
| B3 | ≤ 8 | 0 | 17.50 |
| B4 | ≤ 8 | 0 | 18.85 |
| B5 | ≤ 40 | 0 | 153.02 |
| B6 | ≤ 15 | 0 | 21.55 |
| B7 | ≤ 10 | 0 | 47.93 |
| B8 | ≤ 250 | 0 | 3132.66 |

Each target is at least 1.4 times faster than C# (BENCHMARK §6 only asks for faster) and no looser
than the Go port's pinned prototype on the same machine (`sanamhub/nepdate-go` ADR-0002 §8: B5
30.6 ns, B8 226 ns), since C++ and Rust are meant to be the fastest ports. A missed target
is reported to the human with the numbers and a profile before release; features aren't dropped to
meet it.

## Consequences

- Core tables about 21 KB, patro about 82 KB, all read-only data with zero start-up cost.
- Every conversion is a handful of instructions and two or three loads, and works at compile time.
- The generator has more to check (block steps, tithi gaps, offset widths), and its tests cover
  each check.
- A data change that alters month lengths changes `kCalendarFingerprint`, which forces the patro to
  be rebuilt with the same core.

## Alternatives considered

- **Spec storage as written (2.4 KB, loops and binary search):** two to three times slower in the
  experiment (§8). Rejected.
- **Binary search over `kMonthStart` for AD to BS:** 12 comparisons with unpredictable branches,
  instead of one load and two compares. Rejected.
- **A full serial → date table:** 109,212 × 4 bytes = 437 KB, 20 times the core tables, and a
  cache miss for most random dates. Rejected.
- **C#'s two tables literally:** 115 KB and two sources of truth. Rejected; §8 keeps the AD-start idea
  as a measured option for B1 only.
- **Sorted arrays with binary search for the patro (PATRO §3 suggestion):** larger and up to 11
  comparisons (§7). Rejected.
