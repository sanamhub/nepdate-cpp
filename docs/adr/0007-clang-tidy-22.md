# ADR-0007: The tidy gate runs clang-tidy 22

- Status: Accepted
- Date: 2026-11-17
- Deciders: Sanam Pakuwal
- Amends: ADR-0004 section 1 (gate 6) and section 4 (the `lint` job)

## Context

ADR-0004 ran gate 6 in the `lint` job with the Clang 18 tools. Clang 22 is the newest stable LLVM
release and already builds the code in the `clang`, `sanitizers` and `fuzz` jobs. Its clang-tidy
has checks that clang-tidy 18 did not: missing parentheses in mixed arithmetic, designated
initializers for aggregates, `std::cmp_*` for signed and unsigned comparisons, ranges algorithms,
unchecked container access and more.

The C++20 standard, the oldest supported compilers and the compiler matrix are not part of this
decision and do not change.

## Decision

- Gate 6 runs clang-tidy 22 with `.clang-tidy`, warnings as errors. `tools/ci.cmake` skips the gate,
  or fails it with `STRICT`, when plain `clang-tidy` is not version 22.
- A new `tidy` job on `ubuntu-26.04` runs gate 6 with `clang-tidy-22` and `clang-22` from the
  Ubuntu archive, the same way the `sanitizers` job gets Clang 22. The `lint` job keeps gates 1, 2,
  3 and 10 with the Clang 18 tools.
- The new findings are fixed in the code. Three checks are disabled, each with its reason in
  `.clang-tidy`:
  - `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`: the tables are indexed after a
    range check (ADR-0002 section 3), the reason `pro-bounds-constant-array-index` is already off.
  - `bugprone-invalid-enum-default-initialization`: `errc{}` is the documented "no error" value; a
    zero enumerator would change the public enum.
  - `bugprone-derived-method-shadowing-base-method`: the range views define `size()` for
    `std::ranges::view_interface`, a CRTP base that expects it.

## Consequences

- Contributors need clang-tidy 22 for the tidy gate; Ubuntu 26.04 and the LLVM apt repository have
  it.
- The fixes change no behaviour, public API or ABI: added parentheses keep the order of operations,
  designated initializers name the members the positional ones set, and `std::cmp_greater` replaces
  one signed and unsigned comparison after a check that the value is not negative.
- `tools/codegen` writes the month spelling table with designated initializers, so the generated
  header passes the same checks.
