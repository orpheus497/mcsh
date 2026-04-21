#!/bin/sh
# t007_arith_rsh.sh — right-shift must preserve sign (arithmetic shift semantics)
# @ x = (-8 >> 1) should give -4 on all supported platforms

out=$("$MCSH" -f -c '@ x = (-8 >> 1); echo $x' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d; output: %s\n' "$status" "$out"
    exit 1
fi
case "$out" in
    -4) exit 0 ;;
    *)  printf "expected -4, got: %s\n" "$out"; exit 1 ;;
esac
