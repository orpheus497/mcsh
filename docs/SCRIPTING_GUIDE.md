# Scripting in mcsh: A Practical Guide

Welcome to the `mcsh` scripting guide! This guide will walk you through writing scripts in the Modern C Shell (`mcsh`), offering clear examples and comparing it to what you might know from POSIX shells like `bash` or `zsh`.

`mcsh` uses a syntax that feels more like C programming. Let's dive in!

## 1. Variables

### Setting Variables
In `mcsh`, you use the `set` command for local variables and `setenv` for environment variables. You do *not* use the `=` sign without spaces, like in bash.

**mcsh:**

```csh
set my_file = "data.txt"
set counter = 5
setenv PATH "/usr/local/bin:$PATH"
```

**bash equivalent:**

```bash
my_file="data.txt"
counter=5
export PATH="/usr/local/bin:$PATH"
```

### Using Variables
You access variables with the `$` prefix.

```csh
echo "Processing $my_file"
```

## 2. Arrays (Lists)

Arrays in `mcsh` are simply called lists, and they are defined with parentheses. They are 1-indexed (the first item is at index 1).

**mcsh:**

```csh
set fruits = (apple banana cherry)
echo "First fruit: $fruits[1]"   # Prints 'apple'
echo "All fruits: $fruits"       # Prints all of them
echo "Count: $#fruits"           # Prints '3'
```

**bash equivalent:**

```bash
fruits=(apple banana cherry)
echo "First fruit: ${fruits[0]}"
```

## 3. Conditionals (If / Else)

`mcsh` has excellent support for numerical and string comparisons. Always wrap your conditions in parentheses.

**mcsh:**

```csh
set status = "success"

if ( $status == "success" ) then
    echo "It worked!"
else if ( $status == "error" ) then
    echo "Something broke."
else
    echo "Unknown status."
endif
```

### File Checking
Testing if a file exists or is readable is similar to bash, but notice the syntax:

**mcsh:**

```csh
if ( -e "data.txt" ) then
    echo "File exists."
endif
```

## 4. Loops

### The `foreach` Loop
The easiest way to loop over items or files is `foreach`.

**mcsh:**

```csh
foreach file ( *.txt )
    echo "Processing $file..."
    # Do something with $file
end
```

**bash equivalent:**

```bash
for file in *.txt; do
    echo "Processing $file..."
done
```

### The `while` Loop
For logic that needs to run until a condition is false.

**mcsh:**

```csh
set i = 1
while ( $i <= 5 )
    echo "Number: $i"
    @ i++     # The '@' command is used for math in mcsh
end
```

## 5. Switch Statements

When you have multiple conditions, a `switch` statement is much cleaner than many `if-else` blocks.

**mcsh:**

```csh
set color = "red"

switch ( "$color" )
case "red":
    echo "Stop"
    breaksw
case "green":
    echo "Go"
    breaksw
default:
    echo "Yield"
    breaksw
endsw
```

## 6. Math and Expressions

To do math, use the `@` command. Remember to leave spaces around operators!

**mcsh:**

```csh
set x = 10
@ x = $x + 5
@ x++
echo $x  # Prints 16
```

## 7. Command Substitution

If you want to save the output of a command to a variable, use backticks `` ` ``.

**mcsh:**

```csh
set today = `date +%A`
echo "Today is $today"

# To create an array from command output:
set lines = ( `cat list.txt` )
```

## 8. Real-World Example: Batch Renaming

Here is a practical script that finds all `.jpeg` files and renames them to `.jpg`:

```csh
#!/usr/local/bin/mcsh

set jpegs = ( *.jpeg )

if ( $#jpegs == 0 ) then
    echo "No .jpeg files found."
    exit 0
endif

foreach file ( $jpegs )
    # Extract the filename without the .jpeg extension
    set base = $file:r
    
    echo "Renaming $file to ${base}.jpg"
    mv "$file" "${base}.jpg"
end

echo "Done!"
```
*(Notice the `$file:r` trick—this is a built-in `mcsh` modifier that removes the file extension!)*

---
We hope this guide makes writing `mcsh` scripts enjoyable and clear!
