#!/bin/sh
# t024_variable_modifiers.sh — verify variable modifiers (:h :t :r :e :l :u)

out=$("$MCSH" -f -c '
set p="/a/b/c.txt"
echo "h=${p:h} t=${p:t} r=${p:r} e=${p:e}"
set s="aBc"
echo "l=${s:l} u=${s:u}"
' 2>&1)

expected="h=/a/b t=c.txt r=/a/b/c e=txt
l=abc u=ABC"

if [ "$out" = "$expected" ]; then
    exit 0
else
    printf 'modifiers failed, got:\n%s\n' "$out"
    exit 1
fi
