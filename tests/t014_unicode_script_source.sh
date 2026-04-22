#!/bin/sh
# t014_unicode_script_source.sh — Unicode variable names/values in sourced file

if ! locale -a 2>/dev/null | grep -qi "UTF-8\|utf8"; then
    echo "SKIP: no UTF-8 locale available"
    exit 0
fi

tmpdir=$(mktemp -d /tmp/mcsh_src_XXXXXX) || exit 2
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/script.csh" << 'EOF'
set greeting = "Héllo"
set japanese = "日本語"
if ($greeting != "Héllo") exit 1
if ($japanese != "日本語") exit 1
echo ok
EOF

out=$(LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 "$MCSH" -f "$tmpdir/script.csh" 2>&1)
case "$out" in
    ok) ;;
    *) echo "FAIL: sourced script Unicode: got '$out'"; exit 1 ;;
esac

exit 0
