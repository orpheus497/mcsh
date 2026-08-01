# Data Model — C Workflow State

> **Status: future target** (P2+). The data model below defines the design goal
> for the full compile/build pipeline. The P1 `run` implementation uses a simpler
> in-function state structure (`cw_run_state_t`) rather than these shared tables.

---

## Design principles

- **Data-oriented**: flat tables with stable integer IDs, not pointer graphs.
- **Arena allocation**: transient per-command arenas for scratch; persistent
  arena snapshots for the cache index.
- **Deterministic**: the same inputs always produce the same keys and artifacts.
- **Minimal I/O**: in-memory tables during a build; flushed to disk only at end
  of command (compile/build) or on explicit cache write.
- **Recoverable**: corrupt on-disk state is detected and rebuilt from sources.

---

## Tables

### `file_table`

One row per filesystem path ever seen by the C workflow runtime.

| Column | Type | Notes |
|--------|------|-------|
| `file_id` | `uint32_t` | Stable ID; never reused within a session |
| `path` | `str_id` | Interned canonical (realpath) absolute path |
| `mtime_ns` | `int64_t` | Last-modified time in nanoseconds (POSIX clock) |
| `size_bytes` | `uint64_t` | File size at last scan |
| `content_hash` | `uint8_t[32]` | SHA-256 of file content |
| `hash_valid` | `bool` | Whether `content_hash` is current (mtime-gated) |
| `kind` | `enum file_kind` | `FILE_SRC`, `FILE_HDR`, `FILE_OBJ`, `FILE_BIN`, `FILE_OTHER` |
| `scan_epoch` | `uint32_t` | Build epoch when last scanned |

**Invariant**: `file_id` is monotonically assigned. Paths are deduplicated via
the string intern arena before insertion.

---

### `unit_table`

One row per compilation unit (a `.c` source file under a given profile).

| Column | Type | Notes |
|--------|------|-------|
| `unit_id` | `uint32_t` | Stable ID |
| `src_file_id` | `uint32_t` | FK → `file_table` |
| `profile_id` | `uint32_t` | FK → `profile_table` |
| `language` | `enum lang` | `LANG_C89`, `LANG_C99`, `LANG_C11`, `LANG_C17` |
| `dep_scan_epoch` | `uint32_t` | Epoch when deps were last scanned |
| `compile_epoch` | `uint32_t` | Epoch when last compiled |
| `artifact_id` | `uint32_t` | FK → `artifact_table` (current object) |
| `state` | `enum unit_state` | See lifecycle below |

**Invariant**: `(src_file_id, profile_id)` is a unique key.

---

### `dep_edge_table`

Records `#include` dependency edges from a unit to header files.

| Column | Type | Notes |
|--------|------|-------|
| `edge_id` | `uint32_t` | Stable ID |
| `unit_id` | `uint32_t` | FK → `unit_table` (dependent) |
| `dep_file_id` | `uint32_t` | FK → `file_table` (included header) |
| `include_kind` | `enum inc_kind` | `INC_ANGLE` (`<>`), `INC_QUOTE` (`""`) |
| `include_depth` | `uint8_t` | Nesting depth in include chain |
| `discovered_epoch` | `uint32_t` | Epoch of discovery |

**Invariant**: The set of edges for a unit is replaced atomically on re-scan.
No partial updates; old edges are deleted before new ones are inserted.

---

### `artifact_table`

Compiled outputs: object files, linked binaries, shared objects.

| Column | Type | Notes |
|--------|------|-------|
| `artifact_id` | `uint32_t` | Stable ID |
| `kind` | `enum artifact_kind` | `ART_OBJ`, `ART_BIN`, `ART_SHARED` |
| `cache_key` | `uint8_t[32]` | SHA-256 cache key (see CACHE-DESIGN.md) |
| `path` | `str_id` | Absolute path within `~/.mcsh_cache/cworkflow/` |
| `size_bytes` | `uint64_t` | Artifact size |
| `created_ns` | `int64_t` | Creation timestamp |
| `last_used_ns` | `int64_t` | Last access (for LRU eviction) |
| `producer_cmd` | `str_id` | Exact compiler command used to produce it |
| `valid` | `bool` | Whether artifact is considered fresh |

---

### `profile_table`

Normalized compile/link option vectors.

| Column | Type | Notes |
|--------|------|-------|
| `profile_id` | `uint32_t` | Stable ID |
| `profile_hash` | `uint8_t[32]` | SHA-256 of normalized flag vector |
| `flags_str` | `str_id` | Space-separated sorted flag string |
| `include_paths` | `str_id[]` | Ordered `-I` paths |
| `defines` | `str_id[]` | Ordered `-D` defines |
| `target_triple` | `str_id` | e.g. `x86_64-unknown-freebsd14.0` |
| `toolchain_id` | `uint32_t` | FK → `toolchain_table` |

**Invariant**: profiles are interned — identical flag sets share one `profile_id`.

---

### `toolchain_table`

Identity record for the compiler backend being used.

| Column | Type | Notes |
|--------|------|-------|
| `toolchain_id` | `uint32_t` | Stable ID |
| `cc_path` | `str_id` | Absolute path to compiler binary |
| `cc_version` | `str_id` | Version string (`clang --version` first line) |
| `cc_hash` | `uint8_t[32]` | SHA-256 of compiler binary content |
| `detected_epoch` | `uint32_t` | When this toolchain was last verified |

**Invariant**: a change in `cc_path`, `cc_version`, or `cc_hash` creates a new
`toolchain_id` and invalidates all artifacts produced by the old one.

---

### `diagnostic_table`

Compiler-emitted messages associated with a unit.

| Column | Type | Notes |
|--------|------|-------|
| `diag_id` | `uint32_t` | Stable ID |
| `unit_id` | `uint32_t` | FK → `unit_table` |
| `file_id` | `uint32_t` | FK → `file_table` (where the diagnostic originates) |
| `line` | `uint32_t` | 1-based line number |
| `col` | `uint32_t` | 1-based column number |
| `severity` | `enum diag_sev` | `DIAG_NOTE`, `DIAG_WARN`, `DIAG_ERR`, `DIAG_FATAL` |
| `message` | `str_id` | Diagnostic message text |
| `tool_source` | `str_id` | Tool that emitted it (e.g. `"clang-18"`) |
| `epoch` | `uint32_t` | Build epoch when recorded |

---

### `job_table`

Per-command execution pipeline state (transient; not persisted to disk).

| Column | Type | Notes |
|--------|------|-------|
| `job_id` | `uint32_t` | Session-scoped ID |
| `kind` | `enum job_kind` | `JOB_COMPILE`, `JOB_LINK`, `JOB_RUN` |
| `unit_id` | `uint32_t` | FK → `unit_table` (0 for link/run jobs) |
| `artifact_id` | `uint32_t` | Target artifact |
| `status` | `enum job_status` | See lifecycle below |
| `pid` | `pid_t` | Child process PID (0 if not started) |
| `exit_code` | `int` | Exit code when complete |
| `start_ns` | `int64_t` | Wall-clock start time |
| `end_ns` | `int64_t` | Wall-clock end time |
| `dep_count` | `uint32_t` | Number of prerequisite jobs not yet done |

---

## Arenas

### String intern arena

- One global intern arena per mcsh session.
- Maps `const char *` → `str_id` (uint32_t offset into arena buffer).
- Deduplicates paths, flag strings, version strings.
- Survives across `compile`/`build`/`run` invocations within a session.

### Transient command arena

- Allocated at the start of each `compile`/`build`/`run` invocation.
- Holds temporary dep-scan results, partial job tables, scratch buffers.
- Freed atomically at end of invocation (cleanup_push pattern).

### Persistent metadata arena

- Serialized to `~/.mcsh_cache/cworkflow/index.db` at end of each build.
- Contains: `file_table`, `unit_table`, `dep_edge_table`, `artifact_table`,
  `profile_table`, `toolchain_table`.
- NOT `job_table` or `diagnostic_table` (transient).

---

## State transitions

### Unit lifecycle (`enum unit_state`)

```
UNKNOWN ──scan──► PENDING ──compile──► COMPILED
                     │                    │
                  (error)            (dep changed)
                     │                    │
                     ▼                    ▼
                  FAILED              STALE ──recompile──► COMPILED
```

| State | Meaning |
|-------|---------|
| `UNKNOWN` | Not yet seen this session |
| `PENDING` | Source discovered, deps not yet fully resolved |
| `STALE` | Source or dep hash changed since last compile |
| `COMPILED` | Artifact is current and valid |
| `FAILED` | Last compile produced errors |

### Job lifecycle (`enum job_status`)

```
QUEUED ──start──► RUNNING ──done──► DONE
                     │
                  (error)
                     │
                     ▼
                  FAILED
```

---

## Invariants

1. A `unit_table` row for `(src_file_id, profile_id)` always has a corresponding
   `artifact_table` row if `state == COMPILED`.
2. `dep_edge_table` rows for a unit are fully replaced (never partially updated)
   when deps are re-scanned.
3. `artifact_table.valid` is set to `false` before any recompilation begins,
   preventing stale artifact use if the build is interrupted.
4. No two `profile_table` rows have the same `profile_hash`.
5. `toolchain_table` rows are append-only; old entries remain for historical
   artifact attribution.
6. `job_table` is transient and is never written to the persistent index.

---

## Persistence and serialization

### Format choices

| Concern | Choice | Rationale |
|---------|--------|-----------|
| Index storage | Custom binary flat-file | No external dependency; trivial to validate header magic |
| String intern blob | Length-prefixed byte array | Compact; O(1) lookup by offset |
| Cache key | Raw 32-byte SHA-256 | Portable; no encoding overhead |
| Metadata files per artifact | `.meta` sidecar (plain text key=value) | Human-readable; no parser needed |
| Diagnostics | Not persisted (recompile to re-emit) | Avoids stale message accumulation |

### Index file layout

```
[magic: 8 bytes]          "MCSHDX01"
[version: uint32_t]       format version
[epoch: uint32_t]         build epoch counter
[table offsets: N×8]      byte offset of each table section
[string arena blob]       packed interned strings
[file_table section]      fixed-width rows
[unit_table section]      fixed-width rows
[dep_edge_table section]  fixed-width rows
[artifact_table section]  fixed-width rows
[profile_table section]   fixed-width rows
[toolchain_table section] fixed-width rows
[checksum: uint32_t]      CRC32 of entire file
```

On open, the magic, version, and checksum are verified. A mismatch triggers
full cache invalidation and a warning to stderr.

### Sidecar artifact metadata

Each artifact has a `.meta` file alongside it:

```
cache_key=<hex-sha256>
producer=clang-18.0.0
target=x86_64-unknown-freebsd14.0
source=<hex-sha256-of-source>
created=<unix-timestamp-ns>
```

Used to validate an artifact without loading the full index.
