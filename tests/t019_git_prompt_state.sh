#!/bin/sh
# t019_git_prompt_state.sh — the %G and %v prompt escapes report the
# repository as it is *now*, on the prompt that follows the command that
# changed it.
#
# tprintf() (tc.prompt.c) runs once per prompt, which is once per command, so
# a wall-clock throttle on the staleness poll does not save repeated work
# inside a prompt: it only withholds the result of the command just run.  The
# default poll interval is therefore 0, and $GIT_POLL_INTERVAL raises it for
# anyone who wants the old behaviour.  Both halves are checked here.
#
# Skipped when the pty driver cannot be built, or when git(1) is absent.

. ./lib_pty.sh

command -v git >/dev/null 2>&1 || exit 77
pty_setup || exit 77

REPO=
ANC=
cleanup() {
    [ -n "$REPO" ] && rm -rf "$REPO"
    [ -n "$ANC" ] && rm -rf "$ANC"
    pty_cleanup
}
trap cleanup EXIT INT TERM

REPO=$(mktemp -d) || exit 77
(
    cd "$REPO" || exit 1
    git init -q -b main . &&
    git config user.email mcsh@test &&
    git config user.name mcsh &&
    echo one > a.txt &&
    git add a.txt &&
    git commit -qm one &&
    git checkout -q -b feature &&
    git checkout -q main
) >/dev/null 2>&1 || exit 77

fail=0

# run_session INTERVAL -> the terminal stream, CRs removed, so that each
# echoed command line is followed by the next prompt on the line after it.
run_session() (
    unset COLORTERM
    TERM=dumb; export TERM
    HOME=$PTY_DIR; export HOME
    if [ -n "$1" ]; then
        GIT_POLL_INTERVAL=$1; export GIT_POLL_INTERVAL
    else
        unset GIT_POLL_INTERVAL
    fi
    printf '%s' "set prompt=\"<%G|%v>% \"
cd $REPO
git checkout -q feature
echo dirty >> a.txt
git checkout -q -- a.txt
" | pty_run 100 24 2>/dev/null | tr -d '\r'
)

# follows COMMAND EXPECTED-PROMPT-PREFIX
# Assert that the prompt printed immediately after COMMAND was echoed starts
# with EXPECTED-PROMPT-PREFIX.  Anchoring on the *next* line is what makes
# this a test of immediacy rather than of eventual correctness.
follows() {
    printf '%s\n' "$stream" |
    awk -v cmd="$1" -v want="$2" '
        hit { print (index($0, want) == 1) ? "yes" : "no"; exit }
        index($0, cmd) > 0 { hit = 1 }
        END { if (!hit) print "missing" }'
}

stream=$(run_session '')
for pair in 'git checkout -q feature|<feature|>' \
            'echo dirty >> a.txt|<feature|*1>' \
            'git checkout -q -- a.txt|<feature|>'; do
    cmd=${pair%%|*}
    want=${pair#*|}
    got=$(follows "$cmd" "$want")
    if [ "$got" != yes ]; then
        printf 'after "%s" the next prompt was not "%s" (%s)\n' \
            "$cmd" "$want" "$got"
        fail=1
    fi
done

# With the throttle raised the branch change must NOT have been picked up by
# the next prompt: that is the behaviour $GIT_POLL_INTERVAL exists to restore,
# and checking it keeps the default from being silently re-introduced.
# The session above left the repository on `feature'; put it back first, so
# that what is measured is the throttle and not the leftover state.
git -C "$REPO" checkout -q main >/dev/null 2>&1 || exit 77
stream=$(run_session 3600)
got=$(follows 'git checkout -q feature' '<main|')
if [ "$got" != yes ]; then
    printf 'GIT_POLL_INTERVAL=3600 did not throttle the staleness poll (%s)\n' \
        "$got"
    fail=1
fi

# --- a repository that appears at an ancestor, without changing directory ---
# The cached "not a repository" path used to probe only $cwd/.git, so a
# repository created at an ancestor while the shell stayed put was never
# noticed; it now re-runs the whole detection walk.  Nothing here changes
# directory after the first prompt, so this is the only case that covers it.
ANC=$(mktemp -d) || exit 77
mkdir -p "$ANC/sub" || exit 77	# removed by cleanup(), whatever happens next

anc_session() (
    unset COLORTERM
    unset GIT_POLL_INTERVAL
    TERM=dumb; export TERM
    HOME=$PTY_DIR; export HOME
    printf '%s' "set prompt=\"<%G>% \"
cd $ANC/sub
git init -q -b probe $ANC
echo still-here
" | pty_run 100 24 2>/dev/null | tr -d '\r'
)
stream=$(anc_session)

# Before: a prompt with no branch.  After: the branch of the new ancestor
# repository, on the very next prompt and with no cd in between.
got=$(follows "cd $ANC/sub" '<>')
if [ "$got" != yes ]; then
    printf 'in a plain directory the branch was not empty (%s)\n' "$got"
    fail=1
fi
got=$(follows "git init -q -b probe" '<probe>')
if [ "$got" != yes ]; then
    printf 'a repository created at an ancestor was not detected on the next '
    printf 'prompt (%s)\n' "$got"
    fail=1
fi

exit $fail
