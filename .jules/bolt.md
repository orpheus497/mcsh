## 2026-04-20 - [Performance Optimization in Git Prompt]
**Learning:** Found an opportunity to optimize repeated file path strings within a loop in the `%g`/`%G` prompt cache lookup in `tc.prompt.c`. The optimization works by doing the base string formatting outside the loop, saving its length, and formatting only the variable part per loop iteration inside the remaining buffer space.
**Action:** Applied the string formatting optimization to avoid redundant parsing and formatting inside hot path loops. Ensure string offset pointers are carefully handled using limits.
## 2026-07-21 - Size Calculation for Wide Characters
**Learning:** In the mcsh codebase, the `Char` type is often a wide character (e.g., `short` or `wchar_t`), not a standard 1-byte `char`. When using byte-oriented memory functions like `xmalloc`, `memcpy`, or `memmove` with `Char` strings, you must explicitly multiply the string length or character count by `sizeof(Char)` to calculate the correct byte size.
**Action:** Always multiply string lengths by `sizeof(Char)` when allocating memory or copying blocks of memory involving the `Char` type to avoid buffer under-allocations.
