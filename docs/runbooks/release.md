# Runbook: release `nepdate-cpp`

Decided in ADR-0005. Owner: maintainer. Approver: a second person (the `production` environment's
required reviewer). One tag releases both libraries, `nepdate` and `nepdate-patro`.

## Before tagging

1. `main` is green on every CI job, including `packages` and `isolation`. `cmake -P tools/ci.cmake`
   passes locally.
2. `CHANGELOG.md` has `## [X.Y.Z] - YYYY-MM-DD`, moved out of `[Unreleased]`.
3. The version is X.Y.Z in `CMakeLists.txt`, `include/nepdate/version.hpp`,
   `patro/CMakeLists.txt`, `ports/*/vcpkg.json` and `conan/*/conanfile.py`:
   `cmake -DVERSION=X.Y.Z -P tools/release_check.cmake` passes. For a pre-release such as
   `X.Y.Z-beta.1`, see [Pre-releases](#pre-releases).
4. If the calendar month lengths changed since the last release, the changelog says so: the
   fingerprint check (ADR-0002 §7) forces users to rebuild the patro with the new core.

## Dry run

`release.yml` also runs from a `workflow_dispatch` with a `version` input. That run does preflight
(version and changelog agree) and the full CI, and the `release` job stays skipped because there is
no tag. Use it to check a prepared version before tagging; the AC for the workflow is a green
preflight and CI with `release` skipped.

## Releasing

```bash
git tag -s vX.Y.Z -m "vX.Y.Z"
git push origin vX.Y.Z
```

`release.yml` runs preflight and CI, waits for the `production` approval, then creates the GitHub
Release. The notes are the changelog section plus an install block (FetchContent with the archive
URL and its SHA-256) and a verification block. The assets are exactly `nepdate-cpp-X.Y.Z.tar.gz`,
`SHA256SUMS` and `SHA512SUMS`, and the archive gets a provenance attestation. Then it calls
`verify-published.yml`, which downloads the assets, checks both sums and `gh attestation verify`,
and builds `tests/consumer` on Linux, macOS and Windows twice: through FetchContent from the
release URL, and through `find_package` after installing the archive. `verify-published.yml` can
also be run by hand from the Actions tab for any released version.

## Pre-releases

A tag such as `vX.Y.Z-beta.1` goes through the same workflow and becomes a GitHub pre-release.
`project()` holds numbers only, so the version is split:

| Location | Holds |
|---|---|
| `project()` in both CMakeLists, `NEPDATE_VERSION_MAJOR/MINOR/PATCH` | `X.Y.Z` |
| `NEPDATE_VERSION_PRERELEASE` | `beta.1` (empty for a stable version) |
| `NEPDATE_VERSION_STRING`, Conan `version` and every `requires("nepdate/...")` | `X.Y.Z-beta.1` |
| vcpkg ports | `"version-semver": "X.Y.Z-beta.1"`, and the patro port's `version>=` floor |
| `CHANGELOG.md` | `## [X.Y.Z-beta.1] - YYYY-MM-DD` |

A stable version uses `"version"` in the ports and an empty pre-release. `release_check.cmake`
checks all of it, and a CTest case requires the stable `X.Y.Z` to fail while the files say beta, so
a beta can't go out under the stable tag.

## After releasing

1. `verify-published` is green. If it is not, the release stays up (the tag can't move) and the fix
   is a new version.
2. **vcpkg** (first release: new ports; later: version bumps). In a fork of `microsoft/vcpkg`:
   copy `ports/nepdate` and `ports/nepdate-patro`, set `REF vX.Y.Z` and the `SHA512` of GitHub's tag
   archive, which `vcpkg_from_github` downloads (not the attached `nepdate-cpp-X.Y.Z.tar.gz`, so not
   the value in `SHA512SUMS`; `vcpkg install` with `SHA512 0` prints the real one), run
   `vcpkg install nepdate nepdate-patro` locally, then `vcpkg x-add-version --all`, and open the PR.
   Answer review comments; the merge can take days.
3. **Conan Center.** In a fork of `conan-io/conan-center-index`: copy `conan/nepdate` and
   `conan/nepdate-patro` to `recipes/<name>/all/`, add `conandata.yml` with the archive URL and
   SHA-256 and `recipes/<name>/config.yml` with the version, run `conan create` for both, open the
   PR. The first PR for a new recipe goes through the Conan team's access request.
4. Add an empty `## [Unreleased]` to the changelog.
5. Add the release to the hub `README.md` port table if it isn't listed.

## Rolling back

Never move or delete the tag: vcpkg and Conan pin the archive hash. Fix forward with `vX.Y.(Z+1)`,
say in the changelog which version to avoid and why, and update both package-manager PRs (or open
new ones) to the fixed version.
