#!/bin/sh
# t022_function_args.sh — verify function argument passing.

out=$("$MCSH" -f -c '
function test_args
    echo "arg1=$1 arg2=$2 all=$argv"
return
test_args foo bar
' 2>&1)

if [ "$out" = "arg1=foo arg2=bar all=foo bar" ]; then
    exit 0
else
    printf 'function args failed, got: %s\n' "$out"
    exit 1
fi
