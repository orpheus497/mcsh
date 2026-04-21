#!/bin/sh
# t003_shortcircuit.sh — $?a && "$a" must not raise "Undefined variable"
# when $a is unset.  Should produce no output and exit 0.

out=$("$MCSH" -f -c 'unset a; if ($?a && "$a" != "") echo yes; endif' 2>&1)
if [ $? -ne 0 ] || [ -n "$out" ]; then
    echo "expected silence and exit 0, got: $out"
    exit 1
fi
exit 0
