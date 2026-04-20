# mcsh — Consolidation Issue Log

Running log of bugs, obsolete code, and modernisation tasks noticed while
consolidating `tcsh` and `etcsh` into `mcsh`. Items are notes, not yet
triaged or prioritised; they will be burned down as polishing proceeds.

## 1. Identity / branding

- `configure.ac` still declares `AC_INIT([Tcsh], ...)` with a `Tcsh`
  package name and `6.24.13` version carried over from etcsh. These need
  to be re-stamped for mcsh (new package name, new version baseline,
  new bug-report URL).
- `tcsh.man.in` — rename to `mcsh.man.in`; update `.TH` header, `.SH NAME`,
  and every self-reference so the installed manual page is `mcsh(1)`.
- `Makefile.in` installs the program as `tcsh`. Update `PROGRAM`, `MAN`,
  and related rules to install as `mcsh`, with an optional `tcsh`
  compatibility symlink.
- `patchlevel.h.in` — currently only substitutes tcsh origin strings;
  add an `MCSH_*` block so `tc.vers.c` can print the mcsh identity.
- `dot.login`, `dot.tcshrc` still refer to `tcsh` by name in comments.

## 2. Legacy / obsolete platform code to review

- **VMS**: `vms.termcap.c`, `termcap.vms`, plus `#ifdef _VMS_POSIX`
  blocks throughout `sh.*` and `tc.os.c`. OpenVMS is effectively dead
  for a modern shell; propose removal after an audit, recording the
  deletion in a single well-documented commit.
- **Imake**: `Imakefile`, `imake.config`, and `Makefile.std` pre-date
  autoconf usability; autotools alone suffices. Plan removal.
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

## 5. Consolidation decisions made during this pass

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
  `push-tcsh-git-mirror`, `svn`, `foo`, `bar`, `tcsh.man2html`,
  `tcsh.vcproj`.
- `Copyright` from upstream was renamed to `UPSTREAM-COPYRIGHT` to
  clearly distinguish it from mcsh's own `LICENSE` file. No wording
  was altered.
