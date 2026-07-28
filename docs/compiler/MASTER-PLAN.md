# Compiler Program — Master Plan

> **Status: planning / documentation only.**
> No compile/build/run runtime is implemented yet.

---

## Vision

mcsh is a modern C shell. Its value proposition is that the shell itself
understands C — not as a parser that must tokenize C syntax, but as a
runtime that can compile, cache, and execute C units natively, making
the C development feedback loop as fluid as shell scripting.

The end state is a shell where a developer can:

```csh
run main.c          # instant: compile-if-needed, execute, print output
compile src/*.c     # explicit unit compilation with incremental cache
build .             # multi-file project with auto-discovery and linking
```

without touching a Makefile, build system, or external compiler driver script.
The `.mcsh` script remains the orchestration language for everything else.

---

## Scope

### In scope

- Three new builtin commands: `compile`, `build`, `run`.
- C source file/project input only (`.c`, `.h`, project directory).
- Incremental compilation cache backed by a data-oriented state model.
- Automatic header/include dependency discovery.
- Minimal, BSD-licensed toolchain integration (clang/BSD cc as backends).
- Clear, actionable diagnostics surfaced through the shell.
- `.mcsh` scripts as the natural orchestration layer (no Makefile replacement needed).

### Out of scope

- Implementing a full C lexer/parser/codegen within mcsh.
- Supporting languages other than C (no C++, Fortran, Rust at this stage).
- A REPL or interactive C expression evaluator.
- Replacing or wrapping `make`, `cmake`, or other build systems.
- GPL-licensed toolchain dependencies in the critical path.
- Modifications to `.mcsh` script execution semantics.

### Non-goals

| Non-goal | Reason |
|----------|--------|
| mcsh becomes a C compiler | Scope explosion; delegate codegen to clang/BSD cc backend |
| `run` works on `.mcsh` scripts | Violates command contract; scripts execute directly |
| Nested command families (`cc.compile`, `cc.link`, etc.) | Dilutes UX; flat three-verb model is sufficient |
| Full standards-conformance testing | Out of scope for the mcsh layer |
| Dynamic linking of mcsh itself to compiler libraries | Increases dependency surface; unneeded |

---

## Command contract

This contract is immutable for all phases:

| Invocation | Behavior |
|------------|----------|
| `mcsh script.mcsh` | Execute as normal shell script. No change. |
| `./script.mcsh` | Execute as shebang script. No change. |
| `source script.mcsh` | Source into current shell. No change. |
| `compile file.c [...]` | Compile C translation unit(s). |
| `build [target] [...]` | Build C project/target. |
| `run file.c [...]` | Compile-if-needed + execute C target. |
| `run script.mcsh` | **Error**: `run` is for C files. Suggest direct execution. |

`.mcsh` scripts are never passed through C workflow commands. This boundary
is enforced with an explicit error message, not silent fallback.

---

## Phased roadmap

### P0 — Foundation (prerequisite, non-functional)

**Goal:** Lay groundwork without behavior changes.

- [x] Baseline architecture analysis (`docs/c-workflow-analysis.md`)
- [x] Command contract document (`docs/c-commands-spec.md`)
- [x] Comprehensive planning suite (`docs/compiler/`)
- [ ] Add `TODO(mcsh-c-workflow)` markers in `sh.init.c`, `sh.sem.c`, `sh.func.c`
- [ ] Confirm build system can incorporate a new `sh.cworkflow.c` source file
- [ ] Identify BSD-licensed backend toolchain for P1 (clang via `PATH` lookup)

**Exit criteria:** All planning docs present; integration points annotated; build
system confirmed capable; no behavior changes.

---

### P1 — `run file.c` MVP

**Goal:** `run hello.c` compiles hello.c (if needed), executes it, returns exit status.

- [ ] Implement `dorun()` in new `sh.cworkflow.c`
- [ ] Register `"run"` in `bfunc[]` (`sh.init.c`) in sorted position
- [ ] Add `extern void dorun(Char **, struct command *)` in `sh.decls.h`
- [ ] Input validation: accept `.c` files only; reject `.mcsh` with guidance error
- [ ] Cache key: SHA-256 of source content + compiler identity + flags
- [ ] Cache directory: `~/.mcsh_cache/cworkflow/`
- [ ] Compile with discovered system cc (clang preferred, BSD cc fallback)
- [ ] Execute compiled binary; propagate child exit status
- [ ] Basic diagnostics: compile errors printed to stderr with file:line format

**Exit criteria:** `run hello.c` works end-to-end; `run script.mcsh` emits
correct error; cache avoids recompile on unchanged source.

**Dependencies:** P0 complete; `sh.init.c` TODO marker resolved.

---

### P2 — `compile` unit pipeline + dependency tracking

**Goal:** Explicit compilation with per-unit caching and header dep tracking.

- [ ] Implement `docompile()` in `sh.cworkflow.c`
- [ ] Register `"compile"` in `bfunc[]`
- [ ] Include dependency scanner: fast `#include` line scan (no full preprocessor)
- [ ] Build `dep_edge_table` per unit
- [ ] Cache key: source hash + resolved transitive dep hashes + flags + target
- [ ] Emit object artifacts to cache directory
- [ ] Diagnostics: structured output compatible with existing `stderror()` style
- [ ] Invalidate stale units when headers change

**Exit criteria:** `compile src/foo.c` produces/reuses object; changing a
header causes recompilation of dependent units; unchanged units are not recompiled.

**Dependencies:** P1 complete.

---

### P3 — `build` project graph + link orchestration

**Goal:** `build .` discovers C sources, compiles, links, produces binary.

- [ ] Implement `dobuild()` in `sh.cworkflow.c`
- [ ] Register `"build"` in `bfunc[]`
- [ ] Source auto-discovery: walk project tree for `.c` files
- [ ] Infer entry point from `main()` presence
- [ ] Dependency DAG construction over all units
- [ ] Parallel compilation scheduling (per-unit jobs, bounded concurrency)
- [ ] Link orchestration: ordered object list → linker invocation
- [ ] Profile presets: debug / release / sanitize

**Exit criteria:** `build .` on a multi-file project produces a working binary;
incremental rebuild skips unchanged units; `build .` on a single-file project
matches `run` compilation quality.

**Dependencies:** P2 complete; `dep_edge_table` stable.

---

### P4 — Cache hardening and toolchain identity

**Goal:** Robust incremental behavior across compiler versions and flag changes.

- [ ] Encode toolchain identity (path, version string) in cache key
- [ ] Detect compiler version changes and invalidate affected artifacts
- [ ] Implement cache pruning with LRU eviction and size cap
- [ ] Corruption recovery: detect malformed artifacts, rebuild on demand
- [ ] Persistent metadata index (`~/.mcsh_cache/cworkflow/index.db`)
- [ ] `compile --clean` / `build --clean` flags to wipe cache

**Exit criteria:** Changing `$CC` or compiler version triggers full rebuild;
corrupt cache entries are replaced without error.

**Dependencies:** P3 complete; index persistence design from DATA-MODEL.md finalized.

---

### P5 — Developer experience polish

**Goal:** Make compile/build/run feel native and fast in everyday use.

- [ ] `run` warm-start: daemon/socket for zero-fork overhead on cached binaries
- [ ] Progress reporting for `build` (unit N of M)
- [ ] `build --jobs N` explicit parallelism control
- [ ] Compiler flag presets via shell variables (`$mcsh_cflags`)
- [ ] `compile --emit-db` outputs compile_commands.json for IDE integration
- [ ] Integration with `dot.mcshrc` completions (`.c` file extension hints)

**Exit criteria:** Build times and UX are competitive with a basic Makefile
workflow on a 20-file project.

**Dependencies:** P4 complete.

---

### P6 — JIT scripting and `.mcsh` integration

**Goal:** C snippets and C units invocable from `.mcsh` scripts with shell-native
ergonomics; no Makefile needed for common project layouts.

- [ ] `run` accepts shell-variable-defined sources: `run $sources`
- [ ] `build` reads optional `build.mcsh` for project-level config
- [ ] `compile` supports per-invocation profile override: `compile -p release`
- [ ] Shebang-line execution: `#!/usr/bin/env mcsh run` for `.c` scripts
- [ ] Persistent warm-compile daemon for sub-100ms `run` latency on cache hits

**Exit criteria:** A developer can write a 10-file C project controlled by a
50-line `build.mcsh` script with zero external tools.

**Dependencies:** P5 complete.

---

## Phase dependencies summary

```
P0 (foundation)
  └─ P1 (run MVP)
       └─ P2 (compile + deps)
            └─ P3 (build + link)
                 └─ P4 (cache hardening)
                      └─ P5 (DX polish)
                           └─ P6 (JIT + .mcsh integration)
```

Each phase is a gating checkpoint. No phase begins implementation until all
exit criteria of its predecessor are verified.

---

## Gating checks before any implementation begins

1. **Contract confirmation**: `.mcsh` execution path is confirmed unchanged after
   P1 builtin additions (regression test pass).
2. **Sorted table check**: `bfunc[]` order verified after each new entry.
3. **License audit**: backend cc/clang dependency confirmed BSD/Apache-2.0 permissive.
4. **Cache directory safety**: `~/.mcsh_cache/` creation does not clobber existing files.
5. **Diagnostics consistency**: new builtin error paths use `stderror()` or equivalent,
   not ad-hoc `fprintf`.
