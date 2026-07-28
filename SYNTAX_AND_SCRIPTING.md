# mcsh Syntax and Scripting Guide

This document is a comprehensive reference for writing shell scripts in `mcsh`.
While `mcsh` is backward-compatible with `tcsh` and `csh`, it introduces several
modern language extensions (from the `etcsh` fork) that significantly improve
scripting ergonomics.

---

## 1. Shell Grammar Overview

`mcsh` parses input in roughly this order:
1. **Lexing**: Splitting into words (separated by spaces/tabs).
2. **History Substitution**: `!` and `^` expansions.
3. **Alias Substitution**: Expanding the first word if it matches an alias.
4. **Variable Substitution**: `$` expansions.
5. **Command Substitution**: Backtick `` `cmd` `` evaluation.
6. **Filename Expansion**: Globbing (`*`, `?`, `[]`, `{}`).
7. **Execution**: Setting up redirections and pipelines, then running.

### Interactive Comments
In `mcsh`, a `#` on the command line starts a comment, even in interactive mode
(unlike traditional `tcsh` where it only works in scripts).

---

## 2. Quoting Reference

Quoting protects characters from the parser's expansion phases:

- **Single Quotes (`' '`)**: Protects everything except other single quotes. Variables (`$var`) and history (`!`) are **not** expanded.
- **Double Quotes (`" "`)**: Protects spaces and most glob characters, but **allows** variable substitution (`$var`) and command substitution (`` `cmd` ``).
- **Backslash (`\`)**: Escapes the single following character. Useful for escaping `!` to prevent history substitution.

---

## 3. Variable Substitution

All variables are accessed using `$`.

| Form | Meaning |
|------|---------|
| `$var` | Value of `var`. |
| `${var}` | Value of `var`, isolating the name from surrounding text. |
| `$#var` | Number of words (elements) in the array `var`. |
| `$?var` | Evaluates to `1` if `var` is set, `0` otherwise. |
| `$%var` | The string length (number of characters) of `var`. |
| `$var[n]` | The *n*th word of the array `var` (1-indexed). |
| `$var[m-n]` | A slice of the array `var` from *m* to *n*. |

### Special Built-in Variables
| Variable | Meaning |
|----------|---------|
| `$<` | Reads a single line from standard input. |
| `$$` | The process ID (PID) of the shell. |
| `$!` | The PID of the most recently started background job. |
| `$?` or `$status` | The exit status of the last executed command. |
| `$argv` or `$*` | The argument list passed to the current script or function. |

---

## 4. Variable Modifiers

Variables can be modified during substitution using `:`. Modifiers can be chained.

| Modifier | Action | Example (`set p="/a/b/c.txt"`) |
|----------|--------|----------------------------------|
| `:h` | Head (directory path) | `echo $p:h` → `/a/b` |
| `:t` | Tail (basename) | `echo $p:t` → `c.txt` |
| `:r` | Root (strip extension) | `echo $p:r` → `/a/b/c` |
| `:e` | Extension | `echo $p:e` → `txt` |
| `:l` | Lowercase | `echo $p:l` → `/a/b/c.txt` |
| `:u` | Uppercase | `echo $p:u` → `/A/B/C.TXT` |
| `:q` | Quote the expanded words | Prevents further globbing |
| `:x` | Quote words independently | Prevents globbing, keeps array structure |
| `:g` | Global modifier prefix | e.g. `:gh` applies `:h` to all array elements |

---

## 5. Arithmetic Expressions (`@`)

The `@` builtin evaluates C-like arithmetic expressions.

```csh
@ x = ( 5 + 3 ) * 2
@ i++
@ y = $x << 2
```

### Operators (in order of decreasing precedence)
1. `()` (Grouping)
2. `~` (Bitwise NOT), `!` (Logical NOT)
3. `*`, `/`, `%` (Multiplication, Division, Remainder)
4. `+`, `-` (Addition, Subtraction)
5. `<<`, `>>` (Bitwise Shift — **unsigned** in mcsh to avoid UB)
6. `<`, `>`, `<=`, `>=` (Relational)
7. `==`, `!=`, `=~`, `!~` (Equality / Pattern Match)
8. `&` (Bitwise AND)
9. `^` (Bitwise XOR)
10. `|` (Bitwise OR)
11. `&&` (Logical AND)
12. `||` (Logical OR)

> **Note on short-circuiting**: `mcsh` safely short-circuits logical operators.
> `if ($?var && "$var" == "x")` is completely safe; the right side will not throw
> an "Undefined variable" error if the left side evaluates to false.

---

## 6. Redirection & Pipelines

| Syntax | Effect |
|--------|--------|
| `>` | Redirect stdout, overwriting file. |
| `>>` | Redirect stdout, appending to file. |
| `>&` | Redirect stdout **and** stderr, overwriting. |
| `>>&` | Redirect stdout **and** stderr, appending. |
| `>!` / `>&!` | Force overwrite even if `noclobber` is set. |
| `<` | Redirect stdin from file. |
| `<< EOF` | Here-document. |
| `\|` | Pipe stdout to next command. |
| `\|&` | Pipe stdout **and** stderr to next command. |

### Block Redirection (mcsh extension)
You can redirect the output of an entire block inside a conditional:
```csh
if ({ grep -q "foo" file >& /dev/null }) then
    echo "Found foo"
endif
```

### Pipe-to-Variable (mcsh extension)
You can capture a pipe or redirection directly into a variable using `set`:
```csh
echo "hello" | set msg
set content < file.txt
```

---

## 7. Control Flow

### `if` / `else`
```csh
if ( $var == "yes" ) then
    echo "Agreed"
else if ( $var == "no" ) then
    echo "Refused"
else
    echo "Unknown"
endif
```

### `switch` / `case`
```csh
switch ( $arg )
    case "-h":
    case "--help":
        echo "Help text"
        breaksw
    case "*.txt":
        echo "Text file"
        breaksw
    default:
        echo "Unknown argument"
        breaksw
endsw
```

### Loops (`while` and `foreach`)
```csh
set i = 0
while ( $i < 5 )
    echo $i
    @ i++
end

foreach file ( *.c )
    echo "Compiling $file"
    cc -c $file
end
```

---

## 8. Functions (mcsh extension)

Unlike traditional tcsh (which only supports aliases or external scripts),
`mcsh` provides a `function` builtin.

```csh
function greet
    # Arguments are available via $1, $2, ... and $argv
    echo "Hello $1"
    
    if ( $#argv > 1 ) then
        echo "Extra arguments: $argv[2-]"
    endif
return

greet "World" "foo" "bar"
```

- Functions share the shell's variables (there is no local scope).
- Recursion is permitted but strictly limited to 100 levels.
- A function cannot be redefined in the same session once declared.

---

## 9. Signal Handling

You can trap interrupts (like `Ctrl-C`) in scripts using the `onintr` label jump.

```csh
#!/bin/mcsh -f

onintr cleanup

echo "Doing work... Press Ctrl-C to abort"
sleep 10
echo "Done"
exit 0

cleanup:
    echo "Aborted by user! Cleaning up..."
    rm -f temp.txt
    exit 1
```

---

## 10. Filename Glob Patterns

| Pattern | Matches |
|---------|---------|
| `*` | Any string of zero or more characters. |
| `?` | Any single character. |
| `[...]` | Any one of the enclosed characters. |
| `[^...]` | Any character NOT in the enclosed list. |
| `{a,b}` | Expands to `a` then `b` (brace expansion). |
| `~` | Current user's home directory. |
| `~user` | `user`'s home directory. |

---

## 11. Scripting Best Practices

1. **Shebang**: Always start scripts with `#!/usr/bin/env mcsh -f` or `#!/bin/mcsh -f`. The `-f` flag ensures user startup files (`~/.mcshrc`) are skipped, making your script's execution fast and predictable.
2. **Error Handling**: Check `$status` immediately after a critical command, or run the script with the `-e` flag to abort automatically on the first error.
3. **Quoting**: Quote variables in `if` statements to prevent syntax errors if the variable is empty or contains spaces: `if ( "$var" == "" )`
