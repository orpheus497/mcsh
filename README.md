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

## Features added over upstream tcsh

These enhancements are already landed in the source tree and will ship
in the first numbered release:

| Feature | Description |
|---------|-------------|
| **Interactive comments** | `#` is a comment character in interactive mode as well as in scripts (tcsh PR #89) |
| **Expression short-circuit** | `$?a && "$a" != ""` no longer throws when `a` is unset; variable expansion is deferred until after the short-circuit is resolved (tcsh PR #107) |
| **Pipe-to-variable** | `echo foo \| set x` and `set x < file` assign the piped / redirected text to `x` (tcsh PR #105) |
| **`function` builtin** | Named shell functions can be defined with `function name { body }` (tcsh PR #77) |
| **Redirect in `{ }` blocks** | `if ( { cmd >& /dev/null } )` correctly honours the redirection (tcsh issue #113) |

## Bug fixes over upstream tcsh

| Fix | Description |
|-----|-------------|
| `%j` prompt token | Now counts only live job leaders, not all process list entries |
| `getn()` overflow | `@ x = (1 << 63)` no longer raises "Badly formed number"; uses `strtoll` with overflow/errno checking |
| Shift operator UB | `<<` and `>>` now use unsigned arithmetic to eliminate signed-shift undefined behaviour |
| `crypt` link failure | `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` handles the modern `libxcrypt` split |

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
| `nls/`                        | National Language Support catalogues          |
| `system/`                     | Per-platform compile-time config fragments    |
| `acaux/`, `m4/`, `build/`     | Autoconf/autotools auxiliary                  |
| `configure.ac`, `Makefile.in` | GNU Autotools build                           |
| `complete.mcsh`, `complete.tcsh` | Programmable completions (mcsh-native and tcsh-compat) |
| `csh-mode.el`                 | Emacs major mode                              |
| `tcsh.man.in`                 | Manual page template                          |
| `dot.login`, `dot.tcshrc`     | Example user start-up files                   |

## Building

### Linux / BSD / macOS

```sh
autoreconf -fi       # regenerate configure (needed once, or after editing configure.ac)
./configure
make
sudo make install    # installs mcsh + tcsh symlink
```

### Windows (WSL)

mcsh has no native Win32 support. On Windows, build and run mcsh inside
WSL (any WSL 1 or WSL 2 distro):

```sh
# inside an Ubuntu/Debian WSL shell
sudo apt install build-essential autoconf
autoreconf -fi
./configure
make
```

The resulting binary runs identically to the Linux build. To set mcsh as
your default WSL shell, add the installed path to `/etc/shells` and call
`chsh`.

### Cygwin

Cygwin is supported as a genuine POSIX environment:

```sh
# inside a Cygwin terminal (install autoconf, gcc via Cygwin setup)
autoreconf -fi
./configure
make
```

## Compatibility notes

- mcsh sources `~/.mcshrc` on startup; falling back to `~/.tcshrc` then
  `~/.cshrc`. No existing tcsh/csh configuration needs to be renamed.
- `complete.mcsh` is the mcsh-native completion file and the preferred
  choice for new mcsh setups: it checks `$?mcsh` first and only falls back to
  the legacy `$?tcsh` guard for shells that predate the `$mcsh` variable.
  `complete.tcsh` is retained solely for legacy tcsh-only configurations where
  `$mcsh` is never set.
- The `tcsh` binary symlink created by `make install` means existing
  scripts, `/etc/shells` entries, and chsh configurations continue to work.

## Licensing

mcsh itself is BSD 3-Clause (see `LICENSE`). The upstream tcsh / etcsh
source that forms the bulk of this tree is BSD 3-Clause as well
(see `UPSTREAM-COPYRIGHT`). Both licenses are compatible and preserved;
redistribution must carry both notices — see `NOTICE` for details.

## Status

See `ISSUES.md` for the running log of bugs, compatibility items, and
modernisation tasks noticed during consolidation. See `PLAN.md` for the
full phased execution plan.
