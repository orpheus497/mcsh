# Cache Design

> **Status: P1 cache layout implemented** (object and binary cache paths in `sh.cworkflow.c`).
> `index.db`, toolchain identity cache, and sidecar metadata are planned for P2/P3.

---

## 1. Cache directory layout

```
~/.mcsh_cache/
└── cworkflow/
    ├── index.db          ← persistent metadata index (binary)
    ├── index.db.tmp      ← atomic write staging file
    ├── objects/
    │   └── <prefix2>/    ← first 2 hex chars of cache key
    │       └── <keyhex>.o        ← compiled object artifact
    │       └── <keyhex>.o.meta   ← sidecar metadata
    ├── binaries/
    │   └── <prefix2>/
    │       └── <keyhex>          ← linked binary artifact
    │       └── <keyhex>.meta     ← sidecar metadata
    └── toolchain/
        └── <toolchain_id>.json   ← toolchain identity cache
```

**Two-level prefix**: objects and binaries are stored in a 2-char prefix
subdirectory (first two hex chars of cache key) to avoid large flat directories
(performance degrades above ~10k entries in a flat dir on most filesystems).

---

## 2. Cache key composition

### 2.1 P1 `run` — project hash

`cw_run_prepare_state()` computes a **project_hash** that covers all source and
header inputs.  Sorted `.c` and `.h` file paths are collected into the
fingerprint list; their contents are then hashed in sorted order:

```
project_hash = SHA-256(
    content(fingerprints[0])    ← sorted .c/.h file content
    content(fingerprints[1])
    ...
    content(fingerprints[N-1])
)
```

Sorting ensures the hash is independent of discovery order.  Any change to a
source or header file produces a different project_hash, which in turn
invalidates both object and binary cache entries.

### 2.2 P1 `run` — object cache key

Each compiled object is keyed by its own source content, its position within
the project, and the compiler identity:

```
key = SHA-256(
    "obj"                             [3 bytes]   domain separator
    source_path                       [N bytes]   absolute source file path
    SHA-256(source_file)              [32 bytes]  source content hash
    project_hash                      [32 bytes]  SHA-256 of sorted project inputs (§2.1)
    cc_hash                           [32 bytes]  SHA-256 of compiler binary (or path)
)
```

Cache is invalidated by: any source or header change (via project_hash), a
different source file (source_hash or source_path change), or a toolchain
change (cc_hash).

### 2.3 P1 `run` — binary cache key

The final linked binary is keyed by the full project input set and the
compiler/linker identity:

```
key = SHA-256(
    "bin"                             [3 bytes]   domain separator
    project_hash                      [32 bytes]  SHA-256 of sorted project inputs (§2.1)
    cc_hash                           [32 bytes]  SHA-256 of compiler binary (or path)
)
```

Any change to sources, headers, or compiler invalidates the binary cache.
Changes to compiler flags, include search paths, link flags, source order, or
object hash order also invalidate directory-scoped cached artifacts because they
alter project_hash or cc_hash.

### 2.4 Planned P2/P3 — extended object cache key (future)

The `compile` command (P2) will use a richer key to support per-unit
incremental caching with dependency tracking:

```
key = SHA-256(
    content_hash(source_file)         [32 bytes]
  + dep_hash                          [32 bytes]  (see DEPENDENCY-DISCOVERY.md §6)
  + profile_hash                      [32 bytes]  (see DATA-MODEL.md § profile_table)
  + toolchain.cc_hash                 [32 bytes]
  + target_triple_hash                [32 bytes]  SHA-256 of target triple string
)
```

### 2.5 Planned P2/P3 — extended binary cache key (future)

The `build` command (P3) will key binaries on an ordered sequence of per-unit
object keys with linker flags:

```
key = SHA-256(
    len(object_cache_keys) as uint32_le     [4 bytes]   number of objects
    object_cache_keys[0]                    [32 bytes]  in link order
    ...
    object_cache_keys[N-1]                  [32 bytes]
  + link_flags_hash                         [32 bytes]  SHA-256 of linker argv (original order)
  + toolchain.cc_hash                       [32 bytes]
  + target_triple_hash                      [32 bytes]
)
```

Object keys are hashed as a **length-delimited ordered sequence** to preserve
multiplicity and order.  The linker argv is hashed in its **original order**
so that order-sensitive flags produce distinct keys.

### 2.6 Normalization before hashing

- Remove redundant flags (duplicate `-O`, etc.).
- Normalize path separators.

Include paths (`-I`) and linker flags are **not sorted** because their order is
semantically significant (include search priority; static library resolution
order).

---

## 3. Artifact layout and metadata

### 3.1 Object artifact

```
objects/<prefix2>/<keyhex>.o        ← ELF/Mach-O object file
objects/<prefix2>/<keyhex>.o.meta   ← text sidecar
```

Sidecar format (plain text, key=value one per line):

```
magic=MCSHOBJ1
cache_key=<64-char hex>
source_path=<canonical source path>
source_hash=<64-char hex>
dep_hash=<64-char hex>
profile_hash=<64-char hex>
toolchain_hash=<64-char hex>
producer=clang-18.0.0
target=x86_64-unknown-freebsd14.0
created_ns=<unix ns timestamp>
```

### 3.2 Binary artifact

```
binaries/<prefix2>/<keyhex>         ← linked executable (mode 0755)
binaries/<prefix2>/<keyhex>.meta    ← text sidecar
```

Sidecar format:

```
magic=MCSHBIN1
cache_key=<64-char hex>
object_keys=<key1>,<key2>,...
link_flags_hash=<64-char hex>
toolchain_hash=<64-char hex>
target=x86_64-unknown-freebsd14.0
created_ns=<unix ns timestamp>
entry_point=main
```

---

## 4. Cache lookup protocol

```
1. Compute cache_key for the requested artifact
2. Construct path: <cache_dir>/objects/<key[0:2]>/<key_hex>.o
3. If path does not exist → cache miss
4. Read sidecar .meta file:
     a. Verify magic string matches expected artifact type
     b. Re-read and re-hash source file; compare to source_hash in meta
     c. If hash mismatch → treat as miss (stale artifact)
5. Cache hit: artifact is valid, skip compilation
```

The meta file double-check (step 4b) guards against scenarios where the
index.db is out of date (e.g., after a crash or manual cache edit).

---

## 5. Pruning and eviction

### 5.1 LRU eviction

Each artifact's `last_used_ns` is updated on access (cache hit for compilation
or binary execution via `run`).

Eviction is triggered at the start of `compile`/`build`/`run` if:

```
total_cache_size > $mcsh_cache_max_bytes   (default: 512 MiB)
```

Eviction procedure:
1. Load all artifact records from `artifact_table`.
2. Sort by `last_used_ns` ascending (oldest first).
3. Delete artifacts (disk files + meta) until `total_cache_size <= target_size`.
4. `target_size = 0.8 × $mcsh_cache_max_bytes` (headroom to avoid thrashing).
5. Update `artifact_table` to remove evicted rows.
6. Flush index.

### 5.2 Manual cleanup

```csh
compile --clean file.c      # remove specific unit's artifacts
build --clean .             # remove all artifacts for this project
```

`--clean` does not wipe the entire cache; it targets only the units in scope.

A future `mcsh --cache-clean` subcommand may wipe the entire cache.

### 5.3 TTL-based pruning

Optionally (phase P4): artifacts not accessed within `$mcsh_cache_ttl_days`
(default: 30) are eligible for eviction on next access even if under size cap.

---

## 6. Corruption recovery

### 6.1 Corruption detection

At cache open:
- Read `index.db`, verify magic `MCSHDX01` and CRC32 checksum.
- If mismatch → **full index rebuild**:
  - Walk `objects/` and `binaries/` directories.
  - Re-read each `.meta` sidecar.
  - Reconstruct `artifact_table` from sidecars.
  - Re-verify source hashes; mark artifacts with missing/changed sources as invalid.
  - Write fresh `index.db`.

At artifact access:
- If sidecar is missing or malformed → treat as cache miss; recompile.
- If artifact file is missing → treat as cache miss.
- If artifact file fails a basic header check (ELF magic / Mach-O magic) → delete
  and treat as miss.

### 6.2 Partial write recovery

`index.db` is always written atomically via rename:
```
write → index.db.tmp
rename(index.db.tmp, index.db)   ← atomic on POSIX
```

If `index.db.tmp` exists on open (crash during write), it is deleted and the
existing `index.db` is used. This may lose the most recent cache entries but
never corrupts the cache.

### 6.3 Object file corruption

Object files are not verified beyond the meta-sidecar check. If a compiled
binary fails to execute (bad ELF), the user runs `run hello.c --clean` to
force recompilation.

---

## 7. Determinism and reproducibility

### 7.1 Deterministic keys

Given the same source tree, the same flags, and the same toolchain version,
the cache key is always identical across machines and runs. This enables:

- Shared caches across a development team (future: networked cache backend).
- Reproducible builds where the cache can be verified by key.

### 7.2 Toolchain identity pinning

The `toolchain.cc_hash` component ensures that upgrading the compiler
invalidates all prior artifacts. Downgrading also invalidates (different hash).

### 7.3 Non-determinism sources (known)

| Source | Impact | Mitigation |
|--------|--------|-----------|
| Timestamp embedded in object by compiler | Breaks key equality on rebuild | Pass `-fno-record-gcc-switches` or equivalent; file hash still differs |
| Absolute path embedded in debug info | Same binary produces different hash | Use `-fdebug-prefix-map` (optional; debug builds only) |
| Randomized address layout | Does not affect cached object key | N/A |
| Concurrent build on NFS | mtime resolution may be coarse | Use content hash as primary, mtime as fast-path only |

True byte-for-byte reproducibility is a P4+ goal. P1–P3 targets correctness,
not full determinism.
