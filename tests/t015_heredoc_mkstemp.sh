#!/bin/sh

MCSH=$1
if [ -z "$MCSH" ]; then
    MCSH=../mcsh
fi

$MCSH << 'SH_END' > result.tmp 2>&1
cat << END > out.tmp
hello world
this is a heredoc test
END
cat out.tmp
rm out.tmp
SH_END

if grep -q "hello world" result.tmp; then
    rm result.tmp
    # Success
else
    echo "Heredoc test failed"
    cat result.tmp
    rm result.tmp
    test 1 -eq 0
fi
