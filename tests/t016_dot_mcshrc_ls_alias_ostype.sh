#!/bin/sh
# t016_dot_mcshrc_ls_alias_ostype.sh — dot.mcshrc's interactive `ls` alias
# must switch on $OSTYPE: any value starting with "linux" gets the GNU
# coreutils form (`ls -F --color=auto`), everything else keeps the native
# BSD form (`ls -F -G`). The switch pattern is case-sensitive, so an
# OSTYPE of "Linux" (capital L) must NOT match "linux*" and must fall
# through to the default (BSD) branch.
#
# The interactive-only section of dot.mcshrc (which defines the `ls`
# alias) is guarded by `if ( $?prompt )`, so $prompt is set before
# sourcing to exercise that branch, exactly as a real interactive shell
# would.

DOTMCSHRC="$(dirname "$0")/../dot.mcshrc"
if [ ! -r "$DOTMCSHRC" ]; then
    echo "SKIP: dot.mcshrc not found at $DOTMCSHRC"
    exit 77
fi

tmphome=$(mktemp -d /tmp/mcsh_lsalias_XXXXXX) || exit 2
trap 'rm -rf "$tmphome"' EXIT INT TERM

# check_ls_alias OSTYPE_VALUE EXPECTED_ALIAS LABEL
check_ls_alias() {
    ostype=$1
    expected=$2
    label=$3

    out=$(HOME="$tmphome" "$MCSH" -f -c \
        "set prompt = '%'; setenv OSTYPE '$ostype'; source '$DOTMCSHRC'; alias ls" 2>&1)
    status=$?
    if [ $status -ne 0 ]; then
        printf 'FAIL [%s]: mcsh exited %d; output: %s\n' "$label" "$status" "$out"
        exit 1
    fi
    if [ "$out" != "$expected" ]; then
        printf 'FAIL [%s]: OSTYPE=%s expected alias ls -> "%s", got: "%s"\n' \
            "$label" "$ostype" "$expected" "$out"
        exit 1
    fi
}

# Values whose 2nd-switch (BSD stty setup) case labels are NOT matched, so
# the assertions below exercise only the `ls` alias switch in isolation.
check_ls_alias "linux-gnu" "ls -F --color=auto" "linux-gnu selects GNU form"
check_ls_alias "linux"     "ls -F --color=auto" "bare 'linux' selects GNU form"
check_ls_alias "solaris"   "ls -F -G"           "unmatched OSTYPE falls to default BSD form"
check_ls_alias "Linux"     "ls -F -G"           "capitalized 'Linux' is case-sensitive, falls to default"

exit 0