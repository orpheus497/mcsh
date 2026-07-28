#!/bin/sh
# t017_cd_minus_n.sh — cd -N jumps to Nth entry from bottom of directory stack.
# Build a 3-entry stack, then verify cd -2 and cd -1 land on the right paths.

d0=$(mktemp -d) || exit 77
d1=$(mktemp -d) || exit 77
d2=$(mktemp -d) || exit 77
trap 'rm -rf "$d0" "$d1" "$d2"' EXIT

# Stack after cd+pushd sequence (dirs output, top=0):
#   0  d2  (current)
#   1  d1
#   2  d0
# cd -2 == entry at position 2 from bottom == stack[1] == d1
# cd -1 == entry at position 1 from bottom == stack[2] == d0
out=$("$MCSH" -f << MCSHSCRIPT 2>&1
cd "$d0"
pushd "$d1" >& /dev/null
pushd "$d2" >& /dev/null
cd -2
echo \$cwd
cd -1
echo \$cwd
MCSHSCRIPT
)

expected="$d1
$d0"

if [ "$out" = "$expected" ]; then
    exit 0
else
    printf 'cd -N: expected:\n%s\ngot:\n%s\n' "$expected" "$out"
    exit 1
fi
