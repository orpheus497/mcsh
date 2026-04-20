# mcsh — Consolidation Issue Log

Running log of bugs, obsolete code, and modernisation tasks noticed while
consolidating `tcsh` and `etcsh` into `mcsh`. Items are notes, not yet
triaged or prioritised; they will be burned down as polishing proceeds.

## 1. Identity / branding

mcsh is now branded end-to-end as **Modern C Shell** (`mcsh`), while
preserving full backward compatibility with tcsh and csh configurations.

Rebrand completed in this pass:

- `configure.ac` — `AC_INIT([mcsh], 0.1.0, ...)` with the mcsh bug-report
  URL; the upstream tcsh version is preserved as `TCSH_BASELINE_VERSION`
  so consumers that need to know the historic snapshot can still probe it.
- `patchlevel.h.in` — exposes `MCSH_NAME`, `MCSH_LONG_NAME`,
  `MCSH_VERSION`, `MCSH_DATE`, `MCSH_ORIGIN`, and
  `TCSH_BASELINE_VERS`/`TCSH_BASELINE_DATE`.
- `tc.vers.c` — the runtime banner now reads
  `mcsh <ver> (<origin>) ... [tcsh baseline <tcsh-ver>] options ...`
  and `$version` mirrors it. Both `$mcsh` and `$tcsh` shell variables are
  set (to the same version string) so existing `if ($?tcsh)` guards
  continue to work.
- `tc.const.c` — adds `STRmcsh[]` and `STRsldotmcshrc[]` alongside the
  historic `STRtcsh[]` / `STRsldottcshrc[]`; externs for both appear in
  the auto-generated `tc.const.h`.
- `sh.c` — per-user start-up file search order is now
  `~/.mcshrc` → `~/.tcshrc` → `~/.cshrc`, so mcsh looks for its own
  dotfile first but still sources an existing tcsh or csh configuration.
- `Makefile.in` — `PROGRAM=mcsh`, `BUILD=$(PROGRAM)$(EXEEXT)`. `install`
  installs the binary as `mcsh` and creates a `tcsh` backward-compat
  symlink in the same `bindir`; `install.man` installs `mcsh.1` and a
  `tcsh.1` symlink; tarballs are produced as `mcsh-<ver>.tar.gz`;
  `RELEASE_UPLOAD_TARGET` is a `upload.example.com` placeholder
  (project has no default upstream upload host).
- `pathnames.h` — defines `_PATH_MCSHELL` (default
  `/usr/local/bin/mcsh`); `_PATH_TCSHELL` is retained with the historic
  `/usr/local/bin/tcsh` value for the compat symlink.
- `tcsh.man.in` — the `.Dt` / `.Sh NAME` / `.Nd` blocks have been
  re-stamped for mcsh; configure now emits the substituted page as
  `mcsh.man` (via `AC_CONFIG_FILES([mcsh.man:tcsh.man.in])`) so the
  installed page is `mcsh(1)` without renaming the source template.
- `dot.login`, `dot.tcshrc` — opening comments identify the file as an
  mcsh example and document the `.mcshrc → .tcshrc → .cshrc` fallback
  chain; a new `dot.mcshrc` is included as the canonical template.

Still deferred (documentation sweep — cosmetic, does not affect
identity/behaviour):

- The body of `tcsh.man.in` still contains thousands of descriptive
  `tcsh` references that document shell features inherited from tcsh.
  These should be audited in a focused pass that disambiguates
  "the shell" (write as `mcsh` / `.Nm`) from "the tcsh-compat surface"
  (keep as `tcsh`).
- `complete.tcsh` is installed unchanged; the file tests `$?tcsh`, which
  still holds because mcsh sets `$tcsh` for compatibility. Renaming the
  source template to `complete.mcsh` is deferred.
- `src.desc`, `eight-bit.me`, `csh-mode.el` carry historical `tcsh`
  references that are documentation-only; sweep in a follow-up.
- NLS catalogues in `nls/` may need regeneration via `catgen` if any
  message strings embed the package name; spot-check done, none found.

## 2. Legacy / obsolete platform code to review

- **VMS**: `vms.termcap.c`, `termcap.vms`, plus `#ifdef _VMS_POSIX`
  blocks throughout `sh.*` and `tc.os.c`. OpenVMS is effectively dead
  for a modern shell; propose removal after an audit, recording the
  deletion in a single well-documented commit.
- **Imake**: `Imakefile` and `imake.config` pre-date autoconf usability;
  autotools alone suffices. Plan removal.
- **`tcsh.vcproj`** (Visual Studio 2008 project) was already excluded
  from this consolidation pass — replace with a modern Windows build
  path (CMake or meson) rather than re-adding it.
- **`win32/` hand-rolled stdio / fork shims**: large hand-maintained
  emulation layer. On modern Windows, Cygwin / MSYS2 / WSL already
  provide a POSIX layer; evaluate whether `win32/` can be retired in
  favour of Cygwin-only support.
- **`system/`**: contains fragments for defunct platforms (Alliant,
  Amdahl, Apollo, CLIPPER, Convex, Cray, DG/UX, DNIX, EWS, FPS500,
  HCX, HK68, HP/3.2, ETA-10, Fortune, ...). Most of these architectures
  have not shipped a Unix in 25+ years. A big chunk can be retired.

## 3. Source-hygiene items spotted in passing

- `sh.char.h` / `sh.h` — K&R-era prototypes in the `#ifndef __STDC__`
  branches should be removable (C89 is the assumed floor everywhere now).
- `tc.alloc.c` still contains a bundled malloc implementation guarded by
  `#ifdef SYSMALLOC`; modern glibc / jemalloc / mimalloc make this
  strictly worse — plan to delete and always link the system allocator.
- `sh.types.h` — large `typedef` thicket for systems without `<stdint.h>`.
  Collapse onto `<stdint.h>` and `<stddef.h>` now that C99 is ubiquitous.
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

## 4. Bugs / warnings already flagged upstream (carry forward)

- Open upstream CVEs against tcsh (none outstanding at time of import);
  track the Astron advisory list so we pull in security fixes.
- Known warning spew with modern GCC (`-Wdeprecated-non-prototype`,
  `-Wimplicit-function-declaration`) in `ed.screen.c`, `tw.parse.c`,
  `sh.exp.c`. Clean up when touching.
- `tests/testsuite.at` needs to be re-audited after the autoconf
  version gets bumped in `configure.ac`; some macros are deprecated
  under autoconf ≥ 2.72.

## 5. Scope of this consolidation push

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
