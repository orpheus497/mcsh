#!/bin/sh
# t011_unicode_dollar_lt.sh — $< stdin read with multibyte content

. "$(dirname "$0")/lib_locale.sh"

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
