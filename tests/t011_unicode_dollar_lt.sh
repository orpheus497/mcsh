#!/bin/sh
# t011_unicode_dollar_lt.sh — $< stdin read with multibyte content

utf8_locale=$(locale -a 2>/dev/null | grep -ix 'en_US\.UTF-\?8' | head -n 1)
if [ -z "$utf8_locale" ]; then
    utf8_locale=$(locale -a 2>/dev/null | grep -ix 'C\.UTF-\?8' | head -n 1)
fi
if [ -z "$utf8_locale" ]; then
    utf8_locale=$(locale -a 2>/dev/null | grep -i 'UTF-\?8' | head -n 1)
fi
if [ -z "$utf8_locale" ]; then
    echo "SKIP: no UTF-8 locale available"
    exit 0
fi

# Use command substitution; $< reads from stdin of the child shell
out=$(echo 'café' | LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set x = $<; echo $x' 2>&1)
case "$out" in
    café) ;;
    *) echo "FAIL: \$< café: got '$out'"; exit 1 ;;
esac

out=$(echo '漢字' | LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set x = $<; echo $x' 2>&1)
case "$out" in
    漢字) ;;
    *) echo "FAIL: \$< 漢字: got '$out'"; exit 1 ;;
esac

exit 0
