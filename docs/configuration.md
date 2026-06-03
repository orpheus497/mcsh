# Configuration

How to configure the shell using dotfiles.

## dot.mcshrc Reference

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

## Startup and Shutdown

A login shell begins by executing commands from the system files /etc/csh.cshrc and /etc/csh.login . It then executes commands from files in the user's home directory: first ~/.tcshrc (+) or, if ~/.tcshrc is not found, ~/.cshrc , then the contents of ~/.history (or the value of the histfile shell variable) are loaded into memory, then ~/.login , and finally ~/.cshdirs (or the value of the dirsfile shell variable) (+). The shell may read /etc/csh.login before instead of after /etc/csh.cshrc , and ~/.login before instead of after ~/.tcshrc or ~/.cshrc and ~/.history , if so compiled; see the version shell variable. (+)

Non-login shells read only /etc/csh.cshrc and ~/.tcshrc or ~/.cshrc on startup.

For examples of startup files, please consult: http://tcshrc.sourceforge.net

Commands like stty(1) and tset(1) , which need be run only once per login, usually go in one's ~/.login file. Users who need to use the same set of files with both csh(1) and mcsh can have only a ~/.cshrc which checks for the existence of the tcsh shell variable before using mcsh-specific commands, or can have both a ~/.cshrc and a ~/.tcshrc which source s (see the builtin command) ~/.cshrc . The rest of this manual uses ~/.tcshrc to mean ~/.tcshrc or, if ~/.tcshrc is not found, ~/.cshrc .

In the normal case, the shell begins reading commands from the terminal, prompting with >\

(Processing of arguments and the use of the shell to process files containing command scripts are described later.) The shell repeatedly reads a line of command input, breaks it into words, places it on the command history list, parses it and executes each command in the line.

One can log out by typing ^D on an empty line, logout or login or via the shell's autologout mechanism (see the autologout shell variable). When a login shell terminates it sets the logout shell variable to normal or automatic as appropriate, then executes commands from the files /etc/csh.logout and ~/.logout . The shell may drop DTR on logout if so compiled; see the version shell variable.

The names of the system login and logout files vary from system to system for compatibility with different csh(1) variants; see FILES .
