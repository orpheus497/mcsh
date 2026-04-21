#!/bin/sh
# t004_pipe_to_var.sh — "set x" reads from piped stdin (pipe-to-variable feature)
# The form tested: pipe data into the shell, then "set x" reads from stdin.

out=$(echo "foo" | "$MCSH" -f -c 'set x; echo $x' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d; output: %s\n' "$status" "$out"
    exit 1
fi
case "$out" in
    foo) exit 0 ;;
    *)   printf "expected 'foo', got: %s\n" "$out"; exit 1 ;;
esac
