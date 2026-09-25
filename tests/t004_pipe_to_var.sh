#!/bin/sh
# t004_pipe_to_var.sh — reading a variable's value from standard input.
#
# `set var < file' assigns the first line of the file to var.  Reading has to
# be asked for explicitly: it used to be inferred from standard input not being
# a terminal, which redefined the plain `set var' idiom in every script,
# every `mcsh -c ...' and every start-up file sourced with redirected input.
# There `set var' did not set the variable at all - it read a line from
# whatever standard input happened to be, and blocked indefinitely when that
# was an open pipe or socket with nothing in it yet.
#
# The cases below pin both halves: the redirection form works, and the bare
# form is an ordinary assignment that never touches standard input.

fail=0
tmp=$(mktemp -d) || exit 77
trap 'rm -rf "$tmp"' EXIT INT TERM

printf 'bar\nsecond\n' > "$tmp/one"
: > "$tmp/empty"

# --- set var < file: the first line, and nothing after it ------------------
out=$("$MCSH" -f -c "set y < $tmp/one; echo \"[\$y]\"" < /dev/null 2>&1)
if [ "$out" != '[bar]' ]; then
    printf 'set y < file: expected [bar], got: %s\n' "$out"
    fail=1
fi

# --- an empty file yields an empty value, not an error ---------------------
out=$("$MCSH" -f -c "set e < $tmp/empty; echo \"[\$e]\"" < /dev/null 2>&1)
if [ "$out" != '[]' ]; then
    printf 'set e < empty-file: expected [], got: %s\n' "$out"
    fail=1
fi

# --- a subscript is assigned in place -------------------------------------
# Note the asymmetry, which predates this test and is left as it is: the plain
# form stops at the first newline, while the subscripted form takes the whole
# of standard input, trailing newline included.  That embedded newline is why
# the value is not compared inside csh double quotes - a variable holding one
# cannot be substituted there at all ("Unmatched '"'").
printf 'bar\n' > "$tmp/single"
out=$("$MCSH" -f -c "set a=(1 2); set a[1] < $tmp/single; echo n=\$#a e2=\$a[2]" \
      < /dev/null 2>&1)
if [ "$out" != 'n=2 e2=2' ]; then
    printf 'set a[1] < file: expected "n=2 e2=2", got: %s\n' "$out"
    fail=1
fi
out=$("$MCSH" -f -c "set a=(1 2); set a[1] < $tmp/single; echo \$a[1]" \
      < /dev/null 2>&1)
case "$out" in
    bar*) ;;
    *) printf 'set a[1] < file: element 1 is not the file text: %s\n' "$out"
       fail=1 ;;
esac

# --- a bare `set var' must not read standard input -------------------------
# The pipe is deliberately left open with nothing in it until well after the
# shell should have finished.  Before this was fixed the shell blocked here
# until the timeout, which is what made `set syntax' in a start-up file hang a
# non-interactive shell.
out=$( { (sleep 5; echo late) | "$MCSH" -f -c 'set v; echo "[$v]"'; } 2>&1 )
if [ "$out" != '[]' ]; then
    printf 'bare set read standard input: expected [], got: %s\n' "$out"
    fail=1
fi

# --- and neither does it when standard input is a terminal ----------------
# (covered by the case above for the blocking half; here only the value.)
out=$("$MCSH" -f -c 'set v; echo "[$v]"' < /dev/null 2>&1)
if [ "$out" != '[]' ]; then
    printf 'bare set with empty stdin: expected [], got: %s\n' "$out"
    fail=1
fi

# --- the pipeline form reads, but in a child --------------------------------
# Every csh runs the stages of a pipeline in child processes, so a variable
# `set' in the last stage belongs to that child and is gone when it exits.
# Asserted here so the limitation is recorded rather than assumed.
out=$("$MCSH" -f -c 'echo foo | set x; echo "[$x]"' < /dev/null 2>&1)
if [ "$out" != '[]' ]; then
    printf 'echo foo | set x: expected [] in the parent, got: %s\n' "$out"
    fail=1
fi

exit $fail
