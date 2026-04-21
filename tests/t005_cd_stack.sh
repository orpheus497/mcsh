#!/bin/sh
# t005_cd_stack.sh — pushd/popd directory stack, cd -1 navigation

tmpdir=$(mktemp -d)
dir1="$tmpdir/d1"
dir2="$tmpdir/d2"
mkdir "$dir1" "$dir2"

out=$("$MCSH" -f -c "pushd $dir1; pushd $dir2; cd -1; echo \$cwd" 2>&1 | tail -1)
expected=$(cd "$dir1" && pwd)

rm -rf "$tmpdir"

if [ "$out" = "$expected" ]; then
    exit 0
else
    echo "expected '$expected', got: $out"
    exit 1
fi
