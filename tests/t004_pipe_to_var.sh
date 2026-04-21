#!/bin/sh
# t004_pipe_to_var.sh — "set x" reads from piped stdin (pipe-to-variable feature)
# The form tested: pipe data into the shell, then "set x" reads from stdin.

out=$(echo "foo" | "$MCSH" -f -c 'set x; echo $x' 2>&1)
case "$out" in
    foo) exit 0 ;;
    *)   echo "expected 'foo', got: $out"; exit 1 ;;
esac
