# ADR-0005: Releases

- Status: Accepted
- Date: 2026-10-08
- Deciders: Sanam Pakuwal

## Decision

- One tag releases both libraries, `nepdate` and `nepdate-patro`, with the same version (hub
  ADR-0004 §3). A data update that only changes the patro is still a new patch version of both.
- Semver tags `vX.Y.Z` on `main`, signed (SSH or GPG). Stay on 0.x until the API settles.
- The version lives in four places that must agree, checked by
  `cmake -DVERSION=X.Y.Z -P tools/release_check.cmake`: `project(nepdate VERSION ...)` in
  `CMakeLists.txt`, `NEPDATE_VERSION_*` in `include/nepdate/version.hpp`, the two vcpkg overlay
  ports (`ports/*/vcpkg.json` `version`) and the two Conan recipes (`conan/*/conanfile.py`
  `version`), plus a `## [X.Y.Z] - YYYY-MM-DD` section in `CHANGELOG.md` (Keep a Changelog).
- `release.yml` on tag `v*`: preflight (release check, tag equals `v` + version), the full CI
  workflow, then `environment: production` (required reviewer) creates the GitHub Release with
  notes from the changelog section and attaches the source archive
  `nepdate-cpp-X.Y.Z.tar.gz` (from `git archive`, including `shared/`) and its SHA-256 and SHA-512.
  The archive is attested with `actions/attest-build-provenance`.
- **Package managers, after the GitHub Release, maintainer tasks:**
  - vcpkg: a PR to `microsoft/vcpkg` adding `ports/nepdate` and `ports/nepdate-patro` (copied from
    the overlay ports, `REF vX.Y.Z`, `SHA512` of the release archive) and the version database
    entries (`vcpkg x-add-version`).
  - Conan: a PR to `conan-io/conan-center-index` adding `recipes/nepdate` and
    `recipes/nepdate-patro` (copied from `conan/`, `conandata.yml` with the archive URL and
    SHA-256).
  Both reviews take days to weeks and may ask for changes; the next release uses the same flow
  with version bumps only.
- **Never move or delete a published tag**: vcpkg and Conan pin the archive hash. A bad release is
  fixed with a new patch version; the changelog says which version to avoid.
- The weekly `shared-sync` PR, once merged, is released as a patch version.

## Consequences

- The library is usable straight from the GitHub Release (CMake `FetchContent` or
  `find_package` after `cmake --install`) before the package-manager PRs merge.
- A release touches two external repositories, and each needs a pull request from the maintainer.
