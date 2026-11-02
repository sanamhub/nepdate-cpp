# Other C and C++ code for the Bikram Sambat calendar

Checked 2026-11-02 against GitHub repository search for "nepali date", "bikram sambat" and "nepdate"
in C and C++, the vcpkg ports tree and the Conan Center recipes. The selection follows
[`shared/spec/BENCHMARK.md`](../shared/spec/BENCHMARK.md) section 7. Usage is GitHub stars plus
dependents, read on 2026-11-02, and whether the code is in vcpkg or Conan. The bar is 25 stars or a
vcpkg or Conan port, first release at least 12 months before 2026-11-02, a release or commit in the
last 36 months, a license, and a build on the current toolchain.

## Candidate table

| Code | Stars | Last change | License | Kind | Verdict |
|---|---:|---|---|---|---|
| `nepali-bhasa/nepali-calendar` (`nepalical`) | 38 | 2017-01-26 | GPL-3.0 | C++ classes, no package | fails the 36-month bar and is not in vcpkg or Conan |
| `khumnath/nepdate` | 16 | 2026-07-13 | GPL-3.0 | Qt desktop application | fails 25 stars and is an application, not a library |
| `khumnath/nepdate-cli` | 7 | 2026-02-05 | GPL-3.0 | CLI with a header class | fails 25 stars and is a CLI, not a library |
| `aatishnn/nepdate` | 2 | 2015-03-18 | GPL-2.0 | small C library | fails 25 stars and the 36-month bar |
| `aayushlbef/Tithify` | 2 | 2026-09-22 | Apache-2.0 | Win32 application | fails 25 stars and is an application |
| `pakosaan/np-date` | 0 | 2026-02-02 | none | waybar module | fails a license and 25 stars |

vcpkg and Conan Center have no C or C++ Bikram Sambat library. The only code that exposes a
conversion API as a library is `aatishnn/nepdate`, which has two stars and no change since 2015.
The other repositories are applications or command line tools, not libraries a C or C++ project can
depend on.

## Result

No candidate passes the bar. C++ has no mature library to compare, so the benchmark reports NepDate
only and the report says so. The three competitors measured in earlier reports are removed from the
harness and their old numbers are not kept.

## Sources

https://github.com/khumnath/nepdate-cli, https://github.com/khumnath/nepdate,
https://github.com/aatishnn/nepdate, https://github.com/nepali-bhasa/nepali-calendar,
https://github.com/aayushlbef/Tithify, https://github.com/pakosaan/np-date,
https://github.com/microsoft/vcpkg/tree/master/ports,
https://github.com/conan-io/conan-center-index/tree/master/recipes.
