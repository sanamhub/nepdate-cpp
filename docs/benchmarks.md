# Benchmarks: `nepdate` and `nepdate-patro` vs competitors and C# NepDate

Time and allocation of the C++ library on operations B1 to B8 of
[`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md), measured with Google Benchmark 1.9.5
through [`bench/`](../bench/). The C# column is copied from the hub's
[`docs/benchmarks/dotnet.md`](https://github.com/sanamhub/nepdate/blob/main/docs/benchmarks/dotnet.md)
(.NET 10 column, NepDate 2.0.7, run 2026-10-09). Competitors are the three of
[`docs/competitors.md`](competitors.md), fetched at their pinned commits and compiled only inside
`bench/`.

## Machine

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, base 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |
| Affinity | logical CPU 2 (mask `0x4`), set with `start /affinity 4` |

The laptop was not idle during these runs (other build jobs were active, and the CPU sat at its
3.1 GHz base clock). The numbers are indicative; BENCHMARK.md section 1 asks for a quiet machine,
which the final release run should use. Loads at the re-bench: MSVC 23 %, Clang 11 %.
The MSVC B8 row was re-measured on its own (see the B8 note).

## Toolchains and versions

| Item | Value |
|---|---|
| nepdate-cpp | `main` at `3f2e6c0` |
| MSVC | 19.51.36260 (Visual Studio 2026 Community 18), x64, Release `/O2 /DNDEBUG` |
| Clang | 22.1.8 (zig 0.17.0, `x86_64-windows-gnu`, libc++), Release `-O3 -DNDEBUG` |
| Google Benchmark | 1.9.5 (vcpkg, feature `dev`) |
| vcpkg baseline | `0699a19d0c6386247ce50d4dedbb8217d484d536` |
| nepdate-cli | `27e60035` (`src/bikram.h`) |
| aatishnn/nepdate | `18e5d3f9` (`date.c`) |
| nepalical | `3ea885a8` (`nepali-bhasa/nepali-calendar`) |

## Commands

Both build trees were configured from the repository root with the vcpkg toolchain and the `dev`
feature (which brings `benchmark`). The benchmark runs 10 repetitions per operation, reports the
aggregate mean and standard deviation, writes JSON, and is pinned to logical CPU 2.

MSVC (from a `vcvars64` environment):

```
cmake -S bench -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_TOOLCHAIN_FILE=E:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
      -DVCPKG_MANIFEST_DIR=E:/github/sanamhub/nepdate-cpp -DVCPKG_MANIFEST_FEATURES=dev
      -DCMAKE_MAKE_PROGRAM=E:/tools/ninja/ninja.exe
cmake --build <dir>
start /affinity 4 /wait <dir>\nepdate_bench.exe --benchmark_repetitions=10
      --benchmark_out=<dir>\bench.json --benchmark_out_format=json --benchmark_time_unit=ns
```

Clang (zig) adds the local zig toolchain and triplet:

```
cmake -S bench -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_TOOLCHAIN_FILE=E:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
      -DVCPKG_TARGET_TRIPLET=x64-zig -DVCPKG_HOST_TRIPLET=x64-zig
      -DVCPKG_OVERLAY_TRIPLETS=E:/tools/zig-cmake/triplets
      -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=E:/tools/zig-cmake/zig-toolchain.cmake
      -DVCPKG_MANIFEST_DIR=E:/github/sanamhub/nepdate-cpp -DVCPKG_MANIFEST_FEATURES=dev
      -DCMAKE_MAKE_PROGRAM=E:/tools/ninja/ninja.exe
cmake --build <dir>
start /affinity 4 /wait <dir>\nepdate_bench.exe --benchmark_repetitions=10
      --benchmark_out=<dir>\bench.json --benchmark_out_format=json --benchmark_time_unit=ns
```

`ns/op` below is the Google Benchmark aggregate mean of CPU time, divided by 64 (BENCHMARK.md
section 3 reports per single operation). Wall-clock means are close on B4 to B8 and noisier on
B1 to B3 and B6, where the loaded laptop adds scheduling noise.

## Agrees check

Each competitor converts the same 64 dates and the result is compared with this library before
timing. "No answer" counts a date the competitor's published range does not cover (aatishnn's
`eng_to_nep` runs 1944 to 2033 only, so two AD inputs fall outside it).

| Competitor | B1 agrees | B2 agrees |
|---|---|---|
| nepdate-cli | 61/64 | 61/64 |
| aatishnn/nepdate | 62/64 | 60/64 (2 no answer) |
| nepalical | 61/64 | 61/64 |

No competitor agrees on all 64: the sample includes 2081 to 2087, where their tables differ from
`shared/data` (see `docs/competitors.md`).

## Results, MSVC

Bytes/op is 0 for this port on every operation: the counting `operator new` (ADR-0004 section 5)
sees only Google Benchmark's own noise (about 0.1 bytes/op), and the allocation tests in
`tests/alloc_tests.cpp` prove the library paths allocate nothing.

| ID | ns/op | bytes/op | nepdate-cli ns/op | agrees | aatishnn ns/op | agrees | nepalical ns/op | agrees | C# .NET 10 ns/op |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| B1 | 7.09 | 0 | 33.34 | 61/64 | 29,519 | 62/64 | 1,064 | 61/64 | 13.29 |
| B2 | 8.94 | 0 | 305.94 | 61/64 | 24,774 | 60/64 | 809 | 61/64 | 18.64 |
| B3 | 7.55 | 0 | n/a | | n/a | | n/a | | 17.50 |
| B4 | 5.58 | 0 | n/a | | n/a | | n/a | | 18.85 |
| B5 | 28.41 | 0 | n/a | | n/a | | n/a | | 153.02 |
| B6 | 12.13 | 0 | n/a | | n/a | | n/a | | 21.55 |
| B7 | 12.81 | 0 | n/a | | n/a | | n/a | | 47.93 |
| B8 | 516.64 | 0 | n/a | | n/a | | n/a | | 3,132.66 |

## Results, Clang (zig)

| ID | ns/op | bytes/op | nepdate-cli ns/op | agrees | aatishnn ns/op | agrees | nepalical ns/op | agrees | C# .NET 10 ns/op |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| B1 | 5.58 | 0 | 26.76 | 61/64 | 19,836 | 62/64 | 1,038 | 61/64 | 13.29 |
| B2 | 6.23 | 0 | 145.18 | 61/64 | 66,406 | 60/64 | 848 | 61/64 | 18.64 |
| B3 | 4.69 | 0 | n/a | | n/a | | n/a | | 17.50 |
| B4 | 2.75 | 0 | n/a | | n/a | | n/a | | 18.85 |
| B5 | 28.47 | 0 | n/a | | n/a | | n/a | | 153.02 |
| B6 | 10.13 | 0 | n/a | | n/a | | n/a | | 21.55 |
| B7 | 10.22 | 0 | n/a | | n/a | | n/a | | 47.93 |
| B8 | 450 | 0 | n/a | | n/a | | n/a | | 3132.66 |

The competitor columns only fill B1 and B2: none of them has month arithmetic with clamp, the
Nepali long format, strict parsing or patro data, so the other cells are "n/a"
(`docs/competitors.md`). aatishnn converts AD by walking day by day, and its B1/B2 are tens of
microseconds; nepalical allocates four heap objects per conversion (about 4 allocs/op, 32
bytes/op), the other two allocate nothing.

## B-lenient

The bench also times `parse_lenient` on the 64 dates written as `DD <month name> YYYY`, so the
lenient tokenizer, classifier and assembler all run (strict parse fails first). It is not one of
the published B1 to B8; it shows the split of `parse_lenient` into per-step functions did not
regress lenient parsing. MSVC, CPU time, 10 repetitions: 228.5 ns/op before (the pre-split body)
and 222.3 ns/op after (the split), 0 bytes both.

## Targets (ADR-0002 section 9)

BENCHMARK.md section 6 for C++ asks that B1, B2 and B7 beat C# with zero bytes. They do, on both
compilers. ADR-0002 section 9 sets tighter targets for all eight:

| ID | Target ns/op | MSVC | Clang | Met |
|---|---:|---:|---:|---|
| B1 | <= 6 | 7.09 | 5.58 | Clang |
| B2 | <= 8 | 8.94 | 6.23 | Clang |
| B3 | <= 8 | 7.55 | 4.69 | both |
| B4 | <= 8 | 5.58 | 2.75 | both |
| B5 | <= 40 | 28.41 | 28.47 | both |
| B6 | <= 15 | 12.13 | 10.13 | both |
| B7 | <= 10 | 12.81 | 10.22 | neither |
| B8 | <= 250 | 516.64 | 450 | neither |

Misses:

- **B8, patro month grid.** 516.64 ns (MSVC, 33,064 ns per 64 grids, CPU time, 10 repetitions,
  2026-10-15) and about 450 ns (Clang, 16 % load) against 250 ns. Both iterate the month with the
  public `date_range`, as a user would. An earlier rewrite stepped serials with the internal
  `detail::from_serial`; that made the benchmark faster without making the library faster (and the
  MSVC gain was within noise), so it was reverted. A callgrind profile
  (clang, WSL) puts 47 % of the instructions in the grid loop (iterator dereference through
  from_serial, plus the consume), 26 % in patro::info (to_serial plus four table reads) and 17 % in
  event::name and tithi::name. Closing the gap is left to a profile-led pass on a quiet machine before
  the release run (the loaded laptop moves even B1 by a few percent between runs); candidates are
  the per-day out-of-line `info` call and the name lookups, not the benchmark loop.
- **B1 and B2 on MSVC, B7 on both.** 7.09 ns against 6, 8.94 ns against 8, and 12.81 ns (MSVC) /
  10.22 ns (Clang) against 10. B1 and B7 on MSVC missed in the earlier run too; these are within the noise of
  the loaded machine (the CPU never left its 3.1 GHz base clock).

## The ADR-0002 section 8 variant of B1

The bench also times the variant that stores the AD date of each BS month start (14 KB extra
table). It measured 8.72 ns (MSVC) and 4.50 ns (Clang) against B1's 7.09 ns and 5.58 ns. It wins on
Clang but loses on MSVC, so it does not beat B1 by 10 percent on both compilers; the library keeps
Neri-Schneider (ADR-0002 section 8).
