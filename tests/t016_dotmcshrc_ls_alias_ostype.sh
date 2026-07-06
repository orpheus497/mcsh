#!/bin/sh
# t016_dotmcshrc_ls_alias_ostype.sh — dot.mcshrc picks the `ls` colorization
# alias based on $OSTYPE: GNU `--color=auto` on Linux, BSD `-G` elsewhere.

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
RCFILE="$SCRIPT_DIR/../dot.mcshrc"

if [ ! -r "$RCFILE" ]; then
    printf 'dot.mcshrc not found or not readable at %s\n' "$RCFILE"
    exit 1
fi

# check_alias FORCED_OSTYPE EXPECTED_ALIAS
# $OSTYPE is forced *before* sourcing the rc file so the test is independent
# of the host platform actually running the test suite. dot.mcshrc never
# assigns $OSTYPE itself, only reads it, so the forced value survives into
# the "switch ( \"$OSTYPE\" )" that selects the ls alias. Interactive-only
# section 6 (STRUCTURAL ALIASES) is gated on "if ( \$?prompt )", so prompt
# must be set for the alias block to run at all.
check_alias() {
    forced_ostype=$1
    expected=$2

    out=$("$MCSH" -f -c "set prompt = '% '; set OSTYPE = '$forced_ostype'; source '$RCFILE'; alias ls" 2>&1)
    status=$?
    if [ $status -ne 0 ]; then
        printf 'mcsh exited %d sourcing dot.mcshrc (OSTYPE=%s); output: %s\n' \
            "$status" "$forced_ostype" "$out"
        return 1
    fi

    got=$(printf '%s\n' "$out" | tail -1)
    if [ "$got" != "$expected" ]; then
        printf "OSTYPE=%s: expected ls alias '%s', got: %s\n" \
            "$forced_ostype" "$expected" "$got"
        return 1
    fi
    return 0
}

fail=0

# Linux (and Linux-like, e.g. "linux-gnu") must get the GNU coreutils alias.
check_alias 'linux-forced-test' 'ls -F --color=auto' || fail=1
check_alias 'linux-gnu' 'ls -F --color=auto' || fail=1

# Anything not matching "linux*" (BSD, Darwin, or unknown) falls through to
# the default case and keeps the original BSD-style alias.
check_alias 'FreeBSD' 'ls -F -G' || fail=1
check_alias 'darwin19' 'ls -F -G' || fail=1
check_alias 'generic-unix-test' 'ls -F -G' || fail=1

exit $fail