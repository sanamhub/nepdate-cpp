# Benchmarks: `nepdate` and `nepdate-patro` against C# NepDate

Time and allocation of the C++ library on operations B1 to B8 of
[`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md), measured with Google Benchmark 1.9.5
through [`bench/`](../bench/). C++ has no mature competitor that passes
[`docs/competitors.md`](competitors.md), so this run reports this port only. The C# column is copied
from the hub's [`docs/benchmarks/dotnet.md`](https://github.com/sanamhub/nepdate/blob/main/docs/benchmarks/dotnet.md)
(.NET 10 column, NepDate 2.0.7, run 2026-11-16).

## Machine

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, base 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |
| Affinity | logical CPU 2 (mask `0x4`), set with `start /affinity 4` |
| Idle gate before the run | 6.2 percent average, 11.3 peak (30 one-second samples) |
| Load during the run | 16.3 percent average, 22.0 peak over 70 one-second samples; 10.8 and 17.0 on the CPUs other than CPU 2 |

The machine was idle: no other build, test suite or benchmark ran during the run. The load during
the run includes the benchmark's own pinned core and the sampler. The samples are in
[`raw/2026-11-16/cpu-samples.txt`](raw/2026-11-16/cpu-samples.txt).

## Toolchains and versions

| Item | Value |
|---|---|
| nepdate-cpp | `main` at the benchmark commit |
| MSVC | 19.52.37029 (Visual Studio 2026 Community 18, toolset 14.52.37025), x64, Release `/O2 /DNDEBUG` |
| Google Benchmark | 1.9.5 (vcpkg, feature `dev`) |
| vcpkg baseline | `0699a19d0c6386247ce50d4dedbb8217d484d536` |
| Competitors | none; no C++ library passes the bar |

## Command

The build tree was configured from the repository root with the vcpkg toolchain and the `dev`
feature. The benchmark runs 10 repetitions per operation, reports the aggregate mean and standard
deviation, writes JSON, and is pinned to logical CPU 2.

```
cmake -S bench -B build\bench -G Ninja -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
      -DVCPKG_MANIFEST_DIR=%CD% -DVCPKG_MANIFEST_FEATURES=dev
cmake --build build\bench
start /affinity 4 /wait build\bench\nepdate_bench.exe --benchmark_repetitions=10
      --benchmark_report_aggregates_only=true
      --benchmark_out=docs\raw\2026-11-16\bench-msvc.json
      --benchmark_out_format=json --benchmark_time_unit=ns
```

The raw files are [`raw/2026-11-16/bench-msvc.txt`](raw/2026-11-16/bench-msvc.txt),
[`raw/2026-11-16/bench-msvc.json`](raw/2026-11-16/bench-msvc.json) and the load log
[`raw/2026-11-16/bench-msvc.log`](raw/2026-11-16/bench-msvc.log).

## Results

`ns/op` is the Google Benchmark aggregate mean of CPU time divided by 64 (BENCHMARK section 3
reports per single operation). The standard deviation is the aggregate standard deviation divided by
64. Bytes/op is 0 for every row: the counting operator new reported no allocation during any
operation.

| ID | Operation | nepdate ns/op | std ns | bytes/op | C# .NET 10 ns/op | C# bytes/op | ADR-0004 target ns/op |
|---|---|---:|---:|---:|---:|---:|---:|
| B1 | BS to AD | 6.58 | 0.05 | 0 | 13.27 | 0 | ≤ 8 |
| B2 | AD to BS | 7.84 | 0.08 | 0 | 18.70 | 0 | ≤ 8 |
| B3 | add 1000 days | 6.81 | 0.08 | 0 | 20.06 | 0 | ≤ 8 |
| B4 | add 13 months, Clamp | 4.77 | 0.07 | 0 | 19.30 | 0 | ≤ 8 |
| B5 | long Nepali date with weekday | 22.41 | 0.24 | 0 | 139.04 | 435 | ≤ 40 |
| B6 | strict parse `YYYY/MM/DD` | 10.01 | 0.16 | 0 | 21.13 | 0 | ≤ 20 |
| B7 | patro info of a day | 11.64 | 0.11 | 0 | 48.63 | 51 | ≤ 10 |
| B8 | patro month grid | 503.55 | 5.40 | 0 | 3,274.50 | 1,429 | ≤ 250 |

Every B-row beats the C# column and allocates 0 bytes. B7 misses its ADR-0004 target of 10 ns
(11.64 ns) and B8 misses 250 ns (503.55 ns); both are reported, not hidden.

## Notes

- B8 is 19 percent slower than the 2026-11-02 run (423.53 ns) and B6 is 15 percent faster (11.84 ns).
  The source did not change; the MSVC compiler did (19.51.36260 then, 19.52.37029 now). A second
  run of B6 to B8 on the same binary, after the main run, gave B6 9.61 ns, B7 11.03 ns and B8
  487.19 ns, so the shift is not noise.

- The benchmark also has an experimental B1 variant and a lenient parse row. They are not part of
  B1 to B8 and are not in the table.
- The counting operator new in `alloc_counter.cpp` feeds a `benchmark::MemoryManager`; it reported
  no allocation for any operation in this run.
- A fast wrong answer is never ranked first. There is no competitor row this time.
