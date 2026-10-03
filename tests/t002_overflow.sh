#!/bin/sh
# t002_overflow.sh — left-shift overflow must not invoke UB (uses unsigned path)
# @ x = (1 << 31) must produce 2147483648, not a negative number or crash.

out=$("$MCSH" -f -c '@ x = (1 << 31); echo $x' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'mcsh exited %d; output: %s\n' "$status" "$out"
    exit 1
fi
case "$out" in
    2147483648) ;;
    *)          printf "expected 2147483648, got: %s\n" "$out"; exit 1 ;;
esac

# Past either end of the range, arithmetic wraps, as bash's does; 1 << 63 is
# README's own example.  The most negative value used to print as "-8" (gcc
# -O2), "-(" (-O0) or correctly (clang -O2), because putn() negated it in
# signed arithmetic - and every parenthesised result passes through putn(), so
# the wrong value then fed whatever came next: (1 << 63) + 1 gave -7.
check() {
    out=$("$MCSH" -f -c "$1; echo \$x" 2>&1)
    status=$?
    if [ $status -ne 0 ] || [ "$out" != "$2" ]; then
        printf '%s: expected %s, got [%s] (exit %d)\n' "$1" "$2" "$out" "$status"
        exit 1
    fi
}
min=-9223372036854775808
max=9223372036854775807
check '@ x = (1 << 63)'                  $min
check '@ x = -9223372036854775808'       $min
check '@ x = (1 << 63) + 1'              -9223372036854775807
check '@ x = 9223372036854775807 + 1'    $min
check '@ x = -9223372036854775808 - 1'   $max
check '@ x = 4611686018427387904 * 2'    $min

# The minimum divided by -1 has no representable answer, and the hardware
# division traps: the shell was killed by SIGFPE.  It wraps like the rest, and
# x % -1 is 0 for every x.
check '@ x = -9223372036854775808 / -1'  $min
check '@ x = -9223372036854775808 % -1'  0
check '@ x = 7 / -1'                     -7
check '@ x = 7 % -1'                     0

exit 0
