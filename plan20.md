Ah! The reviewer caught exactly what I encountered in `test_use_after_free.csh` where the loop was not freed and we got a Use-After-Free!
The reviewer states:
"If the current seek depth is shallower than the loop's start context, the loop is inactive and should be freed (`start_ok = 0`).
If the current seek depth is deeper, we are nested inside the loop, so we keep it (`start_ok = 1`)."

And suggests calculating `o_depth`, `start_depth`, `end_depth` based on:
`TCSH_E_SEEK` ? 3 : `TCSH_A_SEEK` ? 2 : `TCSH_F_SEEK` ? 1 : 0.

Wait! Does `o_depth` strictly correlate to nesting?
Yes, `F_SEEK` is depth 1.
`A_SEEK` is depth 2.
`E_SEEK` is depth 3.
Is `eval` always deeper than `alias`? Or can `alias` be inside `eval`?
If `alias` is inside `eval`, `TCSH_A_SEEK` is inside `TCSH_E_SEEK`.
Wait! If `alias` is inside `eval`, `TCSH_A_SEEK` is evaluated by the lexer, so `aret` becomes `TCSH_A_SEEK`.
But what if we have multiple levels?
If `TCSH_E_SEEK = 3`, `TCSH_A_SEEK = 1` or `2`?
Let's look at `sh.lex.c`.
