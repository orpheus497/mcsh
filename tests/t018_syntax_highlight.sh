#!/bin/sh
# t018_syntax_highlight.sh — smoke test for 'set syntax'
# Ensures enabling syntax highlighting doesn't crash the shell or cause errors.

out=$("$MCSH" -f -c 'set syntax; echo $?syntax' 2>&1)
status=$?

if [ $status -ne 0 ]; then
    printf 'set syntax failed: %s\n' "$out"
    exit 1
fi

if [ "$out" = "1" ]; then
    exit 0
else
    printf 'expected syntax=1, got: %s\n' "$out"
    exit 1
fi
