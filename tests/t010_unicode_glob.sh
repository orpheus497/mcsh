#!/bin/sh
# t010_unicode_glob.sh — glob expansion with Unicode filenames

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

tmpdir=$(mktemp -d /tmp/mcsh_unicode_XXXXXX) || exit 2
trap 'rm -rf "$tmpdir"' EXIT

touch "$tmpdir/café.txt" "$tmpdir/漢字.txt" "$tmpdir/emoji😀.txt"

# * glob
out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    "set f = ($tmpdir/café*); echo \$#f \$f[1]" 2>&1)
case "$out" in
    "1 $tmpdir/café.txt") ;;
    *) echo "FAIL: café glob: got '$out'"; exit 1 ;;
esac

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    "set f = ($tmpdir/漢字*); echo \$#f" 2>&1)
case "$out" in
    1) ;;
    *) echo "FAIL: 漢字 glob: got '$out'"; exit 1 ;;
esac

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f -c \
    "set f = ($tmpdir/emoji*); echo \$#f" 2>&1)
case "$out" in
    1) ;;
    *) echo "FAIL: emoji glob: got '$out'"; exit 1 ;;
esac

exit 0
