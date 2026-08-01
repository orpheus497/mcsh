# PLAN — mcsh Execution Plan

This document tracks the high-level phased execution plan for mcsh.
For detailed task tracking, see `ISSUES.md`.

---

## Phase 1 — Baseline and modernisation (complete)

- [x] Fork and consolidate tcsh + etcsh into mcsh
- [x] Rename binary and shell identity to `mcsh` / Modern C Shell
- [x] Fish-style predictive autocomplete (`set predict`)
- [x] Interactive syntax highlighting (`set syntax`)
- [x] Git branch prompt escapes (`%g`, `%G`)
- [x] zsh-style directory stack display and `cd -N`
- [x] `function` builtin
- [x] Pipe-to-variable (`echo foo | set x`)
- [x] Expression short-circuit fix (`$?a && "$a" != ""`)
- [x] Unicode / wide-character fix (Round 9)
- [x] Various arithmetic, SGR, and build-system fixes (see `ISSUES.md`)

---

## Phase 2 — C workflow commands (in progress)

The goal is to add first-class C workflow commands (`compile`, `build`, `run`)
to mcsh while preserving all existing shell semantics.

**Current status**: architecture analysis and program plan complete (P0).
No runtime implementation yet.

See the full compiler program plan in `docs/compiler/`:

- [`docs/compiler/INDEX.md`](docs/compiler/INDEX.md) — navigation hub and reading order
- [`docs/compiler/MASTER-PLAN.md`](docs/compiler/MASTER-PLAN.md) — vision, roadmap P0–P6
- [`docs/compiler/ARCHITECTURE.md`](docs/compiler/ARCHITECTURE.md) — integration points
- [`docs/compiler/MILESTONES.md`](docs/compiler/MILESTONES.md) — M0–M6 acceptance criteria
- [`docs/compiler/TODO.md`](docs/compiler/TODO.md) — granular checkbox backlog

### Command contract summary

- `.mcsh` scripts execute directly and require no changes.
- `compile` — compile C translation units.
- `build` — compile + link C projects.
- `run` — compile-if-needed + execute C target (flagship command).
- `run script.mcsh` emits a clear error; scripts are not C files.

### Immediate next milestone

**M1 ✓ shipped**: `run` builtin implemented in `sh.cworkflow.c`. Supports single
C file and directory/project targets, object and binary caching, `--clean`, `-v`,
`--` arg separator, and `.mcsh` guard.

**M2**: `compile` unit pipeline — `docompile()` with include scanner, dep hash,
and full 5-component cache key. See `docs/compiler/MILESTONES.md#m2`.

---

## Phase 3 — Future (not yet planned)

- REPL / interactive C expression evaluation
- Networked shared build cache
- Deeper `.mcsh` + C integration (JIT scripting, P6)

---

## Cross-references

- Bug/task tracker: `ISSUES.md`
- Compiler program plan: `docs/compiler/INDEX.md`
- Architecture baseline: `docs/c-workflow-analysis.md`
- Command contract: `docs/c-commands-spec.md`
