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

The laptop was not idle during these runs (other agent jobs were active, and the CPU sat at its
3.1 GHz base clock). The numbers are indicative; BENCHMARK.md section 1 asks for a quiet machine,
which the final release run should use.

## Toolchains and versions

| Item | Value |
|---|---|
| nepdate-cpp | `main` at `89faf97` (C0 to C4 merged), plus the C5 branch |
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

MSVC (the reviewer's `vcvars64` environment, reused):

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
| B1 | 6.91 | 0 | 33.34 | 61/64 | 29,519 | 62/64 | 1,064 | 61/64 | 13.29 |
| B2 | 7.67 | 0 | 305.94 | 61/64 | 24,774 | 60/64 | 809 | 61/64 | 18.64 |
| B3 | 7.62 | 0 | n/a | | n/a | | n/a | | 17.50 |
| B4 | 4.94 | 0 | n/a | | n/a | | n/a | | 18.85 |
| B5 | 21.97 | 0 | n/a | | n/a | | n/a | | 153.02 |
| B6 | 23.38 | 0 | n/a | | n/a | | n/a | | 21.55 |
| B7 | 11.12 | 0 | n/a | | n/a | | n/a | | 47.93 |
| B8 | 487.19 | 0 | n/a | | n/a | | n/a | | 3,132.66 |

## Results, Clang (zig)

| ID | ns/op | bytes/op | nepdate-cli ns/op | agrees | aatishnn ns/op | agrees | nepalical ns/op | agrees | C# .NET 10 ns/op |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| B1 | 4.31 | 0 | 26.76 | 61/64 | 19,836 | 62/64 | 1,038 | 61/64 | 13.29 |
| B2 | 5.35 | 0 | 145.18 | 61/64 | 66,406 | 60/64 | 848 | 61/64 | 18.64 |
| B3 | 4.12 | 0 | n/a | | n/a | | n/a | | 17.50 |
| B4 | 2.35 | 0 | n/a | | n/a | | n/a | | 18.85 |
| B5 | 24.14 | 0 | n/a | | n/a | | n/a | | 153.02 |
| B6 | 15.66 | 0 | n/a | | n/a | | n/a | | 21.55 |
| B7 | 8.69 | 0 | n/a | | n/a | | n/a | | 47.93 |
| B8 | 267.08 | 0 | n/a | | n/a | | n/a | | 3132.66 |

The competitor columns only fill B1 and B2: none of them has month arithmetic with clamp, the
Nepali long format, strict parsing or patro data, so the other cells are "n/a"
(`docs/competitors.md`). aatishnn converts AD by walking day by day, and its B1/B2 are tens of
microseconds; nepalical allocates four heap objects per conversion (about 4 allocs/op, 32
bytes/op), the other two allocate nothing.

## Targets (ADR-0002 section 9)

BENCHMARK.md section 6 for C++ asks that B1, B2 and B7 beat C# with zero bytes. They do, on both
compilers. ADR-0002 section 9 sets tighter targets for all eight:

| ID | Target ns/op | MSVC | Clang | Met |
|---|---:|---:|---:|---|
| B1 | <= 6 | 6.91 | 4.31 | Clang |
| B2 | <= 8 | 7.67 | 5.35 | both |
| B3 | <= 8 | 7.62 | 4.12 | both |
| B4 | <= 8 | 4.94 | 2.35 | both |
| B5 | <= 40 | 21.97 | 24.14 | both |
| B6 | <= 15 | 23.38 | 15.66 | neither |
| B7 | <= 10 | 11.12 | 8.69 | Clang |
| B8 | <= 250 | 487.19 | 267.08 | neither |

Misses to report to the human:

- **B6, strict parse.** 23.38 ns (MSVC) and 15.66 ns (Clang) against 15 ns, and slower than C#'s
  21.55 ns on MSVC. `parse` scans the input character by character (digit, Devanagari byte or
  separator check per byte), about ten branches per date. A branchless scan or a digit lookup
  table would close the gap; that is a library change for a later task, not this release.
- **B8, patro month grid.** 487 ns (MSVC) and 267 ns (Clang) against 250 ns. One grid is about 30
  `from_serial` calls plus 30 `info` lookups; MSVC's codegen for that loop is about 1.8 times
  Clang's. The Go port's pinned prototype measured 226 ns on this machine.
- **B1 and B7 on MSVC.** 6.91 ns against 6 and 11.12 ns against 10. Both met on Clang; the MSVC
  misses are within the noise of the loaded machine (the CPU never left its 3.1 GHz base clock).

No profile was captured: the machine is shared and the misses above are each a few dozen
instructions per operation, not a hot loop that a profiler would newly reveal.

## The ADR-0002 section 8 variant of B1

The bench also times the variant that stores the AD date of each BS month start (14 KB extra
table). It measured 7.60 ns (MSVC) and 3.81 ns (Clang) against B1's 6.91 ns and 4.31 ns. It wins on
Clang but loses on MSVC, so it does not beat B1 by 10 percent on both compilers; the library keeps
Neri-Schneider (ADR-0002 section 8).
