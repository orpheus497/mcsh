# 05 - Operators and File Tests

C Shell provides a rich set of operators for comparing strings, performing math, evaluating logic, and inspecting files on the filesystem.

## 1. Comparison and Logical Operators

When using `if` or `while` statements, you will often need to compare variables. C Shell uses operators identical to the C programming language.

### Equality and Relational Operators
*   `==` : Equal to
*   `!=` : Not equal to
*   `=~` : String matches pattern (e.g., `"apple" =~ "a*"`)
*   `!~` : String does not match pattern
*   `<`  : Less than
*   `>`  : Greater than
*   `<=` : Less than or equal to
*   `>=` : Greater than or equal to

### Logical Operators
You can combine multiple conditions using logical operators:
*   `&&` : Logical AND (both must be true)
*   `||` : Logical OR (either must be true)
*   `!`  : Logical NOT (inverts the condition)

```csh
set age = 25
set user = "admin"

if ( $age > 18 && "$user" == "admin" ) then
    echo "Access granted."
endif
```

## 2. Arithmetic and Bitwise Operators

When working with the `@` command, you have access to a full suite of mathematical operators:

*   **Math:** `+` (add), `-` (subtract), `*` (multiply), `/` (divide), `%` (modulo/remainder)
*   **Bitwise:** `&` (AND), `|` (OR), `^` (XOR), `~` (NOT), `<<` (Left shift), `>>` (Right shift)

```csh
@ result = ( 10 * 2 ) + ( 15 / 3 )
echo $result  # Prints: 25
```

## 3. File Enquiry Operators

One of the most common tasks in a shell script is checking if a file exists, if it is a directory, or if you have permission to read it. C Shell provides built-in operators for this.

The syntax is `-X filename`, where `X` is the test you want to perform. It returns `1` (true) if the condition is met, and `0` (false) otherwise.

### Common File Tests:
*   `-e` : Does the file/directory **e**xist?
*   `-f` : Is it a plain **f**ile?
*   `-d` : Is it a **d**irectory?
*   `-r` : Is it **r**eadable by the current user?
*   `-w` : Is it **w**ritable by the current user?
*   `-x` : Is it e**x**ecutable by the current user?
*   `-z` : Is the file empty (has **z**ero size)?

**Example:**
```csh
set target = "/tmp/config.txt"

if ( -e "$target" ) then
    if ( -d "$target" ) then
        echo "Error: $target is a directory, not a file."
    else if ( -r "$target" ) then
        echo "File exists and is readable!"
        cat "$target"
    else
        echo "File exists but I do not have permission to read it."
    endif
else
    echo "File does not exist!"
endif
```

---
**Next up:** [06 - Advanced Features](06-Advanced-Features.md)