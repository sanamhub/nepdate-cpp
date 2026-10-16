<!-- What this changes and why, in two or three sentences. -->

## What to look at first

-

## How it was tested

## Checks

- [ ] `cmake -P tools/ci.cmake` passes; last line:
- [ ] Tests added or updated for the change; the shared vector suites pass
- [ ] No public name beyond `docs/API.md`; the libraries depend only on the standard library
- [ ] Allocation tests still report 0 for the paths in ADR-0002 section 6
- [ ] `CHANGELOG.md` `[Unreleased]` updated, if a user would notice
