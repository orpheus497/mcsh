#!/bin/sh
# t009_unicode_vars.sh — multibyte variable assignment and $% character count

# Skip if locale unavailable
if ! locale -a 2>/dev/null | grep -qi "UTF-8\|utf8"; then
    echo "SKIP: no UTF-8 locale available"
    exit 0
fi

# Round-trip variable assignment
out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set v = "café"; echo $v' 2>&1)
case "$out" in
    café) ;;
    *) echo "FAIL: café roundtrip: got '$out'"; exit 1 ;;
esac

out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set v = "漢字"; echo $v' 2>&1)
case "$out" in
    漢字) ;;
    *) echo "FAIL: 漢字 roundtrip: got '$out'"; exit 1 ;;
esac

out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set v = "😀"; echo $v' 2>&1)
case "$out" in
    😀) ;;
    *) echo "FAIL: emoji roundtrip: got '$out'"; exit 1 ;;
esac

# $% must count Unicode characters, not bytes
out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set v = "café"; echo $%v' 2>&1)
case "$out" in
    4) ;;
    *) echo "FAIL: \$%café expected 4 chars, got '$out'"; exit 1 ;;
esac

exit 0
