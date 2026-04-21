# mcsh — Master Development Plan

Canonical execution plan for the mcsh project. Derived from the consolidation
issue log (`ISSUES.md`), a full audit of the source tree, and a sweep of all
open issues and pull requests in the upstream `tcsh-org/tcsh` repository.

**Key policy decisions:**
- Windows (Win32, WINNT, MSVC) support is **dropped**. WSL users can build
  under a POSIX layer. No hand-rolled `win32/` shims, no `.vcproj`, no
  `#ifdef WINNT_NATIVE` guards.
- Cygwin is **retained** as it provides a genuine POSIX environment.
- VMS is **dropped** entirely.
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
                            → Phase 7 (docs + tests)
```

Rationale: purging dead platforms first (Phase 2) minimises the surface area every
subsequent pass must touch. A solid build system is required before upstream
bug/feature cherry-picks can be validated. Features go last — they need a
clean, tested base.

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
| 1.2 | `sh.c:88` | `static const char tcshstr[] = "tcsh"` → rename to `mcshstr`; audit every use. The `int tcsh` global should be dual-assigned like the shell variable (`tcsh = mcsh_ver`). |
| 1.3 | `sh.c` | Remove dead `#ifndef WINNT_NATIVE` / `#ifdef WINNT_NATIVE` guards in `srcfile` declaration (dead post Phase 2). |
| 1.4 | `complete.tcsh` | Created `complete.mcsh` as the canonical name; `complete.tcsh` retained as backward-compat copy. |
| 1.5 | `src.desc`, `eight-bit.me`, `csh-mode.el` | Cosmetic text sweep — replace shell-identity references with `mcsh`. Documentation-only, no logic impact. |

---

## Phase 2 — Dead Platform / Windows / VMS Purge

Status: **complete**

### 2a. Windows — Complete Removal

- Delete `system/win32`, `system/uwin`, `system/emx`.
- Strip all `#ifdef WINNT_NATIVE`, `#ifdef _WIN_NT`, `#ifdef __WIN32__`
  guards from every source file. The `#ifdef __CYGWIN__` POSIX-mode guards
  are **retained**.
- `Makefile.in` — remove any win32/WINNT-specific variable or target.

### 2b. VMS — Complete Removal

- Remove VMS artifacts:
  - `vms.termcap.c`
  - `termcap.vms`
  - `system/vms`
  - All `#ifdef _VMS_POSIX` / `#ifdef __VMS` blocks from `sh.*.c`,
    `tc.os.c`, `ed.*.c`, `tc.*.c`.
- `Makefile.in` — remove lines 231–232, 363, 422, 545 (VMS_POSIX
  comments and defines).

### 2c. Imake — Retire

- Delete `Imakefile`.
- Delete `imake.config`.
  Autotools is the sole build system going forward.

### 2d. `system/` — Prune Defunct Entries

**Retained** (actively supported POSIX platforms):

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

**Removed** (no hardware has shipped in 25+ years, or platform is extinct):

`alliant`, `amdahl`, `amiga`, `apollo`, `bs2000`, `clipper`, `coh3`,
`convex`, `cray`, `csos`, `dgux`, `dgux5.4`, `dnix5.3`, `eta10`, `ews`,
`fortune`, `fps500`, `hcx`, `hk68`, `hp-3.2`, `hp-5.2`, `hpbsd2`,
`hpux7`, `iconuxv`, `intel` (i860), `isc202`, `isc3.0`, `isc4.0`,
`lynx2.1`, `mac2`, `mach`, `machten`, `masscomp`, `mips` (old standalone),
`mtXinu`, `opus`, `powermaxos`, `pyr`, `pyratt`, `sco+odt`, `sco32v2`,
`sco32v4`, `sco32v5`, `sequent`, `sinix`, `stellar`, `sunos35`, `sunos40`,
`sunos41`, `sunos413`, `supermax`, `superux8`, `sxa`, `sysV68`, `sysV88`,
`sysv`, `sysv2`, `sysv3`, `sysv4`, `tc2000`, `tekXD88`, `ultrix`,
`unixpc`, `uwin`, `emx`, `xenix`.

Update the platform-detection `case` in `configure.ac` to remove all
corresponding branches.

---

## Phase 3 — Source Hygiene

Status: **partial**

### Completed

| # | File(s) | Task |
|---|---------|------|
| 3.1 | `sh.char.h`, `sh.h` | Remove K&R `#ifndef __STDC__` prototype branches. C89 is the assumed floor. |
| 3.2 | `tc.alloc.c` | Delete the bundled malloc (`#ifndef SYSMALLOC` block). Always link the system allocator. Remove `SYSMALLOC` from the options string in `tc.vers.c`. |
| 3.3 | `sh.types.h` | Collapse platform typedef thicket onto `<stdint.h>` and `<stddef.h>`. Keep only types genuinely absent from C99. |
| 3.6 | `ed.screen.c` | Remove obsolete terminal-type `#ifdef` ladders. Retain only curses/terminfo paths. |
| 3.7 | `tc.os.c` | Audit `#ifdef _AIX`, `#ifdef sun`, etc. Remove all dead branches corresponding to Phase 2 removals. |
| 3.8 | `ed.screen.c`, `tw.parse.c`, `sh.exp.c` | Fix all `-Wdeprecated-non-prototype` and `-Wimplicit-function-declaration` warnings emitted by modern GCC/Clang. |

### Remaining

| # | File(s) | Task |
|---|---------|------|
| 3.4 | `gethost.c`, `host.defs` | Replace the compiled-in `host.defs` table parser with `getaddrinfo(3)`. `gethost.c` becomes a thin POSIX wrapper. |
| 3.5 | `glob.c`, `glob.h`, `configure.ac` | Probe for POSIX `glob(3)` in `configure.ac`; delegate to libc when available. Keep in-tree copy as fallback for platforms that lack it. |
| 3.9 | `nls/` | Run `catgen` + `gencat` with modern toolchain; verify all catalogues regenerate cleanly. Fix any broken catalogue. See also ISSUES.md. |

---

## Phase 4 — Bug Fixes (from upstream tcsh-org/tcsh)

Status: **partial**

### Completed

| Upstream | Severity | File(s) | Fix |
|----------|----------|---------|-----|
| **#99** | High | `configure.ac`, `tc.func.c` | `undefined reference to 'crypt'` on modern glibc systems where crypt was split into `libxcrypt`. Fix: add `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` to `configure.ac`. |
| **#101** (PR) | Medium | `sh.exp.c` | Signed integer overflow: `@ x = (1 << 63)` raises "Badly formed number". Fix: use unsigned arithmetic with explicit overflow detection. |

### Remaining

| Upstream | Severity | File(s) | Fix |
|----------|----------|---------|-----|
| **#119** | Critical | `sh.proc.c` | `unshare --user --pid tcsh` hangs. Fork retry loop calls `sleep()` with interrupts disabled. Fix: check for disabled-interrupt condition; use `SIGALRM`-based timeout or `nanosleep` with signal unblocking. |
| **#117 / #121** | Critical | `sh.lex.c`, `sh.dol.c` | Unicode regression since 6.24.14: emoji/wide chars stripped from filenames passed to `source`, and from variable assignment via command substitution. Root cause: byte vs. character length confusion in the wide-string path. Fix: cherry-pick the upstream fix or bisect the 6.24.14 diff. |
| **#110** (PR) | Medium | `tc.prompt.c` | `%j` job-count in prompt does not update until after the next fork. Fix: copy the `dojobs()` update call into the prompt renderer. |
| **#107** (PR) | Medium | `sh.exp.c` | `$?a && "$a" != ""` throws if `a` is unset because expansion runs before short-circuit evaluation. Fix: postpone variable expansion inside expression operands. |
| **#93** | Low | `tw.color.c` | `ls-F` colour test failures with `CLICOLOR_FORCE`, `LSCOLORS`, `LS_COLORS` env vars. Audit colour detection logic and correct environment-variable precedence. |
| **#102 / #82** | Low | `tcsh.man.in` | Acute accent lintian warning; missing stdout/stderr pipe workaround in man page. Trivial text patches. See also ISSUES.md. |

---

## Phase 5 — Feature Enhancements (upstream PRs worth merging)

Status: **complete**

| Upstream PR | Feature | Primary Files | Notes |
|-------------|---------|---------------|-------|
| **#77** | `function` built-in | `sh.func.c`, `sh.init.c`, `sh.parse.c`, `tc.decls.h` | 23-commit PR. Most-requested missing csh feature. Adds named function definitions. Needs full review and adaptation. Highest complexity item in the plan. |
| **#89** | Interactive comments (`#`) | `sh.lex.c`, `sh.parse.c` | `#` works in scripts but is ignored interactively. Bash/zsh parity. PR has several SIGSEGV-fix iterations — apply the final clean version only. |
| **#105** | Variable assignment from pipes/redirections | `sh.sem.c`, `sh.set.c` | `set x < file` and pipe-to-variable. High scripting value. |
| **#113** | Redirection in expression blocks | `sh.exp.c`, `sh.sem.c` | `if ( { cmd >& /dev/null } )` — redirection inside `{ }` expression blocks currently silently ignored. |

---

## Phase 6 — Build System

Status: **partial**

### Completed

| # | File | Task |
|---|------|------|
| 6.1 | `configure.ac` | Add `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` for modern glibc `libxcrypt` split (fixes #99). |
| 6.2 | `configure.ac` | Bump `AC_PREREQ` to `[2.72]`; audit and fix deprecated macros in `configure.ac` and `atlocal.in`. |
| 6.3 | `configure.ac` | Remove all dead platform-detection branches corresponding to Phase 2 `system/` removals. |
| 6.4 | `configure.ac` | Add `AC_CHECK_FUNC([glob], ...)` probe for libc `glob(3)` delegation (Phase 3.5). |
| 6.5 | `Makefile.in` | Strip remaining VMS and win32 dead targets/comments. |

### Remaining

| # | File | Task |
|---|------|------|
| 6.6 | `tests/` | Initialise the autotest harness (deferred in consolidation). Minimum suite: startup file order, `$mcsh`/`$tcsh` variable correctness, unicode filename round-trip, expression overflow, job-count prompt. |

---

## Phase 7 — Documentation

Status: **partial**

### Completed

| # | File | Task |
|---|------|------|
| 7.3 | `README.md` | Describe: what mcsh is, build instructions, WSL usage, tcsh/csh compatibility layer, where to report bugs. |
| 7.4 | `ISSUES.md` | Update as items are resolved; mark completed phases. |

### Remaining

| # | File | Task |
|---|------|------|
| 7.1 | `tcsh.man.in` | Full body-text pass completing Phase 1.1: "the shell" → `.Nm` / `mcsh`; "tcsh-compat surface" → `tcsh`. Fix acute-accent lintian warnings (from #102). See also ISSUES.md. |
| 7.2 | `tcsh.man.in` | Add sections documenting new features landed in Phase 5: `function`, interactive comments, variable assignment from pipes. |

---

## Risk Register

| Phase | Risk | Mitigation |
|-------|------|------------|
| 2 | Platform detection branch removal breaks rare builds | Keep retained `system/` files unchanged; only delete. Build-test on Linux and FreeBSD before closing phase. |
| 3 | malloc replacement causes allocator mismatch | Replace in one commit; run full test suite immediately. |
| 3 | `gethost.c` replacement removes obscure compatibility | Document the behavioural change; the new path is strictly more correct on modern systems. |
| 4 | Unicode fix (#117/#121) re-introduces wide-string bugs | Write a regression test as part of the fix commit. |
| 5 | `function` built-in (PR #77) conflicts with existing parser | Full parser review required; do not merge as-is — adapt commit-by-commit. |
| 5 | Interactive comments (PR #89) re-introduces freeze/SIGSEGV | Apply only the final iteration of the PR, not intermediate commits. |
| 6 | autoconf 2.72 deprecations cause configure failures | Run `autoreconf -fi --warnings=all` and clear all warnings before considering the phase done. |

---

## Changelog

| Date | Entry |
|------|-------|
| 2026-04-20 | Plan drafted from ISSUES.md audit + tcsh-org/tcsh open issues/PRs sweep. |
| 2026-04-20 | Phases 1–2 complete. Phases 3, 4, 6, 7 partial — see Remaining tables. Phase 5 features pending upstream review. |
| 2026-04-21 | Corrected phase statuses to reflect outstanding work (3.4, 3.5, 3.9, #119, #117/#121, #110, #107, #93, #102/#82, 6.6, 7.1, 7.2). |
