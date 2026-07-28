# Dependency Discovery

> **Status: planning / documentation only.**
> No implementation exists yet.

---

## 1. Goal

Determine, for each C translation unit, the complete set of header files
whose content contributes to the compiled output. This set is used to
compute accurate cache keys and drive incremental invalidation.

---

## 2. Include scanning strategy

### 2.1 Approach: fast line scanner (not a full preprocessor)

mcsh uses a **dedicated fast scanner** rather than invoking the preprocessor
(`-MM`/`-MF` flags) or re-implementing cpp macros. Rationale:

- Preprocessor invocation is slow and requires a working compiler for every
  file on every build, even for unchanged files.
- Full macro expansion is not needed to find `#include` directives; almost all
  `#include` lines are unconditional or trivially conditional.
- The scanner is deterministic and cache-friendly.

**Known limitation**: conditional `#include` blocks (`#ifdef FOO / #include`)
are scanned regardless of the condition. This over-approximates the dependency
set (safe) but may cause unnecessary recompiles when the condition is false.
Acceptable trade-off for phase P2; a smarter evaluator can be added in P4+.

### 2.2 Scanner algorithm

```
for each line in source_file:
    skip leading whitespace
    if line does not begin with '#': continue
    skip '#' and whitespace
    if token is not 'include': continue
    extract path token:
        '"path"'  → quote-include, path is relative-or-search
        '<path>'  → angle-include, path is system search only
    emit (parent_file, path, kind)
```

Implementation: a small `dep_scan_file()` function in `sh.cworkflow.c`,
using a byte-at-a-time state machine over a `read()`-buffered file.
No dynamic allocation inside the inner loop.

### 2.3 Include resolution order

For `#include "path"`:
1. Directory of the including source file.
2. Each `-I` directory from the compile profile, in order.
3. System include directories (detected via `cc -v -x c /dev/null 2>&1`).

For `#include <path>`:
1. Each `-I` directory from the compile profile, in order.
2. System include directories.

Resolution stops at the first match. If no match is found, the include is
recorded as **unresolved** (a warning, not an error at scan time; the
compiler will error if it is truly missing).

### 2.4 Recursive scanning

After resolving a header path to a `file_id`:
- If the header has not yet been scanned in this build epoch, scan it recursively.
- Track visited `file_id` set to prevent cycles (mutual `#include` is rare but exists).
- Maximum recursion depth: 64 levels (configurable via `$mcsh_dep_depth`).
- Depth exceeded: record a `DIAG_WARN` and stop recursing; over-approximation is safe.

---

## 3. Dependency graph construction

### 3.1 Graph structure

The dependency graph is stored in `dep_edge_table` (see DATA-MODEL.md):

```
unit_id → {dep_file_id, include_kind, depth}*
```

This is a directed acyclic graph (DAG) where units are roots and headers
are leaves (headers can also be roots of their own sub-graphs).

### 3.2 Transitive closure for cache keys

The cache key for a unit requires the **transitive closure** of all
included headers:

```
deps(unit) = {file_id : there is a path from unit to file_id in dep graph}
```

Computed with a simple BFS/DFS over `dep_edge_table` starting from the unit.

### 3.3 Reverse dependency index

For incremental invalidation, maintain a reverse index:

```
file_id → {unit_id : unit depends on file_id}
```

Stored in `dep_edge_table` (materialized on demand via reverse scan).
When a header changes (mtime or content hash), look up affected units via
the reverse index and mark them `STALE`.

---

## 4. Incremental invalidation rules

### 4.1 Trigger conditions

A unit transitions from `COMPILED` to `STALE` when any of the following
are detected at the start of a `compile`, `build`, or `run` invocation:

| Condition | Detection method |
|-----------|-----------------|
| Source file content changed | `content_hash` differs |
| Source file replaced on disk | `mtime_ns` changed → re-hash |
| Header content changed | `content_hash` of any `dep_file_id` differs |
| Header deleted | Resolution returns no match; treat as changed |
| New header added to project | New file in dep scan → new edge → re-hash |
| Compile profile changed | `profile_hash` differs |
| Toolchain changed | `toolchain_id` differs |

### 4.2 Propagation rules

Changes propagate through the reverse dependency index:

```
header H changed
  → all units that include H (directly or transitively) become STALE
  → STALE propagates transitively through the reverse dep graph
```

Propagation is bounded: only marks units in the same project scope.
It does not mark unrelated units in the cache (only the current build's units).

### 4.3 mtime-gated hashing

Content hashing is expensive for large projects. Gate it on mtime:

```
if file.mtime_ns == stored.mtime_ns and file.size_bytes == stored.size_bytes:
    assume content_hash is still valid
else:
    re-hash and update stored values
```

This is the same strategy used by `make`, `ccache`, and most build systems.
It is safe as long as the filesystem has sub-second mtime resolution (all
modern POSIX filesystems do).

### 4.4 Edge cases

| Scenario | Behavior |
|----------|----------|
| Header moved to different `-I` path | Old `dep_file_id` becomes unresolvable → treated as changed → recompile |
| Symlink target changes | `realpath()` changes → new `file_id` → recompile |
| Two `-I` paths contain same header name | First match wins (same rule as compiler) |
| Header outside project tree (e.g. `/usr/include`) | Scanned and hashed; cached; system include changes are detected |
| `__has_include` conditionals | Not evaluated at scan time; over-approximated as present |
| Computed include paths (`#include MACRO_PATH`) | Not resolved; recorded as unresolved; compile will verify |

---

## 5. System include path detection

System includes (`/usr/include`, `/usr/local/include`, compiler built-ins)
are discovered once per toolchain identity:

```sh
cc -v -x c -E /dev/null 2>&1 | grep '^ /' | tr -d ' '
```

Result is stored in `toolchain_table` and cached per toolchain identity.
Recomputed when `toolchain_id` changes (compiler version bump).

System headers are scanned and hashed the same as project headers.
Their hashes are cached by `file_id` with mtime gating.

---

## 6. Dependency scan output

After scanning, the following are available per unit:

```
deps_direct:   set of directly-included file_ids (depth == 1)
deps_transitive: BFS closure of all included file_ids
dep_hash:      SHA-256(sorted content_hashes of deps_transitive)
```

`dep_hash` feeds directly into the cache key computation (see CACHE-DESIGN.md).
