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
| **Fish-style predictive autocomplete** | As you type, the most recent matching history entry is shown as inline ghost text; press Right-Arrow to accept |
| **Native git branch in prompt** | `%g` expands to the current branch name; `%G` also appends the operation state (`main\|MERGING`, etc.). Both are empty outside a git repository |
| **Filetype colouring in completion** | `set color` enables coloured filetype indicators in tab-completion listings (driven by `LSCOLORS`/`LS_COLORS`) |
| **Interactive syntax highlighting** | `set syntax` enables per-keystroke ANSI colour highlighting of keywords, builtins, commands (ok/bad), operators, variables, strings, comments, and unmatched quotes |

## Bug fixes over upstream tcsh

| Fix | Description |
|-----|-------------|
| `%j` prompt token | Now counts only live job leaders, not all process list entries |
| `getn()` overflow | `@ x = (1 << 63)` no longer raises "Badly formed number"; uses `strtoll` with overflow/errno checking |
| Shift operator UB | `<<` and `>>` now use unsigned arithmetic to eliminate signed-shift undefined behaviour |
| `crypt` link failure | `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` handles the modern `libxcrypt` split |
| `vms.termcap.c` OOB scans | `tgetnum`, `tgetflag`, `tgetstr` colon-scan loops stop at `'\0'`; `sscanf` uses `%[^|:]` + `strcmp` for exact name matching; `fgets` continuation loop tracks remaining buffer capacity |
| `vms.termcap.c` tgoto | Static buffer enlarged to 64 bytes; `%d` uses `snprintf` for multi-digit coordinates; bounds checked throughout |
| `vms.termcap.c` octal escapes | Octal cases `'4'`–`'7'` now handled; continuation digits validated as `<= '7'` |
| `acaux/install-sh` name patterns | Case patterns guarding against names starting with `-`, `=`, `(`, `)`, `!` now use `*` suffix to catch multi-character names |
| `m4/lib-prefix.m4` | `dn;` comment typo corrected to `dnl` |
| `m4/po.m4` C# DLL cleanup | Error cleanup in generated Makefile rule removes the actual DLL target, not the `.msg` source |
| `configure.ac` patchlevel | `PACKAGE_PATCHLEVEL` is stripped of leading zeros via `sed`, preventing invalid C integer literals like `08` |
| `configure.ac` baseline version | `TCSH_BASELINE_VERSION` uses the `TCSH_VERSION` M4 macro rather than a duplicated string literal |
| `sh.func.c` doif truncation | `doif()` local variable changed from `int` to `tcsh_number_t` to avoid truncating wide expression results |
| `ed.defns.c` catalog collision | `predict-accept` uses NLS catalog ID 124 (was 122, colliding with `newline-and-hold`) |
| `dch-template.in` distribution | Template uses `UNRELEASED` instead of `unstable`, with real release notes |
| `alacritty.toml` portability | Shell invoked by name via `PATH`; pywal import commented out |

## Prompt reference

| Escape | Expands to |
|--------|-----------|
| `%g` | Current git branch name (empty outside a git repo) |
| `%G` | Branch name plus operation state: `main\|MERGING`, `main\|REBASING-i`, etc. (empty outside a git repo) |

Example — show git branch in standout on the right:

```csh
set rprompt = '%S%G%s'
```

## Source layout

The tree preserves the traditional tcsh flat layout so upstream-familiar
contributors stay oriented:

| Prefix / file                    | Purpose                                       |
| -------------------------------- | --------------------------------------------- |
| `sh.*.c` / `sh.*.h`              | Core shell (parser, executor, history, etc.)  |
| `ed.*.c` / `ed.*.h`              | Command-line editor                           |
| `tc.*.c` / `tc.*.h`              | tcsh extensions (prompts, bindings, NLS, ...) |
| `tw.*.c` / `tw.*.h`              | Tab / word completion                         |
| `glob.c` / `glob.h`              | Pattern globbing                              |
| `dotlock.c` / `dotlock.h`        | History file locking                          |
| `mi.*`, `ma.setp.c`              | POSIX / BSD compatibility shims               |
| `gethost.c`, `host.defs`         | Host-table generator                          |
| `nls/`                           | National Language Support catalogues          |
| `system/`                        | Per-platform compile-time config fragments    |
| `acaux/`, `m4/`, `build/`        | Autoconf/autotools auxiliary                  |
| `configure.ac`, `Makefile.in`    | GNU Autotools build                           |
| `complete.mcsh`, `complete.tcsh` | Programmable completions                      |
| `csh-mode.el`                    | Emacs major mode                              |
| `tcsh.man.in`                    | Manual page template                          |
| `dot.login`, `dot.tcshrc`, `dot.mcshrc` | Example user start-up files           |

## Building

### Quick dev build (FreeBSD / Linux / macOS)

```sh
make              # build in-tree; produces ./mcsh
sudo make install # installs mcsh + tcsh symlink
```

### From a clean checkout

```sh
autoreconf -fi   # regenerate configure (needed once after editing configure.ac)
./configure
make
sudo make install
```

### Windows (WSL)

mcsh has no native Win32 support. Build inside WSL:

```sh
sudo apt install build-essential autoconf
autoreconf -fi && ./configure && make
```

### Cygwin

```sh
autoreconf -fi && ./configure && make
```

## dot.mcshrc

`dot.mcshrc` is the reference start-up file for mcsh. Copy it to `~/.mcshrc`:

```sh
cp dot.mcshrc ~/.mcshrc
```

Key settings it provides:

- Full Wayland/GPU environment (Intel + NVIDIA hybrid), PATH, EDITOR, PAGER
- `set color` — filetype colouring in tab-completion listings
- `set syntax` — interactive syntax highlighting (keywords, builtins, commands, variables, strings, comments)
- `set symlinks=chase`, `histdup=erase`, `history=10000`
- Arrow-key history search, Ctrl+Arrow word navigation, Home/End keybindings
- Programmable completions for `cc`, `clang`, `make`, `man`, `kill`,
  `sysctl`, `service`, `ifconfig`, `cd`, `tar`
- Full alias set (`ls -F -G`, `ll`, `df`, `du`, `..`, `cd` stack)
- Prompt uses native `%g`/`%G` git escapes — no starship dependency
- `rprompt='%S%G%s'` — current git branch in standout on the right

## Compatibility notes

- mcsh sources `~/.mcshrc` on startup; falling back to `~/.tcshrc` then
  `~/.cshrc`. No existing tcsh/csh configuration needs to be renamed.
- `complete.mcsh` is the mcsh-native completion file. `complete.tcsh` is
  retained for legacy setups that check only `$?tcsh`.
- The `tcsh` binary symlink created by `make install` ensures existing
  scripts and `/etc/shells` entries continue to work.

## Licensing

mcsh is BSD 3-Clause (see `LICENSE`). The upstream tcsh / etcsh source is
also BSD 3-Clause (see `UPSTREAM-COPYRIGHT`). Redistribution must carry
both notices — see `NOTICE` for details.

## Status

See `ISSUES.md` for the running log of bugs, compatibility items, and
modernisation tasks. See `PLAN.md` for the full phased execution plan.
