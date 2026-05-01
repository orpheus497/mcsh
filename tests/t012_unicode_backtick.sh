#!/bin/sh
# t012_unicode_backtick.sh — backquote command substitution with multibyte output

. "$(dirname "$0")/lib_locale.sh"

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
