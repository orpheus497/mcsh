# 06 - Advanced Features

Once you understand the basics of variables and control flow, you can start leveraging the true power of the shell: combining commands.

## 1. Pipelines

Pipelines allow you to take the output of one command and feed it directly into the input of another command. You do this using the pipe character `|`.

```csh
# List all files, find lines containing ".txt", and count them
ls -l | grep "\.txt" | wc -l
```

### Redirecting Errors in Pipelines
By default, the `|` character only passes the "standard output" (stdout) down the pipeline. If the first command produces an error (stderr), it will print to the screen, not go into the pipe.

If you want to pipe *both* standard output and errors, use `|&`:
```csh
# If my_program crashes, grep will still be able to search the error message
./my_program |& grep "Fatal"
```

## 2. Command Substitution (Backticks)

Often, you want to run a command and save its output into a variable. You do this using backticks `` ` ``. The shell runs the command inside the backticks, takes whatever it printed, and substitutes it into your script.

```csh
# Save the current date to a variable
set current_date = `date "+%Y-%m-%d"`
echo "Today is $current_date"

# Find out who is logged in and save it as an array (word list)
set logged_in_users = ( `whoami` )
```

## 3. Redirecting Output to Files

You can save the output of a command to a file using `>` and `>>`.

*  `>` : Overwrites the file.
*  `>>` : Appends to the file.

```csh
echo "Log started" > log.txt  # Creates or overwrites log.txt
echo "Process running" >> log.txt # Adds to the bottom of log.txt
```

### Redirecting Errors to Files
Just like pipelines, standard error (stderr) is not redirected by `>` by default.

*  To redirect *both* standard output and standard error into a file, use `>&`.
*  To append *both*, use `>>&`.

```csh
# Save output and errors to the same log file
./my_script.csh >& output_and_errors.log
```

## 4. Job Control (Running in the Background)

If you have a command that takes a long time to run, you can tell the shell to run it in the background by appending an ampersand `&` to the end of the command. The script will immediately move on to the next line without waiting.

```csh
echo "Starting large backup..."
tar -czf backup.tar.gz /var/www &
echo "Backup is running in the background. Script continuing..."
```

---
**Next up:** [07 - POSIX vs. C Shell Quick Reference](07-POSIX-vs-CShell.md)