## 2026-06-05 - [Fix buffer overflows by replacing strcpy and strcat]
**Vulnerability:** Unsafe buffer copy functions like strcpy and strcat were used.
**Learning:** Replaced strcpy and strcat with safer variants xsnprintf and xasprintf respectively to guarantee buffer bounds check and prevent buffer overflows.
**Prevention:** Always use safe bounds-checked buffer copy variants.
