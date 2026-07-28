# ISSUES — Bug and Task Tracker

This document is the running log of bugs, compatibility items, and
modernisation/feature tasks for mcsh. Resolved items are retained for
historical reference.

---

## Open — C workflow compiler feature

See [`docs/compiler/INDEX.md`](docs/compiler/INDEX.md) for the full program plan.

### CW-001 Implement `run file.c` MVP (M1)

**Priority**: high  
**Phase**: P1  
**Files**: `sh.cworkflow.c` (new), `sh.init.c`, `sh.decls.h`, `Makefile.in`  
**Description**: Add `dorun()` builtin. Compile-if-needed, execute, return
child exit code. Reject `.mcsh` input with golden error message.  
See [`docs/compiler/MILESTONES.md#m1`](docs/compiler/MILESTONES.md),
[`docs/compiler/TODO.md#p1`](docs/compiler/TODO.md).

### CW-002 Implement `compile` unit pipeline (M2)

**Priority**: medium  
**Phase**: P2  
**Files**: `sh.cworkflow.c`, `sh.init.c`, `sh.decls.h`  
**Description**: `docompile()` with fast include scanner, dep hash, full
5-component cache key, `index.db` persistence.  
See [`docs/compiler/MILESTONES.md#m2`](docs/compiler/MILESTONES.md).

### CW-003 Implement `build` project pipeline (M3)

**Priority**: medium  
**Phase**: P3  
**Files**: `sh.cworkflow.c`, `sh.init.c`, `sh.decls.h`  
**Description**: `dobuild()` with source discovery, parallel compile,
link orchestration.  
See [`docs/compiler/MILESTONES.md#m3`](docs/compiler/MILESTONES.md).

### CW-004 Cache hardening (M4)

**Priority**: medium  
**Phase**: P4  
**Description**: Toolchain identity in cache key; LRU eviction; corruption
recovery from sidecars.  
See [`docs/compiler/CACHE-DESIGN.md`](docs/compiler/CACHE-DESIGN.md).

---

## Open — Shell features

*(Add new shell feature issues here.)*

---

## Resolved — Modernisation and bug fixes

| ID | Summary | Fix location |
|----|---------|-------------|
| R-001 | Unicode wide-char fix (Round 9): `mbtowc` loops over-read | `sh.lex.c`, `sh.dol.c` |
| R-002 | Short-circuit eval: unset var expansion in `&&`/`\|\|` | `sh.dol.c` |
| R-003 | `getn()` overflow with `@ x = (1 << 63)` | `sh.exp.c` |
| R-004 | Shift operator signed UB | `sh.exp.c` |
| R-005 | `crypt` link failure with libxcrypt | `configure.ac` |
| R-006 | `vms.termcap.c` OOB scans, tgoto buffer, octal | `vms.termcap.c` |
| R-007 | `acaux/install-sh` name pattern edge cases | `acaux/install-sh` |
| R-008 | `m4/lib-prefix.m4` `dn;` comment typo | `m4/lib-prefix.m4` |
| R-009 | `m4/po.m4` C# DLL cleanup wrong target | `m4/po.m4` |
| R-010 | `configure.ac` patchlevel leading zero | `configure.ac` |
| R-011 | `configure.ac` baseline version string | `configure.ac` |
| R-012 | `sh.func.c` doif truncation | `sh.func.c` |
| R-013 | `ed.defns.c` catalog collision (predict-accept) | `ed.defns.c` |
| R-014 | `ed.screen.c` SGR desync | `ed.screen.c` |
| R-015 | `ed.refresh.c` ghost SGR | `ed.refresh.c` |
| R-016 | `ed.inputl.c` double Refresh on CC_NORM | `ed.inputl.c` |
| R-017 | `tc.prompt.c` marker mtime tracking | `tc.prompt.c` |
| R-018 | `%j` prompt token counts wrong | `tc.prompt.c` |
| R-019 | `dch-template.in` distribution | `dch-template.in` |
