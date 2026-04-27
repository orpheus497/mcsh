#!/bin/sh
# t002_overflow.sh — left-shift overflow must not invoke UB (uses unsigned path)
# @ x = (1 << 31) must produce 2147483648, not a negative number or crash.

out=$("$MCSH" -f -c '@ x = (1 << 31); echo $x' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d; output: %s\n' "$status" "$out"
    exit 1
fi
case "$out" in
    2147483648) exit 0 ;;
    *)          printf "expected 2147483648, got: %s\n" "$out"; exit 1 ;;
esac
