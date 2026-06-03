Let's see: `e_digit` checks `LastCmd == F_ARGFOUR`.
What if we change `e_digit` to NOT use `LastCmd`?
Instead of checking `LastCmd == F_ARGFOUR`, we could add a specific global state for "the last thing was an argfour multiplier".
Actually, `DoingArg` already tells us if we are accumulating an argument.
Can we just add `KEYCMD LastCmd` and `KEYCMD LastCmd_unaltered`? No.
Let's add a new variable in `ed.inputl.c`:
`KEYCMD MacroCmd` or just let `LastCmd` behave as it did, but add `KEYCMD PrevCmd` for `yank_pop`?
If we do:
```c
	if (retval != CC_ERROR && retval != CC_FATAL && retval != CC_ARGHACK)
	    LastCmd = cmdnum;
	else if (cmdnum == F_ARGFOUR || cmdnum == F_DIGIT) {
	    /* Need to record that we are doing an argument, but wait! */
            /* If we just leave LastCmd alone for all arghacks, e_digit wouldn't know if the PREVIOUS arghack was F_ARGFOUR or F_DIGIT! */
            /* Wait, e_digit ONLY cares about whether the PREVIOUS command was F_ARGFOUR. */
            /* We can add a boolean `JustArgFour` that is set to 1 in `e_argfour`, and 0 everywhere else. */
	}
```
Wait, if we add a global `int JustArgFour`?
No, we can just do:
```c
	if (retval == CC_ERROR || retval == CC_FATAL) {
	    /* don't update LastCmd */
	} else if (retval == CC_ARGHACK) {
	    /* Emacs doesn't update last-command for prefix args.
	       But we need some way to know if F_ARGFOUR happened.
	       So let's just let LastCmd = cmdnum for CC_ARGHACK?
	       If we do that, yank-pop with an argument fails. */
	}
```

What if `e_yank_pop` doesn't check `LastCmd`? What if it checks some other variable `LastYankCmd`?
No, because `C-y` followed by `M-f` followed by `M-y` SHOULD fail! `LastYankCmd` would still be 1 from `C-y`. We DO want to check if the LAST command was a yank.
So `M-y` needs to know if the last REAL command was a yank.
If `LastCmd` is only updated for real commands (non-arghack, non-error):
1. `C-y` -> `retval` is `CC_REFRESH`. `LastCmd = F_YANK_KILL`.
2. `M-2` -> `retval` is `CC_ARGHACK`. `LastCmd` remains `F_YANK_KILL`.
3. `M-y` -> sees `LastCmd == F_YANK_KILL`. Succeeds!
This works PERFECTLY for `yank-pop`!

Now, how to fix `F_ARGFOUR` and `F_DIGIT`?
If `LastCmd` is NOT updated on `CC_ARGHACK`, then when `e_digit` runs after `F_ARGFOUR`, it will see `LastCmd` as whatever was BEFORE `F_ARGFOUR`!
So `LastCmd == F_ARGFOUR` will be FALSE.
To fix this, we can introduce a new variable `int ArgCmd`.
Or simply use `cmdnum` inside `ed.inputl.c` to set a flag `LastCmdWasArgFour`.
Actually, why not just change `e_digit`?
```c
    if (DoingArg) {		/* if doing an arg, add this in... */
	if (DoingArg == 2) {	/* if last command was ^U */
	    Argument = c - '0';
	    DoingArg = 1;
	} else {
	    if (Argument > 1000000)
		return CC_ERROR;
	    Argument = (Argument * 10) + (c - '0');
	}
	return(CC_ARGHACK);
    }
```
And in `e_argfour`:
```c
CCRETVAL
e_argfour(Char c)
{				/* multiply current argument by 4 */
    USE(c);
    if (Argument > 1000000)
	return CC_ERROR;
    DoingArg = 2; // Was 1
    Argument *= 4;
    return(CC_ARGHACK);
}
```
Wait! `DoingArg` is reset to 0 all over the place. Setting it to 2 and then back to 1 inside `e_digit` works perfectly!
Let's verify where `DoingArg` is checked.
Most places do `if (DoingArg)`. If it's 2, `if (DoingArg)` is still true!
Where else is it checked?
`ed.inputl.c:170:	if ((!DoingArg) && (Argument != 1))`
This also works perfectly.
Let's search for `DoingArg` checks.
`grep "DoingArg == 1" ed.*.c` -> none.
`grep "DoingArg == 0" ed.*.c` -> none.
`grep "DoingArg = 1" ed.*.c`:
`ed.chared.c:1580:	DoingArg = 1;` (in `e_digit` when starting an argument).
`ed.chared.c:3121:    DoingArg = 1;` (in `e_argfour`).

Wait, there is `v_zero`!
```c
CCRETVAL
v_zero(Char c)			/* command mode 0 for vi */
{
    if (DoingArg) {		/* if doing an arg, add this in... */
	if (Argument > 1000000)
	    return CC_ERROR;
	Argument = (Argument * 10) + (c - '0');
	return(CC_ARGHACK);
    }
```
If we type `C-u` then `0` in vi mode? `DoingArg` is 2. `v_zero` does `Argument = (Argument * 10) + (c - '0')`. Which makes it `40`. Is that right?
Wait, if you do `C-u` then `0`, `Argument` is 4. Then `v_zero` makes it `40`. But `e_digit` makes it `0`!
Why does `v_zero` not check `LastCmd == F_ARGFOUR`?
Because `F_ARGFOUR` is `^U`, which is an emacs binding?
Wait, `v_zero` is a vi command. Does vi have `F_ARGFOUR`? No, vi doesn't have `C-u` argument multiplier.
So `v_zero` works as is!

Wait, what if we just add a variable `int LastCmdForArg` which is updated for everything? Or just leave `LastCmd` as the REAL last command, and `LastCmd` is NOT updated on `CC_ARGHACK` and `CC_ERROR` and `CC_FATAL`!
Wait! Is there ANY other place that depends on `LastCmd` being updated on `CC_ARGHACK`?
Let's check `LastCmd` usage:
1. `c_hsetpat`: `if (LastCmd != F_UP_SEARCH_HIST && LastCmd != F_DOWN_SEARCH_HIST)`
   If we don't update on `CC_ERROR` or `CC_ARGHACK`, `LastCmd` stays `F_UP_SEARCH_HIST` if the last valid command was a history search.
   If we type an argument `M-2`, it returns `CC_ARGHACK`. Then if we type `F_UP_SEARCH_HIST`, it sees `LastCmd` as `F_UP_SEARCH_HIST` (from before `M-2`), so it DOESN'T reset the pattern! This is EXACTLY correct!
2. `e_digit`: `if (LastCmd == F_ARGFOUR)`
   If we change `LastCmd` to not update on `CC_ARGHACK`, `LastCmd` will never be `F_ARGFOUR`, because `F_ARGFOUR` returns `CC_ARGHACK`.
   So we MUST fix `e_digit` by using `DoingArg = 2`.

Let's verify `DoingArg` usage in `ed.chared.c` for `e_digit` and `e_argfour`.
```c
CCRETVAL
e_digit(Char c)			/* gray magic here */
{
    if (!Isdigit(c))
	return(CC_ERROR);	/* no NULs in the input ever!! */

    if (DoingArg) {		/* if doing an arg, add this in... */
	if (DoingArg == 2) {	/* if last command was ^U */
	    Argument = c - '0';
            DoingArg = 1;
        }
	else {
	    if (Argument > 1000000)
		return CC_ERROR;
	    Argument = (Argument * 10) + (c - '0');
	}
	return(CC_ARGHACK);
    }
    else {
```
Wait! What if you type `M-2`, then `C-u` (`F_ARGFOUR`), then `3`?
If you type `M-2`, `Argument = 2`, `DoingArg = 1`.
If you type `C-u`, `Argument *= 4` (so `8`), `DoingArg = 2`.
If you type `3`, since `DoingArg == 2`, `Argument = 3`! Wait! If you type `M-2 C-u 3`, should it be `3`?
In Emacs: `M-2 C-u 3` sets the argument to `3`. The `C-u` followed by digits overrides the previous argument! Yes!
Let's test this in bash (readline).
