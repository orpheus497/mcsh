# Pipelines — Stage-by-Stage Behavior

> **Status: planning / documentation only.**
> No implementation exists yet.

---

## 1. `compile` pipeline

### Purpose

Compile one or more C translation units to object artifacts.
Does **not** link. Emits diagnostics. Populates/reuses the cache.

### Input

```
compile <file.c> [file2.c ...] [options]
```

### Stages

```
Stage 1: Input validation
Stage 2: Profile resolution
Stage 3: File table update
Stage 4: Dependency scan
Stage 5: Cache key computation
Stage 6: Cache lookup
Stage 7: Compilation (cache miss only)
Stage 8: Artifact storage
Stage 9: Diagnostics output
Stage 10: Metadata flush
```

#### Stage 1 — Input validation

- For each argument:
  - If it begins with `-`, treat as flag/option (pass to profile resolution).
  - If it ends with `.c`, treat as source file.
  - If it ends with `.mcsh`, emit error and exit immediately:
    `compile: '%s' is a shell script, not a C source file.`
  - If it does not exist, emit `compile: '%s': file not found.` and exit.
  - If it is a directory, emit `compile: '%s': is a directory; use 'build' for projects.`
- All validation errors are reported before any compilation begins.
- Exit code `1` on validation failure.

#### Stage 2 — Profile resolution

- Parse flags from arguments and shell variables (`$mcsh_cflags`).
- Normalize: sort `-D` defines, canonicalize `-I` paths (realpath).
- Compute `profile_hash` (SHA-256 of normalized flag vector).
- Look up or create `profile_table` entry.
- Detect toolchain: walk `$PATH` for `clang`, then `cc`. Record in `toolchain_table`.

#### Stage 3 — File table update

- For each source file: compute `file_id` (insert if new, update mtime/size).
- Lazily compute `content_hash` if mtime or size has changed.

#### Stage 4 — Dependency scan

- Fast `#include` line scan (not full preprocessor; see DEPENDENCY-DISCOVERY.md).
- For each `#include "path"`: resolve relative to source file directory and `-I` paths.
- For each `#include <path>`: resolve against system include paths.
- Insert/replace `dep_edge_table` rows for this unit.
- Recursively scan newly-discovered headers (bounded depth; default max 64).

#### Stage 5 — Cache key computation

```
key = SHA-256(
    content_hash(source_file)
  + content_hash(dep_1) + content_hash(dep_2) + ...   [sorted by canonical path]
  + profile_hash
  + toolchain.cc_hash
)
```

See CACHE-DESIGN.md for exact key composition algorithm.

#### Stage 6 — Cache lookup

- Look up `artifact_table` by `cache_key`.
- If hit and `artifact.valid == true`:
  - Verify artifact file exists on disk.
  - If file missing: treat as cache miss.
  - If file present: skip to Stage 9 (output diagnostics, if any were cached).
- If miss: continue to Stage 7.

#### Stage 7 — Compilation (cache miss only)

- Set `artifact.valid = false` in `artifact_table` (corruption-safe).
- Fork child process.
- In child: exec compiler:
  ```
  clang -c <source.c> -o <artifact_path> <profile_flags>
  ```
- Capture stderr for diagnostic parsing.
- Wait for child exit.
- If exit code non-zero: parse stderr → insert rows into `diagnostic_table`; set `unit.state = FAILED`; exit with compiler exit code.

#### Stage 8 — Artifact storage

- Move compiled object to `~/.mcsh_cache/cworkflow/objects/<key_prefix>/<key_hex>.o`.
- Write sidecar `.meta` file.
- Set `artifact.valid = true`; update `artifact_table`.
- Update `unit.state = COMPILED`, `unit.compile_epoch`.

#### Stage 9 — Diagnostics output

- Emit any warnings from `diagnostic_table` to stderr in the format:
  `<file>:<line>:<col>: <severity>: <message>`
- This matches standard compiler output format for tool integration.

#### Stage 10 — Metadata flush

- Serialize updated tables to `~/.mcsh_cache/cworkflow/index.db`.
- Atomic write: write to `.db.tmp`, rename over `.db`.

### Exit codes — `compile`

| Code | Meaning |
|------|---------|
| `0` | All units compiled (or already cached) successfully |
| `1` | Input validation error |
| `2` | Dependency scan error (include resolution failure) |
| `3` | Compiler error (non-zero compiler exit) |
| `4` | Cache I/O error |
| `5` | Toolchain not found |

---

## 2. `build` pipeline

### Purpose

Discover all C sources in a project, compile all units incrementally,
link into a target binary. Orchestrates the full compile→link flow.

### Input

```
build [target-dir] [options]
```

Default target-dir is `.` (current directory).

### Stages

```
Stage 1: Input validation
Stage 2: Source discovery
Stage 3: Profile resolution (project-level)
Stage 4: File + unit table population
Stage 5: Dependency scan (all units)
Stage 6: Incremental invalidation
Stage 7: Parallel compilation
Stage 8: Link
Stage 9: Diagnostics output
Stage 10: Metadata flush
```

#### Stage 1 — Input validation

- If `target-dir` is not a directory, error: `build: '%s': not a directory.`
- If `target-dir` contains no `.c` files (recursively), error with guidance.
- `.mcsh` files in the tree are ignored (not a validation error).

#### Stage 2 — Source discovery

- Walk `target-dir` recursively for `*.c` files.
- Exclude files matching standard ignore patterns: `_test.c`, `.*`, `build/` subdirectory.
- Infer entry point by scanning for `int main(` in each file.
- If zero entry points found: error — no `main()` found.
- If multiple entry points: error or prompt (future: multi-target mode).

#### Stage 3 — Profile resolution

- Same as `compile` Stage 2 but reads project-level config if present:
  - `build.mcsh` in target-dir: sourced as a shell script; may `set mcsh_cflags`, etc.
  - Absent: use defaults + `$mcsh_cflags` from environment.

#### Stage 4 — File + unit table population

- Insert/update `file_table` rows for all discovered `.c` and header files.
- Insert/update `unit_table` rows for all `.c` files under current profile.

#### Stage 5 — Dependency scan (all units)

- Same algorithm as `compile` Stage 4 for each unit in parallel.
- Build complete include DAG for the project.

#### Stage 6 — Incremental invalidation

- For each unit, compare `content_hash` and dep hashes to stored values.
- If any hash differs: mark unit `STALE`, mark all downstream dependents `STALE`.
- Units with `state == COMPILED` and no hash changes: skip compilation.
- Result: minimal set of units to recompile.

#### Stage 7 — Parallel compilation

- Create `job_table` rows for each stale unit (`JOB_COMPILE`).
- Schedule jobs with topological ordering (headers first is not required for objects,
  but depth-first avoids dependency re-scans).
- Bound concurrency: `min(nproc, 8)` by default; overridable via `$mcsh_jobs`.
- Wait for all compile jobs; collect exit codes.
- If any compile failed: print all diagnostics, skip link, exit with code `3`.

#### Stage 8 — Link

- Create `JOB_LINK` job.
- Collect all object paths from `artifact_table` for units in this build.
- Exec linker:
  ```
  clang <obj1.o> <obj2.o> ... -o <target_bin> <link_flags>
  ```
- Wait for linker exit.
- On success: update `artifact_table` for binary artifact.
- On failure: emit linker diagnostics, exit with code `3`.

#### Stage 9 — Diagnostics output

- Same as `compile` Stage 9.

#### Stage 10 — Metadata flush

- Same as `compile` Stage 10.

### Exit codes — `build`

| Code | Meaning |
|------|---------|
| `0` | Build succeeded |
| `1` | Input validation error |
| `2` | Source discovery / dependency error |
| `3` | Compile or link error |
| `4` | Cache I/O error |
| `5` | Toolchain not found |

---

## 3. `run` pipeline

### Purpose

Compile-if-needed and execute a C target. The flagship fast-feedback command.

### Input

```
run <file.c|target> [-- program-args ...]
```

### Stages

```
Stage 1: Input validation (with .mcsh guard)
Stage 2: Fast cache check
Stage 3: Compile + link (cache miss only)
Stage 4: Execute
```

#### Stage 1 — Input validation (with `.mcsh` guard)

- If argument ends with `.mcsh` or is a shell script (detected by shebang):
  ```
  run: '%s' is a shell script. Execute scripts directly: mcsh <script.mcsh>
  run is for C files only.
  ```
  Exit code `1`. No further processing.
- If argument does not end with `.c` and is not a directory: error with usage.
- If file does not exist: error.

#### Stage 2 — Fast cache check

- Compute content hash of source (and transitive deps if already indexed).
- Compute cache key.
- If artifact found on disk and `artifact.valid == true`:
  - Skip stages 3a (compile) and 3b (link).
  - Jump directly to Stage 4.
- This path must complete in < 50ms for a typical single-file source on warm filesystem.

#### Stage 3 — Compile + link (cache miss only)

- Run `compile` pipeline stages 3–8 (abbreviated: dependency scan, compile, artifact storage).
- If compile errors: emit diagnostics to stderr, exit with compile exit code.
- Run link step (as in `build` Stage 8) to produce binary artifact.
- If link errors: emit diagnostics to stderr, exit with link exit code.

#### Stage 4 — Execute

- **Preferred path**: `execvp(binary_path, program_args)`.
  - Replaces the mcsh process with the compiled binary.
  - Binary inherits the full environment.
  - Exit code is exactly the binary's exit code.
  - This is the correct behavior for `run` as a transparent launcher.
- **Note on `execvp`**: because `run` is a builtin, `execute()` in `sh.sem.c`
  has already forked (for pipelines, background jobs, etc.). For a simple
  foreground `run` invocation, the builtin must arrange exec without an extra
  intermediate shell process.
- If exec fails: emit `run: exec failed: <strerror>`, exit code `127`.

### Child process behavior

- Signal disposition: `SIGINT`, `SIGQUIT`, `SIGTERM` set to `SIG_DFL` before exec.
- Environment: inherited from mcsh session without modification.
- Working directory: inherited (not changed to source file directory).
- `$argv` and `$0` in the binary: standard C `argc`/`argv` from the `-- args` section.

### Exit codes — `run`

| Code | Meaning |
|------|---------|
| `0..254` | Exit code of the executed binary (passed through exactly) |
| `1` | Input validation error (including `.mcsh` guard) |
| `3` | Compile or link error |
| `4` | Cache I/O error |
| `5` | Toolchain not found |
| `127` | Exec failed |

**Important**: `run` must not add any wrapper exit codes that could
conflict with the compiled program's own exit codes. Codes `1`, `3`, `4`,
`5`, `127` are only emitted before exec occurs.
