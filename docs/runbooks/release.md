# Runbook: release `nepdate-cpp`

Decided in ADR-0005. Owner: maintainer. Approver: a second person (the `production` environment's
required reviewer). One tag releases both libraries, `nepdate` and `nepdate-patro`.

## Before tagging

1. `main` is green on every CI job, including `packages` and `isolation`. `cmake -P tools/ci.cmake`
   passes locally.
2. `CHANGELOG.md` has `## [X.Y.Z] - YYYY-MM-DD`, moved out of `[Unreleased]`.
3. The version is X.Y.Z in `CMakeLists.txt`, `include/nepdate/version.hpp`, `ports/*/vcpkg.json` and
   `conan/*/conanfile.py`:
   `cmake -DVERSION=X.Y.Z -P tools/release_check.cmake` passes.
4. If the calendar month lengths changed since the last release, the changelog says so: the
   fingerprint check (ADR-0002 §7) forces users to rebuild the patro with the new core.

## Releasing

```bash
git tag -s vX.Y.Z -m "vX.Y.Z"
git push origin vX.Y.Z
```

`release.yml` runs preflight and CI, waits for the `production` approval, then creates the GitHub
Release with the changelog section as notes and attaches `nepdate-cpp-X.Y.Z.tar.gz` with its
SHA-256 and SHA-512 and a provenance attestation.

## After releasing

1. Download the archive from the Release page and check its SHA-512 against the attached file.
2. **vcpkg** (first release: new ports; later: version bumps). In a fork of `microsoft/vcpkg`:
   copy `ports/nepdate` and `ports/nepdate-patro`, set `REF vX.Y.Z` and the archive `SHA512`, run
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
