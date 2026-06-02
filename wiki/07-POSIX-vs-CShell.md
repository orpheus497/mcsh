# 07 - POSIX vs. C Shell Quick Reference

If you are coming from a `bash` or `sh` background, use this cheat sheet to quickly translate your knowledge into C Shell syntax.

## 1. Variables and Math

| Feature | POSIX (`bash` / `sh`) | C Shell (`csh` / `tcsh` / `mcsh`) |
| :--- | :--- | :--- |
| **String Assignment** | `name="John"` | `set name = "John"` |
| **Math Assignment** | `count=5` | `@ count = 5` |
| **Increment** | `((count++))` | `@ count++` |
| **Command Substitution** | `date=$(date)` | `set date = \`date\`` |

## 2. Arrays

| Feature | POSIX (`bash` / `sh`) | C Shell (`csh` / `tcsh` / `mcsh`) |
| :--- | :--- | :--- |
| **Creation** | `arr=("a" "b")` | `set arr = (a b)` |
| **Index Start** | **`0`** | **`1`** |
| **Access Item** | `echo ${arr[0]}` | `echo $arr[1]` |
| **Length of Array** | `echo ${#arr[@]}` | `echo $#arr` |

## 3. Control Flow

| Feature | POSIX (`bash` / `sh`) | C Shell (`csh` / `tcsh` / `mcsh`) |
| :--- | :--- | :--- |
| **If Statement** | `if [ "$a" == "b" ]; then`<br>`echo "yes"`<br>`fi` | `if ( "$a" == "b" ) then`<br>`echo "yes"`<br>`endif` |
| **Else If** | `elif [ "$a" == "c" ]; then` | `else if ( "$a" == "c" ) then` |
| **While Loop** | `while [ "$i" -lt 5 ]; do`<br>`...`<br>`done` | `while ( $i < 5 )`<br>`...`<br>`end` |
| **For Each Loop** | `for x in a b; do`<br>`...`<br>`done` | `foreach x ( a b )`<br>`...`<br>`end` |
| **Switch / Case** | `case "$var" in`<br>`pattern)`<br>`...;;`<br>`esac` | `switch ( "$var" )`<br>`case pattern:`<br>`...`<br>`breaksw`<br>`endsw` |

## 4. Input and Output

| Feature | POSIX (`bash` / `sh`) | C Shell (`csh` / `tcsh` / `mcsh`) |
| :--- | :--- | :--- |
| **Read user input** | `read var` | `set var = $<` |
| **Redirect stdout & stderr** | `cmd > file.log 2>&1` | `cmd >& file.log` |
| **Pipe stdout & stderr** | `cmd 2>&1 \| grep ...` | `cmd \|& grep ...` |

## 5. File Tests

| Feature | POSIX (`bash` / `sh`) | C Shell (`csh` / `tcsh` / `mcsh`) |
| :--- | :--- | :--- |
| **File Exists?** | `if [ -e "$f" ]` | `if ( -e "$f" )` |
| **Is Directory?** | `if [ -d "$d" ]` | `if ( -d "$d" )` |

---
**Congratulations!** You've reached the end of the C Shell scripting guide. You are now equipped to write, read, and maintain scripts written in `csh`, `tcsh`, or `mcsh`.