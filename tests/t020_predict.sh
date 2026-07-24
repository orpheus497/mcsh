#!/bin/sh
# t020_predict.sh — smoke test for 'set predict'
# Ensures enabling predictive autocomplete doesn't crash the shell.

out=$("$MCSH" -f -c 'set predict; echo $?predict' 2>&1)
status=$?

if [ $status -ne 0 ]; then
    printf 'set predict failed: %s\n' "$out"
    exit 1
fi

if [ "$out" = "1" ]; then
    exit 0
else
    printf 'expected predict=1, got: %s\n' "$out"
    exit 1
fi
