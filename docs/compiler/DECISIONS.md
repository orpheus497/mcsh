# Decisions — Architecture Decision Record Log

> This document records key design decisions for the C workflow feature.
> Each ADR follows the format: context → decision → consequences.

---

## ADR template

```
### ADR-NNN — Title

**Status**: Proposed | Accepted | Superseded | Deprecated
**Date**: YYYY-MM-DD
**Deciders**: (list)

#### Context
What situation or problem prompted this decision?

#### Decision
What was decided?

#### Consequences
Positive and negative outcomes of the decision.

#### Alternatives considered
Other options that were rejected and why.
```

---

## ADR-001 — Three-verb command surface (`compile`, `build`, `run`)

**Status**: Accepted  
**Date**: 2026-07-28

#### Context

Early design exploration proposed a richer command family (`cc.compile`,
`cc.link`, `cc.build`, etc.) to namespace C workflow commands clearly.
The project owner explicitly rejected diluted command trees.

#### Decision

Expose exactly three first-class builtin verbs:
- `compile` — compile one or more C translation units
- `build` — compile + link a C project
- `run` — compile-if-needed + execute (flagship command)

No sub-commands, no nested namespaces.

#### Consequences

- **+** Minimal surface; easy to document and remember.
- **+** Consistent with shell philosophy (one verb per intent).
- **-** Command names are broad (`build`, `run`) and shadow any existing
  external commands or aliases with those names.
- **Mitigation**: documented escape hatch via `\run`, `command run`.

#### Alternatives considered

- `cc.compile`, `cc.link`, etc.: rejected (diluted, not shell-native).
- Single `cc` command with subcommands: rejected (hides the primary feature).

---

## ADR-002 — Compiler backend: clang/cc via PATH, not linked

**Status**: Accepted  
**Date**: 2026-07-28

#### Context

mcsh must not introduce GPL dependencies. The question is whether to bundle
a compiler, link to a compiler library, or invoke an external compiler via
`fork+exec`.

#### Decision

Invoke the compiler as an external process via `execvp`. Default search order:
1. `$mcsh_cc` shell variable (user override).
2. `clang` on `$PATH` (Apache-2.0 / LLVM license; permissive).
3. `cc` on `$PATH` (system default; typically BSD-licensed on BSD targets).

No compiler source is bundled into mcsh. No compiler library is linked.
`gcc` is only reached as a fallback `cc` on systems where `cc` → `gcc`.

#### Consequences

- **+** No license risk. GPL compiler invoked as external tool is not a
  license issue (only linking/incorporating GPL code matters for copyleft).
- **+** Picks up the best available compiler on the system without bundling.
- **-** Requires a working cc/clang installation (soft dependency).
- **-** Compiler version changes can invalidate the cache (feature, not bug).

#### Alternatives considered

- Bundling Chibicc (MIT): too limited for production C; maintenance burden.
- Bundling TCC (LGPL): license concern; LGPL is borderline for a BSD project.
- Linking libclang: large dependency; complicates build and portability.

---

## ADR-003 — `.mcsh` scripts are never passed through C workflow

**Status**: Accepted  
**Date**: 2026-07-28

#### Context

The command `run` is intended as the flagship fast-feedback command. There
is a natural temptation to make `run script.mcsh` equivalent to `mcsh script.mcsh`
for user convenience. The project owner explicitly rejected this.

#### Decision

`run`, `compile`, and `build` reject `.mcsh` input with an explicit error
message and exit code 1. `.mcsh` scripts continue to execute directly through
the normal shell execution path (`srcfile()`/`srcunit()`).

The error message directs users to the correct invocation:
```
run: 'script.mcsh' is a shell script, not a C file.
Execute scripts directly: mcsh script.mcsh
```

#### Consequences

- **+** Clean, unambiguous command model.
- **+** Prevents accidental interpretation of shell scripts as C programs.
- **-** Marginally less convenient for users who try `run script.mcsh`.
- **-** Users must learn the distinction once.

#### Alternatives considered

- Silent fallback to `mcsh script.mcsh`: rejected (violates principle of least
  surprise; hides bugs where user made a mistake).
- Warning then fall through: rejected (ambiguous; masks errors).

---

## ADR-004 — Data-oriented flat tables instead of object graph

**Status**: Accepted  
**Date**: 2026-07-28

#### Context

The internal state for compilation units, dependencies, artifacts, and jobs
could be modeled as a tree/graph of C structs with pointer links, or as flat
tables with stable integer IDs. The project owner specified a data-oriented
design.

#### Decision

Use flat tables with stable `uint32_t` IDs as foreign keys:
- `file_table`, `unit_table`, `dep_edge_table`, `artifact_table`,
  `profile_table`, `toolchain_table`, `diagnostic_table`, `job_table`.
- String interning via a single arena; all paths/messages are `str_id` offsets.
- Tables are arrays of fixed-width rows (SoA where beneficial).

#### Consequences

- **+** Cache-friendly iteration (linear scan over arrays).
- **+** Trivial serialization (write fixed-width rows as binary blob).
- **+** Deterministic memory layout; easier to reason about.
- **-** Slightly more boilerplate for "follow FK" operations vs pointer dereference.
- **-** Table IDs must never be reused within a session (monotonic allocation).

#### Alternatives considered

- Pointer-graph: rejected (harder to serialize; pointer invalidation on realloc).
- `sqlite3` embedded DB: rejected (external dependency; license and portability
  concerns; overkill for the problem size).

---

## ADR-005 — Cache index format: custom binary flat-file

**Status**: Accepted  
**Date**: 2026-07-28

#### Context

The persistent cache index needs a storage format. Options include SQLite,
JSON, binary custom format, or plain-text key-value.

#### Decision

Custom binary flat-file (`MCSHDX01` magic) with CRC32 integrity check.
Plain-text `.meta` sidecar files for per-artifact human-readable metadata.

#### Consequences

- **+** Zero external dependencies.
- **+** O(1) load: mmap or single `read()` call.
- **+** Trivial corruption detection (magic + CRC).
- **-** Not human-readable (sidecars compensate for artifacts).
- **-** Format version must be managed; bumping version requires migration or wipe.

#### Alternatives considered

- SQLite: rejected (dependency; GPL-exception license acceptable but still a
  library to maintain and ship).
- JSON: rejected (slow parse; large; no schema enforcement).
- CBOR/MessagePack: rejected (additional dependency; no benefit over custom binary).

---

## ADR-006 — `run` uses `execvp` (not a wrapper subprocess)

**Status**: Accepted  
**Date**: 2026-07-28

#### Context

`run` must execute the compiled binary. The question is whether to:
a) `execvp` the binary (replacing the current process), or
b) `fork()` + `execvp` and `waitpid()` (always launch as child).

#### Decision

When `run` is invoked in a forked context (most foreground invocations),
prefer `execvp` to avoid an extra process layer. When not forked (edge cases
in pipelines), fork first then exec.

Inspect `execute()` in `sh.sem.c` to determine fork state at builtin call site.

#### Consequences

- **+** Binary inherits the exact shell environment with no wrapper overhead.
- **+** Exit code passes through exactly (no extra layer to mask it).
- **-** Must carefully handle the non-forked context; incorrect exec would
  kill the shell.
- **Mitigation**: See R-T02 in RISKS.md; always fork when in doubt.

#### Alternatives considered

- Always fork + waitpid: simpler but adds process overhead; masks some
  exec-failure error codes.
- spawn via `posix_spawn`: avoids fork overhead but complicates environment
  inheritance; deferred to P5+.
