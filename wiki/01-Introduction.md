# 01 - Introduction to C Shell

Welcome! If you are new to scripting, or new to C Shell in particular, this is the perfect place to start.

## What is a Shell?
A **shell** is a program that takes commands you type on your keyboard and passes them to the operating system to execute. It's essentially a text-based interface to your computer. Shells also have built-in programming languages, allowing you to write **scripts** (files containing a sequence of commands) to automate tasks.

## What is the C Shell?
The **C Shell** (often abbreviated as `csh`) is a Unix shell created by Bill Joy in the 1970s. Its primary goal was to offer a scripting language whose syntax resembled the **C programming language**, making it easier for programmers to write scripts.

Over the years, improved versions of the C Shell were created:
* **tcsh (TENEX C Shell):** The most popular and widely used modern version of the C Shell. It added command-line completion, editing, and history mechanisms. On most modern systems (like macOS or FreeBSD), `csh` is actually just a link to `tcsh`.
* **mcsh (Modern C Shell):** This is the shell this repository is built for! It is a modernised, fully compatible reincarnation of the Berkeley C Shell that fuses features from `tcsh` and `etcsh` (another advanced fork). It features syntax highlighting, predict-autocomplete, and bug fixes, while remaining 100% backward compatible as a drop-in replacement.

## How does C Shell differ from POSIX Shells (like `bash` or `sh`)?
If you've written scripts before, they were likely for POSIX shells like `bash` (Bourne Again Shell) or `sh` (Bourne Shell). C Shell takes a very different approach to syntax:

### 1. Variables and Math
In `bash`, you set a variable like this:
```bash
# bash
name="John"
count=5
((count++))
```

In **C Shell**, variable assignment is explicitly split between strings/arrays (using `set`) and mathematical expressions (using `@`):
```csh
# csh
set name = "John"
@ count = 5
@ count++
```
*(Notice the spaces around the `=` sign in csh, whereas bash forbids them!)*

### 2. Arrays (Word Lists)
In C Shell, arrays are called "word lists", and their index starts at **1** (not 0 like in most programming languages or `bash`). They are incredibly easy to use.
```csh
# csh
set colors = (red green blue)
echo $colors[1]    # Prints: red
```

### 3. Control Flow (If / While / Foreach)
C shell control structures look much more like the C programming language.

**If statement in `bash`:**
```bash
if [ "$name" == "John" ]; then
    echo "Hello John"
fi
```

**If statement in `csh`:**
```csh
if ( "$name" == "John" ) then
    echo "Hello John"
endif
```

## Why use C Shell?
* **Readability:** Many people find C Shell's C-like syntax easier to read and structure than the sometimes cryptic syntax of POSIX shells.
* **History:** It was the default shell for many BSD-based systems for decades and remains heavily used in engineering, academic, and scientific computing environments (e.g., EDA tools in chip design).
* **Powerful Interactive Features:** Especially with `mcsh` and `tcsh`, the interactive command-line experience is incredibly robust out-of-the-box.

---
**Next up:** [02 - Basic Syntax](02-Basic-Syntax.md)