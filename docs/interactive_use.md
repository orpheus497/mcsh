# Interactive Use

This section covers features you will use when interacting directly with the terminal.

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
| **Fish-style predictive autocomplete** | `set predict` | As you type, the most recent matching history entry, file path, or command is shown as inline ghost text (dimmed). Press Right-Arrow or `^F` to accept the full suggestion. Includes a filesystem/PATH cache to ensure zero latency. |
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
set red  = "%{\033[1;31m%}"
set green = "%{\033[1;32m%}"
set blue  = "%{\033[1;34m%}"
set reset = "%{\033[0m%}"
set prompt = "${green}%n@%m${reset}:${blue}%B%c02%b${reset} [${red}%?${reset}] %# "
```

---

mcsh adds zsh-style directory stack tree display and `cd -N` navigation.

```
% pushd ~/projects/foo  # push new directory
0→  ~/projects/foo
1  ~/projects
2  ~

% pushd ~/etc
0→  ~/etc
1  ~/projects/foo
2  ~/projects
3  ~

% cd -2  # jump to entry 2 from bottom (oldest visible non-cwd)
0→  ~/projects/foo
1  ~/etc
2  ~/projects
3  ~

% popd
0→  ~/etc
1  ~/projects
2  ~

% dirs -v  # explicit numbered listing
0→  ~/etc
1  ~/projects
2  ~
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

## Command-line Editor

Command-line input can be edited using key sequences much like those used in emacs(1) or vi(1) . The editor is active only when the edit shell variable is set, which it is by default in interactive shells. The bindkey builtin can display and change key bindings to editor commands (see Editor commands (+) ) . emacs(1) -style key bindings are used by default (unless the shell was compiled otherwise; see the version shell variable), but bindkey can change the key bindings to vi(1) -style bindings en masse.

The shell always binds the arrow keys (as defined in the TERMCAP environment variable) to editor commands:

* **Key**
Editor command

* `down`
down-history
* `up`
up-history
* `left`
backward-char
* `right`
forward-char

unless doing so would alter another single-character binding. One can set the arrow key escape sequences to the empty string with settc to prevent these bindings. The ANSI/VT100 sequences for arrow keys are always bound.

Other key bindings are, for the most part, what emacs(1) and vi(1) users would expect and can easily be displayed by bindkey , so there is no need to list them here. Likewise, bindkey can list the editor commands with a short description of each. Certain key bindings have different behavior depending if emacs(1) or vi(1) -style bindings are being used; see vimode for more information.

Note that editor commands do not have the same notion of a word as does the shell. The editor delimits words with any non-alphanumeric characters not in the shell variable wordchars , while the shell recognizes only whitespace and some of the characters with special meanings to it, listed under Lexical structure .

## Completion and Listing

The shell is often able to complete words when given a unique abbreviation. For example, typing part of a word ls /usr/lost and hit the tab key to run the complete-word editor command. The shell completes the filename /usr/lost to /usr/lost+found/ , replacing the incomplete word with the complete word in the input buffer. (Note the terminal Pa / ; completion adds a / to the end of completed directories and a space to the end of other completed words, to speed typing and provide a visual indicator of successful completion. The addsuffix shell variable can be unset to prevent this.) If no match is found (perhaps /usr/lost+found doesn't exist), the terminal bell rings. If the word is already complete (perhaps there is a /usr/lost on your system, or perhaps you were thinking too far ahead and typed the whole thing) a / or space is added to the end if it isn't already there.

Completion works anywhere in the line, not at just the end; completed text pushes the rest of the line to the right. Completion in the middle of a word often results in leftover characters to the right of the cursor that need to be deleted.

Commands and variables can be completed in much the same way. For example, typing em[tab] would complete em to emacs if emacs were the only command on your system beginning with em . Completion can find a command in any directory in path or if given a full pathname.

Typing echo $ar[tab] would complete $ar to $argv if no other variable began with ar .

The shell parses the input buffer to determine whether the word you want to complete should be completed as a filename, command or variable. The first word in the buffer and the first word following ; , | , |& , && , or || is considered to be a command. A word beginning with $ is considered to be a variable. Anything else is a filename. An empty line is completed as a filename.

You can list the possible completions of a word at any time by typing ^D to run the delete-char-or-list-or-eof editor command. The shell lists the possible completions using the ls-F builtin and reprints the prompt and unfinished command line, for example:
```
> ls /usr/l[^D]
lbin/  lib/  local/  lost+found/
> ls /usr/l
```

If the autolist shell variable is set, the shell lists the remaining choices (if any) whenever completion fails:
```
> set autolist
> nm /usr/lib/libt[tab]
libtermcap.a@ libtermlib.a@
> nm /usr/lib/libterm
```

If the autolist shell variable is set to ambiguous , choices are listed only when completion fails and adds no new characters to the word being completed.

A filename to be completed can contain variables, your own or others' home directories abbreviated with ~ (see Filename substitution ) and directory stack entries abbreviated with = (see Directory stack substitution (+) ) . For example,
```
> ls ~k[^D]
kahn  kas  kellogg
> ls ~ke[tab]
> ls ~kellogg/
```

or
```
> set local = /usr/local
> ls $lo[tab]
> ls $local/[^D]
bin/ etc/ lib/ man/ src/
> ls $local/
```

Note that variables can also be expanded explicitly with the expand-variables editor command.

delete-char-or-list-or-eof lists at only the end of the line; in the middle of a line it deletes the character under the cursor and on an empty line it logs one out or, if the ignoreeof variable is set, does nothing. M-^D , bound to the editor command list-choices , lists completion possibilities anywhere on a line, and list-choices (or any one of the related editor commands that do or don't delete, list and/or log out, listed under delete-char-or-list-or-eof ) can be bound to ^D with the bindkey builtin command if so desired.

The complete-word-fwd and complete-word-back editor commands (not bound to any keys by default) can be used to cycle up and down through the list of possible completions, replacing the current word with the next or previous word in the list.

The shell variable fignore can be set to a list of suffixes to be ignored by completion. Consider the following:
```
> ls
Makefile  condiments.h~  main.o  side.c
README  main.c  meal  side.o
condiments.h  main.c~
> set fignore = (.o \~)
> emacs ma[^D]
main.c  main.c~  main.o
> emacs ma[tab]
> emacs main.c
```

main.c~ and main.o are ignored by completion (but not listing), because they end in suffixes in fignore . Note that a \ was needed in front of ~ to prevent it from being expanded to home as described under Filename substitution . fignore is ignored if only one completion is possible.

If the complete shell variable is set to enhance , completion 1) ignores case and 2) considers periods, hyphens and underscores . , - , and _ to be word separators and hyphens and underscores to be equivalent. If you had the following files
```
comp.lang.c  comp.lang.perl  comp.std.c++
comp.lang.c++  comp.std.c
```

and typed mail -f c.l.c[tab] it would be completed to mail -f comp.lang.c and typing mail -f c.l.c[^D] would list comp.lang.c and comp.lang.c++ .

Typing mail -f c..c++[^D] would list comp.lang.c++ and comp.std.c++ .

Typing rm a--file[^D] in the following directory
```
A_silly_file  a-hyphenated-file  another_silly_file
```

would list all three files, because case is ignored and hyphens and underscores are equivalent. Periods, however, are not equivalent to hyphens or underscores.

If the complete shell variable is set to Enhance , completion ignores case and differences between a hyphen and an underscore word separator only when the user types a lowercase character or a hyphen. Entering an uppercase character or an underscore will not match the corresponding lowercase character or hyphen word separator.

Typing rm a--file[^D] in the directory of the previous example would still list all three files, but typing rm A--file would match only A_silly_file and typing rm a__file[^D] would match just A_silly_file and another_silly_file because the user explicitly used an uppercase or an underscore character.

Completion and listing are affected by several other shell variables: recexact can be set to complete on the shortest possible unique match, even if more typing might result in a longer match:
```
> ls
fodder  foo  food  foonly
> set recexact
> rm fo[tab]
```

just beeps, because fo could expand to fod or foo , but if we type another o ,
```
> rm foo[tab]
> rm foo
```

the completion completes on foo , even though food and foonly also match. autoexpand can be set to run the expand-history editor command before each completion attempt, autocorrect can be set to spelling-correct the word to be completed (see Spelling correction (+) ) before each completion attempt and correct can be set to complete commands automatically after one hits return. matchbeep can be set to make completion beep or not beep in a variety of situations, and nobeep can be set to never beep at all. nostat can be set to a list of directories and/or patterns that match directories to prevent the completion mechanism from stat(2)ing those directories. listmax and listmaxrows can be set to limit the number of items and rows (respectively) that are listed without asking first. recognize_only_executables can be set to make the shell list only executables when listing commands, but it is quite slow.

Finally, the complete builtin command can be used to tell the shell how to complete words other than filenames, commands and variables. Completion and listing do not work on glob-patterns (see Filename substitution ) , but the list-glob and expand-glob editor commands perform equivalent functions for glob-patterns.
