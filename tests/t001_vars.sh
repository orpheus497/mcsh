#!/bin/sh
# t001_vars.sh — $mcsh and $tcsh variables must be set on startup

out=$("$MCSH" -f -c 'if ($?mcsh && $?tcsh) echo ok' 2>&1)
case "$out" in
    ok) exit 0 ;;
    *)  echo "expected 'ok', got: $out"; exit 1 ;;
esac
