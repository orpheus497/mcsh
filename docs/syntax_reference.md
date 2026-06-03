# Syntax Reference

A deep dive into the syntax rules and substitutions supported by mcsh.

## Lexical Structure

The shell splits input lines into words at blanks and tabs. The special characters & , | , ; , < , > , ( , and ) , and the doubled characters && , || , << , and >> are always separate words, whether or not they are surrounded by whitespace.

When the shell's input is not a terminal, the character
#
is taken to begin a comment. Each
#
and the rest of the input line on which it appears is discarded before further parsing.

A special character (including a blank or tab) may be prevented from having its special meaning, and possibly made part of another word, by preceding it with a backslash \ or enclosing it in single ' , double " , or backward ` quotes. When not otherwise quoted a newline preceded by a \ is equivalent to a blank, but inside quotes this sequence results in a newline.

Furthermore, all Substitutions except History substitution can be prevented by enclosing the strings (or parts of strings) in which they appear with single quotes or by quoting the crucial character(s) (e.g., $ or ` for Variable substitution or Command substitution respectively) with \ . ( Alias substitution is no exception: quoting in any way any character of a word for which an alias has been defined prevents substitution of the alias. The usual way of quoting an alias is to precede it with a backslash.) History substitution is prevented by backslashes but not by single quotes. Strings quoted with double or backward quotes undergo Variable substitution and Command substitution , but other substitutions are prevented.

Text inside single or double quotes becomes a single word (or part of one). Metacharacters in these strings, including blanks and tabs, do not form separate words. Only in one special case (see Command substitution ) can a double-quoted string yield parts of more than one word; single-quoted strings never do. Backward quotes are special: they signal Command substitution , which may result in more than one word.

C-style escape sequences can be used in single quoted strings by preceding the leading quote with $ . (+) See Escape sequences (+) for a complete list of recognized escape sequences.

Quoting complex strings, particularly strings which themselves contain quoting characters, can be confusing. Remember that quotes need not be used as they are in human writing! It may be easier to quote not an entire string, but only those parts of the string which need quoting, using different types of quoting to do so if appropriate.

The backslash_quote shell variable can be set to make backslashes always quote \ , ' , and " (+). This may make complex quoting tasks easier, but it can cause syntax errors in csh(1) scripts.

## Substitutions

We now describe the various transformations the shell performs on the input in the order in which they occur. We note in passing the data structures involved and the commands and variables which affect them. Remember that substitutions can be prevented by quoting as described under Lexical structure .

## Command, filename and directory stack substitution

The remaining substitutions are applied selectively to the arguments of builtin commands. This means that portions of expressions which are not evaluated are not subjected to these expansions. For commands which are not internal to the shell, the command name is substituted separately from the argument list. This occurs very late, after input-output redirection is performed, and in a child of the main shell.

## Command substitution

Command substitution is indicated by a command enclosed in ` . The output from such a command is broken into separate words at blanks, tabs and newlines, and null words are discarded. The output is variable and command substituted and put in place of the original string.

Command substitutions inside double quotes " retain blanks and tabs; only newlines force new words. The single final newline does not force a new word in any case. It is thus possible for a command substitution to yield only part of a word, even if the command outputs a complete line.

By default, the shell since version 6.12 replaces all newline and carriage return characters in the command by spaces. If this is switched off by unsetting csubstnonl , newlines separate commands as usual.

## Filename substitution

If a word contains any of the characters
* ,
? , [ , or { or begins with the character ~ it is a candidate for filename substitution, also known as globbing . This word is then regarded as a pattern ( glob-pattern ) , and replaced with an alphabetically sorted list of file names which match the pattern.

In matching filenames, the character . at the beginning of a filename or immediately following a / , as well as the character / must be matched explicitly (unless either globdot or globstar or both are set (+)). The character * matches any string of characters, including the null string. The character ? matches any single character. The sequence [...] matches any one of the characters enclosed. Within [...] , a pair of characters separated by - matches any character lexically between the two.

(+) Some glob-patterns can be negated: The sequence [^...] matches any single character not specified by the characters and/or ranges of characters in the braces.

An entire glob-pattern can also be negated with ^ :
```
> echo *
bang crash crunch ouch
> echo ^cr*
bang ouch
```

Glob-patterns which do not use ? ,
* ,
or [] , or which use {} or ~ (below) are not negated correctly.

The metanotation a{b,c,d}e is a shorthand for abe ace ade . Left-to-right order is preserved: /usr/source/s1/{oldls,ls}.c expands to /usr/source/s1/oldls.c /usr/source/s1/ls.c The results of matches are sorted separately at a low level to preserve this order: ../{memo,*box} might expand to ../memo ../box ../mbox (Note that memo was not sorted with the results of matching *box . ) It is not an error when this construct expands to files which do not exist, but it is possible to get an error from a command to which the expanded list is passed. This construct may be nested. As a special case the words { , } , and {} are passed undisturbed.

The character ~ at the beginning of a filename refers to home directories. Standing alone, i.e., ~ , it expands to the invoker's home directory as reflected in the value of the home shell variable. When followed by a name consisting of letters, digits and - characters the shell searches for a user with that name and substitutes their home directory; thus ~ken might expand to /usr/ken and ~ken/chmach might expand to /usr/ken/chmach If the character ~ is followed by a character other than a letter or / or appears elsewhere than at the beginning of a word, it is left undisturbed. A command like setenv MANPATH /usr/share/man:/usr/local/share/man:~/lib/man does not, therefore, do home directory substitution as one might hope.

It is an error for a glob-pattern containing
* ,
? , [ , or ~ , with or without ^ , not to match any files. However, only one pattern in a list of glob-patterns must match a file (so that, e.g., rm *.a *.c *.o would fail only if there were no files in the current directory ending in .a , .c , or .o ) , and if the nonomatch shell variable is set a pattern (or list of patterns) which matches nothing is left unchanged rather than causing an error.

The globstar shell variable can be set to allow ** or *** as a file glob pattern that matches any string of characters including / , recursively traversing any existing sub-directories. For example, ls **.c will list all the .c files in the current directory tree. If used by itself, it will match zero or more sub-directories. For example ls /usr/include/**/time.h will list any file named time.h in the /usr/include directory tree; ls /usr/include/**time.h will match any file in the /usr/include directory tree ending in time.h ; and ls /usr/include/**time**.h will match any .h file with time either in a subdirectory name or in the filename itself. To prevent problems with recursion, the ** glob-pattern will not descend into a symbolic link containing a directory. To override this, use *** (+)

The noglob shell variable can be set to prevent filename substitution, and the expand-glob editor command, normally bound to ^X-* , can be used to interactively expand individual filename substitutions.
