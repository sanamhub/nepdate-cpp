# Progress

One row per task. The commit column names the task's own commit. "CI last line" is the last line of
`cmake -P tools/ci.cmake` on the reference machine (Windows, zig 0.17.0 = Clang 22 with libc++,
x86_64-windows-gnu, through local `CMakeUserPresets.json` presets; CONTRIBUTING.md). MSVC and GCC
aren't installed there yet, so every criterion that names them waits for CI.

| Task | Status | Commit | CI last line | Notes |
|---|---|---|---|---|
| C0-01 | partial | f86c901 | `ci: 5/8 gates passed (3 skipped)` | `cmake --workflow --preset local-dev` passes with zig/Clang; MSVC and GCC wait for CI (no GitHub repository yet, HC-7). `cmake --install build/local-dev --prefix <tmp>` installs `include/nepdate/version.hpp` and `lib/cmake/nepdate/nepdateConfig.cmake` |
| C0-02 | partial | dbd2dde | `ci: 5/8 gates passed (3 skipped)` | Baseline `0699a19d0c6386247ce50d4dedbb8217d484d536` resolves Catch2 3.16.0, nlohmann-json 3.12.0 and benchmark 1.9.5. Locally zig's Clang crashes compiling Google Benchmark for `x64-zig`, so the local presets point `VCPKG_MANIFEST_DIR` at a copy of the manifest without `benchmark` (benchmarks start in C5). `tests/CMakeLists.txt` fails the configure if `nepdate`'s `INTERFACE_LINK_LIBRARIES` is not empty |
| C0-03 | done | 066dbda | `ci: 5/8 gates passed (3 skipped)` | CTest `codegen_cli`: two runs give identical bytes, no CR, `--check` exits 0 on a fresh tree and 1 after one byte changes, prints the path, doesn't write. Unit test names `year 2050 month 7` for a length of 33. CMake 3.31+ reserves the target name `codegen`, so CMP0171 is set to OLD |
| C0-04 | done | ee8265d | `ci: 5/8 gates passed (3 skipped)` | A `.cpp` with 2-space indentation fails gate 1 (exit 1); a `0xc3` byte in `include/nepdate/bad.txt` fails gate 2; `release_check -DVERSION=9.9.9` exits 1 (also CTest `release_check_rejects_9.9.9`) |
| C0-05 | partial | f28cc0e | `ci: 5/8 gates passed (3 skipped)` | Gate 1 passes with clang-format 18.1.8. Gate 6 skipped locally: clang-tidy 18 can't parse zig's libc++ 22 headers, and no other standard library is installed; the `lint` job checks it. Added `PointerAlignment: Left` to match `docs/API.md` |
| C0-06 | blocked (human) | aa6138e | `ci: 5/8 gates passed (3 skipped)` | Workflows written, every `uses:` pinned to a 40-character SHA with its version, hub workflows at `94946acd87bf4775c481811c0cdd63a7aaef2c84`, no `secrets: inherit` or `pull_request_target`. "CI green on the first PR" needs the GitHub repository (HC-7) |
| C0 checkpoint | ready for review | aa6138e | `ci: 5/8 gates passed (3 skipped)` (clean clone) | Skipped locally: tidy (clang-tidy 18 vs libc++ 22), sanitizers (zig accepts `-fsanitize=address` for windows-gnu but builds without it; the configure probe catches that), coverage (no gcovr). CI runs all three with `STRICT=ON` |
