#!/bin/sh
# t019_git_prompt.sh — smoke test for git prompt escapes
# Ensures %g and %G can be parsed in the prompt without crashing.

out=$("$MCSH" -f -c 'set prompt="%g%G"; echo $?prompt' 2>&1)
status=$?

if [ $status -ne 0 ]; then
    printf 'git prompt failed: %s\n' "$out"
    exit 1
fi

if [ "$out" = "1" ]; then
    exit 0
else
    printf 'expected prompt=1, got: %s\n' "$out"
    exit 1
fi
