## 2026-04-20 - [Optimize ed.chared.c KillRing]
**Learning:** Checking lengths and individual characters inside frequent string-matching loops (like in `c_push_kill` and its duplication checks) avoids full `Strncmp` invocations and is massively faster.
**Action:** Always rearrange string-match checking inside heavy loops: 1) verify length via terminator position, 2) check first character, 3) perform full `strncmp` / `Strncmp`.
