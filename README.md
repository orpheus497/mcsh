# mcsh — Modern C Shell

**mcsh** is a consolidated, modernised fusion of
[tcsh](https://github.com/tcsh-org/tcsh) (by the
[tcsh-org](https://github.com/tcsh-org) maintainers, originally by
Christos Zoulas and the tcsh community, building on the work of Ken Greer,
Mike Ellis, and Paul Placeway) and
[etcsh](https://github.com/Krush206/etcsh) (by
[Krush206](https://github.com/Krush206)) into a single, polished, fully
compatible reincarnation of the Berkeley C Shell. Both upstream projects
are the authoritative sources of the underlying shell engine and etcsh
language extensions respectively — mcsh exists to merge and maintain them.

The installed program is `mcsh(1)`. Everywhere in this repository, in the
binary, and in the manual page, the shell identifies itself as
**Modern C Shell** — not as tcsh, etcsh, or csh.

---

## Backward compatibility

mcsh is a drop-in replacement for tcsh and csh:

| Compatibility item | Behaviour |
|--------------------|-----------|
| **Start-up files** | Reads `~/.mcshrc` first; falls back to `~/.tcshrc` then `~/.cshrc`. No existing configuration needs renaming. |
| **Binary** | Installs as `mcsh`. A `tcsh` symlink is created alongside it so scripts that invoke `/usr/local/bin/tcsh` keep working. |
| **Manual page** | `man mcsh` is canonical. `man tcsh` is a symlink to the same page. |
| **Shell variables** | Both `$mcsh` and `$tcsh` are set to the running version string, so scripts guarded by `if ($?tcsh)` continue to fire. |
| **`$version`** | Banner reads `mcsh <ver> (<origin>) … [tcsh baseline <upstream-ver>] options …`, preserving the upstream tcsh version that mcsh was consolidated from. |

---

## Features added over upstream tcsh

### Language

| Feature | Description |
|---------|-------------|
| **Interactive comments** | `#` is a comment character in interactive mode as well as in scripts (tcsh PR #89) |
| **Expression short-circuit** | `$?a && "$a" != ""` no longer throws when `a` is unset; variable expansion is deferred until after the short-circuit is resolved (tcsh PR #107) |
| **Pipe-to-variable** | `echo foo \| set x` and `set x < file` assign the piped / redirected text to `x` (tcsh PR #105) |
| **`function` builtin** | Named shell functions can be defined with `function name { body }` (tcsh PR #77) |
| **Redirect in `{ }` blocks** | `if ( { cmd >& /dev/null } )` correctly honours the redirection (tcsh issue #113) |

### Editor / interactive experience

| Feature | `set` variable | Description |
|---------|----------------|-------------|
| **Fish-style predictive autocomplete** | *(always active)* | As you type, the most recent matching history entry is shown as inline ghost text (dimmed). Press Right-Arrow or `^F` to accept the full suggestion. |
| **Interactive syntax highlighting** | `set syntax` | Per-keystroke ANSI colour highlighting of keywords, builtins, commands (ok/bad), operators, variables, strings (double/single/backtick), comments, and unmatched-quote errors. A 32-entry LRU cache avoids repeated `stat(2)` calls per `$PATH` lookup. |
| **Filetype colouring in completion** | `set color` | Coloured filetype indicators in tab-completion listings, driven by `LSCOLORS` / `LS_COLORS`. |

### Prompt

| Feature | Description |
|---------|-------------|
| **Native git branch** | `%g` expands to the current branch name; `%G` also appends the operation state (`main\|MERGING`, `main\|REBASING-i`, etc.). Both are empty outside a git repository. Cached per-CWD with independent HEAD and state-marker mtime tracking so merges, rebases, and cherry-picks are detected immediately without false refreshes. |

### Directory stack (zsh-style navigation)

| Feature | Description |
|---------|-------------|
| **Numbered tree display** | `pushd`, `popd`, and `cd` show the directory stack as a numbered vertical list after every navigation. The current directory (index 0) is marked with `→`. |
| **`dirs -v` arrow marker** | `dirs -v` marks index 0 with `→` so the current position is always visible at a glance. |
| **`cd -N`** | Jumps to stack entry N counted from the **bottom** (oldest entry), mirroring zsh's `cd -N` semantics. Complements the existing `cd +N` (forward from current). A bare `cd -` still switches to `$owd`. |
| **`pushd +N` / `popd +N`** | Unchanged: rotate or pop the Nth entry from the top. |

#### Syntax highlighting token colours

| Token | Default colour |
|-------|---------------|
| Keyword (`if`, `while`, `foreach`, …) | Bold cyan |
| Builtin (`set`, `alias`, `cd`, …) | Bold green |
| Command — found on `$PATH` | Green |
| Command — not found | Bold red |
| Operator (`\|`, `;`, `&&`, …) | Yellow |
| Variable (`$var`, `$?var`) | Magenta |
| Double-quoted string | Yellow |
| Single-quoted string | Yellow |
| Backtick substitution | Cyan |
| Comment | Bright black (grey) |
| Unmatched quote / error | Bold red |

---

## Bug fixes over upstream tcsh

| Fix | Description |
|-----|-------------|
| `%j` prompt token | Counts only live job leaders, not all process-list entries |
| `getn()` overflow | `@ x = (1 << 63)` no longer raises "Badly formed number"; uses `strtoll` with overflow/errno checking |
| Shift operator UB | `<<` and `>>` use unsigned arithmetic to eliminate signed-shift undefined behaviour |
| `crypt` link failure | `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` handles the modern `libxcrypt` split |
| `vms.termcap.c` OOB scans | Colon-scan loops stop at `'\0'`; `sscanf` uses `%[^|:]` + `strcmp` for exact name matching; `fgets` continuation tracks remaining buffer capacity |
| `vms.termcap.c` tgoto | Static buffer enlarged to 64 bytes; `%d` uses `snprintf`; bounds checked throughout |
| `vms.termcap.c` octal | Octal digits `4`–`7` handled; continuation digits validated as `<= '7'` |
| `acaux/install-sh` name patterns | Case patterns use `*` suffix to catch multi-character names beginning with `-`, `=`, `(`, `)`, `!` |
| `m4/lib-prefix.m4` | `dn;` comment typo corrected to `dnl` |
| `m4/po.m4` C# DLL cleanup | Error cleanup removes the actual DLL target, not the `.msg` source |
| `configure.ac` patchlevel | `PACKAGE_PATCHLEVEL` stripped of leading zeros, preventing invalid C integer literals like `08` |
| `configure.ac` baseline version | `TCSH_BASELINE_VERSION` correctly expands `TCSH_VERSION` to a quoted C string literal in `config.h` |
| `sh.func.c` doif truncation | `doif()` uses `tcsh_number_t` to avoid truncating wide expression results |
| `ed.defns.c` catalog collision | `predict-accept` uses NLS catalog ID 124 (was 122, colliding with `newline-and-hold`) |
| `ed.screen.c` SGR desync | `SetSGRColor()` emits `ESC[22;39m` (not `ESC[0m`) for default-fg/no-bold, preserving `cur_atr` synchronisation |
| `ed.refresh.c` ghost SGR | `DrawGhost()` resets with `ESC[22;39m` (not `ESC[0m`) so `cur_atr` stays consistent on the incremental path |
| `ed.inputl.c` extra refresh | `CC_NORM` + `set syntax` calls `syntax_colorize()` directly without promoting to `CC_REFRESH`, eliminating the double `Refresh()` per keystroke |
| `tc.prompt.c` marker mtime | Git cache tracks HEAD mtime and state-marker max-mtime independently — a live `MERGE_HEAD` no longer forces a refresh on every prompt |
| `dch-template.in` distribution | Template uses `UNRELEASED` instead of `unstable` |
| `alacritty.toml` portability | Shell invoked by name via `PATH`; pywal import commented out as optional |

---

## Prompt reference

| Escape | Expands to |
|--------|-----------|
| `%g` | Current git branch name (empty outside a git repo) |
| `%G` | Branch name plus operation state: `main\|MERGING`, `main\|REBASING-i`, etc. (empty outside a git repo) |
| `%?` | Exit status of the last command |
| `%B` / `%b` | Bold on / off |
| `%U` / `%u` | Underline on / off |
| `%S` / `%s` | Standout (reverse video) on / off |
| `%{…%}` | Literal (zero-width) escape sequences |
| `%n` | Username |
| `%m` | Hostname (first component) |
| `%c02` / `%~` | Trailing 2 components of CWD / CWD with `~` substitution |
| `%j` | Number of running jobs |
| `%#` | `#` for root, `%` otherwise |

Example — right-prompt showing git branch in standout:

```csh
set rprompt = '%S%G%s'
```

Example — full colour prompt with git and exit status:

```csh
set red   = "%{\033[1;31m%}"
set green = "%{\033[1;32m%}"
set blue  = "%{\033[1;34m%}"
set reset = "%{\033[0m%}"
set prompt = "${green}%n@%m${reset}:${blue}%B%c02%b${reset} [${red}%?${reset}] %# "
```

---

## Directory stack navigation

mcsh adds zsh-style directory stack tree display and `cd -N` navigation.

```
% pushd ~/projects/foo      # push new directory
0→  ~/projects/foo
1   ~/projects
2   ~

% pushd ~/etc
0→  ~/etc
1   ~/projects/foo
2   ~/projects
3   ~

% cd -2                     # jump to entry 2 from bottom (oldest visible non-cwd)
0→  ~/projects/foo
1   ~/etc
2   ~/projects
3   ~

% popd
0→  ~/etc
1   ~/projects
2   ~

% dirs -v                   # explicit numbered listing
0→  ~/etc
1   ~/projects
2   ~
```

Keybindings / aliases set by `dot.mcshrc`:

| Alias | Command |
|-------|---------|
| `pd` | `pushd` |
| `po` | `popd` |
| `d` | `dirs -v` |
| `..` | `cd ..` |
| `...` | `cd ../..` |

---

## Source layout

The tree preserves the traditional tcsh flat layout:

| Prefix / file | Purpose |
|---------------|---------|
| `sh.*.c` / `sh.*.h` | Core shell (parser, executor, history, jobs, directory stack, …) |
| `ed.*.c` / `ed.*.h` | Command-line editor (readline equivalent, syntax highlighting, ghost text) |
| `tc.*.c` / `tc.*.h` | tcsh extensions (prompts, key bindings, NLS, completion, …) |
| `tw.*.c` / `tw.*.h` | Tab / word completion and filetype colouring |
| `glob.c` / `glob.h` | Pattern globbing |
| `dotlock.c` / `dotlock.h` | History file locking |
| `mi.*`, `ma.setp.c` | POSIX / BSD compatibility shims |
| `gethost.c`, `host.defs` | Host-table generator |
| `ed.syntax.c` / `ed.syntax.h` | Native interactive syntax highlighting engine |
| `nls/` | National Language Support catalogues |
| `system/` | Per-platform compile-time config fragments |
| `acaux/`, `m4/` | Autoconf / autotools auxiliary files |
| `configure.ac`, `Makefile.in` | GNU Autotools build system |
| `complete.mcsh`, `complete.tcsh` | Programmable completion rules |
| `csh-mode.el` | Emacs major mode for csh/mcsh scripts |
| `tcsh.man.in` | Manual page template |
| `dot.login`, `dot.tcshrc`, `dot.mcshrc` | Example user start-up files |

---

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

### WSL (Windows Subsystem for Linux)

mcsh has no native Win32 support. Build inside WSL:

```sh
sudo apt install build-essential autoconf automake
autoreconf -fi && ./configure && make
```

### Cygwin

```sh
autoreconf -fi && ./configure && make
```

---

## dot.mcshrc reference

`dot.mcshrc` is the canonical start-up file. Copy it to `~/.mcshrc`:

```sh
cp dot.mcshrc ~/.mcshrc
```

Sections and what they provide:

| Section | Key settings |
|---------|-------------|
| **1 — Display server** | Wayland env vars gated behind `/dev/dri/card0` + `$WAYLAND_DISPLAY` presence check. Machine-specific GPU overrides go in `~/.mcshrc.local`. |
| **2 — System environment** | Prepends `~/.local/bin` to `$path`; sets `EDITOR`, `VISUAL`, `PAGER`, `LESS`, `BLOCKSIZE`, `CLICOLOR`, `LSCOLORS`. |
| **3 — Core execution engine** | `set autorehash`, `autolist=ambiguous`, `autoexpand`, `autocorrect`, `color`, **`syntax`**, `correct=cmd`, `ellipsis`, `filec`, `listjobs=long`, `listlinks`, `listmax=100`, `matchbeep=never`, `rmstar`, `symlinks=chase`; history 10 000 entries with merge-dedup to `~/.mcsh_history`. |
| **4 — Key bindings** | Emacs mode; Up/Down arrow history-search; Ctrl+Arrow word navigation; Home/End for xterm/vt100/rxvt/application-cursor; `magic-space`, `backward-delete-word`, `run-fg-editor`, `kill-region`. |
| **5 — Completions** | `cc`/`clang`/`gcc` (file extensions + `-I`/`-L`); `make` (reads live target list); `man`, `kill`, `sysctl`, `service`, `ifconfig`, `cd`, `tar`/`gzip`/`xz`/`bzip2`. |
| **6 — Aliases** | `ls -F -G`, `l`, `ll`, `df -h`, `du -ch`, `..`, `...`, `pd`/`po`/`d` (pushd/popd/dirs), `dis` (objdump Intel syntax), `cclean`, `h`, `j`, `m`, `g`. |
| **7 — Prompt** | `%g`/`%G` git escapes; colour-coded `prompt` with user@host, CWD, exit status; `rprompt='%S%G%s'`; `prompt2` and `prompt3` for multi-line and correction. |
| **8 — Host completion** | Builds `$hosts` from `~/.hosts`, `~/.rhosts`, `~/.ssh/known_hosts` for SSH/rlogin completion. |
| **9 — System-specific** | Sets `stty status ^G` + binds `stuff-char` on BSD/Darwin/FreeBSD/NetBSD; `set time` coloured format at the end so startup commands are not timed. |
| **Root guard** | Unsets `savehist`; sets `LESSHISTFILE=-` and `VIMINIT='set viminfo='` when `$uid == 0`. |
| **Local overrides** | Sources `~/.mcshrc.local` last if it exists — machine-specific GPU vars, paths, tokens go there. |

---

## Compatibility notes

- mcsh sources `~/.mcshrc` on startup, falling back to `~/.tcshrc` then `~/.cshrc`. No existing configuration needs to be renamed.
- `complete.mcsh` is the mcsh-native completion file. `complete.tcsh` is retained for legacy setups that test `$?tcsh`.
- The `tcsh` binary symlink created by `make install` ensures existing scripts and `/etc/shells` entries keep working.

---

## Known Limitations

### Unicode / wide-character regression — fixed (Round 9)

Multi-byte characters — emoji, CJK, Latin Extended, and any character whose
UTF-8 encoding is longer than one byte — were previously silently dropped or
corrupted during filename glob expansion and variable assignment. This was a
byte-vs-character length bug inherited from tcsh 6.24.14 (tracked as tcsh
issues #117 / #121).

**Fix (`sh.lex.c`, `sh.dol.c`):** the `mbtowc` accumulation loops in
`wide_read()` and the `$<` line-read primitive now bound partial-byte
lookahead by the runtime `MB_CUR_MAX` instead of the compile-time
`MB_LEN_MAX`. After a stray invalid byte the loop no longer over-reads up to
15 bytes of subsequent valid UTF-8.

**Tests:** `tests/t009_unicode_vars.sh` through
`tests/t014_unicode_script_source.sh` cover variable round-trip,
`$%` character count, glob expansion, `$<` stdin read, backquote
substitution, invalid-byte recovery, and sourced-script Unicode.
See `ISSUES.md` Round 9 for details.

### Short-circuit evaluation — fixed (dev4, improved in Round 7)

`if ($?a && "$a" != "")` previously threw "Undefined variable" even when
`$a` was unset because `Dfix()` expanded all `$` tokens before `&&`
short-circuiting could suppress evaluation. Fixed in `sh.dol.c`: unset
variables now silently expand to `""`, matching bash/zsh semantics.
Modifier expressions like `${unset:h}` and dimen expressions like `$#unset`
also no longer error — they return `""` and `0` respectively.
See `ISSUES.md` Rounds 6–7.

### Test coverage

An initial regression suite is in `tests/` covering startup variables,
arithmetic overflow/shift semantics, short-circuit evaluation, pipe-to-var,
directory stack, the `function` builtin, signed right-shift, and unset variable
modifier handling. This covers the core new features but is not exhaustive. Run with:

```sh
make -C tests MCSH=./mcsh check
```

---

## Licensing

mcsh is BSD 3-Clause (see `LICENSE`). The upstream tcsh / etcsh source is also BSD 3-Clause (see `UPSTREAM-COPYRIGHT`). Redistribution must carry both notices — see `NOTICE` for details.

---

## Credits & Upstream

mcsh is built on the shoulders of two excellent upstream projects:

| Project | Repository | Role in mcsh |
|---------|-----------|--------------|
| **tcsh** | [github.com/tcsh-org/tcsh](https://github.com/tcsh-org/tcsh) | The authoritative upstream shell engine. Originally written by Ken Greer and Mike Ellis at Carnegie Mellon, extended by Paul Placeway, and now maintained by Christos Zoulas and the tcsh community. The bulk of `sh.*`, `ed.*`, `tc.*`, `tw.*`, NLS, and build system derive from this project. |
| **etcsh** | [github.com/Krush206/etcsh](https://github.com/Krush206/etcsh) | An enhanced fork of tcsh by [Krush206](https://github.com/Krush206), adding function declarations, interactive comments, pipe-to-variable, expression short-circuit fixes, and other language improvements — all of which are incorporated into mcsh. |

All original copyright notices are preserved in `UPSTREAM-COPYRIGHT` and `NOTICE`.
Bug fixes and feature PRs that originated in the upstream repositories are
credited in `ISSUES.md` and `PLAN.md` with their upstream issue/PR numbers.

---

## Status

See `ISSUES.md` for the running log of bugs, compatibility items, and modernisation tasks.  
See `PLAN.md` for the full phased execution plan.
