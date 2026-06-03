# Scripting Guide

This guide explains the mechanics of writing shell scripts in mcsh, covering variables, control flow, and expressions.

## Variables

The shell maintains a list of variables, each of which has as value a list of zero or more words. The values of shell variables can be displayed and changed with the set and unset commands. The system maintains its own list of environment variables. These can be displayed and changed with printenv , setenv , and unsetenv .

(+) Variables may be made read-only with set -r Read-only variables may not be modified or unset; attempting to do so will cause an error. Once made read-only, a variable cannot be made writable, so set -r should be used with caution. Environment variables cannot be made read-only.

Some variables are set by the shell or referred to by it. For instance, the argv variable is an image of the shell's argument list, and words of this variable's value are referred to in special ways. Some of the variables referred to by the shell are toggles; the shell does not care what their value is, only whether they are set or not. For instance, the verbose variable is a toggle which causes command input to be echoed. The v command line option sets this variable. Special shell variables lists all variables which are referred to by the shell.

Other operations treat variables numerically. The @ command permits numeric calculations to be performed and the result assigned to a variable. Variable values are, however, always represented as (zero or more) strings. For the purposes of numeric operations, the null string is considered to be zero, and the second and subsequent words of multi-word values are ignored.

After the input line is aliased and parsed, and before each command is executed, variable substitution is performed keyed by $ characters. This expansion can be prevented by preceding the $ with a \ except within " pairs where it always occurs, and within ' pairs where it never occurs. Strings quoted by \` are interpreted later (see Command substitution ) so $ substitution does not occur there until later, if at all. A $ is passed unchanged if followed by a blank, tab, or end-of-line.

Input/output redirections are recognized before variable expansion, and are variable expanded separately. Otherwise, the command name and entire argument list are expanded together. It is thus possible for the first (command) word (to this point) to generate more than one word, the first of which becomes the command name, and the rest of which become arguments.

Unless enclosed in " or given the :q modifier the results of variable substitution may eventually be command and filename substituted. Within " , a variable whose value consists of multiple words expands to a (portion of a) single word, with the words of the variable's value separated by blanks. When the :q modifier is applied to a substitution the variable will expand to multiple words with each word separated by a blank and quoted to prevent later command or filename substitution.

The editor command expand-variables , normally bound to ^X-$ , can be used to interactively expand individual variables.

## Control Flow

The shell contains a number of commands which can be used to regulate the flow of control in command files (shell scripts) and (in limited but useful ways) from terminal input. These commands all operate by forcing the shell to reread or skip in its input and, due to the implementation, restrict the placement of some of the commands.

The foreach , switch , and while statements, as well as the if ... then ... else form of the if statement, require that the major keywords appear in a single simple command on an input line as shown below.

If the shell's input is not seekable, the shell buffers up input whenever a loop is being read and performs seeks in this internal buffer to accomplish the rereading implied by the loop. (To the extent that this allows, backward goto s will succeed on non-seekable inputs.)

## Expressions

The if , while , and exit builtin commands use expressions with a common syntax. The expressions can include any of the operators described in the next three sections. Note that the @ builtin command has its own separate syntax.

These operators are similar to those of C and have the same precedence.

The operators, in descending precedence, with equivalent precedence per line, are:
* `(`  `)`
* `~`
* `!`
* `*`  `/`  `%`
* `+`  `-`
* `<<`  `>>`
* `<=`  `>=`  `<`  `>`
* `==`  `!=`  `=~`  `!~`
* `&`
* `^`
* `|`
* `&&`
* `||`

The == != =~ and !~ operators compare their arguments as strings; all others operate on numbers. The operators =~ and !~ are like == and != except that the right hand side is a glob-pattern (see Filename substitution ) against which the left hand operand is matched. This reduces the need for use of the switch builtin command in shell scripts when all that is really needed is pattern matching.

Null or missing arguments are considered 0 . The results of all expressions are strings, which represent decimal numbers. It is important to note that no two components of an expression can appear in the same word; except when adjacent to components of expressions which are syntactically significant to the parser & , | , < , > , ( , ) they should be surrounded by spaces.
