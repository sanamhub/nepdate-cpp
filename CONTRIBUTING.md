# Contributing

Bug reports, fixes and documentation improvements are welcome. For a new feature or an API change,
open an issue first so the design can be agreed before you write code.

This project follows the [Code of Conduct](CODE_OF_CONDUCT.md). Report security problems as
described in [SECURITY.md](SECURITY.md), not in a public issue.

## Scope

The shared calendar and patro data, the spec and the golden vectors live in the hub
([`sanamhub/nepdate`](https://github.com/sanamhub/nepdate)) and are copied into `shared/`
read-only. A wrong date, holiday or tithi is a data issue: report it in the hub, not here.

## Setup

You need CMake 3.25 or newer, Ninja, a C++20 compiler (GCC 12, Clang 16, Apple Clang 16 or MSVC
19.44, or newer) and vcpkg with `VCPKG_ROOT` set. The test dependencies (Catch2, Google Benchmark,
nlohmann-json) come from the vcpkg manifest's `dev` feature.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

## Gates

`cmake -P tools/ci.cmake` runs every gate in order and stops at the first failure. `-DONLY=...`
runs a subset. The gates are `format`, `ascii`, `codegen`, `build-and-test`, `no-exceptions`,
`tidy`, `sanitizers`, `coverage`, `isolation` and `docs`.

- `format` needs clang-format 18.
- `tidy` needs clang-tidy 18 and a toolchain whose standard library it can parse.
- `sanitizers` needs Clang with ASan and UBSan. ThreadSanitizer runs with `cmake --preset tsan`.
- `coverage` needs GCC and gcovr.
- `docs` is a plain CMake script and needs nothing.

On a machine where the default compiler is not the one you want, copy `CMakePresets.json` to the
gitignored `CMakeUserPresets.json` and add `local-*` presets; the script picks them up when it sees
`local-dev`.

## Pull requests

- Keep a pull request to one change.
- Add or update a test for any behaviour change. The shared vector suites must keep passing.
- Public API changes go in `docs/API.md` in the same commit.
- Add a line to the `Unreleased` section of [CHANGELOG.md](CHANGELOG.md) for anything a user would
  notice.
- A significant design decision gets an ADR in `docs/adr/`. Accepted ADRs are not edited; a later
  ADR supersedes an earlier one.
- CI must pass.

## Commit messages

[Conventional Commits](https://www.conventionalcommits.org/): `type(scope): summary`, imperative,
lower case, no trailing period. Types: `feat`, `fix`, `docs`, `chore`, `refactor`, `test`, `build`,
`ci`, `perf`. The body says why, not what.

## Releases

Releasing and rolling back are described in
[`docs/runbooks/release.md`](docs/runbooks/release.md).
