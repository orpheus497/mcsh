#!/bin/sh
# t015_dot_mcshrc_ls_colors.sh — dot.mcshrc must export LS_COLORS (GNU/Linux
# color mapping) in addition to the pre-existing BSD LSCOLORS variable, so
# that GNU coreutils `ls --color` picks up the same color scheme as native
# FreeBSD `ls -G`.

DOTMCSHRC="$(dirname "$0")/../dot.mcshrc"
if [ ! -r "$DOTMCSHRC" ]; then
    echo "SKIP: dot.mcshrc not found at $DOTMCSHRC"
    exit 77
fi

tmphome=$(mktemp -d /tmp/mcsh_lscolors_XXXXXX) || exit 2
trap 'rm -rf "$tmphome"' EXIT INT TERM

expected='di=1;34:ln=1;36:so=1;35:pi=1;33:ex=1;32:bd=1;34;46:cd=1;34;43:su=30;41:sg=30;46:tw=30;42:ow=30;43'

out=$(HOME="$tmphome" "$MCSH" -f -c "source '$DOTMCSHRC'; echo \$LS_COLORS" 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'FAIL: mcsh exited %d sourcing dot.mcshrc; output: %s\n' "$status" "$out"
    exit 1
fi

if [ "$out" != "$expected" ]; then
    printf 'FAIL: expected LS_COLORS=%s, got: %s\n' "$expected" "$out"
    exit 1
fi

# Sourcing must also leave the pre-existing BSD variables intact.
out2=$(HOME="$tmphome" "$MCSH" -f -c "source '$DOTMCSHRC'; echo \$?LS_COLORS \$?LSCOLORS \$?CLICOLOR" 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'FAIL: mcsh exited %d checking related vars; output: %s\n' "$status" "$out2"
    exit 1
fi
case "$out2" in
    "1 1 1") exit 0 ;;
    *) printf 'FAIL: expected LS_COLORS/LSCOLORS/CLICOLOR all defined (1 1 1), got: %s\n' "$out2"; exit 1 ;;
esac