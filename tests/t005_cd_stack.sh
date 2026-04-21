#!/bin/sh
# t005_cd_stack.sh — pushd/popd directory stack, cd -1 navigation

tmpdir=$(mktemp -d)
dir1="$tmpdir/d1"
dir2="$tmpdir/d2"

# Ensure tmpdir is removed even on early exit or interrupt
trap 'rm -rf "$tmpdir"' EXIT INT TERM

mkdir "$dir1" "$dir2"

# Resolve symlinks on both sides so /tmp vs /private/tmp mismatches don't fail
out=$("$MCSH" -f -c "pushd $dir1; pushd $dir2; cd -1; echo \$cwd" 2>&1 | tail -1)
expected=$(cd "$dir1" && pwd -P)
# Also canonicalize the mcsh output in case $cwd contains a symlink prefix
out_canon=$(cd "$out" 2>/dev/null && pwd -P)
if [ -n "$out_canon" ]; then
    out="$out_canon"
fi

if [ "$out" = "$expected" ]; then
    exit 0
else
    printf "expected '%s', got: %s\n" "$expected" "$out"
    exit 1
fi
