# Risks

> This document tracks technical and product risks for the C workflow
> compiler feature. Severity: High (H) / Medium (M) / Low (L).
> Likelihood: High (H) / Medium (M) / Low (L).

---

## Technical risks

### R-T01 — Sorted builtin table corruption

| Field | Value |
|-------|-------|
| **Severity** | H |
| **Likelihood** | M |
| **Phase** | P1–P3 |
| **Description** | `bfunc[]` in `sh.init.c` must be sorted for binary search (`isbfunc()`). Inserting `build`, `compile`, or `run` in the wrong position silently breaks lookup for the inserted command and adjacent entries. |
| **Mitigation** | Verify sorted order with `awk` script after any `bfunc[]` edit. Add a compile-time or runtime assertion that checks adjacency. Add regression test that invokes all three new commands after insertion. |
| **Contingency** | Revert `sh.init.c` change; re-insert in correct position. |

---

### R-T02 — `execvp` in a non-forked builtin context

| Field | Value |
|-------|-------|
| **Severity** | H |
| **Likelihood** | M |
| **Phase** | P1 |
| **Description** | `run` needs to `execvp` the compiled binary, replacing the process. But shell builtins may be called without a prior `fork()` in certain contexts (e.g., last command in a pipeline). Calling `execvp` directly would replace the mcsh shell itself. |
| **Mitigation** | Inspect `execute()` in `sh.sem.c` — specifically the `COMMAND` / `NODE_COMMAND` branch at `sh.sem.c:272` — to understand when the builtin runs in a forked child. In non-forked contexts, `dorun` must `fork()` first, then `execvp` in the child, and `waitpid()` in the parent. |
| **Contingency** | Always fork before exec in `dorun`; accept slight overhead for safety. |

---

### R-T03 — Incremental invalidation false negatives

| Field | Value |
|-------|-------|
| **Severity** | H |
| **Likelihood** | M |
| **Phase** | P2–P3 |
| **Description** | If the dependency scanner misses an `#include` (e.g., computed include path via macro, or include inside `#if 0`), a header change will not invalidate the unit. The user gets a stale binary without knowing it. |
| **Mitigation** | Document limitation; over-approximate by scanning all `#include` lines regardless of `#if` conditions. Add a `--verify` flag (P4) that re-scans and compares dep sets. |
| **Contingency** | User runs `build --clean` or `run --clean` to force rebuild. |

---

### R-T04 — Cache corruption after interrupted build

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | L |
| **Phase** | P1–P3 |
| **Description** | A build interrupted by SIGKILL or power loss could leave `index.db.tmp` or partial artifact files on disk, causing spurious cache hits or misses on next run. |
| **Mitigation** | Set `artifact.valid = false` before writing; use atomic rename for `index.db`; verify sidecar meta on cache hit. |
| **Contingency** | `index.db` magic/CRC mismatch triggers full rebuild from sidecars (P4 recovery path). |

---

### R-T05 — Compiler child process signal handling

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | M |
| **Phase** | P1–P3 |
| **Description** | If the user presses Ctrl-C during compilation, the SIGINT may reach the mcsh shell and leave orphan compiler processes or a partial artifact in the cache. |
| **Mitigation** | Put compiler child in its own process group. Reset SIGINT/SIGQUIT to SIG_DFL in child before exec. Clean up partial artifact on SIGINT in parent (use `cleanup_push()` pattern). |
| **Contingency** | Stale partial artifact is caught by `artifact.valid` check on next run. |

---

### R-T06 — mtime resolution on network filesystems

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | L |
| **Phase** | P2–P3 |
| **Description** | NFS and some network filesystems have 1-second mtime resolution. Rapid file edits may not bump mtime. mtime-gated hash skipping will then miss changes. |
| **Mitigation** | Fall back to content hash whenever mtime equals stored value (not just when it differs). Provide `--no-mtime` flag to always hash. |
| **Contingency** | Document NFS limitation; recommend `--no-mtime` for network builds. |

---

### R-T07 — SHA-256 implementation portability

| Field | Value |
|-------|-------|
| **Severity** | L |
| **Likelihood** | L |
| **Phase** | P1 |
| **Description** | mcsh must not introduce GPL-licensed SHA-256 code. Using system `libcrypto` (OpenSSL) is heavy; a bundled implementation must be BSD-licensed. |
| **Mitigation** | Use the public-domain or BSD-2-Clause `sha2.c` from the OpenBSD tree, which is already BSD-licensed. Verify license before inclusion. |
| **Contingency** | Use `MD5` or `CRC64` as a fallback (weaker but sufficient for cache keys; note in `DECISIONS.md`). |

---

### R-T08 — `Char *` vs `char *` string handling

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | H |
| **Phase** | P1 |
| **Description** | mcsh uses `Char *` (wide-char typedef) throughout. All `sh.cworkflow.c` code that interfaces with the shell must use `short2str()` / `str2short()` / `Strsave()`. Forgetting to convert causes incorrect behavior or crashes. |
| **Mitigation** | Review all string operations in `sh.cworkflow.c` before each phase. Use `static_assert` or lint checks where possible. |
| **Contingency** | Compiler warnings (-Wall) will catch most type mismatches. |

---

## Product risks

### R-P01 — Command name collision with user aliases

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | M |
| **Phase** | P1–P3 |
| **Description** | Users may have aliases or external commands named `run`, `build`, or `compile`. Registering these as builtins will shadow them. |
| **Mitigation** | Document escape hatch (`\run`, `command run`). Inform users in release notes. |
| **Contingency** | Provide a `set no_cworkflow_builtins` variable to disable the three builtins at startup. |

---

### R-P02 — Scope creep: pressure to support non-C inputs

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | H |
| **Phase** | All |
| **Description** | Users may request `run script.py`, `run program.rs`, etc., diluting the C-focused design. |
| **Mitigation** | Enforce `.c`/directory-only input validation; document the C-only contract explicitly. |
| **Contingency** | Reject with clear error; redirect to language-specific tools. |

---

### R-P03 — GPL toolchain dependency introduced accidentally

| Field | Value |
|-------|-------|
| **Severity** | H |
| **Likelihood** | L |
| **Phase** | All |
| **Description** | If gcc (GPL) is auto-selected as the compiler backend and linked into or bundled with mcsh, the GPL's copyleft could affect mcsh's BSD license. |
| **Mitigation** | Use `gcc` / `cc` only as an external runtime tool invoked via `execvp`, never linked. Prefer clang (Apache-2.0) as the default. Document toolchain selection in `DECISIONS.md`. |
| **Contingency** | Remove gcc from auto-detection; require explicit `set mcsh_cc = gcc`. |

---

### R-P04 — Cache fills disk on developer machines

| Field | Value |
|-------|-------|
| **Severity** | M |
| **Likelihood** | M |
| **Phase** | P3–P4 |
| **Description** | On a busy machine with many projects, `~/.mcsh_cache/` could grow unboundedly. |
| **Mitigation** | Default 512 MiB cap with LRU eviction (P4). Warn when cache > 80% of cap. |
| **Contingency** | Document `mcsh --cache-clean` as escape hatch. |

---

### R-P05 — `.mcsh` guard message wording controversy

| Field | Value |
|-------|-------|
| **Severity** | L |
| **Likelihood** | M |
| **Phase** | P1 |
| **Description** | The exact wording of the `.mcsh` guard error message (`run is for C files only`) may confuse users who expect `run` to be a general launcher. |
| **Mitigation** | Message clearly states the alternative (`mcsh script.mcsh`). Golden test captures exact wording so accidental changes are caught. |
| **Contingency** | Adjust wording in `dorun()` without any structural change. |
