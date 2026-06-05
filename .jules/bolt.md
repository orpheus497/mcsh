## 2026-04-20 - [Optimize ed.syntax.c syntax highlighting]
**Learning:** Found significant optimization in `ed.syntax.c` where `in_table` calculates `strlen(*table)` in an inner loop. Replaced with first character match and `(*table)[len] == '\0'` check, speeding it up significantly. We can apply the same optimization to `cache_lookup`. Further optimizations applied include using `memset` instead of inner `for` loop in `classify_word`.
**Action:** Always check inner loops string and cache lookup, they can be optimized to prevent expensive repeated operations.
