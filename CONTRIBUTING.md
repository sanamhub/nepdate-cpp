# Contributing

This is the C++ port of the NepDate family. Behaviour is fixed by the shared spec in `shared/spec/`
and decisions by the ADRs in `docs/adr/`, so most changes follow a task in `docs/plan/`. The rules
for agents, which also apply to people, are in [`AGENTS.md`](AGENTS.md).

## Data and spec changes go to the hub

`shared/` is a read-only copy of [`sanamhub/nepdate`](https://github.com/sanamhub/nepdate). A wrong
month length, holiday or tithi, or a question about the spec, goes there as an issue. Never edit
`shared/` here, and never edit a generated table by hand: change `tools/codegen` and run
`cmake --build build/dev --target codegen`.

## Build and checks

You need CMake 3.25 or later, Ninja, a C++20 compiler (ADR-0001 §1) and vcpkg with `VCPKG_ROOT`
set. Then:

```bash
cmake --workflow --preset dev   # configure, build, test
cmake -P tools/ci.cmake          # every gate; run before a PR
```

The last line of `tools/ci.cmake` reads `ci: <passed>/<total> gates passed (<skipped> skipped)`.
Locally, a gate whose tool is missing (clang-format 18, clang-tidy, gcovr, a sanitizer runtime) is
skipped with a warning; CI runs every gate with `-DSTRICT=ON`.

### A compiler the presets don't find

The committed presets let CMake pick the compiler, which is what CI wants. When your compiler needs
a toolchain file or a vcpkg triplet of its own, put presets named `local-<name>` in
`CMakeUserPresets.json` (git ignores it). Each one inherits a committed preset and adds your
settings; `tools/ci.cmake` uses the `local-` presets on its own when `local-dev` exists. A sketch
for a zig toolchain, with your own paths in place of the `<...>` parts:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local",
      "hidden": true,
      "environment": { "VCPKG_ROOT": "<vcpkg>" },
      "cacheVariables": {
        "VCPKG_TARGET_TRIPLET": "<triplet>",
        "VCPKG_HOST_TRIPLET": "<triplet>",
        "VCPKG_OVERLAY_TRIPLETS": "<triplet dir>",
        "VCPKG_CHAINLOAD_TOOLCHAIN_FILE": "<toolchain>.cmake"
      }
    },
    { "name": "local-dev", "inherits": ["local", "dev"] },
    { "name": "local-noexcept", "inherits": ["local", "noexcept"] }
  ],
  "buildPresets": [
    { "name": "local-dev", "configurePreset": "local-dev" },
    { "name": "local-noexcept", "configurePreset": "local-noexcept" }
  ],
  "testPresets": [
    { "name": "local-dev", "configurePreset": "local-dev" },
    { "name": "local-noexcept", "configurePreset": "local-noexcept" }
  ],
  "workflowPresets": [
    {
      "name": "local-dev",
      "steps": [
        { "type": "configure", "name": "local-dev" },
        { "type": "build", "name": "local-dev" },
        { "type": "test", "name": "local-dev" }
      ]
    }
  ]
}
```

Add `local-tidy`, `local-asan-ubsan` and `local-coverage` the same way. A preset that is missing
makes its gate fail, not skip.

## Rules

- The libraries depend on the standard library only; Catch2, nlohmann-json and Google Benchmark are
  dev dependencies (vcpkg feature `dev`).
- No public name beyond [`docs/API.md`](docs/API.md).
- Headers under `include/` are 7-bit ASCII (ADR-0001 §6).
- Never weaken, skip or delete a test to get CI green. A skipped parity row names its deviation
  from `shared/spec/PARITY.md`.

## Commits and PRs

- [Conventional Commits](https://www.conventionalcommits.org/): `type(scope): summary`, with the
  task ID first when there is one (`feat(nepdate): C1-04 serial conversion`).
- One phase per PR. Fill in the PR template, including "What to look at first" and the
  `tools/ci.cmake` line.
- Prose, comments and commit messages follow
  [`.claude/skills/writing-style/SKILL.md`](.claude/skills/writing-style/SKILL.md): no em dashes,
  no filler, exact numbers and IDs, and no AI attribution lines.

By contributing you agree that your contribution is licensed under the MIT license
([LICENSE](LICENSE)).
