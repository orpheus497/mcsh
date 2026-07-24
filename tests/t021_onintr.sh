#!/bin/sh
# t021_onintr.sh — verify SIGINT in a script jumps to onintr label.

script=$(mktemp) || exit 77
trap 'rm -f "$script"' EXIT

cat > "$script" << 'EOF'
onintr catch
kill -INT $$
echo "failed to jump"
exit 1
catch:
echo "jumped"
exit 0
EOF

out=$("$MCSH" -f "$script" 2>&1)
status=$?

if [ "$out" = "jumped" ] && [ $status -eq 0 ]; then
    exit 0
else
    printf 'onintr failed. expected "jumped" and exit 0, got:\n%s\nstatus: %d\n' "$out" "$status"
    exit 1
fi
