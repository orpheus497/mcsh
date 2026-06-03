# Built-in Commands

A complete reference of all shell builtins.

* `%`  *job*
A synonym for the fg builtin command.
* `%`  *job* `&`
A synonym for the bg builtin command.
* `:`
Does nothing, successfully.

* `@`
* `@` *name* `=` *expr*
* `@` *name*  `[`  *index*  `]` = *expr*
* `@` *name*  `++|--`
* `@` *name*  `[`  *index*  `]++|--`
The first form prints the values of all shell variables.

The second form assigns the value of expr to name .

The third form assigns the value of expr to the index'th component of name ; both name and its index'th component must already exist.

expr may contain the operators
* ,
+ , etc., as in C. If expr contains < , > , & , or | then at least that part of expr must be placed within ( and ) . Note that the syntax of expr has nothing to do with that described under Expressions .

The fourth and fifth forms increment ++ or decrement -- name or its index'th component.

The space between @ and name is required. The spaces between name and = and between = and expr are optional. Components of expr must be separated by spaces.
* `alias` [*name* *wordlist*]
Without arguments, prints all aliases.

With name , prints the alias for name.

With name and wordlist , assigns wordlist as the alias of name . wordlist is command and filename substituted.

name may not be alias or unalias . See also the unalias builtin command.
* `alloc`
Shows the amount of dynamic memory acquired, broken down into used and free memory. With an argument shows the number of free and used blocks in each size category. The categories start at size 8 and double at each step. This command's output may vary across system types, because systems other than the VAX may use a different memory allocator.
* `bg` [`%`  *job* ...]
Puts the specified jobs (or, without arguments, the current job) into the background, continuing each if it is stopped. job may be a number, a string, , % , + , or - as described under Jobs .

* `bindkey` [`-l`  |  `-d`  |  `-e`  |  `-v`  |  `-u` ] (+)
* `bindkey` [`-a` ] `-b` ] `-k` ] `-r` ] `--` ] *key* (+)
* `bindkey` [`-a` ] `-b` ] `-k` ] `-c`  |  `-s` ] `--` ] *key* command (+)
The first form either lists all bound keys and the editor command to which each is bound, lists a description of the commands, or binds all keys to a specific mode.

The second form lists the editor command to which key is bound.

The third form binds the editor command command to key .

Supported bindkey options:
* **tion**
bindkey description
* `-a`
Lists or changes key-bindings in the alternative key map. This is the key map used in vimode command mode.
* `-b`
key is interpreted as a control character written ^ character (e.g., ^A ) or C- character (e.g., C-A ) , a meta character written M- character (e.g., M-A ) , a function key written F- string (e.g., F-string ) , or an extended prefix key written X- character (e.g., X-A ) .
* `-c`
command is interpreted as a builtin or external command instead of an editor command.
* `-d`
Binds all keys to the standard bindings for the default editor, as per e and v .
* `-e`
Binds all keys to emacs(1) -style bindings. Unsets vimode .
* `-k`
key is interpreted as a symbolic arrow key name, which may be one of down , up , left , or right .
* `-l`
Lists all editor commands and a short description of each.
* `-r`
Removes key 's binding. Be careful: bindkey -r does not bind key to self-insert-command , it unbinds key completely.
* `-s`
command is taken as a literal string and treated as terminal input when key is typed. Bound keys in command are themselves reinterpreted, and this continues for ten levels of interpretation.
* `-u` (or any invalid option)
Prints a usage message.
* `-v`
Binds all keys to vi(1) -style bindings. Sets vimode .
* `--`
Forces a break from option processing, so the next word is taken as key even if it begins with - .

key may be a single character or a string. If a command is bound to a string, the first character of the string is bound to sequence-lead-in and the entire string is bound to the command.

Control characters in key can be literal (they can be typed by preceding them with the editor command quoted-insert , normally bound to ^V ) or written caret-character style, e.g., ^A . Delete is written ^? (caret-question mark). key and command can contain backslashed escape sequences (in the style of System V echo(1) ) as follows:
* **Escape**
Description
* `\a`
Bell.
* `\b`
Backspace.
* `\`
Escape.
* `\f`
Form feed.
* `\n`
Newline.
* `\r`
Carriage return.
* `\t`
Horizontal tab.
* `\v`
Vertical tab.
* `\`  *nnn*
The ASCII character corresponding to the octal number nnn .

\ nullifies the special meaning of the following character, if it has any, notably \ and ^ .
* `bs2cmd` *bs2000-command* (+)
Passes bs2000-command to the BS2000 command interpreter for execution. Only non-interactive commands can be executed, and it is not possible to execute any command that would overlay the image of the current process, like /EXECUTE or /CALL-PROCEDURE. (BS2000 only)
* `break`
Causes execution to resume after the end of the nearest enclosing foreach or while . The remaining commands on the current line are executed. Multi-level breaks are thus possible by writing them all on one line.
* `breaksw`
Causes a break from a switch , resuming after the endsw .
* `builtins` (+)
Prints the names of all builtin commands.
* `bye` (+)
A synonym for the logout builtin command. Available only if the shell was so compiled; see the version shell variable.
* `case` *label*  `:`
A label in a switch statement as discussed below.
* `cd` p l n | v - name If a directory name is given, changes the shell's working directory to name . If not, changes to home , unless the cdtohome variable is not set, in which case a name is required. If name is - it is interpreted as the previous working directory (see Other substitutions (+) ) . (+) If name is not a subdirectory of the current directory (and does not begin with / , ./ or ../ ) , each component of the variable cdpath is checked to see if it has a subdirectory name . Finally, if all else fails but name is a shell variable whose value begins with / or . , then this is tried to see if it is a directory, and the p option is implied.

With p , prints the final directory stack, just like dirs . The l , n , and v flags have the same effect on cd as on dirs , and they imply p (+). Using - forces a break from option processing so the next word is taken as the directory name even if it begins with - (+).

See also the implicitcd and cdtohome shell variables.
* `chdir`
A synonym for the cd builtin command.
* `complete` command off word / pattern / list [ : select ] / [ suffix / on ... (+) Without arguments, lists all completions.

With command , lists completions for command .

With command and word ..., defines completions.

command may be a full command name or a glob-pattern (see Filename substitution ) . It can begin with - to indicate that completion should be used only when command is ambiguous.

word specifies which word relative to the current word is to be completed, and may be one of the following:
* *word*
Completion word
* `c`
Current-word completion. pattern is a glob-pattern which must match the beginning of the current word on the command line. pattern is ignored when completing the current word.
* `C`
Like c , but includes pattern when completing the current word.
* `n`
Next-word completion. pattern is a glob-pattern which must match the beginning of the previous word on the command line.
* `N`
Like n , but must match the beginning of the word two before the current word.
* `p`
Position-dependent completion. pattern is a numeric range, with the same syntax used to index shell variables, which must include the current word.

list , the list of possible completions, may be one of the following:
* *list*
Completion item
* `a`
Aliases.
* `b`
Bindings (editor commands).
* `c`
Commands (builtin or external commands).
* `C`
External commands which begin with the supplied path prefix.
* `d`
Directories.
* `D`
Directories which begin with the supplied path prefix.
* `e`
Environment variables.
* `f`
Filenames.
* `F`
Filenames which begin with the supplied path prefix.
* `g`
Groupnames.
* `j`
Jobs.
* `l`
Limits.
* `n`
Nothing.
* `s`
Shell variables.
* `S`
Signals.
* `t`
Plain ( text ) files.
* `T`
Plain ( text ) files which begin with the supplied path prefix.
* `v`
Any variables.
* `u`
Usernames.
* `x`
Like n , but prints select when list-choices is used.
* `X`
Completions.
* `$`  *var*
Words from the variable var .
* `(...)`
Words from the given list.
* `\`...\``
Words from the output of command.

select is an optional glob-pattern. If given, words from only list that match select are considered and the fignore shell variable is ignored. The list types $ var , (...) , and \`...\` may not have a select pattern, and x uses select as an explanatory message when the list-choices editor command is used.

suffix is a single character to be appended to a successful completion. If null, no character is appended. If omitted (in which case the fourth delimiter can also be omitted), a slash is appended to directories and a space to other words.

command invoked from list \`...\` has the additional environment variable COMMAND_LINE set, which contains (as its name indicates) contents of the current (already typed in) command line. One can examine and use contents of the COMMAND_LINE environment variable in a custom script to build more sophisticated completions (see completion for svn 1 included in this package).

Now for some examples. Some commands take only directories as arguments, so there's no point completing plain files.
```
> complete cd 'p/1/d/'
```

completes only the first word following cd p/1 with a directory. p -type completion can also be used to narrow down command completion:
```
> co[^D]
complete compress
> complete -co* 'p/0/(compress)/'
> co[^D]
> compress
```

This completion completes commands (words in position 0, p/0 ) which begin with co (thus matching co* ) to compress (the only word in the list). The leading - indicates that this completion is to be used with only ambiguous commands.
```
> complete find 'n/-user/u/'
```

is an example of n -type completion. Any word following find and immediately following -user is completed from the list of users.
```
> complete cc 'c/-I/d/'
```

demonstrates c -type completion. Any word following cc and beginning with -I is completed as a directory. -I is not taken as part of the directory because we used lowercase c .

Different list s are useful with different commands.
```
> complete alias 'p/1/a/'
> complete man 'p/*/c/'
> complete set 'p/1/s/'
> complete true 'p/1/x:Truth has no options./'
```

These complete words following alias with aliases, man with commands, and set with shell variables. true doesn't have any options, so x does nothing when completion is attempted and prints Truth has no options. when completion choices are listed.

Note that the man example, and several other examples below, could just as well have used 'c/*' or 'n/*' as 'p/*' .

Words can be completed from a variable evaluated at completion time,
```
> complete ftp 'p/1/$hostnames/'
> set hostnames = (rtfm.mit.edu tesla.ee.cornell.edu)
> ftp [^D]
rtfm.mit.edu tesla.ee.cornell.edu
> ftp [^C]
> set hostnames = (rtfm.mit.edu tesla.ee.cornell.edu uunet.uu.net)
> ftp [^D]
rtfm.mit.edu tesla.ee.cornell.edu uunet.uu.net
```

or from a command run at completion time:
```
> complete kill 'p/*/\`ps | awk \{print\ \$1\}\`/'
> kill -9 [^D]
23113 23377 23380 23406 23429 23529 23530 PID
```

Note that the complete command does not itself quote its arguments, so the braces, space and $ in {print $1} must be quoted explicitly.

One command can have multiple completions:
```
> complete dbx 'p/2/(core)/' 'p/*/c/'
```

completes the second argument to dbx with the word core and all other arguments with commands. Note that the positional completion is specified before the next-word completion. Because completions are evaluated from left to right, if the next-word completion were specified first it would always match and the positional completion would never be executed. This is a common mistake when defining a completion.

The select pattern is useful when a command takes files with only particular forms as arguments. For example,
```
> complete cc 'p/*/f:*.[cao]/'
```

completes cc arguments to files ending in only .c , .a , or .o . select can also exclude files, using negation of a glob-pattern as described under Filename substitution . One might use
```
> complete rm 'p/*/f:^*.{c,h,cc,C,tex,1,man,l,y}/'
```

to exclude precious source code from rm completion. Of course, one could still type excluded names manually or override the completion mechanism using the complete-word-raw or list-choices-raw editor commands.

The C , D , F , and T list s are like c , d , f , and t respectively, but they use the select argument in a different way: to restrict completion to files beginning with a particular path prefix. For example, the Elm mail program uses = as an abbreviation for one's mail directory. One might use
```
> complete elm c@=@F:$HOME/Mail/@
```

to complete elm -f = as if it were elm -f ~/Mail/ Note that we used the separator @ instead of / to avoid confusion with the select argument, and we used $HOME instead of ~ because home directory substitution works at only the beginning of a word.

suffix is used to add a nonstandard suffix (not space or / for directories) to completed words.
```
> complete finger 'c/*@/$hostnames/' 'p/1/u/@'
```

completes arguments to finger from the list of users, appends an @ , and then completes after the @ from the hostnames variable. Note again the order in which the completions are specified.

Finally, here's a complex example for inspiration:
```
> complete find \
'n/-name/f/' 'n/-newer/f/' 'n/-{,n}cpio/f/' \
\'n/-exec/c/' 'n/-ok/c/' 'n/-user/u/' \
'n/-group/g/' 'n/-fstype/(nfs 4.2)/' \
'n/-type/(b c d f l p s)/' \
\'c/-/(name newer cpio ncpio exec ok user \
group fstype type atime ctime depth inum \
ls mtime nogroup nouser perm print prune \
size xdev)/' \
'p/*/d/'
```

This completes words following -name , -newer , -cpio , or -ncpio (note the pattern which matches both) to files, words following -exec or -ok to commands, words following -user and -group to users and groups respectively and words following -fstype or -type to members of the given lists. It also completes the switches themselves from the given list (note the use of c -type completion) and completes anything not otherwise completed to a directory. Whew.

Remember that programmed completions are ignored if the word being completed is a tilde substitution (beginning with ~ ) or a variable (beginning with $ ) . See also the uncomplete builtin command.
* `continue`
Continues execution of the nearest enclosing while or foreach . The rest of the commands on the current line are executed.
* `default:`
Labels the default case in a switch statement. It should come after all case labels.

* `dirs` l n | v
* `dirs` S | L filename (+)
* `dirs` c (+) The first form prints the directory stack. The top of the stack is at the left and the first directory in the stack is the current directory. With l , ~ or ~ name in the output is expanded explicitly to home or the pathname of the home directory for user name . (+) With n , entries are wrapped before they reach the edge of the screen. (+) With v , entries are printed one per line, preceded by their stack positions. (+) If more than one of n or v is given, v takes precedence. p is accepted but does nothing.

The second form with S saves the directory stack to filename as a series of cd and pushd commands. The second form with L sources filename , which is presumably a directory stack file saved by the S option or the savedirs mechanism. In either case, dirsfile is used if filename is not given and ~/.cshdirs is used if dirsfile is unset.

Note that login shells do the equivalent of dirs -L on startup and, if savedirs is set, dirs -S before exiting. Because only ~/.tcshrc is normally sourced before ~/.cshdirs , dirsfile should be set in ~/.tcshrc rather than ~/.login .

The third form clears the directory stack.
* `echo` [`-n` ] *word* ...
Writes each word to the shell's standard output, separated by spaces and terminated with a newline. The echo_style shell variable may be set to emulate (or not) the flags and escape sequences of the BSD and/or System V versions of echo(1) ; see Escape sequences (+) and echo(1) .
* `echotc` [`-sv` ] *arg* ... (+)
Exercises the terminal capabilities (see termcap(5) ) in arg . For example, echotc home sends the cursor to the home position, echotc cm 3 10 sends it to column 3 and row 10, and echotc ts 0; echo "This is a test."; echotc fs prints This is a test. in the status line.

If arg is baud , cols , lines , meta , or tabs , prints the value of that capability ( yes or no indicating that the terminal does or does not have that capability). One might use this to make the output from a shell script less verbose on slow terminals, or limit command output to the number of lines on the screen:
```
> set history=\`echotc lines\`
> @ history--
```

Termcap strings may contain wildcards which will not echo correctly. One should use double quotes when setting a shell variable to a terminal capability string, as in the following example that places the date in the status line:
```
> set tosl="\`echotc ts 0\`"
> set frsl="\`echotc fs\`"
> echo -n "$tosl";date; echo -n "$frsl"
```

With s , nonexistent capabilities return the empty string rather than causing an error. With v , messages are verbose.

* `else`
* `end`
* `endif`
* `endsw`
* `return`
See the description of the foreach , if , switch , while , and return statements below.
* `eval` *arg* ...
Treats the arguments as input to the shell and executes the resulting command(s) in the context of the current shell. This is usually used to execute commands generated as the result of command or variable substitution, because parsing occurs before these substitutions. See tset(1) for a sample use of eval .
* `exec` *command* ...
Executes the specified command in place of the current shell.
* `exit` [*expr*]
The shell exits either with the value of the specified expr (an expression, as described under Expressions ) or, without expr , with the value 0.
* `fg` [`%`  *job* ...]
Brings the specified jobs (or, without arguments, the current job) into the foreground, continuing each if it is stopped. job may be a number, a string, , % , + , or - as described under Jobs . See also the run-fg-editor editor command.
* `filetest` -  *op* file ... (+)
Applies op (which is a file inquiry operator as described under File inquiry operators ) to each file and returns the results as a space-separated list.

* `foreach` *name* `(`  *wordlist*  `)`
* `...`
* `end`
Successively sets the variable name to each member of wordlist and executes the sequence of commands between this command and the matching end . (Both foreach and end must appear alone on separate lines.)  The builtin command continue may be used to continue the loop prematurely and the builtin command break to terminate it prematurely. When this command is read from the terminal, the loop is read once prompting with foreach?\ (or prompt2 ) before any statements in the loop are executed. If you make a mistake typing in a loop at the terminal you can rub it out.
* `function` (+)
* `function` *name* (+)
* `...`
* `return`
* `function` *name* arg ... (+)
* *name* arg ... (+) The first form of the command prints the value of all shell functions.

The second form declares a function name . A declaration ends when a return is matched. (Both function and return must appear alone on separate lines.) May not be declared otherwise, and declared functions may not be redeclared or undeclared.

The third form calls a function name , optionally, preceded by arg , which is a list of arguments to be passed. Function calls may be nested or recursive, but too deep a nest or recursion will raise an error.

The fourth form is an alias for the third form.
* `getspath` (+)
Prints the system execution path. (TCF only)
* `getxvers` (+)
Prints the experimental version prefix. (TCF only)
* `glob` *word* ...
Like echo , but the n parameter is not recognized and words are delimited by null characters in the output. Useful for programs which wish to use the shell to filename expand a list of words.
* `goto` *word*
word is filename and command-substituted to yield a string of the form label . The shell rewinds its input as much as possible, searches for a line of the form label : possibly preceded by blanks or tabs, and continues execution after that line.
* `hashstat`
Prints a statistics line indicating how effective the internal hash table has been at locating commands (and avoiding exec 's). An exec is attempted for each component of the path where the hash function indicates a possible hit, and in each component which does not begin with a / .

On machines without vfork 2 , prints only the number and size of hash buckets.

* `history` hTr n
* `history` S | L | M filename (+)
* `history` c (+) The first form prints the history event list. If n is given only the n most recent events are printed or saved. With h , the history list is printed without leading numbers. If T is specified, timestamps are printed also in comment form. This can be used to produce files suitable for loading with history -L or source -h

With r , the order of printing is most recent first rather than oldest first.

The second form with S saves the history list to filename . If the first word of the savehist shell variable is set to a number, at most that many lines are saved. If the second word of savehist is set to merge , the history list is merged with the existing history file instead of replacing it (if there is one) and sorted by time stamp. (+) Merging is intended for an environment like the X Window System with several shells in simultaneous use. If the second word of savehist is merge and the third word is set to lock , the history file update will be serialized with other shell sessions that would possibly like to merge history at exactly the same time.

The second form with L appends filename (which is presumably a history list saved by the S option or the savehist mechanism) to the history list. M is like L , but the contents of filename are merged into the history list and sorted by timestamp. In either case, histfile is used if filename is not given and ~/.history is used if histfile is unset.

Note that history -L is exactly like source -h except that it does not require a filename.

Note that login shells do the equivalent of history -L on startup and, if savehist is set, history -S before exiting. Because only ~/.tcshrc is normally sourced before ~/.history , histfile should be set in ~/.tcshrc rather than ~/.login .

If histlit is set, the first and second forms print and save the literal (unexpanded) form of the history list.

The third form clears the history list.
* `hup` [*command* ] (+)
With command , runs command such that it will exit on a hangup signal and arranges for the shell to send it a hangup signal when the shell exits. Note that commands may set their own response to hangups, overriding hup . Without an argument, causes the non-interactive shell only to exit on a hangup for the remainder of the script. See also Signal handling and the nohup builtin command.
* `if` `(`  *expr*  `)` *command*
If expr (an expression, as described under Expressions ) evaluates true, then command is executed. Variable substitution on command happens early, at the same time it does for the rest of the if command. command must be a simple command, not an alias, a pipeline, a command list or a parenthesized command list, but it may have arguments. Input/output redirection occurs even if expr is false and command is thus not executed; this is a bug.

* `if` `(`  *expr*  `)` then
* `...`
* `else` if `(`  *expr2*  `)` then
* `...`
* `else`
* `...`
* `endif`
If the specified expr is true then the commands to the first else are executed; otherwise if expr2 is true then the commands to the second else are executed, etc. Any number of else if pairs are possible; only one endif is needed. The else part is likewise optional. (The words else and endif must appear at the beginning of input lines; the if must appear alone on its input line or after an else . )

* `inlib` *shared-library* ... (+)
Adds each shared-library to the current environment. There is no way to remove a shared library. (Domain/OS only)

* `jobs` [`-l`]
* `jobs` `-Z` [*title* ] (+)
The first form lists the active jobs. With l , lists process IDs in addition to the normal information. On TCF systems, prints the site on which each job is executing.

The second form with the Z option sets the process title to title using setproctitle 3 where available. If no title is provided, the process title will be cleared.

* `kill` `-l`
* `kill` s signal % job | pid ... The first form lists the signal names.

The second form sends the specified signal (or, if none is given, the TERM (terminate) signal) to the specified jobs or processes. job may be a number, a string, , % , + , or - as described under Jobs . Signals are either given by number or by name (as given in /usr/include/signal.h , stripped of the prefix SIG ) .

There is no default job ; entering just kill does not send a signal to the current job. If the signal being sent is TERM (terminate) or HUP (hangup), then the job or process is sent a CONT (continue) signal as well.
* `limit` [`-h` ] [*resource* *maximum-use*]
Limits the consumption by the current process and each process it creates to not individually exceed maximum-use on the specified resource .

If no maximum-use is given, then the current limit for resource is printed.

If no resource is given, then all limitations are given.

If the h flag is given, the hard limits are used instead of the current limits. The hard limits impose a ceiling on the values of the current limits. Only the super-user may raise the hard limits, but a user may lower or raise the current limits within the legal range.

Controllable resource types currently include (if supported by the OS):
* *resource*
Resource description
* `concurrency`
Maximum number of threads for this process.
* `coredumpsize`
Size of the largest core dump that will be created.
* `cputime`
Maximum number of cpu-seconds to be used by each process.
* `datasize`
Maximum growth of the data+stack region via sbrk 2 beyond the end of the program text.
* `descriptors` or `openfiles`
Maximum number of open files for this process.
* `filesize`
Largest single file which can be created.
* `heapsize`
Maximum amount of memory a process may allocate per brk 2 system call.
* `kqueues`
Maximum number of kqueues allocated for this process.
* `maxlocks`
Maximum number of locks for this user.
* `maxmessage`
Maximum number of bytes in POSIX mqueues for this user.
* `maxnice`
Maximum nice priority the user is allowed to raise mapped from [19...-20] to [0...39] for this user.
* `maxproc`
Maximum number of simultaneous processes for this user id.
* `maxrtprio`
Maximum realtime priority for this user.
* `maxrttime`
Timeout for RT tasks in microseconds for this user.
* `maxsignal`
Maximum number of pending signals for this user.
* `maxthread`
Maximum number of simultaneous threads (lightweight processes) for this user id.
* `memorylocked`
Maximum size which a process may lock into memory using mlock 2 .
* `memoryuse`
Maximum amount of physical memory a process may have allocated to it at a given time.
* `posixlocks`
Maximum number of POSIX advisory locks for this user.
* `pseudoterminals`
Maximum number of pseudo-terminals for this user.
* `sbsize`
Maximum size of socket buffer usage for this user.
* `stacksize`
Maximum size of the automatically-extended stack region.
* `swapsize`
Maximum amount of swap space reserved or used for this user.
* `threads`
Maximum number of threads for this process.
* `vmemoryuse`
Maximum amount of virtual memory a process may have allocated to it at a given time (address space).

maximum-use may be given as a (floating point or integer) number followed by a scale factor. For all limits other than cputime the default scale is k or kilobytes (1024 bytes); a scale factor of m or megabytes (1048576 bytes) or g or gigabytes (1073741824 bytes) may also be used. For cputime the default scaling is seconds , while m for minutes or h for hours, or a time of the form mm : ss giving minutes and seconds may be used.

If maximum-use is unlimited , then the limitation on the specified resource is removed (this is equivalent to the unlimit builtin command).

For both resource names and scale factors, unambiguous prefixes of the names suffice.
* `log` (+)
Prints the watch shell variable and reports on each user indicated in watch who is logged in, regardless of when they last logged in. See also watchlog .
* `login`
Terminates a login shell, replacing it with an instance of /bin/login . This is one way to log off, included for compatibility with sh(1) .
* `logout`
Terminates a login shell. Especially useful if ignoreeof is set.
* `ls-F` switch ... file ... (+) Lists files like ls -F but much faster.

ls-F identifies each type of special file in the listing with a special character suffix:

* **Suffix**
Special file type

* `/`
Directory.
* `*`
Executable.
* `#`
Block device.
* `%`
Character device.
* `|`
Named pipe (systems with named pipes only).
* `=`
Socket (systems with sockets only).
* `@`
Symbolic link (systems with symbolic links only).
* `+`
Hidden directory (AIX only) or context dependent (HP/UX only).
* `:`
Network special (HP/UX only).

If the listlinks shell variable is set, symbolic links are identified in more detail (on only systems that have them, of course):

* **Suffix**
Symbolic link type

* `@`
Symbolic link to a non-directory.
* `>`
Symbolic link to a directory.
* `&`
Orphaned (broken) symbolic link.

listlinks also slows down ls-F and causes partitions holding files pointed to by symbolic links to be mounted.

If the listflags shell variable is set to x , a , or A , or any combination thereof (e.g., xA ) , they are used as flags to ls-F , making it act like
```
ls -xF
ls -Fa
ls -FA
```

or a combination, for example ls -FxA

On machines where ls -C is not the default, ls-F acts like ls -CF unless listflags contains an x , in which case it acts like ls -xF

ls-F passes its arguments to ls(1) if it is given any switches, so alias ls ls-F generally does the right thing.

The ls-F builtin can list files using different colors depending on the file type or extension. See the color shell variable and the CLICOLOR_FORCE , LSCOLORS , and LS_COLORS environment variables.

* `migrate` site pid | % jobid ... (+)
* `migrate` `-` site (+)
The first form migrates the process or job to the site specified or the default site determined by the system path. (TCF only)

The second form is equivalent to migrate - site $$ in that it migrates the current process to the specified site. Migrating the shell itself can cause unexpected behavior, because the shell does not like to lose its tty. (TCF only)
* `newgrp` [`-` ] *group* ] (+)
Equivalent to exec newgrp as per newgrp(1) . Available only if the shell was so compiled; see the version shell variable.
* `nice` [`+`  *number* ] [*command*]
Increments the scheduling priority for the shell by number , or, without number , by 4. With command , runs command at the appropriate priority. The greater the number , the less cpu the process gets. The super-user may decrement the priority by using nice - number ...

command is always executed in a sub-shell, and the restrictions placed on commands in simple if statements apply.
* `nohup` [*command*]
With command , runs command such that it will ignore hangup signals. Note that commands may set their own response to hangups, overriding nohup .

Without an argument, causes the non-interactive shell only to ignore hangups for the remainder of the script. See also Signal handling and the hup builtin command.
* `notify` [`%`  *job* ...]
Causes the shell to notify the user asynchronously when the status of any of the specified jobs (or, without % job , the current job) changes, instead of waiting until the next prompt as is usual. job may be a number, a string, , % , + , or - as described under Jobs . See also the notify shell variable.
* `onintr` [`-`  |  *label*]
Controls the action of the shell on interrupts. Without arguments, restores the default action of the shell on interrupts, which is to terminate shell scripts or to return to the terminal command input level.

With - , causes all interrupts to be ignored.

With label , causes the shell to execute a goto label when an interrupt is received or a child process terminates because it was interrupted.

onintr is ignored if the shell is running detached and in system startup files (see FILES ) , where interrupts are disabled anyway.
* `popd` p l n | v + n Without arguments, pops the directory stack and returns to the new top directory.

With a number + n , discards the n th entry in the stack.

Finally, all forms of popd print the final directory stack, just like dirs . The pushdsilent shell variable can be set to prevent this and the p flag can be given to override pushdsilent . The l , n , and v flags have the same effect on popd as on dirs . (+)
* `printenv` [*name* ] (+)
Prints the names and values of all environment variables or, with name , the value of the environment variable name .
* `pushd` p l n | v name | + n Without arguments, exchanges the top two elements of the directory stack. If pushdtohome is set, pushd without arguments acts as pushd ~ like cd . (+)

With name , pushes the current working directory onto the directory stack and changes to name . If name is - it is interpreted as the previous working directory (see Filename substitution ) . (+) If dunique is set, pushd removes any instances of name from the stack before pushing it onto the stack. (+)

With a number + n , rotates the n th element of the directory stack around to be the top element and changes to it. If dextract is set, however, pushd + n extracts the n th directory, pushes it onto the top of the stack and changes to it. (+)

Finally, all forms of pushd print the final directory stack, just like dirs . The pushdsilent shell variable can be set to prevent this and the p flag can be given to override pushdsilent . The l , n , and v flags have the same effect on pushd as on dirs . (+)
* `rehash`
Causes the internal hash table of the contents of the directories in the path variable to be recomputed. This is needed if the autorehash shell variable is not set and new commands are added to directories in path while you are logged in. With autorehash , a new command will be found automatically, except in the special case where another command of the same name which is located in a different directory already exists in the hash table. Also flushes the cache of home directories built by tilde expansion.
* `repeat` *count* *command*
The specified command , which is subject to the same restrictions as the command in the one line if statement above, is executed count times. I/O redirections occur exactly once, even if count is 0.
* `rootnode` `//`  *nodename* (+)
Changes the rootnode to // nodename , so that / will be interpreted as // nodename . (Domain/OS only)

* `sched` (+)
* `sched` + hh : mm command (+)
* `sched` `-`  *n* (+)
The first form prints the scheduled-event list. The sched shell variable may be set to define the format in which the scheduled-event list is printed.

The second form adds command to the scheduled-event list. For example,
```
> sched 11:00 echo It\'s eleven o\'clock.
```

causes the shell to echo It's eleven o'clock. at 11 AM.

The time may be in 12-hour AM/PM format
```
> sched 5pm set prompt='[%h] It\'s after 5; go home: >'
```

or may be relative to the current time:
```
> sched +2:15 /usr/lib/uucp/uucico -r1 -sother
```

A relative time specification may not use AM/PM format.

The third form removes item n from the event list:
```
> sched
1  Wed Apr  4 15:42  /usr/lib/uucp/uucico -r1 -sother
2  Wed Apr  4 17:00  set prompt=[%h] It's after 5; go home: >
> sched -2
> sched
1  Wed Apr  4 15:42  /usr/lib/uucp/uucico -r1 -sother
```

A command in the scheduled-event list is executed just before the first prompt is printed after the time when the command is scheduled. It is possible to miss the exact time when the command is to be run, but an overdue command will execute at the next prompt. A command which comes due while the shell is waiting for user input is executed immediately. However, normal operation of an already-running command will not be interrupted so that a scheduled-event list element may be run.

This mechanism is similar to, but not the same as, the at(1) command on some Unix systems. Its major disadvantage is that it may not run a command at exactly the specified time. Its major advantage is that because sched runs directly from the shell, it has access to shell variables and other structures. This provides a mechanism for changing one's working environment based on the time of day.

* `set`
* `set` *name* ...
* `set` *name*  `=`  *word* ...
* `set` r f | l name =( wordlist ) ... (+)
* `set` *name*  `[`  *index*  `]=`  *word* ...
* `set` `-r` (+)
* `set` `-r` *name* ... (+)
* `set` `-r` *name*  `=`  *word* ... (+)
The first form of the command prints the value of all shell variables. Variables which contain more than a single word print as a parenthesized word list.

The second form sets name to the null string.

The third form sets name to the single word .

The fourth form sets name to the list of words in wordlist .

In all cases the value is command and filename expanded. If r is specified, the value is set read-only. If f or l are specified, set only unique words keeping their order. f prefers the first occurrence of a word, and l the last.

The fifth form sets the index'th component of name to word ; this component must already exist.

The sixth form lists only the names of all shell variables that are read-only.

The seventh form makes name read-only, whether or not it has a value.

The eighth form is the same as the third form, but make name read-only at the same time.

These arguments can be repeated to set and/or make read-only multiple variables in a single set command. Note, however, that variable expansion happens for all arguments before any setting occurs. Note also that = can be adjacent to both name and word or separated from both by whitespace, but cannot be adjacent to only one or the other. See also the unset builtin command.
* `setenv` [*name* *value*]
Without arguments, prints the names and values of all environment variables.

With name , sets the environment variable name to value or, without value , to the null string.
* `setpath` *path* (+)
Equivalent to setpath(1) . (Mach only)
* `setspath` `LOCAL`  |  *site*  |  *cpu* ... (+)
Sets the system execution path. (TCF only)
* `settc` *cap* value (+)
Tells the shell to believe that the terminal capability cap (as defined in termcap(5) ) has the value value . No sanity checking is done. Concept terminal users may have to settc xn no to get proper wrapping at the rightmost column.
* `setty` d | q | x a [ + | - ] mode (+) Controls which tty modes (see Terminal management (+) ) the shell does not allow to change. d , q , or x tells setty to act on the edit , quote , or execute set of tty modes respectively; without d , q , or x , execute is used.

Without other arguments, setty lists the modes in the chosen set which are fixed on + mode or off - mode . The available modes, and thus the display, vary from system to system. With a , lists all tty modes in the chosen set whether or not they are fixed. With + mode , or mode , fixes mode on or off or removes control from mode in the chosen set. For example, setty +echok echoe fixes echok mode on and allows commands to turn echoe mode on or off, both when the shell is executing commands.
* `setxvers` [*string* ] (+)
Set the experimental version prefix to string , or removes it if string is omitted. (TCF only)
* `shift` [*variable*]
Without arguments, discards argv [1] and shifts the members of argv to the left. It is an error for argv not to be set or to have fewer than one word as value.

With variable , performs the same function on variable .
* `source` [`-h` ] *name* [*args* ...]
The shell reads and executes commands from name . The commands are not placed on the history list. If any args are given, they are placed in argv . (+) source commands may be nested; if they are nested too deeply the shell may run out of file descriptors. An error in a source at any level terminates all nested source commands.

With h , commands are placed on the history list instead of being executed, much like history -L
* `stop` %  *job*  |  *pid* ...
Stops the specified jobs or processes which are executing in the background. job may be a number, a string, , % , + , or - as described under Jobs .

There is no default job ; entering just stop does not stop the current job.
* `suspend`
Causes the shell to stop in its tracks, much as if it had been sent a stop signal with ^Z . This is most often used to stop shells started by su(1) .

* `switch` `(`  *string*  `)`
* `case` *str1*  :
* `\` \ \ \ ...
* `\` \ \ \ breaksw
* `...`
* `default:`
* `\` \ \ \ ...
* `\` \ \ \ breaksw
* `endsw`
Each case label is successively matched, against the specified string which is first command and filename expanded. The file metacharacters
* ,
? , and [...] may be used in the case labels, which are variable expanded. If none of the labels match before a default label is found, then the execution begins after the default label. Each case label and the default label must appear at the beginning of a line. The command breaksw causes execution to continue after the endsw . Otherwise control may fall through case labels and default labels as in C. If no label matches and there is no default, execution continues after the endsw .
* `telltc` (+)
Lists the values of all terminal capabilities (see termcap(5) ) .
* `termname` [*termtype* ] (+)
Tests if termtype (or the current value of TERM if no termtype is given) has an entry in the hosts termcap(5) or terminfo(5) database. Prints the terminal type to stdout and returns 0 if an entry is present otherwise returns 1.
* `time` [*command*]
Executes command (which must be a simple command, not an alias, a pipeline, a command list or a parenthesized command list) and prints a time summary as described under the time variable. If necessary, an extra shell is created to print the time statistic when the command completes.

Without command , prints a time summary for the current shell and its children.
* `umask` [*value*]
Sets the file creation mask to value , which is given in octal. Common values for the mask are 002, giving all access to the group and read and execute access to others, and 022, giving read and execute access to the group and others.

Without value , prints the current file creation mask.
* `unalias` *pattern*
Removes all aliases whose names match pattern . Thus unalias * removes all aliases. It is not an error for nothing to be unalias ed.
* `uncomplete` *pattern* (+)
Removes all completions whose names match pattern . Thus uncomplete * removes all completions. It is not an error for nothing to be uncomplete d.
* `unhash`
Disables use of the internal hash table to speed location of executed programs.
* `universe` *universe* (+)
Sets the universe to universe . (Masscomp/RTU only)
* `unlimit` [`-hf` ] [*resource*]
Removes the limitation on resource or, if no resource is specified, all resource limitations.

With h , the corresponding hard limits are removed. Only the super-user may do this.

Note that unlimit may not exit successful, since most systems do not allow descriptors to be unlimited.

With f errors are ignored.
* `unset` *pattern*
Removes all variables whose names match pattern , unless they are read-only. Thus unset * removes all variables unless they are read-only; this is a bad idea.

It is not an error for nothing to be unset .
* `unsetenv` *pattern*
Removes all environment variables whose names match pattern . Thus unsetenv * removes all environment variables; this is a bad idea.

It is not an error for nothing to be unsetenv ed.
* `ver` [*systype* *command* ] ] (+)
Without arguments, prints SYSTYPE .

With systype , sets SYSTYPE to systype .

With systype and command , executes command under systype . systype may be bsd4.3 or sys5.3 .

(Domain/OS only)
* `wait`
The shell waits for all background jobs. If the shell is interactive, an interrupt will disrupt the wait and cause the shell to print the names and job numbers of all outstanding jobs.
* `warp` *universe* (+)
Sets the universe to universe . (Convex/OS only)
* `watchlog` (+)
An alternate name for the log builtin command. Available only if the shell was so compiled; see the version shell variable.
* `where` *command* (+)
Reports all known instances of command , including aliases, builtins and executables in path .
* `which` *command* (+)
Displays the command that will be executed by the shell after substitutions, path searching, etc. The builtin command is just like which(1) , but it correctly reports aliases and builtins and is 10 to 100 times faster. See also the which-command editor command.

* `while` `(`  *expr*  `)`
* `...`
* `end`
Executes the commands between the while and the matching end while expr (an expression, as described under Expressions ) evaluates non-zero. while and end must appear alone on their input lines. break and continue may be used to terminate or continue the loop prematurely. If the input is a terminal, the user is prompted the first time through the loop as with foreach .
