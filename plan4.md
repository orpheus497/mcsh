Ah! `DoingArg` is not just a flag, but if it is 1, it means we are accumulating arguments.
Wait, `Argument` defaults to 1. `DoingArg` defaults to 0.
When you type `C-u` (which is `F_ARGFOUR`), it does:
```c
    DoingArg = 1;
    Argument *= 4;
```
If you type `C-u` again, it does `Argument *= 4`, so `Argument` becomes 16, and `DoingArg` remains 1.
If you type `3` (which is `F_DIGIT`), `e_digit` is called.
```c
    if (DoingArg) {
	if (LastCmd == F_ARGFOUR)
	    Argument = c - '0';
	else {
	    if (Argument > 1000000)
		return CC_ERROR;
	    Argument = (Argument * 10) + (c - '0');
	}
	return(CC_ARGHACK);
    }
```
If we type `C-u`, `Argument` is 4. If we type `3`, `LastCmd == F_ARGFOUR` is true, so `Argument` becomes 3.
If we then type `4`, `LastCmd` is now `F_DIGIT`, not `F_ARGFOUR`. So it goes to the `else` block:
`Argument = (3 * 10) + 4 = 34`.
This works perfectly!
If `LastCmd` is not updated on `CC_ARGHACK`, then when we type `3`, `LastCmd` is still whatever it was before `C-u`. So it will go to the `else` block, and `Argument` will become `(4 * 10) + 3 = 43`! That is wrong! `C-u 3` should be `3`.

So we CANNOT simply stop updating `LastCmd` on `CC_ARGHACK` without changing `e_digit`.
But what if we change `e_digit` and `e_argfour` so they don't depend on `LastCmd == F_ARGFOUR`?
What if we change `DoingArg`? We can just set `DoingArg = 2` when `F_ARGFOUR` is used!
Then in `e_digit`, if `DoingArg == 2`, we reset `Argument = c - '0'`, and set `DoingArg = 1`.
Let's see if this works:
1. Initialize: `DoingArg = 0, Argument = 1`.
2. `C-u`: `Argument *= 4`, `DoingArg = 2`.
3. `C-u`: `Argument *= 4` (16), `DoingArg = 2`.
4. `3`: `DoingArg` is 2. So `Argument = 3`, `DoingArg = 1`.
5. `4`: `DoingArg` is 1. So `Argument = 34`, `DoingArg = 1`.

This exactly reproduces the behavior without relying on `LastCmd == F_ARGFOUR`!
Let's check `e_argfour`:
```c
CCRETVAL
e_argfour(Char c)
{
    USE(c);
    if (Argument > 1000000)
	return CC_ERROR;
    DoingArg = 2; // Was 1
    Argument *= 4;
    return(CC_ARGHACK);
}
```
Wait, are there other places that check `DoingArg` as a boolean?
Let's check `grep "DoingArg" ed.*.c`
