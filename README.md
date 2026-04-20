# mcsh — Modern C Shell

**mcsh** is a consolidated, modernised fusion of
[tcsh](https://www.tcsh.org/) and the `etcsh` fork into a single, polished,
fully compatible reincarnation of the Berkeley C Shell. The installed
program is `mcsh(1)`. Everywhere in this repository, in the binary, and
in the manual page, the shell identifies itself as **Modern C Shell** —
not as tcsh, etcsh, or csh.

mcsh is a work-in-progress. This tree is the first consolidation pass: the
complete program and package source of etcsh (itself a superset of the
upstream tcsh repository at `orpheus497/tcsh`) has been brought in as the
base, with announcements, release notes, and other non-source bloat omitted.

## Backward compatibility

mcsh is an mcsh-branded shell with full read-compatibility with existing
tcsh / csh setups:

- **Start-up files.** mcsh reads `~/.mcshrc` first; if absent it falls
  back to `~/.tcshrc` and then `~/.cshrc`, so an existing tcsh or csh
  configuration keeps working unchanged.
- **Binary.** `make install` installs the program as `mcsh` with a
  `tcsh` symlink alongside it, so scripts that invoke `tcsh` still run.
- **Manual page.** `man mcsh` is canonical; `man tcsh` is installed as
  a symlink to the same page.
- **Shell variables.** Both `$mcsh` and `$tcsh` are set to the running
  version string, so scripts guarded by `if ($?tcsh)` continue to fire.
- **`$version`.** The banner now reads
  `mcsh <ver> (<origin>) … [tcsh baseline <upstream-ver>] options …`,
  preserving the upstream tcsh version that mcsh was consolidated from
  for any consumer that needs to probe it.

## Source layout

The tree preserves the traditional tcsh flat layout so upstream-familiar
contributors stay oriented:

| Prefix / file                 | Purpose                                       |
| ----------------------------- | --------------------------------------------- |
| `sh.*.c` / `sh.*.h`           | Core shell (parser, executor, history, etc.)  |
| `ed.*.c` / `ed.*.h`           | Command-line editor                           |
| `tc.*.c` / `tc.*.h`           | tcsh extensions (prompts, bindings, NLS, ...) |
| `tw.*.c` / `tw.*.h`           | Tab / word completion                         |
| `glob.c` / `glob.h`           | Pattern globbing                              |
| `dotlock.c` / `dotlock.h`     | History file locking                          |
| `mi.*`, `ma.setp.c`           | POSIX / BSD compatibility shims               |
| `gethost.c`, `host.defs`      | Host-table generator                          |
| `vms.termcap.c`               | VMS legacy (slated for review, see `ISSUES.md`) |
| `nls/`                        | National Language Support catalogues          |
| `system/`                     | Per-platform compile-time config fragments    |
| `acaux/`, `m4/`, `build/`     | Autoconf/autotools auxiliary                  |
| `configure.ac`, `Makefile.in` | GNU Autotools build                           |
| `Imakefile`, `imake.config`   | Legacy imake build (to be retired)            |
| `complete.tcsh`               | Default programmable completions              |
| `csh-mode.el`                 | Emacs major mode                              |
| `tcsh.man.in`                 | Manual page template                          |
| `dot.login`, `dot.tcshrc`     | Example user start-up files                   |

## Licensing

mcsh itself is BSD 3-Clause (see `LICENSE`). The upstream tcsh / etcsh
source that forms the bulk of this tree is BSD 3-Clause as well
(see `UPSTREAM-COPYRIGHT`). Both licenses are compatible and preserved;
redistribution must carry both notices — see `NOTICE` for details.

## Building

The modern autotools build applies:

```sh
autoreconf -fi       # only needed if regenerating configure
./configure
make
```

This still produces a `tcsh` binary in the current pass; renaming of the
installed artefact to `mcsh`, along with package identifier updates in
`configure.ac` and the manual page, is tracked in `ISSUES.md`.

## Status

See `ISSUES.md` for the running log of bugs, compatibility items, and
modernisation tasks noticed during consolidation.
