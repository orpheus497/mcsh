#!/bin/sh
# t013_unicode_invalid_recovery.sh — stray invalid byte does not corrupt
# subsequent valid multibyte characters.
# This directly exercises the MB_LEN_MAX→MB_CUR_MAX regression fix in
# wide_read() (sh.lex.c) and the $< loop (sh.dol.c).

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

tmpdir=$(mktemp -d /tmp/mcsh_invbyte_XXXXXX) || exit 2
trap 'rm -rf "$tmpdir"' EXIT

# Create a script file containing: set v = "<0x80>café"
# 0x80 is a lone UTF-8 continuation byte (invalid as a sequence start).
# With the bug, wide_read() over-reads up to MB_LEN_MAX-1=15 bytes of what
# follows 0x80, dropping 'é' (bytes C3 A9) into the discard window.
script="$tmpdir/script.csh"
# Use octal escapes (POSIX printf) — dash's builtin printf does not
# support \x hex escapes, but \NNN octal is portable.
printf 'set v = "\200caf\303\251"\nif ($v =~ *\303\251) echo ok\n' > "$script"

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f "$script" 2>&1)
case "$out" in
    ok) ;;
    *) echo "FAIL: invalid-byte recovery: é was dropped; got '$out'"; exit 1 ;;
esac

exit 0
