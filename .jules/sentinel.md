
## 2023-10-27 - [Anti-Pattern] `strncpy` with Manual Null-Termination
**Vulnerability:** Widespread use of `strncpy(dest, src, sizeof(dest)-1); dest[sizeof(dest)-1] = '\0';` across caching, UI prompts, and path resolution.
**Learning:** While manually null-terminating prevents strict out-of-bounds reads, it masks silent string truncation. Additionally, `tc.func.c` showed an off-by-one error using `sizeof(dest)` instead of `sizeof(dest)-1`, demonstrating the fragility of the idiom. `strncpy` also suffers from performance overhead due to mandatory null-padding.
**Prevention:** Standardize on `xsnprintf(dest, sizeof(dest), "%s", src);` for all string copies into fixed-size buffers to guarantee null-termination, avoid null-padding overhead, and enable truncation detection if return values are checked.
## 2023-11-23 - [Vulnerability] strcpy without bounds check in tc.who.c
**Vulnerability:** strcpy(wp->who_name, wp->who_new) was used to copy between two fixed-size buffers, risking buffer overflows if who_new was unexpectedly larger.
**Learning:** Even when destination and source buffers are defined with the same length, assuming input strings are properly bounded can lead to regressions if other parts of the parsing logic change.
**Prevention:** Always use bounded copying functions like xsnprintf(dest, sizeof(dest), "%s", src) instead of strcpy.
