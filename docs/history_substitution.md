# History Substitution

How to recall and modify previous commands using the history substitution mechanism.

Each command, or event , input from the terminal is saved in the history list. The previous command is always saved, and the history shell variable can be set to a number to save that many commands. The histdup shell variable can be set to not save duplicate events or consecutive duplicate events.

Saved commands are numbered sequentially from 1 and stamped with the time. It is not usually necessary to use event numbers, but the current event number can be made part of the prompt by placing an ! in the prompt shell variable.

By default history entries are displayed by printing each parsed token separated by space; thus the redirection operator >&! will be displayed as >\0&\0! . The shell actually saves history in expanded and literal (unexpanded) forms. If the histlit shell variable is set, commands that display and store history use the literal form.

The history builtin command can print, store in a file, restore and clear the history list at any time, and the savehist and histfile shell variables can be set to store the history list automatically on logout and restore it on login.

History substitutions introduce words from the history list into the input stream, making it easy to repeat commands, repeat arguments of a previous command in the current command, or fix spelling mistakes in the previous command with little typing and a high degree of confidence.

History substitutions begin with the character ! . They may begin anywhere in the input stream, but they do not nest. The ! may be preceded by a \ to prevent its special meaning; for convenience, a ! is passed unchanged when it is followed by a blank, tab, newline, = or ( .

History substitutions also occur when an input line begins with ^ ; see History substitution abbreviation .

The characters used to signal history substitution ! and ^ can be changed by setting the histchars shell variable. Any input line which contains a history substitution is printed before it is executed.

A history substitution may have an event specification (see History event specification ) , which indicates the event from which words are to be taken, a word designator (see History word designators ) , which selects particular words from the chosen event, and/or a word modifier (see History word modifiers ) , which manipulates the selected words.

## History Event Specification

A history event specification may be one of (with the history substitution character ! shown):
* **!Event**
History event specification
* `!`  *n*
A number, referring to a particular event.
* `!-`  *n*
An offset, referring to the event n before the current event.
* `!#`
The current event. This should be used carefully in csh(1) , where there is no check for recursion. allows 10 levels of recursion. (+)
* `!!`
The previous event, equivalent to !-1 .
* `!`  *s*
The most recent event whose first word begins with the string s .
* `!?`  *s*  `?`
The most recent event which contains the string s . The second ? can be omitted if it is immediately followed by a newline.

For example, consider this bit of someone's history list:
```
 9  8:30  nroff -man wumpus.man
10  8:31  cp wumpus.man wumpus.man.old
11  8:36  vi wumpus.man
12  8:37  diff wumpus.man.old wumpus.man
```

The commands are shown with their event numbers and time stamps. The current event, which we haven't typed in yet, is event 13.

Typing !11 or !-2 refers to event 11.

Typing !! refers to the previous event, 12. !! can be abbreviated ! if it is followed by : , which is described in History word designators and History word modifiers .

Typing !n refers to event 9, which begins with n .

Typing !?old? refers to event 12, which contains old .

Without word designators or modifiers history references simply expand to the entire event, so we might type !cp to redo the cp command (event 10) or !!|more if the diff output in the previous event, 12, scrolled off the top of the screen.

History references may be insulated from the surrounding text with braces { and } if necessary. For example, !vdoc would look for a command beginning with vdoc , and, in this example, not find one, but !{v}doc would expand unambiguously to vi wumpus.mandoc by matching event 11. Even in braces, history substitutions do not nest.

(+) While csh(1) expands, for example, !3d to event 3 with the letter d appended to it, expands it to the last event beginning with 3d ; only completely numeric arguments are treated as event numbers. This makes it possible to recall events beginning with numbers. To expand !3d as in csh(1) type !{3}d

## History Word Designators

To select words from an event we can follow the event specification by a : and a designator for the desired words. The words of an input line are numbered from 0, the first (usually command) word being 0, the second word (first argument) being 1, etc.

The basic word designators are, with columns for a leading : and a leading ! (for the abbreviated word designators - see History substitution abbreviation ) :
* **:Word  !Word  History word designator**

* `:0`
The first (command) word.

* `:`  *n*
The n th argument.

* `:^`  `!^`
The first argument, equivalent to :1 .

* `:$`  `!$`
The last argument.

* `:%`  `!%`
The word matched by an ? s ? search.

* `:`  *x*  `-`  *y*
A range of words.

* `:-`  *y*  `!-`  *y*
Equivalent to :0- y .

* `:*`  `!*`
Equivalent to :^-$ , but returns nothing if the event contains only 1 word.

* `:`  *x*  `*`
Equivalent to : x -$ .

* `:`  *x*  `-`
Equivalent to : x * , but omitting the last word $ .

* `:-`
Equivalent to :0- ; the command and all arguments except the last argument.

Selected words are inserted into the command line separated by single blanks.

For example, the diff command (event 12) in the history list example in History event specification , diff wumpus.man.old wumpus.man might have been typed as diff !!:1.old !!:1 (using :1 to select the first argument from the previous event) or diff !-2:2 !-2:1 to select and swap the arguments from the cp command (event 10). If we didn't care about the order of the diff we might have typed diff !-2:1-2 or simply diff !-2:*

The cp command (event 10) might have been typed cp wumpus.man !#:1.old using
#
to refer to the current event.

Typing !n:- hurkle.man would reuse the first two words from the nroff command (event 9) to expand to nroff -man hurkle.man

The : separating the event specification from the word designator can be omitted if the argument selector begins with a ^ , $ , % , - , or
* .

For example, our diff command (event 12) might have been typed diff !!^.old !!^ or, equivalently, diff !!$.old !!$ However, if !! is abbreviated ! , an argument selector beginning with - will be interpreted as an event specification.

A history reference may have a word designator but no event specification. It then references the previous command.

Continuing our diff command example (event 12), we could have typed simply diff !^.old !^ or, to get the arguments in the opposite order, just diff !*

## History Word Modifiers

The word or words in a history reference can be edited, or modified , by following it with one or more modifiers (with the leading : shown), each preceded by a : :
* **:Word**
History word modifier
* `:h`
Remove a trailing pathname component, leaving the head.
* `:t`
Remove all leading pathname components, leaving the tail.
* `:r`
Remove a filename extension . xxx , leaving the root name.
* `:e`
Remove all but the extension.
* `:u`
Uppercase the first lowercase letter.
* `:l`
Lowercase the first uppercase letter.
* `:s/`  *l*  `/`  *r*  `/`
Substitute l for r . l is simply a string like r , not a regular expression as in the eponymous ed(1) command. Any character may be used as the delimiter in place of / ; a \ can be used to quote the delimiter inside l and r . The character & in the r is replaced by l ; \ also quotes & . If l is empty (  ) , the l from a previous substitution or the s from a previous search or event number in event specification is used. The trailing delimiter may be omitted if it is immediately followed by a newline.
* `:&`
Repeat the previous substitution.
* `:g`
Apply the following modifier once to each word.
* `:a` (+)
Apply the following modifier as many times as possible to a single word. :a and :g can be used together to apply a modifier globally. With the :s modifier, only the patterns contained in the original word are substituted, not patterns that contain any substitution result.
* `:p`
Print the new command line but do not execute it.
* `:q`
Quote the substituted words, preventing further substitutions.
* `:Q`
Same as :q but in addition preserve empty variables as a string containing a NUL. This is useful to preserve positional arguments for example:
```
> set args=('arg 1' '' 'arg 3')
> tcsh -f -c 'echo ${#argv}' $args:gQ
3
```
* `:x`
Like :q , but break into words at blanks, tabs and newlines.

Modifiers are applied to only the first modifiable word (unless :g is used). It is an error for no word to be modifiable.

For example, the diff command (event 12) in the history list example in History event specification , diff wumpus.man.old wumpus.man might have been typed as diff wumpus.man.old !#^:r using :r to remove .old from the first argument on the same line !#^ .

We could type echo hello out there then echo !*:u to capitalize hello , echo !*:au to upper case the first word to HELLO , or echo !*:agu to upper case all words.

We might follow mail -s "I forgot my password" rot with !:s/rot/root to correct the spelling of root (see History word modifiers and Spelling correction (+) for different approaches).

(+) In csh(1) as such, only one modifier may be applied to each history or variable expansion. In mcsh, more than one may be used, for example
```
% mv wumpus.man /usr/share/man/man1/wumpus.1
% man !$:t:r
man wumpus
```

In csh(1) , the result would be wumpus.1:r

A substitution followed by a : may need to be insulated from it with braces:
```
> mv a.out /usr/games/wumpus
> setenv PATH !$:h:$PATH
Bad ! modifier: $.
> setenv PATH !{-2$:h}:$PATH
setenv PATH /usr/games:/bin:/usr/bin:.
```

The first attempt would succeed in csh(1) but fails in mcsh, because it expects another modifier after the second : rather than $ .
