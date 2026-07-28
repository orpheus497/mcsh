# Architecture — C Workflow Integration

> **Status: planning / documentation only.**
> All file/function references are to the current codebase as of the P0 analysis.

---

## 1. Current mcsh execution architecture

### 1.1 High-level flow

```
stdin / file / -c string
         │
         ▼
   sh.c: process()            ← main read/parse/execute loop
         │
         ▼
   sh.lex.c: lex()            ← tokenise input → struct wordent list
         │
         ▼
   sh.parse.c: syntax()       ← build command AST (struct command tree)
         │
         ▼
   sh.sem.c: execute()        ← walk AST, dispatch each command node
         │
    ┌────┴────────────────────────────────────┐
    │                                         │
    ▼                                         ▼
 Builtin                               External command
 sh.func.c: isbfunc()                  sh.exec.c: doexec()
 sh.func.c: func()                     (fork + execvp)
    │
    ▼
 bfunc[] handler (e.g. doalias, doset, …)
```

### 1.2 Concrete file and function references

| Stage | File | Function | Line reference |
|-------|------|----------|----------------|
| Main loop | `sh.c` | `process()` | `sh.c:1910` |
| Tokeniser | `sh.lex.c` | `lex()` | `sh.lex.c` (top-level) |
| Parser | `sh.parse.c` | `syntax()` | `sh.parse.c:206` |
| AST executor | `sh.sem.c` | `execute()` | `sh.sem.c:80` |
| Command dispatch | `sh.sem.c` | `execute()` command branch | `sh.sem.c:272` |
| Builtin lookup | `sh.func.c` | `isbfunc()` | `sh.func.c:66` |
| Builtin invoke | `sh.func.c` | `func()` | `sh.func.c:126` |
| Builtin invoke site | `sh.sem.c` | `execute()` builtin branch | `sh.sem.c:619` |
| External command | `sh.exec.c` | `doexec()` | `sh.exec.c:144` |
| Builtin registry | `sh.init.c` | `bfunc[]` | `sh.init.c:42` |

### 1.3 `.mcsh` script execution path

The script/source path is entirely separate from C workflow and must remain unchanged:

| Step | File | Function | Notes |
|------|------|----------|-------|
| Script open | `sh.c` | `srcfile()` | `sh.c:1472` — opens file, calls `srcunit()` |
| Script run | `sh.c` | `srcunit()` | `sh.c:1654` — saves state, runs `process(0)`, restores |
| `source` builtin | `sh.c` | `dosource()` | `sh.c:2111` — wraps `srcfile()` |
| Shebang execution | kernel → `sh.c` | `main()` + `srcfile()` | OS hands file to mcsh via `#!` path |

**This path is read-only from the perspective of C workflow.** Nothing in
`compile`/`build`/`run` should touch `srcfile()`, `srcunit()`, or `dosource()`.

---

## 2. Builtin registration mechanism

### 2.1 Registry structure

```c
/* sh.init.c:42 — bfunc[] is the complete builtin table */
const struct biltins bfunc[] = {
    { ":",        dozip,    0, INF },
    { "@",        dolet,    0, INF },
    { "alias",    doalias,  0, INF },
    /* … sorted alphabetically … */
};
```

`struct biltins` (defined in `sh.h`):

```c
struct biltins {
    const char *bname;          /* command name (ASCII, sorted) */
    void      (*bfunct)(Char **, struct command *);  /* handler */
    int        minargs;         /* minimum argument count */
    int        maxargs;         /* maximum argument count (INF = unlimited) */
};
```

### 2.2 Lookup algorithm

`isbfunc()` in `sh.func.c:66` performs a **binary search** over `bfunc[]`.
The table **must remain sorted** by `bname`. Misordering silently breaks lookup.

### 2.3 Invocation

`func()` in `sh.func.c:126` validates argument counts and calls `bp->bfunct`.
Called from `execute()` at `sh.sem.c:619` after `isbfunc()` returns non-NULL.

---

## 3. Integration points for C workflow

The integration is additive: new entries in sorted positions, new handler
functions, new declarations. Existing code paths are not modified.

### 3.1 Builtin registration — `sh.init.c`

Current `TODO(mcsh-c-workflow)` marker is at `sh.init.c:42` (in `bfunc[]`).

Sorted insertion positions:

| Command | Sorted position | Neighbors |
|---------|-----------------|-----------|
| `"build"` | after `"bg"`, before `"builtins"` | `bg` … `builtins` |
| `"compile"` | after `"complete"`, before `"continue"` | `complete` … `continue` |
| `"run"` | after `"return"`, before `"sched"` | `return` … `sched` |

Example additions (not yet implemented):

```c
/* sh.init.c — future additions, positions relative to sorted bfunc[] */
{ "build",    dobuild,    0, INF },   /* after "bg", before "builtins"  */
{ "compile",  docompile,  1, INF },   /* after "complete", before "continue" */
{ "run",      dorun,      1, INF },   /* after "return", before "sched"  */
```

### 3.2 Function declarations — `sh.decls.h`

Add to `sh.decls.h` in the `sh.func.c` section:

```c
extern void docompile(Char **, struct command *);
extern void dobuild  (Char **, struct command *);
extern void dorun    (Char **, struct command *);
```

### 3.3 Handler implementation — new `sh.cworkflow.c`

Create `sh.cworkflow.c` alongside the other `sh.*.c` files. Add it to the
`SRCS` variable in `Makefile.in`.

```c
/* sh.cworkflow.c — C workflow builtin implementations */
/* TODO(mcsh-c-workflow): implement docompile, dobuild, dorun here */

void dorun    (Char **v, struct command *c) { /* Phase 1 */ }
void docompile(Char **v, struct command *c) { /* Phase 2 */ }
void dobuild  (Char **v, struct command *c) { /* Phase 3 */ }
```

### 3.4 Build system — `Makefile.in`

Add `sh.cworkflow.c` / `sh.cworkflow.o` to `SRCS` and object list.
Pattern matches the existing flat-file build structure.

---

## 4. Boundary: `.mcsh` script path vs C workflow path

```
User invokes command
        │
        ├─── Is it a builtin? ─── isbfunc() lookup ─────────────────────┐
        │                                                                │
        │    (future builtins:                                           │
        │     "build", "compile", "run" ──────── C workflow path)       │
        │                                                                │
        ├─── External path? ── doexec() ── fork+exec                    │
        │                                                                │
        └─── Script sourcing?                                            │
             srcfile() / srcunit() / dosource()                         │
             (.mcsh execution — NEVER touches C workflow path)          │
                                                                         ▼
                                                               sh.cworkflow.c
                                                               dorun / docompile / dobuild
```

The two paths **never intersect**. The `run` builtin rejects `.mcsh` input
with an explicit error before entering any C workflow logic.

---

## 5. Component diagram — compile/build/run pipeline

```
                    User command line
                          │
              ┌───────────┼───────────┐
              │           │           │
           compile      build        run
              │           │           │
              └─────┬─────┘           │
                    │                 │
             ┌──────▼──────┐    ┌─────▼─────┐
             │  Validator   │    │  Validator │
             │ (input type  │    │ (C-only,   │
             │  checks)     │    │  .mcsh err)│
             └──────┬──────┘    └─────┬─────┘
                    │                 │
             ┌──────▼──────┐    ┌─────▼─────┐
             │  Dep scanner │    │  Dep scan  │
             │  (include    │    │  + cache   │
             │   graph)     │    │  lookup    │
             └──────┬──────┘    └─────┬─────┘
                    │                 │
             ┌──────▼──────┐    ┌─────▼─────┐
             │  Cache index │    │  Cache hit?│
             │  lookup      │    │  Yes→skip  │
             └──────┬──────┘    └─────┬─────┘
                    │           (cache miss)
             ┌──────▼──────┐    ┌─────▼─────┐
             │  Compiler    │    │  Compiler  │
             │  invocation  │    │  + linker  │
             │  (cc/clang)  │    │  invoc.    │
             └──────┬──────┘    └─────┬─────┘
                    │                 │
             ┌──────▼──────┐    ┌─────▼─────┐
             │  Artifact    │    │  Execute   │
             │  storage     │    │  binary    │
             │  + metadata  │    │  (execvp)  │
             └─────────────┘    └───────────┘
```

---

## 6. Key design constraints from current codebase

1. **Binary-search table** — `bfunc[]` must stay sorted (`sh.func.c:66-83`).
2. **Char \* string type** — all builtin args are `Char *` (wide-char typedef in
   `sh.types.h`); use `short2str()` / `str2short()` for conversion.
3. **Error API** — use `stderror(ERR_NAME, ...)` or `setname()` + `stderror()`
   for user-visible errors, matching existing builtin patterns in `sh.func.c`.
4. **Process management** — `execute()` has complex fork/no-fork logic; builtin
   handlers should not fork themselves in P1 (delegate exec to `execvp` after
   compilation instead).
5. **Cleanup protocol** — tcsh uses `cleanup_push()`/`cleanup_until()` for
   resource management; C workflow handlers must follow this pattern.
6. **Signal safety** — long-running child processes (compiler) need
   `signal(SIGINT, SIG_DFL)` before exec; interrupted builds should leave
   cache in a consistent state.
