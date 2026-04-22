#!/bin/sh
# t012_unicode_backtick.sh — backquote command substitution with multibyte output

if ! locale -a 2>/dev/null | grep -qi "UTF-8\|utf8"; then
    echo "SKIP: no UTF-8 locale available"
    exit 0
fi

out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set x = `echo café`; echo $x' 2>&1)
case "$out" in
    café) ;;
    *) echo "FAIL: backtick café: got '$out'"; exit 1 ;;
esac

out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set x = `echo 漢字`; echo $x' 2>&1)
case "$out" in
    漢字) ;;
    *) echo "FAIL: backtick 漢字: got '$out'"; exit 1 ;;
esac

exit 0
