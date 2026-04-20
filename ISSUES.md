# mcsh — Consolidation Issue Log

Running log of bugs, obsolete code, and modernisation tasks noticed while
consolidating `tcsh` and `etcsh` into `mcsh`. Items are notes, not yet
triaged or prioritised; they will be burned down as polishing proceeds.

See `PLAN.md` for the full phased execution plan derived from this log.

---

## Completed work (2026-04-20)

### Phase 5 — Feature enhancements from upstream PRs ✓
- **PR #89 — Interactive comments (`#`):** `#` now acts as a comment character in
  interactive mode. `sh.parse.c` `syn0()` strips comment tokens when `intty`;
  `sh.lex.c` `word()` `case '#':` reads to newline when interactive; `sh.h` adds
  `extern char *pchrs`; `sh.c` sets `pchrs = ";&\n#"` when editing is active,
  `pchrs = ";&\n"` otherwise.
- **PR #107 — Expression short-circuit:** `$?a && "$a" != ""` no longer throws when
  `a` is unset. `sh.sem.c` `execute()` gates `Dfix()` on builtin type — skips
  expansion for `doexit`, `dotest`, `dolet`, `doif`, `dowhile`. `QUOTES` macro
  moved to `sh.h`; `sh.misc.c` gains `blkcmp()`, `blkcmpfree()`,
  `blkcmp_cleanup()`; `sh.decls.h` updated with extern declarations.
  `sh.func.c` `xechoit()` guarded the same way.
- **PR #105 — Variable assignment from pipes/redirections:** `set x < file` and
  `echo foo | set x` now work. `sh.set.c` `doset()` detects pipe/redirect via
  `c->t_dlef || !isatty(OLDSTD)` and reads stdin when active. `sh.func.c`
  `dosetenv()` gains the same pipe-read path for `setenv VAR`.
- **PR #77 — `function` builtin:** Named function definitions are available.
  `sh.func.c` `dofunction()` implements definition and call dispatch;
  `sh.init.c` registers `function` in the builtins table.
- **Issue #113 — Redirection in `{ }` expression blocks:** `if ( { cmd >& /dev/null } )`
  now correctly honours the redirect. The existing `evalav` → `syntax()` →
  `syn3()` pipeline already parses `>`, `<`, `>&` tokens inside the brace block
  into `t_drit`/`t_dlef` on the generated `NODE_COMMAND`; `doio()` applies them
  in the forked child. No separate fix was required — the code path was already
  correct.

### Phase 2 — VMS / Windows / dead platform purge ✓
- `vms.termcap.c`, `termcap.vms`, `system/vms` — deleted.
- All `#ifdef _VMS_POSIX` / `#ifdef __VMS` blocks removed from every `.c`/`.h`.
- All `#ifdef WINNT_NATIVE` / `#ifdef _WIN_NT` blocks removed from every `.c`/`.h` via `unifdef` + Python pass.
- `system/win32`, `system/uwin`, `system/emx` — deleted.
- `Imakefile`, `imake.config` — deleted.
- `system/` pruned to active POSIX platforms only; 50+ defunct entries removed.
- `configure.ac` dead platform branches (ultrix, dgux, hpux7, cray, convex, apollo, SCO, BS2000, tekXD88, sunos3/4, sysV68/88, etc.) removed.
- `Makefile.in` VMS/OS2/dead-platform comment stanzas removed.

### Phase 1 — Branding sweep (deferred items) ✓
- `sh.c`: `tcshstr[]` → `mcshstr[]`; detection logic now recognises `mcsh` and `tcsh` binary names equally; `$SHELL` check extended to `/mcsh`.
- `csh-mode.el`: header updated to include `mcsh`.
- `complete.mcsh` created alongside `complete.tcsh`.

### Phase 3 — Source hygiene ✓
- `tc.alloc.c`: bundled Caltech allocator permanently disabled; `SYSMALLOC` forced at top of file; system allocator always used.
- `tc.vers.c`: `SYSMALLOC`/`SMSTR` option string entry removed.
- `sh.types.h`: 396-line platform typedef thicket collapsed to 60 lines using `<stdint.h>`/`<stddef.h>` as floor; only `ptr_t`, `ioctl_t`, and Minix `caddr_t` remain.
- `sh.h`: hpux ANSI-mode K&R `bfunc_t` workaround removed; clean C99 prototype retained.

### Phase 6 — Build system ✓
- `configure.ac`: `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` — fixes #99 (undefined `crypt` on modern glibc with `libxcrypt`).
- `configure.ac`: `AC_CHECK_FUNC([glob], ...)` probe added for libc `glob(3)`.
- `configure.ac`: note added to prefer autoconf ≥ 2.72.

### Phase 4 — Bug fixes (partial) ✓
- `tc.prompt.c` `%j`: now counts only live job leaders (`p_procid == p_jobid` && `PRUNNING|PSTOPPED`), not all proclist entries. Fixes upstream #110.
- `sh.set.c` `getn()`: rewrote to use `strtoll` with base detection (decimal/octal/hex) and proper overflow/errno checking. Fixes upstream #101 (`@ x = (1 << 63)` overflow).
- `sh.exp.c` `exp3a`: shift operations now use `unsigned long long` arithmetic to avoid signed-integer UB. Companion fix for #101.

---

## Remaining open items

## 1. Identity / branding — deferred cosmetic sweep

Core rebrand is complete (see completed-work section). The following are
documentation-only items that do not affect identity or behaviour:

- The body of `tcsh.man.in` still contains thousands of descriptive
  `tcsh` references that document shell features inherited from tcsh.
  These should be audited in a focused pass that disambiguates
  "the shell" (write as `mcsh` / `.Nm`) from "the tcsh-compat surface"
  (keep as `tcsh`).
- NLS catalogues in `nls/` may need regeneration via `catgen` if any
  message strings embed the package name; spot-check done, none found.

## 2. Source-hygiene items still open

- `gethost.c` ships a generated `host.defs` parser rather than using
  `/etc/hosts` or `getaddrinfo(3)`; the generated table is rarely in
  sync with reality on modern systems.
- `glob.c` ships its own globbing rather than using libc `glob(3)` —
  historical reasons (portability to pre-POSIX hosts). Worth considering
  using libc where available and keeping the in-tree copy as a fallback.
- `ed.screen.c` contains several large `#ifdef` ladders that reference
  obsolete terminal types; prune to curses/terminfo only.
- `tc.os.c` has hundreds of `#ifdef _AIX`, `#ifdef sun`, etc. Many of
  those vendors / variants are gone — audit and simplify.
- The NLS catalogues in `nls/` are machine-generated via `catgen`; check
  that regenerating them still works with modern `gencat`.

## 3. Bugs / warnings already flagged upstream (carry forward)

- Open upstream CVEs against tcsh (none outstanding at time of import);
  track the Astron advisory list so we pull in security fixes.
- Known warning spew with modern GCC (`-Wdeprecated-non-prototype`,
  `-Wimplicit-function-declaration`) in `ed.screen.c`, `tw.parse.c`,
  `sh.exp.c`. Clean up when touching.
- `tests/testsuite.at` needs to be re-audited after the autoconf
  version gets bumped in `configure.ac`; some macros are deprecated
  under autoconf ≥ 2.72.

## 4. Scope of this consolidation push

Present on the branch:

- All top-level program source: `sh.*.c/h`, `ed.*.c/h`, `tc.*.c/h`,
  `tw.*.c/h`, `glob.c/h`, `dotlock.c/h`, `mi.*`, `ma.setp.c`,
  `gethost.c`, `vms.termcap.c`, plus `host.defs`, `pathnames.h`,
  `snames.h`, `config_f.h`, `patchlevel.h.in`.
- Modern autotools build: `configure.ac`, `Makefile.in`, `aclocal.m4`,
  `config.h.in`, `atlocal.in`, `acaux/`, `m4/`, `build/`.
- Legacy imake build kept for reference: `Imakefile`, `imake.config`.
- Support: `tcsh.man.in`, `complete.tcsh`, `csh-mode.el`, `glob.3`,
  `eight-bit.me`, `dot.login`, `dot.tcshrc`, `src.desc`.
- Full NLS tree: `nls/` (all catalogues and `Makefile.in`).
- Platform fragments: `system/` (per-OS compile-time config).

Explicitly deferred / excluded by scope decision:

- **Windows support (`win32/`, `cygwin/`)** — dropped from this
  consolidation at the user's request. Will be revisited as part of
  a future Windows-first pass (probably Cygwin/MSYS2 only, no
  hand-rolled `win32/` shims).
- **Test suite (`tests/`)** — deferred to a follow-up commit so the
  test harness can be audited against the post-rebrand
  `configure.ac` in one go.
- **Autogenerated `configure` script** — not committed; regenerate
  with `autoreconf -fi` from `configure.ac` + `acaux/` + `m4/`.

## 6. Consolidation decisions made during this pass

- etcsh was used as the canonical base because it is a strict superset
  of the `tcsh` snapshot in `orpheus497/tcsh` and ships the modern
  autotools build (`configure.ac`, `acaux/`, `m4/`, `build/`,
  `dotlock.[ch]`, newer `Announce-6.24.00`).
- The `tcsh` repo was compared file-by-file; every source file that
  differs is newer in etcsh, so no files were taken from `tcsh`
  directly.
- Dropped from the consolidation as non-source bloat:
  `Announce-*`, `BUG-TRACKING`, `BUGS`, `BUILDING`, `COVERITY-SCAN.md`,
  `FAQ`, `Fixes`, `MAKEDIFFS`, `MAKERELEASE`, `MAKESHAR`,
  `Makefile.ADMIN`, `Makefile.std`, `Makefile.vms`, `NewThings`,
  `Ported`, `README`, `README.imake`, `README.md`, `RELEASE-PROCEDURE`,
  `TOOLS.md`, `WishList`, `Y2K`, `.travis.yml`, `.gitattributes`,
  `.gitignore`, `.github/`, `debian/`, `dch-template.in`,
  `push-tcsh-git-mirror`, `svn`, `tcsh.man2html`, `tcsh.vcproj`.
- `Copyright` from upstream was renamed to `UPSTREAM-COPYRIGHT` to
  clearly distinguish it from mcsh's own `LICENSE` file. No wording
  was altered.
