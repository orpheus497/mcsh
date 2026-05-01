#!/bin/sh
# t001_vars.sh — $mcsh and $tcsh variables must be set on startup

out=$("$MCSH" -f -c 'if ($?mcsh && $?tcsh) echo ok' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d; output: %s\n' "$status" "$out"
    exit 1
fi
case "$out" in
    ok) exit 0 ;;
    *)  printf "expected 'ok', got: %s\n" "$out"; exit 1 ;;
esac
