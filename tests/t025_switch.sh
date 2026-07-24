#!/bin/sh
# t025_switch.sh — verify switch/case/breaksw/endsw statement

out=$("$MCSH" -f -c '
set val="foo"
switch ($val)
case "bar":
    echo bar
    breaksw
case "foo":
    echo foo
    breaksw
default:
    echo default
    breaksw
endsw
' 2>&1)

if [ "$out" = "foo" ]; then
    exit 0
else
    printf 'switch failed, got:\n%s\n' "$out"
    exit 1
fi
