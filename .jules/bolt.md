## 2026-04-20 - [Performance Optimization in Git Prompt]
**Learning:** Found an opportunity to optimize repeated file path strings within a loop in the `%g`/`%G` prompt cache lookup in `tc.prompt.c`. The optimization works by doing the base string formatting outside the loop, saving its length, and formatting only the variable part per loop iteration inside the remaining buffer space.
**Action:** Applied the string formatting optimization to avoid redundant parsing and formatting inside hot path loops. Ensure string offset pointers are carefully handled using limits.
