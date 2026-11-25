# Benchmarks: `nepdate` and `nepdate-patro`

Time and allocation of the C++ library on operations B1 to B8 of
[`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md), measured with Google Benchmark 1.9.5
through [`bench/`](../bench/). C++ has no mature competitor that passes
[`docs/competitors.md`](competitors.md), so this run reports this port only.

## Machine

| Item | Value |
|---|---|
| CPU | 12th Gen Intel Core i5-12500H (4 performance + 8 efficiency cores, 16 threads, base 3.1 GHz) |
| RAM | 31.7 GB |
| OS | Windows 11 Home 10.0.26300 |
| Power plan | Ultimate Performance |
| Affinity | logical CPU 2 (mask `0x4`), set with `start /affinity 4` |
| Idle gate before the run | 6.1 percent average, 13.2 peak (30 one-second samples) |
| Load during the run | 12.7 percent average, 25.3 peak over 67 one-second samples; 6.9 and 20.0 on the CPUs other than CPU 2 |

The machine was idle: no other build, test suite or benchmark ran during the run. The load during
the run includes the benchmark's own pinned core and the sampler. The samples are in
[`raw/2026-11-17/cpu-samples.txt`](raw/2026-11-17/cpu-samples.txt).

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

The raw files are [`raw/2026-11-17/bench-msvc.txt`](raw/2026-11-17/bench-msvc.txt),
[`raw/2026-11-17/bench-msvc.json`](raw/2026-11-17/bench-msvc.json) and the load log
[`raw/2026-11-17/bench-msvc.log`](raw/2026-11-17/bench-msvc.log).

## Results

`ns/op` is the Google Benchmark aggregate mean of CPU time divided by 64 (BENCHMARK section 3
reports per single operation). The standard deviation is the aggregate standard deviation divided by
64. Bytes/op is 0 for every row: the counting operator new reported no allocation during any
operation.

The comparison with C# NepDate, the original library, and the target check from `spec/BENCHMARK.md` section 6 are in the hub: [performance page](https://nepdate.com/docs/project/performance/).

| ID | Operation | nepdate ns/op | std ns | bytes/op | ADR-0004 target ns/op |
|---|---|---:|---:|---:|---:|
| B1 | BS to AD | 6.76 | 0.21 | 0 | ≤ 8 |
| B2 | AD to BS | 8.15 | 0.17 | 0 | ≤ 8 |
| B3 | add 1000 days | 6.97 | 0.09 | 0 | ≤ 8 |
| B4 | add 13 months, Clamp | 4.73 | 0.15 | 0 | ≤ 8 |
| B5 | long Nepali date with weekday | 22.71 | 0.70 | 0 | ≤ 40 |
| B6 | strict parse `YYYY/MM/DD` | 10.29 | 0.17 | 0 | ≤ 20 |
| B7 | patro info of a day | 9.44 | 0.15 | 0 | ≤ 10 |
| B8 | patro month grid | 369.48 | 5.17 | 0 | ≤ 250 |

Every row allocates 0 bytes. B7 now meets its ADR-0004 target of 10 ns (9.44 ns). B8 still misses
250 ns (369.48 ns); this is reported, not hidden.

## Notes

- This run follows the move of the patro tables into a header (ADR-0008), which lets the month
  grid inline its table reads. Against the 2026-11-16 run on the same compiler, B7 went from
  11.64 ns to 9.44 ns and B8 from 503.55 ns to 369.48 ns (27 percent faster). B1 to B6 did not
  change in the source; their small shifts (up to 4 percent) are run-to-run spread.
- Two earlier attempts the same day were dropped: a browser kept CPU 2 busy and real time ran 10
  to 50 percent above CPU time. In this run real time and CPU time agree within 1.5 percent.

- The benchmark also has an experimental B1 variant and a lenient parse row. They are not part of
  B1 to B8 and are not in the table.
- The counting operator new in `alloc_counter.cpp` feeds a `benchmark::MemoryManager`; it reported
  no allocation for any operation in this run.
- A fast wrong answer is never ranked first. There is no competitor row this time.
