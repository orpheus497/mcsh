#!/bin/sh
# t015_dotmcshrc_ls_colors.sh — dot.mcshrc must export LS_COLORS for GNU/Linux
# `ls --color` support, in addition to the pre-existing BSD LSCOLORS mapping.

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
RCFILE="$SCRIPT_DIR/../dot.mcshrc"

if [ ! -r "$RCFILE" ]; then
    printf 'dot.mcshrc not found or not readable at %s\n' "$RCFILE"
    exit 1
fi

expected='di=1;34:ln=1;36:so=1;35:pi=1;33:ex=1;32:bd=1;34;46:cd=1;34;43:su=30;41:sg=30;46:tw=30;42:ow=30;43'

# Source the rc file non-interactively (-f skips the user's normal startup
# files so we control exactly what gets loaded) and print $LS_COLORS.
out=$("$MCSH" -f -c "source '$RCFILE'; echo \$LS_COLORS" 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d sourcing dot.mcshrc; output: %s\n' "$status" "$out"
    exit 1
fi

got=$(printf '%s\n' "$out" | tail -1)
if [ "$got" != "$expected" ]; then
    printf "expected LS_COLORS='%s', got: %s\n" "$expected" "$got"
    exit 1
fi

# The pre-existing BSD LSCOLORS mapping and CLICOLOR toggle must still be
# set alongside the new LS_COLORS variable (regression: new var must not
# replace or clobber the existing ones).
out=$("$MCSH" -f -c "source '$RCFILE'; echo \$CLICOLOR:\$LSCOLORS" 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d sourcing dot.mcshrc; output: %s\n' "$status" "$out"
    exit 1
fi

got=$(printf '%s\n' "$out" | tail -1)
expected='1:ExGxFxDxCxEgEdxbxgxcxd'
if [ "$got" != "$expected" ]; then
    printf "expected CLICOLOR:LSCOLORS='%s', got: %s\n" "$expected" "$got"
    exit 1
fi

exit 0