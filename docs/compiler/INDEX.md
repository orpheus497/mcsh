# docs/compiler — Navigation Index

> **Status: planning / documentation only.**
> No compile/build/run runtime is implemented yet.
> Normal `.mcsh` scripts continue to execute directly and require no changes.

This directory contains the complete program plan for introducing first-class
C workflow commands (`compile`, `build`, `run`) into mcsh.

---

## Reading order for new contributors

Start here and read in the order listed:

| # | Document | What you will learn |
|---|----------|---------------------|
| 1 | [MASTER-PLAN.md](MASTER-PLAN.md) | Vision, scope, non-goals, phased roadmap (P0–P6) |
| 2 | [ARCHITECTURE.md](ARCHITECTURE.md) | How mcsh executes today; where integration hooks go |
| 3 | [DATA-MODEL.md](DATA-MODEL.md) | Core tables, arenas, IDs, invariants, serialization |
| 4 | [PIPELINES.md](PIPELINES.md) | Step-by-step stage behavior for compile/build/run |
| 5 | [CLI-SPEC.md](CLI-SPEC.md) | Command syntax, option matrix, error rules |
| 6 | [DEPENDENCY-DISCOVERY.md](DEPENDENCY-DISCOVERY.md) | Include scanning and dependency graph |
| 7 | [CACHE-DESIGN.md](CACHE-DESIGN.md) | Cache key composition, artifact layout, pruning |
| 8 | [TEST-PLAN.md](TEST-PLAN.md) | Unit/integration/regression/perf test matrices |
| 9 | [MILESTONES.md](MILESTONES.md) | Milestone tasks and acceptance criteria |
| 10 | [TODO.md](TODO.md) | Granular checkbox backlog mapped to source files |
| 11 | [RISKS.md](RISKS.md) | Technical and product risks with mitigations |
| 12 | [DECISIONS.md](DECISIONS.md) | ADR-style decision log |

---

## Quick-reference links

- **Core contract** → [MASTER-PLAN.md § Command contract](MASTER-PLAN.md#command-contract)
- **MVP milestone** → [MILESTONES.md § M1](MILESTONES.md#m1-run-filec-mvp)
- **Immediate next TODO** → [TODO.md § Phase-P1 queue](TODO.md#p1-run-filec-mvp)
- **Where to add builtins** → [ARCHITECTURE.md § Integration points](ARCHITECTURE.md#integration-points)
- **Cache key algorithm** → [CACHE-DESIGN.md § Key composition](CACHE-DESIGN.md#key-composition)
- **Dependency scanning** → [DEPENDENCY-DISCOVERY.md § Include scanning](DEPENDENCY-DISCOVERY.md#include-scanning-strategy)

---

## Related docs outside this directory

| Document | Relationship |
|----------|-------------|
| [`docs/c-workflow-analysis.md`](../c-workflow-analysis.md) | Earlier architecture baseline analysis — superseded in detail by `ARCHITECTURE.md` but retained as reference |
| [`docs/c-commands-spec.md`](../c-commands-spec.md) | Earlier command contract sketch — superseded in detail by `CLI-SPEC.md` and `PIPELINES.md` |
| [`README.md`](../../README.md) | Top-level README with compiler section linking here |
| [`PLAN.md`](../../PLAN.md) | Project-wide execution plan with compiler phase references |
| [`ISSUES.md`](../../ISSUES.md) | Bug/task tracker with compiler work items |
