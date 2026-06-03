Let's see: `LastCmd` is used in a few places:
1. `ed.chared.c:1534`: `if (LastCmd == F_ARGFOUR)` in `e_digit` - if the last command was ^U, the next digit starts a new argument instead of multiplying.
2. `ed.chared.c:1835`: `if (LastCmd != F_UP_SEARCH_HIST && LastCmd != F_DOWN_SEARCH_HIST)` in `c_hsetpat` - don't reset the history search pattern if we're repeatedly searching up/down.

If we want `e_yank_pop` to correctly check if the previous command was a yank, we need to handle the fact that arguments (like M-2) might intervene between `M-y` and `C-y` or `M-y` and `M-y` (actually, M-y can take an argument to pop multiple elements).
Wait, if you do `C-y` (yank), then `M-y` (yank-pop), the `M-y` replaces the yanked text with the previous kill ring item.
If you do `M-2 M-y`, it goes back two kill ring items.
If `M-2` sets `LastCmd = F_DIGIT`, then when `M-y` is processed, `LastCmd` is `F_DIGIT`, not `F_YANK_KILL` or `F_YANK_POP`. Thus, `M-y` would fail if it strictly requires `LastCmd == F_YANK_KILL || LastCmd == F_YANK_POP`.
We could fix this by introducing a new variable, say `LastKillCmd` or simply keeping track of the last command that was *not* an argument hack. Or better, update `LastCmd` appropriately.
In `ed.inputl.c`:
```c
	/* now do the real command */
	retval = (*CcFuncTbl[cmdnum]) (ch);

	if (adrof(STRsyntax) && retval == CC_NORM)
	    syntax_colorize();

	/* save the last command here */
	if (retval != CC_ERROR && retval != CC_FATAL && retval != CC_ARGHACK)
	    LastCmd = cmdnum;
```
If we do this, `F_ARGFOUR` will return `CC_ARGHACK`. But wait! `F_ARGFOUR` itself returns `CC_ARGHACK`, so `LastCmd` won't be set to `F_ARGFOUR`! Then `e_digit`'s check `if (LastCmd == F_ARGFOUR)` will break.
Actually, if we look at `e_argfour` in `ed.chared.c`:
```c
CCRETVAL
e_argfour(Char c)
{
    USE(c);
    if (Argument > 1000000)
	return CC_ERROR;
    DoingArg = 1;
    Argument *= 4;
    return(CC_ARGHACK);
}
```
If `LastCmd` isn't updated on `CC_ARGHACK`, `LastCmd` won't be `F_ARGFOUR` after `e_argfour`.
But what if we update `LastCmd` ONLY in `ed.inputl.c` like it is now, but `e_yank_pop` checks something else?
Or what if `LastCmd` is updated on `CC_ARGHACK`, but NOT on `CC_ERROR` and `CC_FATAL`?
If `LastCmd` is updated on `CC_ARGHACK`, then `M-2 M-y` means the first command is `M-2` (`F_DIGIT`), returning `CC_ARGHACK`. `LastCmd` becomes `F_DIGIT`. The next command is `M-y`, `LastCmd` is `F_DIGIT`. Then `M-y` will fail the `#if 0` check.
Wait, if you do `C-y` (yank), then `M-2 M-y`, does the `M-2` mean "replace the yanked text with the 2nd previous kill ring item"? Yes. Gnu Emacs allows arguments to `yank-pop`.
So the check should be: "was the last non-argument command a yank?"
We could introduce `KEYCMD LastCmd` and `KEYCMD LastYankCmd`? Or `KEYCMD LastNonArgCmd`?
Or we can just look at `LastCmd` and not update `LastCmd` on `CC_ARGHACK`, but fix `e_digit`!
Wait, in `e_digit`:
```c
	if (LastCmd == F_ARGFOUR)	/* if last command was ^U */
	    Argument = c - '0';
	else {
...
```
Why does `e_digit` care if the last command was `F_ARGFOUR`? Because `^U` sets `Argument = 4`. If you then type `3`, you want `Argument` to become `3`, not `43` or something.
But wait, if we don't update `LastCmd` on `CC_ARGHACK`, how can we know if the last command was `F_ARGFOUR`?
What if we add a `JustArgFour` flag? Or `LastCmd` is updated, but we add a new variable `LastYankCmd` which is set to 1 when a yank is done and cleared otherwise?
