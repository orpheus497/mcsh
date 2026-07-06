# Deep Analysis: `strncpy` and Manual Null-Termination Pattern

## The Anti-Pattern
Across the codebase, there is a recurring pattern of using `strncpy` followed by manual null-termination to copy strings into fixed-size buffers.

```c
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

### Vulnerability & Risks
1. **Truncation Without Detection:** If `src` is larger than `sizeof(dest) - 1`, `strncpy` truncates the string. The application proceeds without knowing truncation occurred, which can lead to incomplete paths, malformed git branches, or partial hostnames.
2. **Performance Overhead:** `strncpy` is required to pad the *entire* remaining buffer with null bytes if `src` is shorter than `dest`. In large buffers, this causes unnecessary performance degradation.
3. **Maintenance Hazard:** It is an outdated idiom that is easily prone to off-by-one errors during refactoring.

## Replications in Codebase

### 1. `ed.chared.c` (Path and Match Caching)
Applies to: Autocompletion directory paths, prefixes, and cache buffers.
- `predict_file()`: Copying `mb` and `expanded` strings into the `word` buffer. (Fixed previously)
- `f_cache` updates (lines 4087-4091): `strncpy(f_cache.cwd, current_cwd, sizeof(f_cache.cwd) - 1);`
- Match caching (line 4074): `strncpy(match, name + pfxlen, sizeof(match) - 1);`

### 2. `tc.prompt.c` (Git Prompt Parsing)
Applies to: Git branch names and operation states (e.g., REBASING, MERGING).
- Branch parsing (lines 311, 314, 318): `strncpy(branch, path + 16, branchsz - 1);`
- Operation states (lines 334): `strncpy(op, "MERGING", opsz - 1);`

### 3. `ed.syntax.c` (Syntax Command Cache)
Applies to: Command name caching for syntax highlighting.
- Command cache update (line 208): `strncpy(cmd_cache[victim].name, name, CMD_CACHE_NAMELEN - 1);`

### 4. `tc.func.c` (Host Resolution)
Applies to: Canonname host resolution buffer.
- Hostname resolution (line 1973): `strncpy(hbuf, res->ai_canonname, sizeof(hbuf));`
*(Note: line 1973 uses `sizeof(hbuf)` instead of `sizeof(hbuf) - 1`. This is a bounded copy that prevents overflow but may silently truncate the canonical name if it equals or exceeds `sizeof(hbuf)`, relying entirely on the manual terminator at line 1974. This masks silent truncation failures during host resolution).*

### 5. `tc.who.c` (UTMP Who Parsing)
Applies to: Usernames, hostnames, and TTYs parsed from utmp.
*(Note: The `strncpy` calls here use constants like `UTNAMLEN` and `UTHOSTLEN` without explicit manual null termination immediately following, but still represent bounded copies using `strncpy` without explicit truncation checks).*

## Recommendation
Replace these instances with the bounded, null-terminating safe function `xsnprintf`:
```c
xsnprintf(dest, sizeof(dest), "%s", src);
```
