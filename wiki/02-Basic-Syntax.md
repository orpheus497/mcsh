# 02 - Basic Syntax

In this section, we will cover the fundamentals of writing and running a C Shell script.

## 1. The Shebang (`#!`)
Every shell script should begin with a special line called a "shebang". This tells the operating system which program should be used to interpret the script.

For a C Shell script, the very first line of your file should be:
```csh
#!/bin/csh -f
```
*(Alternatively, you might use `#!/usr/bin/env csh`, `#!/bin/tcsh -f`, or `#!/usr/local/bin/mcsh -f` depending on your system).*

**Important Note on the `-f` flag:**
You will almost always see `-f` used in the shebang of C Shell scripts. The `-f` stands for "fast". It tells the shell **not** to load the user's personal configuration files (like `~/.cshrc` or `~/.tcshrc`) when starting up. This ensures your script runs in a clean, predictable environment and doesn't unexpectedly break because a user has a weird alias set up!

## 2. Hello World (Outputting Text)
To print text to the screen, use the `echo` command.

```csh
#!/bin/csh -f

echo "Hello, World!"
```

### Formatting Output with `printf`
If you need more control over how text is formatted (like padding numbers or avoiding automatic newlines), you can use `printf` (which works just like the `printf` function in the C programming language).

```csh
printf "Hello %s, you have %d messages.\n" "Alice" 5
```
*Note: `\n` is required to print a newline when using `printf`.*

## 3. Comments
Comments are notes in the code written for humans to read. The shell ignores them. In C Shell, any line starting with a `#` is a comment (except for the shebang on line 1).

```csh
#!/bin/csh -f

# This is a comment. It explains what the script does.
echo "Starting process..." # This is an inline comment
```

## 4. Making the Script Executable
Once you have written a script and saved it to a file (for example, `myscript.csh`), you need to give the file "execute permissions" before you can run it.

You do this from the terminal using the `chmod` command:
```sh
chmod +x myscript.csh
```

## 5. Running the Script
To run your newly executable script, simply type `./` followed by the name of the script in your terminal:
```sh
./myscript.csh
```

---
**Next up:** [03 - Variables and Arrays](03-Variables-and-Arrays.md)