# Security policy

`nepdate-cpp` is two libraries with no dependencies, no network access and no file access. A
security problem is most likely undefined behaviour or a read past a table on hostile input to
`parse` or `parse_lenient`, or a problem in a workflow (`.github/workflows/`).

## Supported versions

The latest `0.x` release gets fixes. A bad release is fixed by a new patch version; published tags
never move (ADR-0005).

## Reporting a vulnerability

Report privately through
[GitHub security advisories](https://github.com/sanamhub/nepdate-cpp/security/advisories/new).
Don't open a public issue. Expect a first reply within a week. Say which version, compiler and
function are affected and give an input that reproduces it.

A wrong date, holiday or tithi isn't a security problem: report it in the hub,
[`sanamhub/nepdate`](https://github.com/sanamhub/nepdate/issues/new/choose).
