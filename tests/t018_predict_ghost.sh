#!/bin/sh
# t018_predict_ghost.sh — predictive-autocomplete ghost text is rendered
# through the virtual display (ed.refresh.c VdrawGhost, ed.screen.c so_write),
# not written raw to the terminal.
#
# The original renderer printed the suggestion straight to the terminal after
# Refresh() had positioned the cursor, then erased it again with spaces and
# backspaces.  That worked only while the suggestion stayed on one screen row:
# once it crossed the right margin the result depended on the emulator's
# margin behaviour, and the cell-for-cell differ in update_line() never knew
# the columns were occupied, so stale suggestion text was left behind.
#
# These checks pin the properties that fix depends on:
#
#   1. a suggestion is emitted dim (ECMA-48 SGR 2) on a colour terminal;
#   2. no suggestion at all on a terminal that advertises no colour, since an
#      undimmed suggestion cannot be told apart from what was typed;
#   3. every SGR 2 is closed by an SGR 22, so the rendition never bleeds;
#   4. a suggestion that wraps past the right margin brings the cursor back
#      with a real cursor motion (CUU / CHA).  The old renderer could only
#      emit backspaces, which do not cross a row boundary;
#   5. the suggestion is clamped to the height of the terminal, so it can
#      never scroll the screen away on its own.
#
# Skipped when the driver cannot be built or no pseudo-terminal is available.

. ./lib_pty.sh

pty_setup || exit 77
trap 'pty_cleanup' EXIT INT TERM

fail=0

# capture TERM COLS ROWS  - drive mcsh with $keys and return the terminal byte
# stream with ESC mapped to '@', so it can be matched with a plain POSIX grep.
# The environment is set in a subshell: `env VAR=v func' cannot work, env(1)
# execs a program and pty_run is a shell function.
capture() (
    unset COLORTERM
    TERM=$1; export TERM
    HOME=$PTY_DIR; export HOME
    printf '%s' "$keys" | pty_run "$2" "$3" 2>/dev/null | tr '\033' '@'
)

# --- 1 & 3: a suggestion on a colour terminal is dim, and is closed ---------
keys='set prompt="% "
set predict
set history=50
echo zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
echo z'
out=$(capture xterm-256color 80 24)

n_on=$(printf '%s' "$out" | grep -o '@\[2m' | wc -l)
n_off=$(printf '%s' "$out" | grep -o '@\[22m' | wc -l)
if [ "$n_on" -lt 1 ]; then
    printf 'no dim rendition emitted for a predicted suffix\n'
    fail=1
fi
if [ "$n_on" != "$n_off" ]; then
    printf 'unbalanced dim rendition: %s starts, %s stops\n' "$n_on" "$n_off"
    fail=1
fi

# --- 2: no colour, no ghost ------------------------------------------------
for t in dumb vt100; do
    out=$(capture "$t" 80 24)
    if printf '%s' "$out" | grep -q '@\[2m'; then
        printf 'TERM=%s advertises no colour but ghost text was drawn\n' "$t"
        fail=1
    fi
done

# --- 4: a wrapped suggestion is unwound with a cursor motion ---------------
# 16 columns: "% echo z" leaves 8, and the suggestion is 39 characters, so it
# must occupy the next row as well.
out=$(capture xterm-256color 16 24)
if ! printf '%s' "$out" | grep -q '@\[[0-9]*A'; then
    printf 'a suggestion wrapped past the margin but the cursor was never '
    printf 'moved back up a row\n'
    fail=1
fi

# --- 5: the suggestion never spans more than the screen --------------------
# On a 4-row terminal no cursor-up may exceed 3 rows.
out=$(capture xterm-256color 16 4)
worst=$(printf '%s' "$out" | grep -o '@\[[0-9]*A' | tr -dc '0-9\n' | sort -n |
        tail -1)
[ -z "$worst" ] && worst=0
if [ "$worst" -gt 3 ]; then
    printf 'suggestion spanned %s rows on a 4-row terminal\n' "$worst"
    fail=1
fi

# --- 6: a suggestion that is not drawn is not accepted either -------------
# VdrawGhost() declines to draw on a terminal without colour, but GhostBuf was
# still filled, so predict-accept (right arrow, ^F) inserted a command suffix
# the user had never seen.  predict_from_history() now declines to compute one
# at all without colour, and e_predict_accept() checks the same thing where the
# insertion happens.
#
# The history line sends its own output to /dev/null, so the marker appears
# only where the terminal echoes it: once for the line as typed, and a second
# time only if the right arrow accepted a suggestion.
accept_keys='set prompt="% "
set predict
set history=50
echo AAA-MARKER-BBB > /dev/null
echo A'

count_marker() (
    unset COLORTERM
    TERM=$1; export TERM
    HOME=$PTY_DIR; export HOME
    { printf '%s' "$accept_keys"; printf '\033[C\n'; } |
        pty_run 100 24 2>/dev/null | grep -c 'AAA-MARKER-BBB'
)

for t in dumb vt100; do
    n=$(count_marker "$t")
    if [ "$n" != 1 ]; then
        printf 'TERM=%s draws no ghost text, but the right arrow still '"$t"
        printf 'accepted a suggestion (marker seen %s times, expected 1)\n' "$n"
        fail=1
    fi
done
# On a colour terminal it must still work, or the guard has gone too far.
n=$(count_marker xterm-256color)
if [ "$n" -lt 2 ]; then
    printf 'on a colour terminal the right arrow did not accept the '
    printf 'suggestion (marker seen %s times, expected at least 2)\n' "$n"
    fail=1
fi

exit $fail
