#!/bin/sh
# t012_unicode_backtick.sh — backquote command substitution with multibyte output

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

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set x = `echo café`; echo $x' 2>&1)
case "$out" in
    café) ;;
    *) echo "FAIL: backtick café: got '$out'"; exit 1 ;;
esac

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set x = `echo 漢字`; echo $x' 2>&1)
case "$out" in
    漢字) ;;
    *) echo "FAIL: backtick 漢字: got '$out'"; exit 1 ;;
esac

exit 0
