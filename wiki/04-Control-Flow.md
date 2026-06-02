# 04 - Control Flow

Control flow allows your script to make decisions and repeat actions. C Shell's control flow syntax is designed to look very similar to the C programming language.

## 1. Conditional Statements (`if` / `else if` / `else`)

An `if` statement checks a condition. If the condition is true, it executes a block of code.

**Important Syntax Rules:**
* The condition must be wrapped in parentheses `( )`.
* The `then` keyword must appear on the same line as the `if`.
* The `else` keyword must be on a line by itself.
* The block must be closed with `endif`.

```csh
set status_code = 200

if ( $status_code == 200 ) then
    echo "Success!"
else if ( $status_code == 404 ) then
    echo "Not Found."
else
    echo "Unknown Error."
endif
```

*(Note: Unlike POSIX bash, C Shell uses standard `==` and `!=` for comparing strings, which is much more intuitive!)*

### The One-Line `if`
If you only need to execute a single command, you can omit the `then` and `endif` entirely.
```csh
if ( $status_code == 500 ) echo "Server Crash!"
```

## 2. Switch Statements (`switch` / `case`)

When you need to compare a single variable against many possible string values or patterns, a `switch` statement is cleaner than a long chain of `else if` statements.

*   You match patterns using `case`.
*   You must end each block with `breaksw` (break switch) to prevent it from continuing into the next case.
*   Use `default:` as a catch-all if no cases match.

```csh
set command = "start"

switch ( "$command" )
    case "start":
        echo "Starting the server..."
        breaksw
    case "stop":
        echo "Stopping the server..."
        breaksw
    case "re*":
        # This matches anything starting with 're' (like restart, reload)
        echo "Restarting..."
        breaksw
    default:
        echo "Unknown command: $command"
        breaksw
endsw
```

## 3. Looping: `foreach`

The `foreach` loop is used to iterate over a list of items (like an array). It assigns each item in the list to a variable, one by one, and runs the block of code.

```csh
set animals = (cat dog bird)

foreach pet ( $animals )
    echo "I have a $pet"
end
```
*(Notice that the block is closed with `end`, not `endforeach`!)*

You can also provide the list directly without creating a variable first:
```csh
foreach file ( *.txt )
    echo "Found text file: $file"
end
```

## 4. Looping: `while`

A `while` loop continues to execute a block of code as long as a condition remains true.

```csh
@ counter = 1

while ( $counter <= 5 )
    echo "Count: $counter"
    @ counter++
end
```

---
**Next up:** [05 - Operators and File Tests](05-Operators-and-File-Tests.md)