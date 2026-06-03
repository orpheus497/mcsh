Wait, what if I type `C-u C-u`?
`DoingArg` becomes 2. Then `DoingArg` becomes 2 again. `Argument` becomes 16.
Then I type `3`. `DoingArg` is 2. `Argument` becomes 3, and `DoingArg` becomes 1.
This works PERFECTLY!

Let's look at `e_yank_pop`:
```c
#if 0
    /* XXX This "should" be here, but doesn't work, since LastCmd
       gets set on CC_ERROR and CC_ARGHACK, which it shouldn't(?).
       (But what about F_ARGFOUR?) I.e. if you hit M-y twice the
       second one will "succeed" even if the first one wasn't preceded
       by a yank, and giving an argument is impossible. Now we "succeed"
       regardless of previous command, which is wrong too of course. */
    if (LastCmd != F_YANK_KILL && LastCmd != F_YANK_POP)
	return(CC_ERROR);
#endif
```
If we un-comment this, and we apply the `LastCmd` update logic:
```c
	/* save the last command here */
	if (retval != CC_ERROR && retval != CC_FATAL && retval != CC_ARGHACK)
	    LastCmd = cmdnum;
```
Then `e_yank_pop` will work properly.
If you type `M-y` (without preceding yank), `LastCmd` might be `F_INSERT`. `e_yank_pop` returns `CC_ERROR`.
Because it returns `CC_ERROR`, `LastCmd` is NOT updated! `LastCmd` remains `F_INSERT`.
So if you hit `M-y` again, it again sees `F_INSERT`, and fails again! This fixes the second bug!

Let's double check all `CC_ARGHACK` returns in `ed.chared.c`:
`grep "return.*CC_ARGHACK" ed.chared.c`
1. `e_digit`: `return(CC_ARGHACK);`
2. `v_zero`: `return(CC_ARGHACK);`
3. `e_argfour`: `return(CC_ARGHACK);`
4. `e_run_command`?: line 2987. Wait, let's check `ed.chared.c:2987`.
5. `e_metanext`? line 3157.
6. `e_extendnext`? line 3166.

Let's check `e_metanext`:
```c
CCRETVAL
e_metanext(Char c)
{
    USE(c);
    MetaNext = 1;
    return(CC_ARGHACK);	/* preserve argument */
}
```
If it returns `CC_ARGHACK`, `LastCmd` is NOT updated. This is correct! `M-` is just a prefix, it shouldn't change the last command!

What about `e_run_command`?
```c
	ActionFlag = c;
	return(CC_ARGHACK);  /* Do NOT clear out argument */
```
This is for vi-mode pending actions. This shouldn't update `LastCmd` either, because it's a prefix!

What about `CC_ERROR` and `CC_FATAL` in `ed.inputl.c`?
```c
	case CC_FATAL:		/* fatal error, reset to known state */
```
If we don't update `LastCmd` on `CC_FATAL`, it's fine.

Is there any other place where `LastCmd` is used?
```bash
grep "LastCmd" ed.*.c
```
1. `ed.chared.c:1212`: `LastCmd = (KEYCMD) newdir; /* avoid c_hsetpat */`
2. `ed.chared.c:1364`: `LastCmd = (KEYCMD) dir; /* avoid c_hsetpat */`
3. `ed.chared.c:1534`: `if (LastCmd == F_ARGFOUR)`
4. `ed.chared.c:1835`: `if (LastCmd != F_UP_SEARCH_HIST && LastCmd != F_DOWN_SEARCH_HIST)`
5. `ed.chared.c:2292`: `if (LastCmd != F_YANK_KILL && LastCmd != F_YANK_POP)`
6. `ed.chared.c:2880`: `LastCmd = (KEYCMD) c;  /* Hack to stop c_hsetpat */`
7. `ed.init.c:577`: `LastCmd = F_UNASSIGNED;`
8. `ed.inputl.c:195`: `LastCmd = cmdnum;`

So only `c_hsetpat`, `e_digit`, and `e_yank_pop` check `LastCmd`.
And we fix `e_digit` by using `DoingArg == 2`.
Wait, what if `e_digit` checks `LastCmd == F_ARGFOUR` but `LastCmd` is no longer updated?
Then we remove the check `LastCmd == F_ARGFOUR` and replace it with `DoingArg == 2`.
Let's see the patch for `e_digit`:
```c
<<<<<<< SEARCH
    if (DoingArg) {		/* if doing an arg, add this in... */
	if (LastCmd == F_ARGFOUR)	/* if last command was ^U */
	    Argument = c - '0';
	else {
	    if (Argument > 1000000)
		return CC_ERROR;
	    Argument = (Argument * 10) + (c - '0');
	}
	return(CC_ARGHACK);
    }
=======
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
>>>>>>> REPLACE
```

Patch for `e_argfour`:
```c
<<<<<<< SEARCH
CCRETVAL
e_argfour(Char c)
{				/* multiply current argument by 4 */
    USE(c);
    if (Argument > 1000000)
	return CC_ERROR;
    DoingArg = 1;
    Argument *= 4;
    return(CC_ARGHACK);
}
=======
CCRETVAL
e_argfour(Char c)
{				/* multiply current argument by 4 */
    USE(c);
    if (Argument > 1000000)
	return CC_ERROR;
    DoingArg = 2;
    Argument *= 4;
    return(CC_ARGHACK);
}
>>>>>>> REPLACE
```

Patch for `ed.inputl.c`:
```c
<<<<<<< SEARCH
	if (adrof(STRsyntax) && retval == CC_NORM)
	    syntax_colorize();

	/* save the last command here */
	LastCmd = cmdnum;

	/* clear ghost text for any command that isn't insert/predict-accept */
=======
	if (adrof(STRsyntax) && retval == CC_NORM)
	    syntax_colorize();

	/* save the last command here */
	if (retval != CC_ERROR && retval != CC_FATAL && retval != CC_ARGHACK)
	    LastCmd = cmdnum;

	/* clear ghost text for any command that isn't insert/predict-accept */
>>>>>>> REPLACE
```

Patch for `ed.chared.c` `e_yank_pop`:
```c
<<<<<<< SEARCH
#if 0
    /* XXX This "should" be here, but doesn't work, since LastCmd
       gets set on CC_ERROR and CC_ARGHACK, which it shouldn't(?).
       (But what about F_ARGFOUR?) I.e. if you hit M-y twice the
       second one will "succeed" even if the first one wasn't preceded
       by a yank, and giving an argument is impossible. Now we "succeed"
       regardless of previous command, which is wrong too of course. */
    if (LastCmd != F_YANK_KILL && LastCmd != F_YANK_POP)
	return(CC_ERROR);
#endif
=======
    if (LastCmd != F_YANK_KILL && LastCmd != F_YANK_POP)
	return(CC_ERROR);
>>>>>>> REPLACE
```

Wait, what if `e_yank_pop` executes and returns `CC_ERROR` (because the kill ring is empty, or the check fails)?
Then `retval = CC_ERROR`, so `LastCmd` is NOT updated! `LastCmd` remains `F_INSERT` or whatever.
Then if the user types `M-y` again, it checks `LastCmd`, which is `F_INSERT`, and returns `CC_ERROR` again.
This completely fixes the bug!
