# CLI Specification — `compile`, `build`, `run`

> **Status: planning / documentation only.**
> No implementation exists yet.

---

## 1. Command syntax overview

```
compile <file.c> [file.c ...] [options]
build   [target]              [options]
run     <file.c|target>    [-- program-args...]
```

All three commands are shell builtins. They follow the same argument
convention as all other mcsh builtins (`Char **v` argv array).

---

## 2. `compile`

### Syntax

```
compile <source-file> [source-file ...] [option ...]
```

At least one source file is required (`minargs = 1` in `bfunc[]`).

### Options

| Option | Argument | Effect |
|--------|----------|--------|
| `-I <dir>` | directory | Add `dir` to include search path |
| `-D <name>[=val]` | define | Define preprocessor symbol |
| `-O <level>` | 0/1/2/3/s | Optimization level (passed to backend cc) |
| `-std <std>` | c89/c99/c11/c17 | C standard to compile against |
| `-Wall` | — | Enable all standard warnings |
| `-Werror` | — | Treat warnings as errors |
| `-g` | — | Emit debug information |
| `-p <profile>` | debug/release/sanitize | Named profile preset |
| `--clean` | — | Remove cached artifacts before compiling |
| `--emit-db` | — | Output compile_commands.json after compilation |
| `--dry-run` | — | Show what would be compiled without executing |
| `-v` | — | Verbose: print compiler invocations |

### Examples

```csh
# Compile a single file (uses defaults: -O0 -g, c11)
compile src/main.c

# Compile multiple files with include path
compile src/main.c src/util.c -Iinclude -O2

# Debug profile preset
compile src/main.c -p debug

# Release profile
compile src/main.c src/util.c -p release

# Clean rebuild
compile src/main.c --clean

# Emit compile_commands.json for IDE
compile src/*.c -Iinclude --emit-db
```

### Argument ambiguity rules

| Argument form | Interpretation |
|---------------|----------------|
| Ends with `.c` | Source file |
| Begins with `-` | Option |
| Ends with `.mcsh` | **Error**: see misuse errors section |
| Is a directory | **Error**: `compile` does not recurse; use `build` |
| Unrecognized option | **Warning** (passed to backend cc unchanged) |
| `--` | End of mcsh options; remaining args passed to cc verbatim |

---

## 3. `build`

### Syntax

```
build [target] [option ...]
```

`target` defaults to `.` (current directory) if omitted.

### Options

| Option | Argument | Effect |
|--------|----------|--------|
| `-o <name>` | binary name | Output binary name (default: directory base name) |
| `-p <profile>` | debug/release/sanitize | Named profile preset |
| `--jobs <N>` | integer | Parallel compile jobs (default: min(nproc, 8)) |
| `--clean` | — | Remove all cached artifacts for this target |
| `--dry-run` | — | Show build plan without executing |
| `--emit-db` | — | Output compile_commands.json |
| `-v` | — | Verbose: print all compiler/linker invocations |
| `-Wall` | — | Enable warnings for all units |
| `-Werror` | — | Warnings are errors for all units |

### Examples

```csh
# Build current directory (default target)
build

# Build specific directory
build examples/hello

# Build with release profile
build . -p release

# Build with explicit output name
build . -o myapp

# Parallel build with 4 jobs
build . --jobs 4

# Clean rebuild
build . --clean

# Dry run to see what would build
build . --dry-run
```

### Target resolution

| Target argument | Interpretation |
|-----------------|----------------|
| `.` or empty | Current directory |
| Relative/absolute directory path | That directory |
| File path ending in `.c` | **Error**: use `compile` for single files, or `run` to compile and execute |
| File path ending in `.mcsh` | **Error**: see misuse errors section |

---

## 4. `run`

### Syntax

```
run <target> [-- program-args...]
```

`target` is required (`minargs = 1`).

Arguments after `--` are passed directly to the compiled binary as `argv`.

### Options (before `--`)

| Option | Argument | Effect |
|--------|----------|--------|
| `-p <profile>` | debug/release/sanitize | Profile for compilation step |
| `-I <dir>` | directory | Include path override |
| `-D <name>[=val]` | define | Preprocessor define override |
| `-g` | — | Include debug info |
| `--clean` | — | Recompile even if cache is fresh |
| `-v` | — | Verbose compile output before execution |

### Examples

```csh
# Compile and run hello.c
run hello.c

# Run with arguments to the binary
run hello.c -- Alice Bob

# Run a multi-file project's default target
run .

# Force recompile then run
run hello.c --clean

# Run with debug build
run hello.c -p debug -- --verbose
```

### Argument rules

| Argument form | Interpretation |
|---------------|----------------|
| Ends with `.c` | C source file: compile if needed, then run |
| Is a directory | Project target: build if needed, then run default binary |
| Ends with `.mcsh` | **Error**: see misuse errors section below |
| `--` | Separator; everything after is passed to binary |
| Unrecognized option before `--` | Error: unknown option |

---

## 5. Misuse errors

### `run` with `.mcsh` input

```
% run script.mcsh
run: 'script.mcsh' is a shell script, not a C file.
Shell scripts execute directly — no 'run' needed:
  mcsh script.mcsh
  ./script.mcsh  (if executable)
'run' is for C files only.
```

Exit code: `1`.

### `compile` with `.mcsh` input

```
% compile script.mcsh
compile: 'script.mcsh' is a shell script, not a C source file.
To execute a script: mcsh script.mcsh
```

Exit code: `1`.

### `compile` with a directory

```
% compile ./src
compile: './src' is a directory.
To build a project directory, use: build ./src
```

Exit code: `1`.

### `run` with no arguments

```
% run
run: missing argument.
Usage: run <file.c|target> [-- program-args...]
```

Exit code: `1`.

### Toolchain not found

```
% run hello.c
run: no C compiler found. Install clang or cc and ensure it is in $PATH.
```

Exit code: `5`.

---

## 6. Option matrix summary

| Option | `compile` | `build` | `run` |
|--------|-----------|---------|-------|
| `-I <dir>` | ✓ | — | ✓ |
| `-D <name>` | ✓ | — | ✓ |
| `-O <level>` | ✓ | — | — |
| `-std <std>` | ✓ | — | — |
| `-Wall` | ✓ | ✓ | — |
| `-Werror` | ✓ | ✓ | — |
| `-g` | ✓ | — | ✓ |
| `-p <profile>` | ✓ | ✓ | ✓ |
| `-o <name>` | — | ✓ | — |
| `--jobs <N>` | — | ✓ | — |
| `--clean` | ✓ | ✓ | ✓ |
| `--emit-db` | ✓ | ✓ | — |
| `--dry-run` | ✓ | ✓ | — |
| `-v` | ✓ | ✓ | ✓ |
| `-- <args>` | — | — | ✓ |

---

## 7. Profile presets

| Profile | Flags applied |
|---------|---------------|
| `debug` | `-O0 -g -Wall -fno-omit-frame-pointer` |
| `release` | `-O2 -DNDEBUG` |
| `sanitize` | `-O1 -g -fsanitize=address,undefined` |

Default (no `-p`): `debug` profile.

Shell variable `$mcsh_cflags` is appended after profile flags and before
per-invocation options, allowing global overrides in `~/.mcshrc`.

---

## 8. Backward compatibility

### Existing shell semantics

- None of `compile`, `build`, or `run` are current builtin names in mcsh.
- If an existing alias or external command named `compile`/`build`/`run` is in
  `$PATH` or `$aliases`, adding these builtins will shadow them.
  - This is intentional and expected.
  - Users may escape via `\compile`, `\build`, `\run` to reach external commands.
- `.mcsh` scripts are **not** affected by any of these additions.

### Escape hatch

The standard tcsh/mcsh builtin escape syntax applies:

```csh
\run       # bypass builtin, use PATH lookup
command run  # same effect
```

### Shell variable integration

The following variables are consulted by C workflow commands:

| Variable | Consumed by | Meaning |
|----------|-------------|---------|
| `$mcsh_cflags` | compile, build, run | Extra flags added to all compilations |
| `$mcsh_jobs` | build | Default parallel job count |
| `$mcsh_cc` | all | Override compiler path (default: auto-detected) |
| `$mcsh_cache_dir` | all | Override cache directory (default: `~/.mcsh_cache/cworkflow`) |
