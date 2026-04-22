#!/bin/sh
# t011_unicode_dollar_lt.sh — $< stdin read with multibyte content

if ! locale -a 2>/dev/null | grep -qi "UTF-8\|utf8"; then
    echo "SKIP: no UTF-8 locale available"
    exit 0
fi

# Use command substitution; $< reads from stdin of the child shell
out=$(echo 'café' | LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set x = $<; echo $x' 2>&1)
case "$out" in
    café) ;;
    *) echo "FAIL: \$< café: got '$out'"; exit 1 ;;
esac

out=$(echo '漢字' | LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f -c \
    'set x = $<; echo $x' 2>&1)
case "$out" in
    漢字) ;;
    *) echo "FAIL: \$< 漢字: got '$out'"; exit 1 ;;
esac

exit 0
