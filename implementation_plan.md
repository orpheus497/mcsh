# mcsh — Comprehensive Documentation, Technical Debt & Bloat Audit Plan

## Background

This plan derives from a verified, multi-pass deep analysis of the entire `mcsh`
source tree — every file in `tests/`, both `tcsh.man.in` (10,855 lines) and
`README.md` (359 lines), `ISSUES.md`, `PLAN.md`, and online research into
upstream `tcsh`/`etcsh` behaviour. Every finding is grounded in actual code
line references, not assumptions.

---

## Key Findings

### A. Man Page Gaps — Confirmed Against Source

Features **present in C source** with **no formal documentation** in `tcsh.man.in`:

| Gap | Source of Truth | Man Page Status |
|-----|----------------|-----------------|
| `cd -N` bottom-indexed navigation | [sh.dir.c:497-519](file:///home/orpheus497/Documents/Projects/mcsh/sh.dir.c) | Mentions `cd +N` and bare `cd -` but **never** `cd -N` |
| Pipe-to-variable full syntax | [sh.set.c:300-380](file:///home/orpheus497/Documents/Projects/mcsh/sh.set.c) | No example or formal description of `cmd \| set var` or `set var < file` |
| Subshell block redirection `{ }` | [sh.exp.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.exp.c) | `if ({ cmd >& /dev/null })` is legal; completely undocumented |
| `set syntax` token palette (12 categories) | [ed.syntax.h:81-95](file:///home/orpheus497/Documents/Projects/mcsh/ed.syntax.h) | One-sentence mention at line 8866; no token table, no colour list |
| `set predict` editor command cross-ref | [ed.chared.c:4410](file:///home/orpheus497/Documents/Projects/mcsh/ed.chared.c) | `predict-accept` editor cmd not cross-referenced |
| `%g` / `%G` caching & operation-state table | [tc.prompt.c:782-870](file:///home/orpheus497/Documents/Projects/mcsh/tc.prompt.c) | Bare one-liners at 8452-8455; no `GIT_POLL_INTERVAL`, no state table |
| `$mcsh` shell variable | [sh.c:81](file:///home/orpheus497/Documents/Projects/mcsh/sh.c), [tc.const.c:59](file:///home/orpheus497/Documents/Projects/mcsh/tc.const.c) | Not listed in Special Shell Variables; only `$tcsh` is listed |
| `function` argument passing (`$1`, `$argv`) | [sh.func.c:2746](file:///home/orpheus497/Documents/Projects/mcsh/sh.func.c) | Three forms described; arg binding, recursion limit (100) omitted |
| Ghost text bypass (known bug) | [ed.refresh.c:352](file:///home/orpheus497/Documents/Projects/mcsh/ed.refresh.c), [ISSUES.md:361](file:///home/orpheus497/Documents/Projects/mcsh/ISSUES.md) | No BUGS section exists in the man page |
| `NEW FEATURES (+)` section not updated | [tcsh.man.in:10173](file:///home/orpheus497/Documents/Projects/mcsh/tcsh.man.in) | Still describes only tcsh-vs-csh history; no mcsh Phase 5/9 features |

### B. README.md Gaps

| Gap | Status |
|-----|--------|
| Invocation flags matrix (`-b -c -e -f -i -l -m -n -s -v -V -x -X`) | Missing |
| Startup file cascade diagram | Missing |
| `@` arithmetic operator precedence table | Missing |
| `switch`/`case`/`breaksw` syntax example | Missing |
| `onintr` signal trapping | Missing |
| Variable modifier table (`:h :t :r :e :l :u :q :x :gh :gt`) | Missing |
| `filetest` operators list | Missing |
| `repeat` and `goto` syntax | Missing |
| `sched` event scheduling examples | Missing |
| 54 of 72 builtins have no README entry | Missing |
| Known limitations / open bugs section | Missing |

### C. Technical Debt — Genuine vs Justified (Corrected)

Initial analysis made incorrect assumptions. After actual code inspection:

| Item | Verdict | Evidence |
|------|---------|----------|
| `vms.termcap.c` | **KEEP** — active Android/z/OS fallback | Guarded `#if defined(_VMS_POSIX) \|\| defined(_OSD_POSIX) \|\| defined(__ANDROID__)`. Compiles to empty `.o` on Linux/macOS/FreeBSD. Required on Android Bionic. |
| `_OSD_POSIX` guards | **KEEP** — active mainframe target | `os390` (z/OS USS) retained in PLAN.md §2d. `bs2cmd` builtin is live. |
| `apollo`, `masscomp`, `_CX_UX`, `TCF`, `WARP`, `KAI`, `_CRAY` guards | **PURGE** | Extinct systems. 13 dead builtin registrations in [sh.init.c:47-175](file:///home/orpheus497/Documents/Projects/mcsh/sh.init.c) that can never execute. |
| `glob.c` in-tree copy | **CONDITIONAL KEEP** | `AC_CHECK_FUNC([glob])` probe already exists. Keep as Android Bionic fallback; complete task 3.5 to actually delegate on POSIX targets. |
| `gethost.c` + `host.defs` | **MIGRATE** | PLAN.md task 3.4 listed as remaining. Delegate to `uname(2)` + `getaddrinfo(3)`. |
| `complete.tcsh` duplicate | **KEEP WITH NOTICE** | 54KB duplicate of `complete.mcsh`. Add deprecation comment; do not delete silently. |
| `#ifdef SUNOS4`, `Lynx`, `MACH`, `_MINIX`, `__clipper__` | **PURGE** | Additional extinct-platform guards in [sh.sem.c:38-49](file:///home/orpheus497/Documents/Projects/mcsh/sh.sem.c) and [sh.func.c:1788](file:///home/orpheus497/Documents/Projects/mcsh/sh.func.c) missed in Phase 2. |
| `DrawGhost()` direct terminal writes | **FIX NEEDED** | Confirmed open bug [ISSUES.md:361](file:///home/orpheus497/Documents/Projects/mcsh/ISSUES.md). Stale ghost tails on wide-char/resize. |

### D. Known Bugs Not Yet Fixed (Confirmed in ISSUES.md §3)

| Bug | Severity | File | Description |
|-----|----------|------|-------------|
| `unshare --user --pid` hang | **Critical** | [sh.proc.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.proc.c) | Fork retry loop sleeps with interrupts disabled |
| Ghost text bypasses virtual display | **Medium** | [ed.refresh.c:352](file:///home/orpheus497/Documents/Projects/mcsh/ed.refresh.c) | Stale tails on wide-char input or terminal resize |
| `ls-F` colour with `CLICOLOR_FORCE` | **Low** | [tw.color.c](file:///home/orpheus497/Documents/Projects/mcsh/tw.color.c) | `#93` — colour detection/env-var precedence wrong |
| Acute accent lintian in man page | **Low** | [tcsh.man.in](file:///home/orpheus497/Documents/Projects/mcsh/tcsh.man.in) | `#102/#82` — roff acute-accent warning |
| NLS `tcsh.cat` path hardcoded | **Low** | [sh.c:147](file:///home/orpheus497/Documents/Projects/mcsh/sh.c) | `stat()` probes for `tcsh.cat` not `mcsh.cat` |

---

## Proposed Changes

---

### Component 1 — Man Page (`tcsh.man.in`)

#### [MODIFY] [tcsh.man.in](file:///home/orpheus497/Documents/Projects/mcsh/tcsh.man.in)

Nine additions or substantial expansions, all grounded in verified source code:

**1.1 — Add `$mcsh` to Special Shell Variables**

After the `$tcsh` entry: document that `$mcsh` is set identically to `$tcsh` at
startup ([sh.c:81](file:///home/orpheus497/Documents/Projects/mcsh/sh.c), [tc.const.c:59](file:///home/orpheus497/Documents/Projects/mcsh/tc.const.c)) so scripts can guard on `if ($?mcsh)`.

**1.2 — `cd` builtin — add `cd -N`**

Current text: `cd`, `cd dir`, `cd -`, `cd +N`. Add:

```
cd -N
    Jump to the Nth entry counted from the BOTTOM (oldest entry) of the
    directory stack. N must be a positive integer. This complements cd +N
    (top-relative) and mirrors zsh cd -N semantics. A bare "cd -" still
    switches to $owd.
```

Cross-reference `pushd`, `popd`, `dirs -v`.

**1.3 — `set` builtin — pipe-to-variable and redirection forms**

`doset()` in [sh.set.c:300](file:///home/orpheus497/Documents/Projects/mcsh/sh.set.c) sets `pipe = 1` when `c->t_dlef` is set or `!isatty(OLDSTD)`. Document:

```csh
echo "hello" | set msg      # assigns "hello" to $msg (newline stripped)
set content < file.txt      # assigns first line of file.txt to $content
echo "foo bar" | set words  # for scalar: assigns "foo bar"
```

Note: reads to first newline (confirmed [sh.set.c:374-378](file:///home/orpheus497/Documents/Projects/mcsh/sh.set.c)).

**1.4 — `function` builtin — argument passing and constraints**

From [sh.func.c:2732](file:///home/orpheus497/Documents/Projects/mcsh/sh.func.c) and [sh.func.c:2746](file:///home/orpheus497/Documents/Projects/mcsh/sh.func.c):

```csh
function greet
    echo "Hello, $1"
    echo "All args: $argv"
return

greet World           # $1 = "World"
greet foo bar baz     # $argv = (foo bar baz)
```

Document: recursion limit 100 levels; functions may not be redeclared; `return`
must appear alone on its own line.

**1.5 — `set syntax` — token categories and SGR palette**

Expand from one sentence to a full subsection:
- All 12 token categories from [ed.syntax.h:81-95](file:///home/orpheus497/Documents/Projects/mcsh/ed.syntax.h) with default ANSI colours
- 32-entry LRU `$PATH` command cache behaviour
- SGR reset uses `ESC[22;39m` not `ESC[0m` (preserves `cur_atr` sync)
- Compile-time palette in `SynPalette[]` ([ed.syntax.c:63-76](file:///home/orpheus497/Documents/Projects/mcsh/ed.syntax.c))

**1.6 — `%g` / `%G` — caching and operation-state table**

From [tc.prompt.c:790-870](file:///home/orpheus497/Documents/Projects/mcsh/tc.prompt.c):

| Marker file | `%G` operation string |
|-------------|----------------------|
| `.git/MERGE_HEAD` | `MERGING` |
| `.git/CHERRY_PICK_HEAD` | `CHERRY-PICKING` |
| `.git/REBASE_HEAD` | `REBASING` |
| `.git/rebase-merge/head-name` | `REBASING-i` |

Document `GIT_POLL_INTERVAL` env var (default 2 seconds). Document that HEAD
mtime and state-marker mtime are tracked independently.

**1.7 — `set predict` — editor command cross-reference**

Add cross-reference to `predict-accept` editor command (Right-Arrow / `^F`).
Note: prediction sources are history, `$PATH` executables (via per-CWD cache),
and local filesystem paths.

**1.8 — Update `NEW FEATURES (+)` section**

Lines 10173-10340 still describe only historical tcsh-vs-csh features.
Append a dedicated **mcsh native enhancements** subsection:

- `function` builtin (Phase 5 / etcsh)
- Interactive comments `#` in interactive mode (Phase 5)
- Pipe-to-variable / redirect-to-variable (Phase 5 / etcsh)
- Subshell block redirection `{ }` in conditionals (Phase 5)
- Expression short-circuit with safe unset expansion (Phase 4)
- Fish-style predictive autocomplete `set predict` (Phase 9)
- Interactive syntax highlighting `set syntax` (Phase 9)
- Filetype colouring in completion `set color` (Phase 9)
- Native git branch prompt escapes `%g`, `%G` (Phase 9)
- zsh-style directory stack tree display and `cd -N` (Phase 8)

**1.9 — Add `BUGS` / Known Limitations section**

New `BUGS` section (standard man page convention):

- Ghost text (`set predict`) writes directly to terminal bypassing the virtual
  display model; stale tails may appear on wide-character input or terminal resize.
- `unshare --user --pid mcsh` may hang due to fork retry loop with signal blocking.
- `ls-F` colour detection has known issues with `CLICOLOR_FORCE`.
- `function` bodies cannot be redeclared in a session once defined.

---

### Component 2 — README.md

#### [MODIFY] [README.md](file:///home/orpheus497/Documents/Projects/mcsh/README.md)

**2.1 — Invocation Flags Matrix**

| Flag | Effect |
|------|--------|
| `-b` | Stop option processing; remaining args are non-option |
| `-c cmd` | Execute `cmd` string and exit |
| `-e` | Exit immediately on non-zero command status |
| `-f` | Fast start: skip all user startup files |
| `-i` | Force interactive mode |
| `-l` | Login shell mode |
| `-m` | Load user startup files even if non-interactive |
| `-n` | Parse commands but do not execute (syntax check mode) |
| `-s` | Read commands from stdin |
| `-v` / `-V` | Verbose: echo input after history expansion |
| `-x` / `-X` | Echo: print commands before executing |

**2.2 — Startup File Cascade Diagram**

```
Login Shell                     Non-Login Shell
───────────                     ───────────────
/etc/csh.cshrc                  /etc/csh.cshrc
/etc/csh.login                  ~/.mcshrc  ─┐
~/.mcshrc  ─┐                   ~/.tcshrc   │ (first found)
~/.tcshrc   │ (first found)     ~/.cshrc  ──┘
~/.cshrc  ──┘
~/.login
         … (session) …
~/.logout  (on exit)
/etc/csh.logout
```

**2.3 — Scripting Quick Reference section**

Cover `@` arithmetic operators, `switch`/`case`/`endsw`, `foreach`/`while`/`end`,
`onintr`, all variable modifiers (`:h :t :r :e :l :u :q :x :gh :gt`),
`filetest` operators, `function` with argument access, and `goto`/label.

**2.4 — Known Limitations section**

User-facing version of the open bugs from ISSUES.md §3.

---

### Component 3 — New File: `SYNTAX_AND_SCRIPTING.md`

#### [NEW] [SYNTAX_AND_SCRIPTING.md](file:///home/orpheus497/Documents/Projects/mcsh/SYNTAX_AND_SCRIPTING.md)

A comprehensive developer/scripter reference. Proposed structure:

1. Shell Grammar Overview (lexer, pipeline topology)
2. Quoting Reference (`''`, `""`, `` ` ``, `\`, interactive `#` comments)
3. Variable Substitution (all `$` forms, array slicing, `$<`, `$$`, `$!`, `$status`)
4. Variable Modifiers (complete table with chaining examples)
5. History Substitution (all `!`-event designators and modifiers)
6. Arithmetic Expressions (`@`) — operator precedence, overflow semantics, unsigned shift rationale
7. Redirection & Pipelines (all forms including `|&`, `>&`, `>>!`, pipe-to-variable)
8. Control Flow (`if/else/endif`, `switch`, `while`, `foreach`, `repeat`, `goto`)
9. Functions — declaration, argument access (`$1`, `$argv`), recursion, `return`
10. Signal Handling — `onintr`, SIGINT in scripts
11. Job Control — `&`, `%job`, `bg`, `fg`, `stop`, `notify`, `hup`
12. Filename Glob Patterns — `*`, `?`, `[...]`, `{...}`, `^pat`, `~user`
13. Startup File Execution Order
14. Special Shell Variables Reference (every variable the shell reads/sets)
15. Scripting Best Practices (`#!/usr/bin/mcsh -f`, error handling)

---

### Component 4 — Technical Debt Purge

#### [MODIFY] [sh.init.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.init.c)

Remove dead builtin registrations for extinct platforms (13 entries):

```
PURGE: att (_CX_UX), dmmode (_CRAY), getspath (TCF), getxvers (TCF),
       inlib (apollo), migrate (TCF), rootnode (apollo), setpath (MACH),
       setspath (TCF), setxvers (TCF), ucb (_CX_UX), universe (masscomp/_CX_UX),
       ver (apollo), warp (WARP), bye (KAI)

KEEP:  bs2cmd (_OSD_POSIX) — z/OS POSIX is an active target
```

Also remove corresponding handler declarations from [tc.decls.h](file:///home/orpheus497/Documents/Projects/mcsh/tc.decls.h).

#### [MODIFY] [sh.sem.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.sem.c)

- Lines 38-43: Remove `SUNOS4`/`CLEX_DUPS` guard block
- Lines 45-49: Remove `sparc`/`MACH`/`Lynx`/`BSD4_4` vfork.h inclusion ladder

#### [MODIFY] [sh.func.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.func.c)

- Line 1788: Remove `!defined(_MINIX) && !defined(__clipper__) && !defined(_CRAY)` guard

#### [MODIFY] [sh.init.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.init.c) — SIGCHLD guard

Line 409: Remove extinct `apollo` and `__EMX__` from:
`#if !defined(SIGCHLD) || defined(SOLARIS2) || defined(apollo) || defined(__EMX__)`

---

### Component 5 — Minor Code Bug Fix

#### [MODIFY] [sh.c](file:///home/orpheus497/Documents/Projects/mcsh/sh.c)

**Line 147** — NLS catalog probe uses `tcsh.cat`:
```c
// Before:
xsnprintf(trypath, sizeof(trypath), "%s/C/LC_MESSAGES/tcsh.cat", path);
// After (fallback approach):
xsnprintf(trypath, sizeof(trypath), "%s/C/LC_MESSAGES/mcsh.cat", path);
if (stat(trypath, &st) == -1)
    xsnprintf(trypath, sizeof(trypath), "%s/C/LC_MESSAGES/tcsh.cat", path);
```

This allows NLS catalogues to be installed under the `mcsh` name while
remaining backward-compatible with existing `tcsh.cat` installations.

---

### Component 6 — Test Suite Expansion

#### [NEW] 9 additional test scripts in `tests/`

| Script | Feature under test |
|--------|--------------------|
| `t017_cd_minus_n.sh` | `cd -N` bottom-indexed stack jump |
| `t018_syntax_highlight.sh` | `set syntax` — no crash on all token types |
| `t019_git_prompt.sh` | `%g`/`%G` — empty outside repo, correct inside |
| `t020_predict.sh` | `set predict` — smoke test, no crash |
| `t021_onintr.sh` | `onintr label` — SIGINT in script jumps to label |
| `t022_function_args.sh` | `function` — `$1`, `$argv`, recursion depth limit |
| `t023_filetest_ops.sh` | `filetest` — `-e`, `-f`, `-r`, `-d`, `-z` operators |
| `t024_variable_modifiers.sh` | `:h :t :r :e :l :u` on a known path |
| `t025_switch.sh` | `switch`/`case`/`default`/`breaksw`/`endsw` |

---

## Open Questions

> [!IMPORTANT]
> **Q1: Pipe-to-variable multi-line behaviour**
> `doset()` at [sh.set.c:374](file:///home/orpheus497/Documents/Projects/mcsh/sh.set.c) reads until `\n` and stops. Is this intentional (read one line only) or a limitation? Users expecting `set x = \`multi-line-cmd\`` equivalence will be surprised. This needs a decision before the man page can accurately document the behaviour.

> [!IMPORTANT]
> **Q2: `complete.tcsh` deprecation timeline**
> The 54KB duplicate of `complete.mcsh` is retained for scripts testing `$?tcsh`. Add a deprecation comment and target removal milestone now, or keep indefinitely?

> [!WARNING]
> **Q3: `DrawGhost()` virtual display bypass — fix in this batch or document only?**
> The confirmed open bug at [ed.refresh.c:352](file:///home/orpheus497/Documents/Projects/mcsh/ed.refresh.c) causes stale ghost tails on wide-character input or terminal resize. Full fix requires integrating ghost rendering into `Refresh()`. Do you want this addressed now (medium complexity) or just documented in BUGS?

> [!NOTE]
> **Q4: `SYNTAX_AND_SCRIPTING.md` audience — user or developer?**
> Should this document target script authors (examples-heavy, minimal internals) or developers (includes interpreter details like `Char` type system, `bfunc[]` table, LRU cache)?

> [!NOTE]
> **Q5: NLS `tcsh.cat` → `mcsh.cat` — fallback now or defer?**
> The [sh.c:147](file:///home/orpheus497/Documents/Projects/mcsh/sh.c) probe fix can be done as a safe fallback immediately. However, actually renaming the catalogues requires Phase 3 task 3.9 (regenerate all `.cat` files with modern `gencat`). Proceed with the fallback-only fix now, or defer entirely?

---

## Execution Order

```
Phase A — Documentation  (highest ROI, zero compile risk)
  A1: tcsh.man.in — 9 additions/expansions
  A2: README.md — flags, startup diagram, scripting reference, known-limitations
  A3: SYNTAX_AND_SCRIPTING.md — new comprehensive document

Phase B — Technical Debt Purge  (low risk, one-shot)
  B1: sh.init.c — remove 13 dead extinct-platform builtin entries
  B2: sh.sem.c — remove SUNOS4/sparc/Lynx/MACH guards
  B3: sh.func.c — remove _MINIX/_clipper_/_CRAY guard
  B4: sh.init.c — clean up SIGCHLD apollo/__EMX__ guard
  B5: sh.c — NLS tcsh.cat → mcsh.cat fallback probe

Phase C — Test Suite Expansion
  C1: Add t017–t025 (9 scripts)

Phase D — Bug Fixes  (risk-graded, requires Q3 decision above)
  D1: DrawGhost() full Refresh() pipeline integration (medium risk)
  D2: unshare hang fix in sh.proc.c (critical upstream #119)
  D3: ls-F CLICOLOR_FORCE fix in tw.color.c (low risk)
```

---

## Verification Plan

### Automated Tests
```sh
tests/run_tests.sh          # full suite including t017–t025 after Phase C
groff -man -Tutf8 tcsh.man.in 2>&1 | grep -c "warning:"  # target: 0
```

### Manual Verification
- `man mcsh` — verify all 9 new/expanded sections render correctly.
- `set syntax` + `set predict` together — verify no double `Refresh()` per keystroke.
- Git repository: verify `%g`/`%G` in prompt correct, cache refreshes after `git merge`.
- `cd -N` through a 4-deep stack — verify correct entry selected.
- `echo foo | set x; echo $x` in both interactive and `-f` script modes.
- Build from clean checkout: `autoreconf -fi && ./configure && make` — zero new warnings.
