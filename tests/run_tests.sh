#!/bin/sh
# mcsh regression test runner
# Usage: sh run_tests.sh [path-to-mcsh]
#
# Each t*.sh script exits 0 on pass, 77 on skip, and non-zero on fail.
# It may emit an optional failure message on stdout/stderr for display.

# Change into the directory containing this script so that the t*.sh glob
# works regardless of where the runner is invoked from.
SCRIPT_DIR=$(dirname "$0")
cd "$SCRIPT_DIR" || { printf 'ERROR: cannot cd to %s\n' "$SCRIPT_DIR"; exit 2; }

MCSH="${1:-../mcsh}"

if [ ! -x "$MCSH" ]; then
    printf 'ERROR: mcsh binary not found at %s\n' "$MCSH"
    printf 'Build first with: make -C .. -j4\n'
    exit 2
fi

export MCSH
pass=0
fail=0
skip=0
total=0

# Guard against no test files matching the glob
set -- t*.sh
if [ ! -e "$1" ]; then
    printf 'No test scripts found (t*.sh)\n'
    exit 1
fi

for t in "$@"; do
    total=$((total + 1))
    result=$(sh "$t" 2>&1)
    status=$?
    if [ $status -eq 0 ]; then
        printf 'PASS  %s\n' "$t"
        pass=$((pass + 1))
    elif [ $status -eq 77 ]; then
        printf 'SKIP  %s\n' "$t"
        if [ -n "$result" ]; then
            printf '      (%s)\n' "$result" | head -1
        fi
        skip=$((skip + 1))
    else
        printf 'FAIL  %s\n' "$t"
        if [ -n "$result" ]; then
            printf '%s\n' "$result" | head -5
        fi
        fail=$((fail + 1))
    fi
done

printf '\nResults: %d passed, %d failed, %d skipped out of %d tests\n' \
    "$pass" "$fail" "$skip" "$total"
[ $fail -eq 0 ]
