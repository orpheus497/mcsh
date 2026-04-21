# mcsh — Master Development Plan

Canonical execution plan for the mcsh project. Derived from the consolidation
issue log (`ISSUES.md`), a full audit of the source tree, and a sweep of all
open issues and pull requests in the upstream `tcsh-org/tcsh` repository.

**Key policy decisions:**
- Windows (Win32, WINNT, MSVC) support is **dropped**. WSL users can build
  under a POSIX layer. No hand-rolled `win32/` shims, no `.vcproj`, no
  `#ifdef WINNT_NATIVE` guards.
- Cygwin is **retained** as it provides a genuine POSIX environment.
- VMS platform support is **discontinued**. VMS-named legacy files (e.g.
  `vms.termcap.c`) are **retained and repurposed** as portable POSIX shims.
- Legacy POSIX platforms (Solaris, AIX, HP-UX, Linux, BSD, macOS, Android,
  QNX 6) are **retained** and actively maintained.
- Backwards compatibility with `tcsh`/`csh` configurations is a hard
  requirement and must never regress.
- Modernity is an explicit goal: new language features, clean compiler output,
  and current POSIX/libc APIs are all in scope.

---

## Execution Order

```text
    → Phase 2 (dead platform purge)
        → Phase 1 (branding finish)
            → Phase 3 (source hygiene)
                → Phase 6 (build system)
                    → Phase 4 (bug fixes)
                        → Phase 5 (features)
                            → Phase 9 (native features)
                                → Phase 8 (code review)
                                    → Phase 7 (docs + tests)
```

---

## Phase 1 — Finish the Rebranding Sweep

Status: **complete**

Core identity work already landed:
`configure.ac`, `patchlevel.h.in`, `tc.vers.c`, `tc.const.c`, `sh.c`,
`Makefile.in`, `pathnames.h`, `dot.mcshrc`, `dot.tcshrc`, `dot.login`.
Binary installs as `mcsh`; backward-compat `tcsh` symlink provided.
`$mcsh` and `$tcsh` both set at runtime.

### Completed tasks

| # | File(s) | Task |
|---|---------|------|
| 1.1 | `tcsh.man.in` | Body-text disambiguation pass: occurrences of "tcsh" that describe *the shell* become `mcsh`/`.Nm`; occurrences that describe the *tcsh-compat surface* stay as `tcsh`. |
| 1.2 | `sh.c:88` | `static const char tcshstr[] = "tcsh"` → renamed to `mcshstr`; every use audited. |
| 1.3 | `sh.c` | Dead `#ifndef WINNT_NATIVE` / `#ifdef WINNT_NATIVE` guards removed from `srcfile` declaration (dead post Phase 2). |
| 1.4 | `complete.tcsh` | Created `complete.mcsh` as the canonical name; `complete.tcsh` retained as backward-compat copy. |
| 1.5 | `src.desc`, `eight-bit.me`, `csh-mode.el` | Cosmetic text sweep — replaced shell-identity references with `mcsh`. |

---

## Phase 2 — Dead Platform / Windows / VMS Purge

Status: **complete**

### 2a. Windows — Complete Removal

- Deleted `system/win32`, `system/uwin`, `system/emx`.
- Stripped all `#ifdef WINNT_NATIVE`, `#ifdef _WIN_NT`, `#ifdef __WIN32__`
  guards from every source file. `#ifdef __CYGWIN__` POSIX-mode guards **retained**.
- `Makefile.in` — removed all win32/WINNT-specific variables and targets.

### 2b. VMS — Platform Support Discontinued

VMS platform support is discontinued. Certain VMS-named legacy source files are
retained and repurposed as portable shims:

- `vms.termcap.c` — **retained and repurposed as a POSIX/Android termcap shim**.
  VMS-specific code removed; file kept for portable termcap fallback use on
  platforms without a system termcap library.
- Removed: `termcap.vms`, `system/vms`, all `#ifdef _VMS_POSIX` / `#ifdef __VMS` blocks.

### 2c. Imake — Retired

- Deleted `Imakefile` and `imake.config`. Autotools is the sole build system.

### 2d. `system/` — Pruned to Active POSIX Platforms

**Retained** (actively supported):

| Directory | Platform |
|-----------|----------|
| `linux` | Linux (all) |
| `android` | Android (Bionic) |
| `bsd` | Generic BSD |
| `bsd4.4` | 4.4BSD / FreeBSD / NetBSD / OpenBSD |
| `bsdreno` | BSD-reno |
| `aix` | IBM AIX |
| `sol2`..`sol29` | Oracle/Sun Solaris 2–10 |
| `hpux8`, `hpux11` | HP-UX 8, 11 |
| `hposf1`, `osf1`, `decosf1`, `parosf1` | Tru64/OSF/1 variants |
| `irix`, `irix62` | SGI IRIX 6.x |
| `qnx6` | QNX 6 (Neutrino) |
| `cygwin` | Cygwin POSIX layer |
| `os390` | IBM z/OS USS |
| `minix` | Minix 3 |

**Removed** (extinct or no hardware in 25+ years): `alliant`, `amdahl`, `amiga`,
`apollo`, `bs2000`, `clipper`, `coh3`, `convex`, `cray`, `csos`, `dgux`,
`dgux5.4`, `dnix5.3`, `eta10`, `ews`, `fortune`, `fps500`, `hcx`, `hk68`,
`hp-3.2`, `hp-5.2`, `hpbsd2`, `hpux7`, `iconuxv`, `intel` (i860), `isc202`,
`isc3.0`, `isc4.0`, `lynx2.1`, `mac2`, `mach`, `machten`, `masscomp`,
`mips` (old standalone), `mtXinu`, `opus`, `powermaxos`, `pyr`, `pyratt`,
`sco+odt`, `sco32v2`, `sco32v4`, `sco32v5`, `sequent`, `sinix`, `stellar`,
`sunos35`, `sunos40`, `sunos41`, `sunos413`, `supermax`, `superux8`, `sxa`,
`sysV68`, `sysV88`, `sysv`, `sysv2`, `sysv3`, `sysv4`, `tc2000`, `tekXD88`,
`ultrix`, `unixpc`, `uwin`, `emx`, `xenix`.

---

## Phase 3 — Source Hygiene

Status: **partial**

### Completed

| # | File(s) | Task |
|---|---------|------|
| 3.1 | `sh.char.h`, `sh.h` | Removed K&R `#ifndef __STDC__` prototype branches. C89 is the assumed floor. |
| 3.2 | `tc.alloc.c` | Deleted the bundled malloc (`#ifndef SYSMALLOC` block). Always link the system allocator. Removed `SYSMALLOC` from the options string in `tc.vers.c`. |
| 3.3 | `sh.types.h` | Collapsed 396-line platform typedef thicket onto `<stdint.h>` and `<stddef.h>`. Only `ptr_t`, `ioctl_t`, and Minix `caddr_t` remain. |
| 3.6 | `ed.screen.c` | Removed obsolete terminal-type `#ifdef` ladders. Retained only curses/terminfo paths. |
| 3.7 | `tc.os.c` | Audited and removed all dead branches corresponding to Phase 2 platform removals. |
| 3.8 | `ed.screen.c`, `tw.parse.c`, `sh.exp.c` | Fixed all `-Wdeprecated-non-prototype` and `-Wimplicit-function-declaration` warnings emitted by modern GCC/Clang. |

### Remaining

| # | File(s) | Task |
|---|---------|------|
| 3.4 | `gethost.c`, `host.defs` | Replace compiled-in `host.defs` table parser with `getaddrinfo(3)`. |
| 3.5 | `glob.c`, `glob.h`, `configure.ac` | Probe for POSIX `glob(3)` in `configure.ac`; delegate to libc when available; keep in-tree copy as fallback. |
| 3.9 | `nls/` | Run `catgen` + `gencat` with modern toolchain; verify all catalogues regenerate cleanly. |

---

## Phase 4 — Bug Fixes (from upstream tcsh-org/tcsh)

Status: **partial**

### Completed

| Upstream | Severity | File(s) | Fix |
|----------|----------|---------|-----|
| **#99** | High | `configure.ac`, `tc.func.c` | `undefined reference to 'crypt'` on modern glibc. Fix: `AC_SEARCH_LIBS([crypt], [crypt xcrypt])`. |
| **#101** (PR) | Medium | `sh.exp.c` | Signed integer overflow: `@ x = (1 << 63)` raises "Badly formed number". Fix: unsigned arithmetic with overflow detection. |
| **#110** | Medium | `tc.prompt.c` | `%j` job-count in prompt counted all proclist entries. Fix: counts only live job leaders (`p_procid == p_jobid` && `PRUNNING\|PSTOPPED`). |
| **#107** (PR) | Medium | `sh.exp.c`, `sh.sem.c` | `$?a && "$a" != ""` throws if `a` is unset. Fix: `Dfix()` skips expansion for expression-evaluating builtins; expansion deferred until after short-circuit. |
| **#116** | Medium | `sh.file.c` | 32-bit `wcscoll` type mismatch: cast through `(const wchar_t *)(const void *)`. |
| **#115** | Low | `config_f.h`, `sh.h` | Shift-JIS: `SIZEOF_WCHAR_T < 4` → `<= 4`; `AUTOSET_KANJI` removes non-macro `CODESET` guard. |
| **#103** | Low | `nls/Makefile.in` | Greek locale `el` (ISO 639-1). Already correct in mcsh. |
| **#104** | Low | `Makefile.in`, `configure.ac` | Cross-build `*_FOR_BUILD` flags for `gethost`. Already applied in mcsh. |

### Remaining

| Upstream | Severity | File(s) | Fix needed |
|----------|----------|---------|------------|
| **#119** | Critical | `sh.proc.c` | `unshare --user --pid tcsh` hangs. Fork retry loop calls `sleep()` with interrupts disabled. Fix: use `SIGALRM`-based timeout or `nanosleep` with signal unblocking. |
| **#117 / #121** | Critical | `sh.lex.c`, `sh.dol.c` | Unicode regression since 6.24.14: emoji/wide chars stripped from filenames and variable assignments. Root cause: byte vs. character length confusion in the wide-string path. |
| **#93** | Low | `tw.color.c` | `ls-F` colour test failures with `CLICOLOR_FORCE`, `LSCOLORS`, `LS_COLORS`. Audit colour detection and environment-variable precedence. |
| **#102 / #82** | Low | `tcsh.man.in` | Acute accent lintian warning; missing stdout/stderr pipe workaround in man page. |

---

## Phase 5 — Feature Enhancements (upstream PRs)

Status: **complete**

| Upstream PR | Feature | Primary Files |
|-------------|---------|---------------|
| **#77** | `function` built-in | `sh.func.c`, `sh.init.c`, `sh.parse.c`, `tc.decls.h` |
| **#89** | Interactive comments (`#`) | `sh.lex.c`, `sh.parse.c` |
| **#105** | Variable assignment from pipes/redirections | `sh.sem.c`, `sh.set.c` |
| **#113** | Redirection in `{ }` expression blocks | `sh.exp.c`, `sh.sem.c` |

---

## Phase 6 — Build System

Status: **partial**

### Completed

| # | File | Task |
|---|------|------|
| 6.1 | `configure.ac` | `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` for modern `libxcrypt` split. |
| 6.2 | `configure.ac` | Bump `AC_PREREQ` to `[2.72]`; deprecated macros audited. |
| 6.3 | `configure.ac` | Removed all dead platform-detection branches from Phase 2 purge. |
| 6.4 | `configure.ac` | `AC_CHECK_FUNC([glob], ...)` probe for libc `glob(3)` delegation. |
| 6.5 | `Makefile.in` | Stripped remaining VMS and win32 dead targets/comments. |

### Remaining

| # | File | Task |
|---|------|------|
| 6.6 | `tests/` | Initialise autotest harness. Minimum suite: startup file order, `$mcsh`/`$tcsh` variable correctness, unicode filename round-trip, expression overflow, job-count prompt. |

---

## Phase 7 — Documentation

Status: **complete (first pass)**

| # | File | Status |
|---|------|--------|
| 7.1 | `tcsh.man.in` | Body-text disambiguation pass done. Remaining: add new-feature sections (Phase 5 features, `set syntax`, git prompt escapes, pushd/popd tree navigation). |
| 7.2 | `tcsh.man.in` | New-feature sections for `function`, interactive comments, pipe-to-variable: **pending**. |
| 7.3 | `README.md` | Fully updated: all features, bug fixes, prompt/directory-stack reference, `dot.mcshrc` section table, source layout. |
| 7.4 | `ISSUES.md` | Updated: completed work annotated, remaining open items current. |
| 7.5 | `PLAN.md` | This document — phased plan with accurate status and changelog. |

---

## Phase 9 — Native mcsh Original Features

Status: **complete**

Features developed natively for mcsh, with no upstream tcsh counterpart.

| Feature | `set` variable | Primary Files | Notes |
|---------|----------------|---------------|-------|
| Fish-style predictive autocomplete | *(always active)* | `ed.chared.c`, `ed.refresh.c`, `ed.inputl.c` | Scans `Histlist` for prefix match; ghost text rendered dimmed after cursor. Right-Arrow / `^F` accepts. |
| Native git branch prompt escapes | *(always active)* | `tc.prompt.c` | `%g` = branch name; `%G` = branch + operation state. Cached per-CWD with independent HEAD and state-marker mtime tracking. |
| Filetype colouring in completion | `set color` | `tw.color.c`, `sh.set.c` | Drives `ls-F` completion listings via `LSCOLORS`/`LS_COLORS`. |
| **Interactive syntax highlighting** | **`set syntax`** | **`ed.syntax.c/h`, `ed.screen.c`, `ed.refresh.c`, `ed.inputl.c`, `sh.set.c`** | Virtual-display pipeline integration. Single-pass tokeniser fills `SyntaxColor[]`; `Draw()` propagates token colour into `Vdisplay[]` via `SYN_PACK()`; `so_write()` emits ANSI SGR per cell via `SetSGRColor()`. 32-entry LRU command cache avoids per-keystroke `stat(2)`. |
| **zsh-style pushd/popd tree display** | *(always active)* | `sh.dir.c` | pushd/popd default to numbered vertical display with `→` marking index 0. `cd -N` jumps to Nth entry from bottom of stack. |

---

## Phase 8 — Code Review Fixes (PR3)

Status: **complete**

All Copilot, CodeRabbit, and Gemini findings from PR #3 resolved across three
iterative rounds. See `ISSUES.md` completed section and the PR comment at
`https://github.com/orpheus497/mcsh/pull/3#issuecomment-4289084043` for the
full itemised response.

Key fixes:
- `configure.ac` TCSH_BASELINE_VERSION string literal fix
- `tc.prompt.c` git cache marker-mtime independence
- `ed.screen.c` `SetSGRColor` `ESC[22;39m` instead of `ESC[0m`
- `ed.refresh.c` `DrawGhost` same SGR fix
- `ed.inputl.c` no double `Refresh()` on `CC_NORM` + `set syntax`
- All `vms.termcap.c` buffer overflow and bounds issues
- All `sh.func.c`, `sh.sem.c`, `sh.set.c`, `sh.exp.c` expression safety fixes
- All `m4/`, `acaux/`, `alacritty.toml`, `dot.mcshrc` portability fixes

---

## Risk Register

| Phase | Risk | Mitigation |
|-------|------|------------|
| 3 | `gethost.c` replacement removes obscure compatibility | Document the behavioural change; new path is strictly more correct on modern systems. |
| 3 | `glob.c` delegation breaks edge cases | Keep in-tree fallback; gate on configure probe. |
| 4 | Unicode fix (#117/#121) re-introduces wide-string bugs | Write regression test as part of the fix commit. |
| 6 | autoconf 2.72 deprecations cause configure failures | Run `autoreconf -fi --warnings=all` and clear all warnings. |

---

## Changelog

| Date | Entry |
|------|-------|
| 2026-04-20 | Plan drafted from ISSUES.md audit + tcsh-org/tcsh open issues/PRs sweep. Phases 1–2 complete. Phases 3, 4, 6, 7 partial. |
| 2026-04-20 | Phase 5 features landed: fish-style predictive autocomplete, native git branch prompt escapes `%g`/`%G`, `set color` filetype colouring. |
| 2026-04-21 | Phase 4b + Phase 8 (round 1): all Gemini + CodeRabbit PR3 review items addressed. `vms.termcap.c` repurposed as portable termcap shim; `sh.func.c` `doif` widened to `tcsh_number_t`; `configure.ac` fixes; `dot.mcshrc` rewritten. README, PLAN, ISSUES updated. |
| 2026-04-21 | Phase 9: native interactive syntax highlighting (`set syntax`) landed. Virtual-display pipeline: `ed.syntax.c/h` tokeniser + LRU cache; `SYN_PACK`/`SYN_TOK`/`SYN_GLYPH` bit-packing into `Vdisplay Char`; `SetSGRColor()` per-cell ANSI SGR. |
| 2026-04-21 | Phase 8 (rounds 2–3): remaining Copilot review findings resolved — `TCSH_BASELINE_VERSION` string literal; git cache marker-mtime independence; `SetSGRColor`/`DrawGhost` `ESC[22;39m` SGR fix; `ed.inputl.c` no double `Refresh()`; zsh-style pushd/popd tree display and `cd -N` navigation added. All documentation updated to reflect current state. |
| 2026-04-22 | Phase 4 upstream sweep: full audit of all tcsh-org/tcsh open + recently closed issues/PRs. Applied: `sh.file.c` 32-bit `wcscoll` cast (#116); `config_f.h` Shift-JIS `<= 4` condition (#115); `sh.h` `AUTOSET_KANJI` CODESET guard removal (#115). Confirmed already-present: #103, #104, #99, #101, #110. Rejected (upstream closed/not merged): #118 FIONREAD, #114 Shift-JIS runtime check. ISSUES.md, PLAN.md, README.md all updated. |
| 2026-04-22 | Phase 4/6 review response (dev4): Three flagged weaknesses addressed. Short-circuit fix: `sh.dol.c` Dgetdol() returns STRNULL for unset vars instead of ERR_UNDVAR. Unicode regression documented (no upstream fix available). Test suite created: `tests/` with 7 scripts + Makefile. Gemini inline fixes: `cache_store()` goto removed (two-loop LRU); `GIT_POLL_INTERVAL` named constant. README Known Limitations section added. ISSUES.md Round 6 appended. |
