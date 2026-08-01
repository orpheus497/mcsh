# C command contracts (`compile`, `build`, `run`)

Status: **`run` implemented (P1 shipped)** — single `.c` file and directory/project targets,
object and binary caching, `--clean`, `-v`, and `--` arg separator are all supported.
`compile` and `build` are planned (P2/P3).

## Non-negotiable command boundary

- `.mcsh` scripts execute directly through normal shell semantics.
  - Examples: `mcsh script.mcsh`, `./script.mcsh`, `source script.mcsh`.
- `compile`, `build`, and `run` are C-focused commands only.
- `run` is not a generic script launcher.

## Command intent

### `compile`

Compile C translation units and emit/reuse object artifacts plus diagnostics.

### `build`

Build C targets/projects by orchestrating multi-unit compile + link steps.

### `run`

Compile-if-needed and execute C file/target; this is the primary fast-feedback command.

## Input validation and user-facing behavior

### Shared validation expectations

- If input cannot be resolved to a C file/target, fail with actionable usage guidance.
- Preserve shell-style exit codes and stderr diagnostics.
- Do not alter `.mcsh` script execution semantics.

### `run` misuse rule (`.mcsh` input)

If the user passes a `.mcsh` script to `run`, reject with a clear C-only error.

Suggested message (exact wording can vary):

`run: .mcsh scripts are executed directly. Use: mcsh <script.mcsh> (run is for C files/targets).`

## Initial CLI syntax proposals

### Phase-1 oriented syntax

- `run file.c`
- `compile file.c`
- `build .`

### Forward-compatible syntax examples

- `compile src/a.c src/b.c`
- `compile src/main.c -O2 -Iinclude`
- `build ./examples/hello`
- `build . --target app`
- `run src/main.c`
- `run .` (default runnable C target when unambiguous)

## Expected behavior examples

1. `mcsh build-script.mcsh`
   - Executes shell script as normal.

2. `run build-script.mcsh`
   - Fails with C-only guidance error.

3. `run hello.c`
   - Compiles if stale/missing artifact, then executes binary.

4. `compile hello.c`
   - Produces/updates object artifact and diagnostics only.

5. `build .`
   - Discovers/uses configured C target graph and links target artifact.

## Exit behavior (proposed)

- `0`: command succeeded.
- non-zero: validation, compile, link, or runtime failure.
- `run` returns executed program exit status when execution occurs.

## Minimal UX principles

- Keep command surface flat: only `compile`, `build`, `run` as first-class verbs.
- Keep `.mcsh` scripts outside `run` semantics.
- Favor explicit, concise error messages over implicit mode switching.
