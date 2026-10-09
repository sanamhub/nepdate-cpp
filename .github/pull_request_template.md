<!-- What this changes and why, in two or three sentences. Name the task IDs (C1-04) or the
     issue (Closes #12). -->

## What to look at first

-

## How it was tested

## Checks

- [ ] `cmake -P tools/ci.cmake` passes; last line:
- [ ] Every acceptance criterion of the task has a test, or a line here saying why it can't
- [ ] No public name beyond `docs/API.md`; the libraries depend only on the standard library
- [ ] Allocation tests still report 0 for the paths in ADR-0002 §6
- [ ] `CHANGELOG.md` `[Unreleased]` updated, if a user would notice
