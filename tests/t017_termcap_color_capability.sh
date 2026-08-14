#!/bin/sh
# t017_termcap_color_capability.sh — the interactive syntax highlighter's
# SetSGRColor() must only emit ANSI SGR escapes when the terminal's termcap
# entry actually advertises color support (Co capability >= 8), instead of
# assuming every terminal is ANSI-capable. T_CanColor (derived from Co in
# GetTermCaps(), and re-derived by settc without needing a fresh termcap
# lookup) gates that. `echotc colors` / `echotc color` expose Val(T_Co) and
# T_CanColor respectively so this can be checked without a pty.

fail=0

# check_term FORCED_TERM EXPECTED_COLOR_YESNO
# Real termcap/terminfo entries: xterm advertises Co=8, dumb/vt100 don't
# advertise color at all (Co absent, tgetnum returns -1).
check_term() {
    forced_term=$1
    expected=$2

    got=$(env TERM="$forced_term" "$MCSH" -f -c 'echotc color' 2>&1)
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

    got=$(env TERM='xterm' "$MCSH" -f -c "settc Co $value; echotc color" 2>&1)
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
out=$(env TERM='dumb' "$MCSH" -f -c 'settc Co 7; echotc color; settc Co 8; echotc color' 2>&1)
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
