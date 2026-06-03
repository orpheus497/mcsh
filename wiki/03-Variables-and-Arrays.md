# 03 - Variables and Arrays

In C Shell, managing data is split into two distinct concepts: standard string variables (assigned with `set`) and numeric variables meant for arithmetic (assigned with `@`).

## 1. String Variables (`set`)

You create and assign values to variables using the `set` command. Unlike POSIX shells (`bash`), you **can and should** use spaces around the equals sign for readability.

```csh
set username = "Alice"
set greeting = "Hello there"
```

To access the value stored in a variable, prepend its name with a dollar sign `$`.

```csh
echo "$greeting, $username!"
# Prints: Hello there, Alice!
```

*(Best Practice: Always wrap your variables in double quotes when using them, e.g., `"$username"`, to prevent issues if the string contains spaces).*

## 2. Numeric Variables and Arithmetic (`@`)

If you want to perform math, you must use the `@` command instead of `set`. The `@` command evaluates expressions.

```csh
@ count = 10
@ total = $count + 5

echo "The total is $total"
```

The `@` command also supports C-style increment, decrement, and compound assignment operators:
```csh
@ total++  # Add 1
@ count--  # Subtract 1
@ total += 5  # Add 5 to total
```

## 3. Arrays (Word Lists)

In C Shell terminology, arrays are called "Word Lists". You define an array by enclosing a space-separated list of items in parentheses.

```csh
set colors = (red green blue yellow)
```

### Accessing Array Elements
**Crucial Note:** C Shell array indexing starts at **1**, not 0!

To access a specific element, use square brackets `[ ]`:
```csh
echo $colors[1]  # Prints: red
echo $colors[3]  # Prints: blue
```

### Array Length
To find out how many items are in an array, use the `$#` syntax:
```csh
echo "There are $#colors colors in the list."
# Prints: There are 4 colors in the list.
```

### Slicing Arrays
You can extract a range of items from an array using a hyphen `-`:
```csh
echo $colors[2-4]
# Prints: green blue yellow
```

## 4. Reading User Input (`$<`)

If you want your script to pause and wait for the user to type something, you use a special built-in variable: `$<`. This reads a single line of input from the terminal.

```csh
#!/bin/csh -f

echo -n "Please enter your name: "  # -n prevents the newline
set name = $<

echo "Welcome to the C Shell, $name!"
```

---
**Next up:** [04 - Control Flow](04-Control-Flow.md)