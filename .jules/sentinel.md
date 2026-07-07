
## 2023-10-27 - [Anti-Pattern] `strncpy` with Manual Null-Termination
**Vulnerability:** Widespread use of `strncpy(dest, src, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0';` across caching, UI prompts, and path resolution.
**Learning:** While manually null-terminating prevents strict out-of-bounds reads, it masks silent string truncation. Additionally, `tc.func.c` showed an off-by-one error using `sizeof(dest)` instead of `sizeof(dest)-1`, demonstrating the fragility of the idiom. `strncpy` also suffers from performance overhead due to mandatory null-padding.
**Prevention:** Standardize on `xsnprintf(dest, sizeof(dest), "%s", src);` for all string copies into fixed-size buffers to guarantee null-termination, avoid null-padding overhead, and enable truncation detection if return values are checked.
