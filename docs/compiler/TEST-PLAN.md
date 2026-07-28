# Test Plan

> **Status: planning / documentation only.**
> No tests exist yet for C workflow commands. This document defines the
> target test matrix for each phase.

---

## 1. Test infrastructure

Tests are shell scripts under `tests/` following the existing pattern
(`t001_vars.sh`, `t002_overflow.sh`, etc.). Run via `tests/run_tests.sh`.

C workflow tests will be numbered `t100_*` through `t199_*` to keep them
separate from shell feature tests.

Each test script:
- Sources `tests/lib_locale.sh` for locale setup.
- Builds a minimal fixture C file in `/tmp/mcsh_test_<N>/`.
- Invokes the mcsh binary with the command under test.
- Asserts exit code and output with `grep` or exact comparison.
- Cleans up temporary files in a trap.

---

## 2. Unit test matrix

### 2.1 Input validation — `run`

| ID | Test description | Expected result |
|----|-----------------|-----------------|
| T100 | `run` with no arguments | Exit 1, usage message on stderr |
| T101 | `run` with `.mcsh` file | Exit 1, `.mcsh` guard message on stderr |
| T102 | `run` with nonexistent file | Exit 1, file-not-found message |
| T103 | `run` with a directory (no build.mcsh) | Exit 1, appropriate message |
| T104 | `run` with valid `.c` file | Exit 0 (compile + run succeeds) |
| T105 | `run` passes exit code through | Binary exits 42; `run` exits 42 |
| T106 | `run` passes `--` args to binary | `argc`/`argv` inside binary matches |

### 2.2 Input validation — `compile`

| ID | Test description | Expected result |
|----|-----------------|-----------------|
| T110 | `compile` with no arguments | Exit 1, usage message |
| T111 | `compile` with `.mcsh` file | Exit 1, type-error message |
| T112 | `compile` with directory | Exit 1, use-`build` suggestion |
| T113 | `compile` with valid `.c` | Exit 0, object in cache |
| T114 | `compile` with nonexistent file | Exit 1, not-found message |

### 2.3 Input validation — `build`

| ID | Test description | Expected result |
|----|-----------------|-----------------|
| T120 | `build` with empty directory | Exit 1, no-sources message |
| T121 | `build` with `.mcsh` as target | Exit 1, type-error message |
| T122 | `build` valid project | Exit 0, binary in cache |
| T123 | `build` with multiple entry points | Exit 1, ambiguous-target message |

### 2.4 Cache behavior

| ID | Test description | Expected result |
|----|-----------------|-----------------|
| T130 | `run` twice on same file (unchanged) | Second run skips compile step |
| T131 | `run` after source change | Recompile occurs |
| T132 | `run` after header change | Recompile occurs |
| T133 | `run --clean` | Always recompiles |
| T134 | `compile` after profile change | Recompile occurs |
| T135 | Cache with two different profiles | Both objects present in cache |

### 2.5 Dependency discovery

| ID | Test description | Expected result |
|----|-----------------|-----------------|
| T140 | Single file, no includes | Dep set is empty; key stable |
| T141 | File includes project header | Header in dep set |
| T142 | File includes system header | System header in dep set |
| T143 | Circular include (mutual) | Handled without infinite loop |
| T144 | Include depth > 64 | Warning, not error; outer deps included |
| T145 | Missing include | Scan completes; compiler emits error |

---

## 3. Integration test matrix

### 3.1 Single-file workflows

| ID | Scenario | Expected result |
|----|---------|-----------------|
| I100 | `run hello.c` where hello.c prints "hello" | stdout is "hello\n"; exit 0 |
| I101 | `compile hello.c` then verify object exists | Object present in cache dir |
| I102 | `run` after `compile` (warm cache) | No recompile; immediate exec |
| I103 | `run` with binary that reads `$argv` | Arguments correctly passed |

### 3.2 Multi-file workflows

| ID | Scenario | Expected result |
|----|---------|-----------------|
| I110 | `build .` on 3-file project | Binary produced; all 3 objects in cache |
| I111 | Change one file, `build .` again | Only changed file recompiles |
| I112 | Change shared header, `build .` | All units including that header recompile |
| I113 | `run .` on multi-file project | Correct binary executes |

### 3.3 Error propagation

| ID | Scenario | Expected result |
|----|---------|-----------------|
| I120 | `run` on file with syntax error | Compiler error on stderr; exit 3 |
| I121 | `build` where one file fails | All compile errors shown; no link; exit 3 |
| I122 | Link error (undefined symbol) | Linker error on stderr; exit 3 |

### 3.4 `.mcsh` script isolation

| ID | Scenario | Expected result |
|----|---------|-----------------|
| I130 | `mcsh script.mcsh` (existing behavior) | Script runs normally; no change |
| I131 | `source script.mcsh` (existing behavior) | Sourced normally; no change |
| I132 | `run script.mcsh` | Exit 1, `.mcsh` guard fires |
| I133 | `compile script.mcsh` | Exit 1, type-error fires |
| I134 | Regression: adding `run` builtin does not break script sourcing | All existing `t001`–`t018` tests pass |

---

## 4. Regression test matrix

Run the full existing test suite (`tests/run_tests.sh`) after every builtin
addition to verify no regressions in shell behavior:

| Test file | Coverage area |
|-----------|--------------|
| `t001_vars.sh` | Variable assignment |
| `t002_overflow.sh` | Arithmetic overflow |
| `t003_shortcircuit.sh` | Short-circuit evaluation |
| `t004_pipe_to_var.sh` | Pipe-to-variable |
| `t005_cd_stack.sh` | Directory stack |
| `t006_function_builtin.sh` | `function` builtin |
| `t007_arith_rsh.sh` | Arithmetic right-shift |
| `t008_unset_modifiers.sh` | Unset variable modifiers |
| `t009`–`t014` | Unicode handling |
| `t015`–`t016` | `.mcshrc` ls/aliases |
| `t017_cd_minus_n.sh` | `cd -N` |
| `t018_syntax_highlight.sh` | Syntax highlighting |

The regression suite must pass **with zero failures** after every C workflow
builtin is added to `bfunc[]`.

---

## 5. Golden tests for diagnostics

Golden tests capture exact diagnostic output to detect regressions in
error message wording or format.

### 5.1 `.mcsh` guard message golden

```
Input:  run script.mcsh
Stdout: (empty)
Stderr:
  run: 'script.mcsh' is a shell script, not a C file.
  Shell scripts execute directly — no 'run' needed:
    mcsh script.mcsh
    ./script.mcsh  (if executable)
  'run' is for C files only.
Exit: 1
```

### 5.2 Compile error format golden

```
Input:  run bad.c   (where bad.c contains: int main(){return oops;})
Stdout: (empty)
Stderr: (contains) bad.c:<line>:<col>: error: use of undeclared identifier 'oops'
Exit: 3
```

### 5.3 Toolchain not found golden

```
Input:  run hello.c  (with PATH stripped of cc/clang)
Stdout: (empty)
Stderr: run: no C compiler found. Install clang or cc and ensure it is in $PATH.
Exit: 5
```

---

## 6. Performance benchmarks

### 6.1 `run` latency targets

| Scenario | Target latency |
|----------|---------------|
| Cache hit, single file, no deps | < 50 ms |
| Cache hit, single file, 10 headers | < 100 ms |
| Cache miss, single file, no deps | < 500 ms (depends on cc speed) |
| Cache hit, 20-file project | < 200 ms |

Benchmarks are measured with `time run hello.c` (wall clock) on a modern
laptop with a warm filesystem cache.

### 6.2 `build` incremental performance target

| Scenario | Target |
|----------|--------|
| No-op build (all cached) | < 200 ms for 20-file project |
| Single file change rebuild (20-file project) | < 500 ms |
| Full cold build (20-file project) | < time for plain `clang *.c -o app` + 10% |

### 6.3 Benchmark test scripts

- `tests/bench_run_latency.sh` — measures `run hello.c` 10× after warm-up.
- `tests/bench_incremental_build.sh` — measures `build .` with/without changes.

Benchmarks are not part of the pass/fail CI matrix; they produce timing reports
for tracking performance over time.
