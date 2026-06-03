Let's see what Emacs does. In Emacs, `yank-pop` requires the previous command to be `yank` or `yank-pop`. If you pass an argument to `yank-pop`, the sequence is `M-2 M-y`. The prefix argument commands (like `M-2` or `C-u`) in Emacs do NOT affect `last-command`.
Wait! In Emacs, prefix argument commands don't set `this-command`, they just set the prefix arg for the next command.
So if mcsh could behave like Emacs, `CC_ARGHACK` should NOT update `LastCmd`.
But wait! If `CC_ARGHACK` does not update `LastCmd`, then `LastCmd` remains whatever it was *before* the argument.
If `LastCmd` remains what it was, then `C-y M-2 M-y` would work perfectly, because `C-y` sets `LastCmd = F_YANK_KILL`, `M-2` returns `CC_ARGHACK` and DOES NOT update `LastCmd`, and `M-y` sees `LastCmd == F_YANK_KILL`.
But what about `F_ARGFOUR`? `F_ARGFOUR` returns `CC_ARGHACK`. If it doesn't update `LastCmd`, then `e_digit` won't know the last command was `F_ARGFOUR`.
Wait! We can simply add `KEYCMD LastCmd` update logic:
```c
if (retval != CC_ARGHACK && retval != CC_ERROR && retval != CC_FATAL)
    LastCmd = cmdnum;
```
If we do this, `F_ARGFOUR` won't update `LastCmd`. Then `e_digit` won't see `LastCmd == F_ARGFOUR`.
Wait, `F_ARGFOUR` *is* a `CC_ARGHACK` command!
Why not just have `F_ARGFOUR` set a specific flag, like `JustArgFour` or `DoingArgFour`? Or `e_digit` can just check `Argument == 4` and `DoingArg`? No, because `^U ^U` makes `Argument = 16`. If you then type `3`, it should be `3`.
Actually, if we look at `e_argfour`, we can just let it set a specific variable, or we can just allow `LastCmd` to be updated ONLY if it's `CC_ARGHACK`, but wait, we want `LastCmd` to NOT be updated on `CC_ARGHACK` except for `F_ARGFOUR`? No, Emacs prefix args don't update `last-command`.
Let's check if we can add a `MacroCmd` or similar, or just a new variable `ActionFlag`? There is `ActionFlag` in `ed.chared.c`.
Wait, `LastCmd` is used in:
- `c_hsetpat`: `if (LastCmd != F_UP_SEARCH_HIST && LastCmd != F_DOWN_SEARCH_HIST)`
- `e_digit`: `if (LastCmd == F_ARGFOUR)`
- `e_yank_pop`: `if (LastCmd != F_YANK_KILL && LastCmd != F_YANK_POP)` (currently `#if 0`)

If we just introduce `KEYCMD PrevCmd` or `LastCmd` and don't update it on `CC_ERROR`?
The comment says: `gets set on CC_ERROR and CC_ARGHACK, which it shouldn't(?). (But what about F_ARGFOUR?)`
Ah! The author knew exactly this dilemma!
If we don't update on `CC_ARGHACK`, `F_ARGFOUR` is broken.
If we do, `F_YANK_POP` with an argument is broken.
So what's the solution?

If we look at `e_yank_pop` in Gnu Emacs, it actually works by checking if `last-command` is `yank` or `yank-pop`. Prefix arguments in Emacs set `prefix-arg` but don't change `last-command`. Wait! In Emacs, `C-u` is a command, but it's a special one that doesn't set `this-command` to itself, it preserves `last-command`. So if we have a way to preserve `LastCmd` for argument commands...
Wait! If `e_digit` just wants to know if the argument was ONLY `^U` so far...
Instead of `if (LastCmd == F_ARGFOUR)`, `e_digit` could check:
`if (Argument == 4 && DoingArg == 1)` ... no, what if the user typed `C-u C-u` (Argument = 16)? Then typing `3` should reset Argument to 3!
So the condition should be: did the user type `C-u` (which multiplies by 4) to start the argument, or did they type digits?
We could just introduce a `char ArgMacro` or `int ArgHackCmd`?
Or better, we can add a new variable `KEYCMD LastCmd` and `KEYCMD LastArgCmd`?
No, we can just look at `cmdnum`.
In `ed.inputl.c`:
```c
	/* save the last command here */
	if (retval == CC_ARGHACK) {
	    /* We only care about LastCmd for F_ARGFOUR, but we don't want to ruin F_YANK_KILL. */
	    /* Wait, maybe just leave LastCmd alone for CC_ARGHACK, and have F_ARGFOUR set a specific flag? */
	}
```
If we add `int ArgFourStatus`? No, simpler: in `ed.inputl.c`, we can just NOT update `LastCmd` on `CC_ERROR` and `CC_FATAL`. What about `CC_ARGHACK`?
If `retval == CC_ARGHACK`, we COULD update `LastCmd` but save the previous non-arghack command in `LastNonArgCmd`?
No, just add `KEYCMD LastCmd` (which updates always) and `KEYCMD LastYank` or just use `LastCmd` but ONLY update it when `retval != CC_ERROR && retval != CC_FATAL && retval != CC_ARGHACK`.
Then for `F_ARGFOUR`, we can just update `LastCmd = cmdnum` manually inside `e_argfour`? Wait, `LastCmd` is already updated globally.
If we change `ed.inputl.c` to:
```c
	if (retval != CC_ERROR && retval != CC_FATAL && retval != CC_ARGHACK)
	    LastCmd = cmdnum;
```
Then `e_argfour` won't update `LastCmd`. `e_digit` won't see `LastCmd == F_ARGFOUR`.
Wait! If we change `e_digit` and `e_argfour` to use a separate flag, say `DoingArg = 1` for digit, `DoingArg = 2` for `F_ARGFOUR`!
Let's look at how `DoingArg` is used.
