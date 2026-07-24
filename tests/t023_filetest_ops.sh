#!/bin/sh
# t023_filetest_ops.sh — verify filetest operators (-e, -f, -d, etc).

tmpdir=$(mktemp -d) || exit 77
tmpfile=$(mktemp) || exit 77
trap 'rm -rf "$tmpdir" "$tmpfile"' EXIT

out=$("$MCSH" -f -c "
if (-d \"$tmpdir\") echo d_ok
if (-f \"$tmpfile\") echo f_ok
if (-e \"$tmpfile\") echo e_ok
if (! -z \"$tmpfile\") echo z_ok
" 2>&1)

expected="d_ok
f_ok
e_ok
z_ok"

if [ "$out" = "$expected" ]; then
    exit 0
else
    printf 'filetest failed, got:\n%s\n' "$out"
    exit 1
fi
