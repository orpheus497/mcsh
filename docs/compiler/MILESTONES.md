# Milestones

> **Status: planning / documentation only.**
> Milestones define acceptance criteria for each implementation phase.

---

## M0 — Foundation complete (P0 exit)

**Objective**: All planning artifacts present; codebase annotated; build system confirmed.

### Tasks

- [x] `docs/c-workflow-analysis.md` authored
- [x] `docs/c-commands-spec.md` authored
- [x] `docs/compiler/` suite authored (this PR)
- [x] `TODO(mcsh-c-workflow)` markers present in `sh.init.c`, `sh.sem.c`, `sh.func.c`
- [x] `Makefile.in` reviewed; confirm adding `sh.cworkflow.c` requires only `SRCS` edit
- [x] BSD/Apache-2.0 license confirmed for clang as backend tool
- [x] No behavior changes (all `t001`–`t018` tests pass on current code)

### Acceptance criteria

- All planning docs are present and internally consistent.
- Source file annotations are non-functional (comments only).
- Test suite passes without modification.

### Rollback

Delete annotation comments. Zero code change means zero risk.

---

## M1 — `run file.c` MVP (P1 exit)

**Objective**: `run hello.c` compiles and executes a single C file.

### Tasks

- [ ] Create `sh.cworkflow.c` with `dorun()` stub
- [ ] Add `sh.cworkflow.o` to `SRCS` in `Makefile.in`
- [ ] Add `extern void dorun(Char **, struct command *)` to `sh.decls.h`
- [ ] Insert `{ "run", dorun, 1, INF }` in sorted position in `bfunc[]` (`sh.init.c`)
- [ ] `dorun()`: validate argument is `.c` file (not `.mcsh`, not directory)
- [ ] `dorun()`: find compiler via `$mcsh_cc` or PATH scan (clang → cc)
- [ ] `dorun()`: compute content hash (SHA-256 of source file)
- [ ] `dorun()`: create `~/.mcsh_cache/cworkflow/{objects,binaries}` directories
- [ ] `dorun()`: check binary cache key; skip compile on hit
- [ ] `dorun()`: fork + exec compiler on cache miss; capture stderr
- [ ] `dorun()`: fork + exec linker to produce binary; capture stderr
- [ ] `dorun()`: `execvp` compiled binary with `--` args
- [ ] `dorun()`: emit `.mcsh` guard error with exact golden message
- [ ] Tests T100–T106, I100–I103 pass
- [ ] All existing `t001`–`t018` tests still pass

### Acceptance criteria

```csh
echo 'int main(){puts("hello");return 0;}' > /tmp/h.c
run /tmp/h.c               # prints "hello", exits 0
run /tmp/h.c               # second run: no recompile, prints "hello", exits 0
run ~/.mcshrc              # exits 1 with .mcsh guard message
echo 'int main(){return 42;}' > /tmp/e.c
run /tmp/e.c; echo $status  # prints 42
```

### Risk checkpoints

- If `bfunc[]` insertion breaks binary search → regression tests catch it immediately.
- If `execvp` replaces shell process in non-forked context → investigate `execute()` fork behavior; may need `fork()` before exec in builtin.

### Rollback

Remove `sh.cworkflow.c`, revert `sh.init.c` and `sh.decls.h` additions,
remove `sh.cworkflow.o` from `Makefile.in`. Zero effect on shell behavior.

---

## M2 — `compile` unit pipeline (P2 exit)

**Objective**: Explicit compilation with incremental dep-tracked object caching.

### Tasks

- [ ] Add `docompile()` to `sh.cworkflow.c`
- [ ] Add `extern void docompile(Char **, struct command *)` to `sh.decls.h`
- [ ] Insert `{ "compile", docompile, 1, INF }` in sorted position in `bfunc[]`
- [ ] Implement `dep_scan_file()` — fast `#include` line scanner
- [ ] Implement `resolve_include()` — resolution order per CLI-SPEC §2.2
- [ ] Implement `compute_dep_hash()` — BFS transitive closure + sorted hash
- [ ] Implement full cache key (source + dep + profile + toolchain)
- [ ] Implement `index.db` read/write (load on command start; flush on success)
- [ ] Implement artifact storage under `objects/<prefix>/`
- [ ] Tests T110–T114, T130–T135, T140–T146 pass
- [ ] All existing tests still pass

### Acceptance criteria

```csh
compile src/foo.c -Iinclude     # produces object in cache
compile src/foo.c -Iinclude     # second run: no recompile (cache hit logged)
touch include/types.h
compile src/foo.c -Iinclude     # recompile triggered by header change
```

### Risk checkpoints

- `dep_scan_file()` must handle edge cases (empty files, BOM, CRLF, very long lines).
- `index.db` atomic write: verify `rename()` is atomic on target platforms.

### Rollback

Remove `docompile()`, revert `sh.init.c` and `sh.decls.h`.
`dorun()` (M1) continues to function independently.

---

## M3 — `build` project pipeline (P3 exit)

**Objective**: `build .` discovers, compiles, and links a multi-file C project.

### Tasks

- [ ] Add `dobuild()` to `sh.cworkflow.c`
- [ ] Add `extern void dobuild(Char **, struct command *)` to `sh.decls.h`
- [ ] Insert `{ "build", dobuild, 0, INF }` in sorted position in `bfunc[]`
- [ ] Implement `source_discover()` — recursive `*.c` walk with exclusion list
- [ ] Implement entry-point detection (`int main(` scan)
- [ ] Implement parallel compile scheduler using `job_table`
- [ ] Implement `link_target()` — collect objects, exec linker
- [ ] Tests T120–T123, I110–I113, I120–I122 pass
- [ ] All existing tests still pass

### Acceptance criteria

```csh
# 3-file project: main.c, util.c, util.h
build .              # produces ./build/app (or named binary)
touch util.c
build .              # only util.c recompiles; main.c skipped
```

### Risk checkpoints

- Parallel compilation: child process management must not interfere with
  mcsh job control. Use separate process group or waitpid loop.
- Link order: ensure object list is deterministic (sorted by file path).

### Rollback

Remove `dobuild()`, revert `sh.init.c` and `sh.decls.h`.
`dorun()` and `docompile()` continue to function.

---

## M4 — Cache hardening (P4 exit)

**Objective**: Robust caching across compiler version changes; LRU eviction;
corruption recovery.

### Tasks

- [ ] Encode `toolchain_id` (cc_hash + version) in every cache key
- [ ] Implement toolchain change detection; invalidate affected artifacts
- [ ] Implement LRU eviction triggered at `$mcsh_cache_max_bytes`
- [ ] Implement `index.db` corruption detection (magic + CRC32)
- [ ] Implement full index rebuild from sidecar `.meta` files on corruption
- [ ] Implement `--clean` flag for all three commands
- [ ] Tests T130 (cache invalidation on toolchain change) pass
- [ ] All existing tests still pass

### Acceptance criteria

```csh
export CC=clang-17
compile src/foo.c       # compiles with clang-17
export CC=clang-18
compile src/foo.c       # recompiles (toolchain changed)
```

---

## M5 — Developer experience polish (P5 exit)

**Objective**: C workflow commands feel native and fast.

### Tasks

- [ ] Completions for `.c` files in `complete.mcsh`
- [ ] `$mcsh_cflags` variable consulted by all commands
- [ ] `$mcsh_jobs` variable for parallel job count
- [ ] `--emit-db` outputs `compile_commands.json`
- [ ] Build progress: `[1/5] Compiling src/foo.c` line on stderr
- [ ] Benchmark scripts in `tests/bench_*.sh`
- [ ] `run` latency target < 50ms on cache hit (measured in bench)

---

## M6 — JIT scripting integration (P6 exit)

**Objective**: C units are first-class citizens inside `.mcsh` scripts.

### Tasks

- [ ] `run $source_var` works (variable expansion before C-file check)
- [ ] `build` reads `build.mcsh` for project-level configuration
- [ ] Shebang `#!/usr/bin/env mcsh run` works for `.c` script files
- [ ] Optional: persistent warm-compile daemon for < 100ms run latency

---

## Milestone dependency graph

```
M0 (foundation)
  └─ M1 (run MVP)
       └─ M2 (compile)
            └─ M3 (build)
                 └─ M4 (cache hardening)
                      └─ M5 (DX polish)
                           └─ M6 (JIT)
```
