# ADR-0006: `result<T>` accessors return by value on an rvalue

- Status: Accepted
- Date: 2026-10-15
- Deciders: Sanam Pakuwal
- Amends: ADR-0003 section 2 (the `result<T>` interface)

## Context

`result<T>::value()` and `operator*` return `const T&`, as `std::expected` does. In C6 the
sanitizer gate found two tests that did

```cpp
for (const date d : nepdate::date_range::of_year(2081).value()) { ... }
```

The range-for binds `auto&& range = of_year(2081).value()`, a reference into the temporary
`result`, which dies at the end of that statement: a stack use after scope. Before C++23 (P2718)
the temporary's lifetime is not extended, and this library supports C++20. It is the most
natural way to write the loop, so users will write it too.

Every `T` in this library is small and trivially copyable (`date` 4 bytes, `date_range` 8,
`fiscal_year` 4, `int`), and `result<T>` already requires trivially copyable `T`.

## Decision

Add rvalue overloads that return `T` by value; keep the lvalue ones:

```cpp
constexpr const T& value() const&;
constexpr T value() &&;                         // also const&&
constexpr const T& operator*() const& noexcept;
constexpr T operator*() && noexcept;
```

`operator->` stays as is (it can't return by value). The same applies to the 4-byte
`result<date>` specialisation. A `static_assert(sizeof(T) <= 16)` in `result<T>` keeps the copy
cheap; a larger `T` would need a new decision.

## Consequences

- `for (d : date_range::of_year(y).value())` and `for (d : *date_range::of_year(y))` are safe in
  C++20: the loop binds a prvalue, whose lifetime is extended.
- Code that took `const T&` from an rvalue result and kept it now gets a copy; for these types the
  copy is a register move. No ABI concern: everything is header-only except patro, which doesn't
  return `result`.
- `docs/API.md` shows the four overloads.
