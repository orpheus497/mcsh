#!/bin/sh
# t017_termcap_color_capability.sh — the interactive syntax highlighter's
# SetSGRColor() is gated on T_CanColor, derived from the terminal's termcap
# Co (max colors) capability, so it only emits ANSI SGR escapes when the
# terminal actually advertises color support (Co >= 8) instead of assuming
# every terminal is ANSI-capable.
#
# This test covers the derivation of T_CanColor/Val(T_Co) from real termcap
# entries and from `settc Co N` overrides, using the `echotc colors` /
# `echotc color` introspection added alongside this test. It does NOT drive
# SetSGRColor()/so_write() themselves — those only run inside the
# interactive line-editor's display path, which batch `-c` invocations
# never reach without a pty. End-to-end verification that no raw escape
# bytes reach the terminal would need pty-driven infrastructure this suite
# doesn't have.

# NOTE: mcsh falls back to $COLORTERM and to colour-suggesting $TERM names when
# the terminfo entry carries no "Co" capability, so every invocation below runs
# with COLORTERM explicitly unset.  Without that this test would pass or fail
# depending on the terminal the developer happened to run it from.
fail=0

# check_term FORCED_TERM EXPECTED_COLOR_YESNO
# Real termcap/terminfo entries: xterm advertises Co=8, dumb/vt100 don't
# advertise color at all (Co absent, tgetnum returns -1).
check_term() {
    forced_term=$1
    expected=$2

    got=$(env -u COLORTERM TERM="$forced_term" "$MCSH" -f -c 'echotc color' 2>&1)
    status=$?
    if [ $status -ne 0 ]; then
        printf 'TERM=%s: mcsh exited %d running echotc color; output: %s\n' \
            "$forced_term" "$status" "$got"
        return 1
    fi
    if [ "$got" != "$expected" ]; then
        printf 'TERM=%s: expected echotc color = %s, got: %s\n' \
            "$forced_term" "$expected" "$got"
        return 1
    fi
    return 0
}

check_term 'xterm' 'yes' || fail=1
check_term 'dumb' 'no' || fail=1
check_term 'vt100' 'no' || fail=1

# check_settc_co VALUE EXPECTED_COLOR_YESNO
# settc directly overrides the Co capability; T_CanColor must be recomputed
# immediately from that override (no termcap re-fetch involved), so the very
# next echotc call in the same session reflects it.
check_settc_co() {
    value=$1
    expected=$2

    got=$(env -u COLORTERM TERM='xterm' "$MCSH" -f -c "settc Co $value; echotc color" 2>&1)
    status=$?
    if [ $status -ne 0 ]; then
        printf 'settc Co %s: mcsh exited %d; output: %s\n' \
            "$value" "$status" "$got"
        return 1
    fi
    if [ "$got" != "$expected" ]; then
        printf 'settc Co %s: expected echotc color = %s, got: %s\n' \
            "$value" "$expected" "$got"
        return 1
    fi
    return 0
}

# Boundary: T_CanColor = (Val(T_Co) >= 8).
check_settc_co 7 'no' || fail=1
check_settc_co 8 'yes' || fail=1

# settc must take effect immediately within the same session, in both
# directions, without needing to reload terminal capabilities.
out=$(env -u COLORTERM TERM='dumb' "$MCSH" -f -c 'settc Co 7; echotc color; settc Co 8; echotc color' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'settc Co 7 then 8: mcsh exited %d; output: %s\n' "$status" "$out"
    fail=1
elif [ "$out" != "no
yes" ]; then
    printf 'settc Co 7 then 8: expected "no\\nyes", got: %s\n' "$out"
    fail=1
fi

exit $fail
