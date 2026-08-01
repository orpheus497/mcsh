# TODO — Granular Implementation Backlog

> Checkboxes are ordered by dependency. Items within the same phase can
> be worked in parallel unless marked `[AFTER: ...]`.
> Tags: `[MUST]` = blocking for phase exit; `[SHOULD]` = high value; `[COULD]` = nice-to-have.

---

## P0 — Foundation (no-behavior-change tasks)

- [x] `[MUST]` Add `TODO(mcsh-c-workflow)` comment block in `sh.init.c:42` describing
  exact sorted insertion positions for `build`, `compile`, `run` in `bfunc[]`.
- [x] `[MUST]` Add `TODO(mcsh-c-workflow)` comment in `sh.sem.c:619` (builtin invocation
  site) noting that C workflow builtins dispatch through the standard path.
- [x] `[MUST]` Add `TODO(mcsh-c-workflow)` comment in `sh.func.c:126` (`func()`) noting
  no special-casing needed.
- [x] `[MUST]` Confirm `Makefile.in` `SRCS` variable accepts an additional `sh.cworkflow.c`
  entry without other changes.
- [x] `[MUST]` Confirm `Char *` ↔ `char *` conversion is available via `short2str()` /
  `str2short()` for use in `sh.cworkflow.c`.
- [x] `[SHOULD]` Read and summarize BSD/Apache-2.0 license status of clang as backend;
  record in `DECISIONS.md` ADR-002.
- [x] `[SHOULD]` Document exact `bfunc[]` insertion order in a comment at the top of
  the `bfunc[]` table in `sh.init.c`.

---

## P1 — `run file.c` MVP ✓ (shipped)

### sh.cworkflow.c (new file)

- [x] `[MUST]` Create `sh.cworkflow.c` with file header matching style of `sh.func.c`.
- [x] `[MUST]` Implement `cw_find_toolchain(char *buf, size_t len)`:
  - Check `$mcsh_cc` shell variable first.
  - Walk `PATH` for `clang`, then `cc`.
  - Return 0 on success, -1 if not found.
  - File: `sh.cworkflow.c`, new function.
- [x] `[MUST]` Implement `cw_sha256_file(const char *path, uint8_t out[32])`:
  - Read file in 64 KiB chunks; compute SHA-256.
  - Pure C; no external library (implement or copy a BSD-licensed SHA-256).
  - File: `sh.cworkflow.c`.
- [x] `[MUST]` Implement `cw_cache_dir(char *buf, size_t len)`:
  - Return `$mcsh_cache_dir` if set, else `~/.mcsh_cache/cworkflow`.
  - Create directories if absent (`mkdir -p` equivalent).
  - File: `sh.cworkflow.c`.
- [x] `[MUST]` Implement `cw_run_validate(Char **v)`:
  - Check argument count ≥ 1.
  - Check argument does not end in `.mcsh` (golden error message).
  - Check argument ends in `.c` or is a directory.
  - Check file exists.
  - File: `sh.cworkflow.c`.
- [x] `[MUST]` Implement `dorun(Char **v, struct command *c)`:
  - Call `cw_run_validate()`.
  - Call `cw_find_toolchain()`.
  - Call `cw_sha256_file()` for source.
  - Compute binary cache key (source hash + cc hash; simplified P1 key).
  - Check binary exists in cache and meta validates.
  - On miss: fork + exec compiler + linker; wait; check exit code.
  - On success: fork + exec binary with program args (directory targets also supported).
  - File: `sh.cworkflow.c`.

### sh.decls.h

- [x] `[MUST]` Add `extern void dorun(Char **, struct command *);` in `sh.decls.h`
  under the `sh.func.c` section.

### sh.init.c

- [x] `[MUST]` Insert `{ "run", dorun, 1, INF }` in `bfunc[]` at the correct sorted
  position (after `"return"`, before `"sched"` or first entry beginning with 's').
  File: `sh.init.c`.

### Makefile.in

- [x] `[MUST]` Add `sh.cworkflow.c` to `SRCS` and `sh.cworkflow.o` to object list
  in `Makefile.in`.

### Tests

- [x] `[MUST]` Write `tests/t100_run_validation.sh` covering T100–T106.
- [x] `[MUST]` Write `tests/t101_run_basic.sh` covering I100–I103.
- [x] `[MUST]` Verify all `t001`–`t018` tests pass after `bfunc[]` addition.

---

## P2 — `compile` unit pipeline

### sh.cworkflow.c additions

- [ ] `[MUST]` Implement `dep_scan_file(const char *path, dep_result_t *out)`:
  - Fast `#include` line scanner (state machine, no malloc in inner loop).
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `resolve_include(const char *inc_path, inc_kind_t kind,
  const char *source_dir, const char **isearch, char *resolved, size_t len)`:
  - Resolution order per DEPENDENCY-DISCOVERY.md §2.3.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `cw_dep_hash(file_id_t unit, dep_edge_table_t *deps,
  file_table_t *files, uint8_t out[32])`:
  - BFS transitive closure; sorted by canonical path; SHA-256.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `cw_object_cache_key(...)`: full 5-component key per
  CACHE-DESIGN.md §2.1.
- [ ] `[MUST]` Implement `cw_index_load(const char *path, cw_index_t *idx)`:
  - Read `index.db`; verify magic + CRC32; populate tables.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `cw_index_flush(const char *path, const cw_index_t *idx)`:
  - Atomic write via `.db.tmp` + rename.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `docompile(Char **v, struct command *c)`.
  - File: `sh.cworkflow.c`.

### sh.decls.h

- [ ] `[MUST]` Add `extern void docompile(Char **, struct command *);`.

### sh.init.c

- [ ] `[MUST]` Insert `{ "compile", docompile, 1, INF }` at sorted position
  (after `"complete"`, before `"continue"`).

### Tests

- [ ] `[MUST]` Write `tests/t110_compile_validation.sh` covering T110–T114.
- [ ] `[MUST]` Write `tests/t111_compile_cache.sh` covering T130–T135.
- [ ] `[MUST]` Write `tests/t112_dep_discovery.sh` covering T140–T145.

---

## P3 — `build` project pipeline

### sh.cworkflow.c additions

- [ ] `[MUST]` Implement `source_discover(const char *root, source_list_t *out)`:
  - Recursive `*.c` walk; apply exclusion patterns.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `detect_entry_point(const source_list_t *srcs, int *count)`:
  - Scan each `.c` for `int main(`.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement parallel compile scheduler using `job_table`:
  - Bounded concurrency via `waitpid()`; SIGCHLD or polling loop.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `cw_link(const artifact_list_t *objs, const char *output,
  const char *cc)`.
  - File: `sh.cworkflow.c`.
- [ ] `[MUST]` Implement `dobuild(Char **v, struct command *c)`.
  - File: `sh.cworkflow.c`.

### sh.decls.h + sh.init.c

- [ ] `[MUST]` Add `extern void dobuild(Char **, struct command *);` to `sh.decls.h`.
- [ ] `[MUST]` Insert `{ "build", dobuild, 0, INF }` at sorted position in `sh.init.c`
  (after `"bg"`, before `"builtins"`).

### Tests

- [ ] `[MUST]` Write `tests/t120_build_validation.sh` covering T120–T123.
- [ ] `[MUST]` Write `tests/t121_build_incremental.sh` covering I110–I113.
- [ ] `[MUST]` Write `tests/t122_build_errors.sh` covering I120–I122.

---

## P4 — Cache hardening

- [ ] `[MUST]` Add toolchain `cc_hash` to all cache keys (`cw_object_cache_key`,
  `cw_binary_cache_key`).
  [AFTER: P2 cache key implementation]
- [ ] `[MUST]` Implement LRU eviction in `cw_cache_maybe_evict()`.
- [ ] `[MUST]` Implement `index.db` corruption detection (magic + CRC32).
- [ ] `[MUST]` Implement `cw_index_rebuild_from_sidecars()` for full recovery.
- [ ] `[SHOULD]` Implement `--clean` flag in all three handlers.

---

## P5 — Developer experience

- [ ] `[SHOULD]` Add `.c` file completions to `complete.mcsh`.
- [ ] `[SHOULD]` Read `$mcsh_cflags` in all three handlers.
- [ ] `[SHOULD]` Read `$mcsh_jobs` in `dobuild()`.
- [ ] `[COULD]` Implement `--emit-db` (compile_commands.json output).
- [ ] `[COULD]` Add build progress `[N/M] Compiling` lines to stderr.
- [ ] `[SHOULD]` Write `tests/bench_run_latency.sh`.
- [ ] `[SHOULD]` Write `tests/bench_incremental_build.sh`.

---

## P6 — JIT scripting integration

- [ ] `[SHOULD]` Handle `run $var` where `$var` expands to a `.c` path.
- [ ] `[SHOULD]` Source `build.mcsh` in `dobuild()` if present in target directory.
- [ ] `[COULD]` Support `#!/usr/bin/env mcsh run` shebang in `.c` files.
- [ ] `[COULD]` Persistent warm-compile daemon (Unix socket + server process).
