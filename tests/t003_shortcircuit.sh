#!/bin/sh
# t003_shortcircuit.sh — $?a && "$a" must not raise "Undefined variable"
# when $a is unset.  Should produce no output and exit 0.

out=$("$MCSH" -f -c 'unset a; if ($?a && "$a" != "") echo yes; endif' 2>&1)
if [ $? -ne 0 ] || [ -n "$out" ]; then
    echo "expected silence and exit 0, got: $out"
    exit 1
fi

# ${undef:q} must not leave the modifier in the input stream
# (regression: fixDolMod() must be called before eatbrac for unset vars)
out=$("$MCSH" -f -c 'unset b; set x = "${b:q}"; echo ok' 2>&1)
if [ $? -ne 0 ] || [ "$out" != "ok" ]; then
    echo "expected 'ok' from \${undef:q}, got: $out"
    exit 1
fi

# The operand a short-circuit skips comes back from exp6() as "", and getn()
# must read "" as 0.  When it raised "Badly formed number" instead, a `{ }'
# command or a file test on the far side of && or || failed - and Fedora's
# /etc/profile.d/less.csh is exactly `$?LESSOPEN && { eval ... }', so the error
# aborted every start-up file after it, ~/.mcshrc included.
for expr in '0 && { true }' '1 || { true }' \
            '0 && -e /nonexistent' '1 || -e /nonexistent'; do
    out=$("$MCSH" -f -c "if ( $expr ) set y; echo ok" 2>&1)
    if [ $? -ne 0 ] || [ "$out" != "ok" ]; then
        echo "if ( $expr ) should evaluate quietly, got: $out"
        exit 1
    fi
done

# The same "" reaches getn() from an empty variable in arithmetic.
out=$("$MCSH" -f -c 'set x = ""; @ y = $x + 1; echo $y' 2>&1)
if [ $? -ne 0 ] || [ "$out" != "1" ]; then
    echo "an empty variable should count as 0 in @, got: $out"
    exit 1
fi

# ...and from `history' when the variable is unset, which also stopped
# `history -S' from saving.  HOME and histfile are both pointed into a scratch
# directory so the test can never write to the real history file.
tmp=$(mktemp -d "${TMPDIR:-/tmp}/t003.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' EXIT INT TERM
out=$(HOME=$tmp "$MCSH" -f -c \
      "set histfile = $tmp/hist; unset history; history; history -S; echo ok" 2>&1)
if [ $? -ne 0 ] || [ "$out" != "ok" ]; then
    echo "history with \$history unset should be quiet, got: $out"
    exit 1
fi

exit 0
