# C workflow analysis baseline (`compile`, `build`, `run`)

Status: **P1 complete** — `run` (compile-if-needed + execute) is implemented in `sh.cworkflow.c`.
`compile` (P2) and `build` (P3) are planned future phases; this document remains the design baseline for both.

## Scope and contract

- `.mcsh` scripts continue to execute via existing shell/script paths (`mcsh script.mcsh`, shebang execution, `source`) and must not require `run`.
- `compile`, `build`, and `run` are reserved for C workflows only.
- Command surface stays flat (three primary verbs), with `run` as the flagship UX.

## Current command parsing and dispatch architecture

### Input loop to execution

1. `process()` in `sh.c` drives the read/parse/execute loop (`sh.c:1910`).
2. `lex()` in `sh.lex.c` tokenizes input, then `syntax()` in `sh.parse.c` builds the command AST (`sh.parse.c:206`).
3. `execute()` in `sh.sem.c` walks AST nodes and dispatches command nodes (`sh.sem.c:80`, command branch around `sh.sem.c:272`).

### Builtin lookup and invocation

- Builtin registry is `bfunc[]` in `sh.init.c` (`sh.init.c:42`).
- Lookup is binary search via `isbfunc()` in `sh.func.c` (`sh.func.c:66`).
- Builtin invocation is `func()` in `sh.func.c` (`sh.func.c:126`), called from `execute()` (`sh.sem.c:619`).
- External commands fall through to `doexec()` in `sh.exec.c` (`sh.exec.c:144`).

### Key implication for new builtins

Because builtin resolution is binary-search over `bfunc[]`, all `compile`, `build`, and `run`
entries must be inserted in sorted order in `sh.init.c`. `run` is already inserted (P1); `compile`
and `build` will follow in P2/P3.

## Current `.mcsh` script execution path

### File script invocation

- Startup and script sourcing use `srcfile()` in `sh.c` (`sh.c:1472`), which opens a file and calls `srcunit()` (`sh.c:1654`).
- `srcunit()` saves shell state (`st_save()`), runs `process(0)`, then restores state (`st_restore()`).
- `source` builtin entry point is `dosource()` in `sh.c` (`sh.c:2111`) and reuses the same `srcfile()`/`srcunit()` path.

### Semantics to preserve

- Script execution is already first-class and independent of any `run` command.
- Existing `.mcsh` behavior should remain unchanged while adding C-specific builtins.

## Integration points with minimal disruption

### 1) Builtin declaration and registration

- `dorun` declared in `sh.decls.h`; implemented in `sh.cworkflow.c` (P1 shipped).
- `docompile` and `dobuild` declarations and implementations are planned for P2/P3.
- All names registered in sorted order in `bfunc[]` in `sh.init.c`.

### 2) Dispatch behavior

- Dispatch flows through existing builtin path (`isbfunc` -> `func` -> builtin function body).
- No special-casing in parser stages (`sh.parse.c`).

### 3) `.mcsh` script path isolation

- `.mcsh` scripts remain on the existing script/source path (`srcfile`/`srcunit`/`process`).
- `run` validates input and fails fast for `.mcsh` paths with a clear guidance error.

## Constraints and risks from current parser/runtime design

1. **Sorted builtin table requirement**  
   `isbfunc()` assumes `bfunc[]` sorted order (binary search). Misordering silently breaks command lookup.

2. **Builtin escape behavior**  
   `isbfunc()` ignores quoted first characters (`sh.func.c:75-80`), preserving legacy escape semantics.

3. **Execution model complexity**  
   `execute()` has nuanced forking/no-fork behavior for builtins, pipes, and job control. C workflow builtins should avoid introducing ad-hoc process management in early phases.

4. **Error messaging consistency**  
   User-facing diagnostics are centralized via `stderror(...)`; new commands should use established patterns for predictable UX.

5. **No parser-level C mode today**  
   Current parser handles shell grammar only. C workflow support should remain command-driven, not parser-driven.

## Phased roadmap (analysis -> MVP)

### Phase 1: `run file.c` and `run .` MVP — **delivered (P1)**

- `run` builtin registered in `bfunc[]` (`sh.init.c`) and implemented in `sh.cworkflow.c`.
- Accepts `.c` source files and directory/project targets; rejects `.mcsh` with a guidance error.
- Cache key derived from sorted source/header content hash (`project_hash`) + compiler identity (`cc_hash`).
- Object and binary artifacts stored under `~/.mcsh_cache/cworkflow/`.
- Executes compiled binary via fork+exec+waitpid; propagates child exit status to shell.

### Phase 2: `compile` unit pipeline + dependency tracking

- Add `compile` builtin for translation units.
- Parse/track include dependencies and per-unit compile options.
- Cache object artifacts and diagnostics by key.
- Emit machine-readable metadata for reuse by `build`/`run`.

### Phase 3: `build` project graph + link orchestration

- Add `build` builtin for multi-unit targets.
- Construct dependency DAG from units + include graph.
- Schedule incremental parallel compiles.
- Perform link orchestration and artifact materialization.

## Proposed data-oriented core model (for caching + incremental rebuilds)

Use flat tables/arenas with stable IDs, not pointer-heavy object graphs.

### Suggested tables

1. `file_table`
   - `file_id`, canonical path, mtime, size, content hash.
2. `unit_table`
   - `unit_id`, primary source `file_id`, profile/flag-set id, language mode.
3. `dep_edge_table`
   - `(unit_id -> file_id)` include edges, include-kind, discovered-at timestamp.
4. `artifact_table`
   - artifact id, kind (`obj`/`bin`), path, producer command, cache key, creation time.
5. `command_profile_table`
   - normalized option vectors for compile/link/run profiles.
6. `diagnostic_table`
   - file id, line/column range, severity, message, toolchain source.
7. `job_table`
   - runnable job records for compile/link/execute stages and status transitions.

### Arena usage

- interned string arena (paths, flags, messages).
- transient parse/discovery arena per command invocation.
- persistent metadata storage for cache index snapshots.

### Cache/incremental invariants

- deterministic key: source + resolved deps + normalized flags + target triple + compiler identity.
- invalidate only affected units via reverse dep edges.
- keep command output derivable from table state for reproducibility.

## Historical design note (P1 coding baseline)

The section below was the immediate next coding milestone when this document was
authored (P0 planning phase).  It is preserved as historical context; Phase 1 is
now complete.

> Implement Phase 1 MVP: `run file.c` compile-if-needed + execute, using builtin
> registration in `sh.init.c` and builtin dispatch in `sh.sem.c`/`sh.func.c`,
> while leaving `.mcsh` script execution path untouched.
