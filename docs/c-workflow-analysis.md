# C workflow analysis baseline (`compile`, `build`, `run`)

Status: analysis/design only. No runtime behavior changes are introduced in this document.

## Scope and contract

- `.mcsh` scripts continue to execute via existing shell/script paths (`mcsh script.mcsh`, shebang execution, `source`) and must not require `run`.
- `compile`, `build`, and `run` are reserved for C workflows only.
- Command surface stays flat (three primary verbs), with `run` as the flagship UX.

## Current command parsing and dispatch architecture

### Input loop to execution

1. `process()` in `/home/runner/work/mcsh/mcsh/sh.c` drives the read/parse/execute loop (`sh.c:1910`).
2. `lex()` in `sh.lex.c` tokenizes input, then `syntax()` in `/home/runner/work/mcsh/mcsh/sh.parse.c` builds the command AST (`sh.parse.c:206`).
3. `execute()` in `/home/runner/work/mcsh/mcsh/sh.sem.c` walks AST nodes and dispatches command nodes (`sh.sem.c:80`, command branch around `sh.sem.c:272`).

### Builtin lookup and invocation

- Builtin registry is `bfunc[]` in `/home/runner/work/mcsh/mcsh/sh.init.c` (`sh.init.c:42`).
- Lookup is binary search via `isbfunc()` in `/home/runner/work/mcsh/mcsh/sh.func.c` (`sh.func.c:66`).
- Builtin invocation is `func()` in `sh.func.c` (`sh.func.c:126`), called from `execute()` (`sh.sem.c:619`).
- External commands fall through to `doexec()` in `/home/runner/work/mcsh/mcsh/sh.exec.c` (`sh.exec.c:144`).

### Key implication for new builtins

Because builtin resolution is binary-search over `bfunc[]`, future `compile`, `build`, `run` entries must be inserted in sorted order in `sh.init.c`.

## Current `.mcsh` script execution path

### File script invocation

- Startup and script sourcing use `srcfile()` in `sh.c` (`sh.c:1472`), which opens a file and calls `srcunit()` (`sh.c:1654`).
- `srcunit()` saves shell state (`st_save()`), runs `process(0)`, then restores state (`st_restore()`).
- `source` builtin entry point is `dosource()` in `sh.c` (`sh.c:2111`) and reuses the same `srcfile()`/`srcunit()` path.

### Semantics to preserve

- Script execution is already first-class and independent of any `run` command.
- Existing `.mcsh` behavior should remain unchanged while adding C-specific builtins.

## Candidate integration points with minimal disruption

### 1) Builtin declaration and registration

- Add function declarations in `/home/runner/work/mcsh/mcsh/sh.decls.h`.
- Implement `docompile`, `dobuild`, `dorun` in `/home/runner/work/mcsh/mcsh/sh.func.c` (or a new tightly scoped C-workflow source file wired into build).
- Register names in sorted order in `bfunc[]` in `sh.init.c`.

### 2) Dispatch behavior

- Keep dispatch through existing builtin path (`isbfunc` -> `func` -> builtin function body).
- Do not special-case in parser stages (`sh.parse.c`) unless future syntax expansion is explicitly required.

### 3) `.mcsh` script path isolation

- Keep `.mcsh` scripts on existing script/source path (`srcfile`/`srcunit`/`process`).
- `run` should validate input and fail fast for `.mcsh` paths with a clear guidance error.

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

### Phase 1: `run file.c` MVP (compile-if-needed + execute)

- Add `run` builtin with C-file validation (`.c` input only in MVP).
- Build deterministic cache key from source hash + flags + target + compiler identity.
- If cache miss/stale: compile and link executable artifact.
- Execute artifact and return child exit status.
- Explicit misuse error for `.mcsh` input (`run` is C-only).

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

## Immediate next coding milestone

Implement Phase 1 MVP: `run file.c` compile-if-needed + execute, using builtin registration in `sh.init.c` and builtin dispatch in `sh.sem.c`/`sh.func.c`, while leaving `.mcsh` script execution path untouched.
