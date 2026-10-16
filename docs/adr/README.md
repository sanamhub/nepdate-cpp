# Architecture Decision Records (C++ port)

Decisions for this repository only. Decisions that bind every port are in the hub
(`sanamhub/nepdate`, `docs/adr/`), in particular hub ADR-0004 (patro in a separate package). Immutable
once Accepted; supersede with a new ADR.

Edited on 2026-10-16 to remove links to internal plans before the first public release; no decision changed.

| ADR | Title | Status |
|---|---|---|
| [0001](0001-layout-toolchain-and-shared-data.md) | Layout, toolchain, packaging and shared data | Accepted |
| [0002](0002-representation-and-performance.md) | Date representation, tables and performance | Accepted |
| [0003](0003-api-conventions-errors-and-parity.md) | API conventions, errors and parity mapping | Accepted |
| [0004](0004-testing-and-quality-gates.md) | Testing and quality gates | Accepted |
| [0005](0005-releases.md) | Releases | Accepted |
| [0006](0006-result-rvalue-accessors.md) | `result<T>` accessors return by value on an rvalue (amends 0003) | Accepted |
