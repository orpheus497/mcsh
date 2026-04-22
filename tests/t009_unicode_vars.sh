#!/bin/sh
# t009_unicode_vars.sh — multibyte variable assignment and $% character count

# Discover an available UTF-8 locale (skips the test if none exists).
. "$(dirname "$0")/lib_locale.sh"

# Round-trip variable assignment
out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set v = "café"; echo $v' 2>&1)
case "$out" in
    café) ;;
    *) echo "FAIL: café roundtrip: got '$out'"; exit 1 ;;
esac

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set v = "漢字"; echo $v' 2>&1)
case "$out" in
    漢字) ;;
    *) echo "FAIL: 漢字 roundtrip: got '$out'"; exit 1 ;;
esac

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set v = "😀"; echo $v' 2>&1)
case "$out" in
    😀) ;;
    *) echo "FAIL: emoji roundtrip: got '$out'"; exit 1 ;;
esac

# $% must count Unicode characters, not bytes
out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    'set v = "café"; echo $%v' 2>&1)
case "$out" in
    4) ;;
    *) echo "FAIL: \$%café expected 4 chars, got '$out'"; exit 1 ;;
esac

exit 0
