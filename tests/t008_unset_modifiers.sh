#!/bin/sh
# t008_unset_modifiers.sh — unset variable with modifiers and $# must not error

# ${unset:h} should produce "" (not "Missing }")
out=$("$MCSH" -f -c 'unset x; echo "${x:h}"' 2>&1)
if echo "$out" | grep -Ei 'Missing|Error|error|Undefined'; then
    echo "FAIL: modifier on unset var caused error: $out"
    exit 1
fi

# $#unset should produce 0 (not error)
out=$("$MCSH" -f -c 'unset x; echo $#x' 2>&1)
case "$out" in
    0) ;;
    *) echo "FAIL: \$#unset expected 0, got: $out"; exit 1 ;;
esac

exit 0
