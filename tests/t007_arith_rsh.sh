#!/bin/sh
# t007_arith_rsh.sh — right-shift must preserve sign (arithmetic shift semantics)
# @ x = (-8 >> 1) should give -4 on all supported platforms

out=$("$MCSH" -f -c '@ x = (-8 >> 1); echo $x' 2>&1)
case "$out" in
    -4) exit 0 ;;
    *)  echo "expected -4, got: $out"; exit 1 ;;
esac
