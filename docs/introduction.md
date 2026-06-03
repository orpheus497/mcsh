# Introduction to mcsh

mcsh is a consolidated, modernised fusion of tcsh and etcsh into a single, polished, fully compatible reincarnation of the Berkeley C Shell. This guide is designed to teach you how the shell works.

## Philosophy and Background

(Modern C Shell) is a consolidated, modernised fusion of tcsh(1) and the etcsh fork into a single, polished, fully compatible reincarnation of the Berkeley UNIX C shell, csh(1) . It installs as mcsh with a backward-compatibility tcsh symlink, and reads ~/.mcshrc as its primary per-user start-up file, falling back to ~/.tcshrc and then ~/.cshrc so existing configurations continue to work unchanged. In the rest of this page the name mcsh is used to describe the running shell; historical occurrences of tcsh in this document should be read as describing the tcsh-derived feature set that mcsh inherits.

mcsh is an enhanced but completely compatible version of the Berkeley UNIX C shell, csh(1) . It is a command language interpreter usable both as an interactive login shell and a shell script command processor. It includes a command-line editor (see The command-line editor (+) ) , programmable word completion (see Completion and listing (+) ) , spelling correction (see Spelling correction (+) ) , a history mechanism (see History substitution ) , job control (see Jobs ) and a C-like syntax. The NEW FEATURES (+) section describes major enhancements of mcsh over csh(1) . Throughout this manual, features of mcsh not found in most `csh(1)` implementations (specifically, the 4.4BSD csh(1) ) are labeled with (+) , and features which are present in csh(1) but not usually documented are labeled with (u) .

## Backward Compatibility

mcsh is a drop-in replacement for tcsh and csh:

| Compatibility item | Behaviour |
|--------------------|-----------|
| **Start-up files** | Reads `~/.mcshrc` first; falls back to `~/.tcshrc` then `~/.cshrc`. No existing configuration needs renaming. |
| **Binary** | Installs as `mcsh`. A `tcsh` symlink is created alongside it so scripts that invoke `/usr/local/bin/tcsh` keep working. |
| **Manual page** | `man mcsh` is canonical. `man tcsh` is a symlink to the same page. |
| **Shell variables** | Both `$mcsh` and `$tcsh` are set to the running version string, so scripts guarded by `if ($?tcsh)` continue to fire. |
| **`$version`** | Banner reads `mcsh <ver> (<origin>) … [tcsh baseline <upstream-ver>] options …`, preserving the upstream tcsh version that mcsh was consolidated from. |

---
