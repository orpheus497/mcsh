#!/bin/sh
# mcsh regression test runner
# Usage: sh run_tests.sh [path-to-mcsh]
#
# Each t*.sh script prints PASS or FAIL and exits 0 on pass, 1 on fail.

MCSH="${1:-../mcsh}"

if [ ! -x "$MCSH" ]; then
    echo "ERROR: mcsh binary not found at $MCSH"
    echo "Build first with: make -C .. -j4"
    exit 2
fi

export MCSH
pass=0
fail=0
total=0

for t in t*.sh; do
    total=$((total + 1))
    result=$(sh "$t" 2>&1)
    status=$?
    if [ $status -eq 0 ]; then
        echo "PASS  $t"
        pass=$((pass + 1))
    else
        echo "FAIL  $t"
        if [ -n "$result" ]; then
            echo "      $result" | head -5
        fi
        fail=$((fail + 1))
    fi
done

echo ""
echo "Results: $pass passed, $fail failed out of $total tests"
[ $fail -eq 0 ]
