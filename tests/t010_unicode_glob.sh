#!/bin/sh
# t010_unicode_glob.sh — glob expansion with Unicode filenames

. "$(dirname "$0")/lib_locale.sh"

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
