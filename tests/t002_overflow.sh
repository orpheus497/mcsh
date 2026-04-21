#!/bin/sh
# t002_overflow.sh — left-shift overflow must not invoke UB (uses unsigned path)
# @ x = (1 << 31) must produce 2147483648, not a negative number or crash.

out=$("$MCSH" -f -c '@ x = (1 << 31); echo $x' 2>&1)
case "$out" in
    2147483648) exit 0 ;;
    *)          echo "expected 2147483648, got: $out"; exit 1 ;;
esac
