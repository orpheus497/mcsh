#!/bin/sh
# t014_unicode_script_source.sh — Unicode variable values in sourced file
#
# Verifies that a script containing multibyte values is parsed and compared
# correctly when sourced via "$MCSH -f script". Variable names themselves
# stay ASCII because tcsh's set/varname grammar restricts names to ASCII
# identifiers; only the values exercise the multibyte path.

. "$(dirname "$0")/lib_locale.sh"

tmpdir=$(mktemp -d /tmp/mcsh_src_XXXXXX) || exit 2
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/script.csh" << 'EOF'
set greeting = "Héllo"
set japanese = "日本語"
if ($greeting != "Héllo") exit 1
if ($japanese != "日本語") exit 1
echo ok
EOF

out=$(LANG="$utf8_locale" LC_ALL="$utf8_locale" "$MCSH" -f "$tmpdir/script.csh" 2>&1)
case "$out" in
    ok) ;;
    *) echo "FAIL: sourced script Unicode: got '$out'"; exit 1 ;;
esac

exit 0
