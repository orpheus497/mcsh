#!/bin/sh
# t020_sysinfo.sh — the `sysinfo' builtin (tc.fetch.c).
#
# The panel is assembled from ordinary file and system calls, with no external
# program involved, and a field that cannot be read is left out rather than
# guessed.  What is checked here is the contract around it: the rows that are
# always obtainable are present, the flag and error handling behave, and no
# terminal escape sequence is emitted when the output is not a terminal.

fail=0

out=$("$MCSH" -f -c 'sysinfo' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'sysinfo exited %d; output: %s\n' "$status" "$out"
    exit 1
fi

# Rows that need nothing but uname(2), the password database and the shell's
# own identity, so they must be present on any system that can run this test.
for want in 'Shell' 'Kernel' 'Arch' 'OS'; do
    if ! printf '%s\n' "$out" | grep -q "$want"; then
        printf 'sysinfo output has no %s row:\n%s\n' "$want" "$out"
        fail=1
    fi
done

# The Shell row names this shell.
if ! printf '%s\n' "$out" | grep -q 'Shell.*mcsh'; then
    printf 'the Shell row does not name mcsh:\n%s\n' "$out"
    fail=1
fi

# Not a terminal: no colour, no escapes.  A captured or redirected panel has
# to be plain text.
if printf '%s\n' "$out" | grep -q "$(printf '\033')"; then
    printf 'sysinfo emitted an escape sequence when stdout was not a tty\n'
    fail=1
fi

# -n drops the logo.  The logo's first row is blank but for the ascender of
# the "h", so the reliable marker is the backslash-and-underscore body, which
# appears in no field value.
withlogo=$("$MCSH" -f -c 'sysinfo' 2>&1)
nologo=$("$MCSH" -f -c 'sysinfo -n' 2>&1)
if ! printf '%s\n' "$withlogo" | grep -q '___'; then
    printf 'the default panel is missing its logo\n'
    fail=1
fi
if printf '%s\n' "$nologo" | grep -q '___'; then
    printf 'sysinfo -n still drew the logo:\n%s\n' "$nologo"
    fail=1
fi
# Dropping the logo must not drop any row.
w=$(printf '%s\n' "$withlogo" | wc -l)
n=$(printf '%s\n' "$nologo" | wc -l)
if [ "$w" != "$n" ]; then
    printf 'sysinfo -n changed the row count: %s with logo, %s without\n' \
        "$w" "$n"
    fail=1
fi

# An unknown flag and a stray operand are both errors, and neither prints a
# panel.
for bad in 'sysinfo -z' 'sysinfo extra'; do
    out=$("$MCSH" -f -c "$bad" 2>&1)
    status=$?
    if [ $status -eq 0 ]; then
        printf '"%s" was accepted\n' "$bad"
        fail=1
    fi
    if printf '%s\n' "$out" | grep -q 'Kernel'; then
        printf '"%s" printed a panel anyway\n' "$bad"
        fail=1
    fi
done

# The variable is what turns the start-up panel on; it must not, by itself,
# make the builtin behave differently.
out=$("$MCSH" -f -c 'set sysinfo; sysinfo -n' 2>&1)
if ! printf '%s\n' "$out" | grep -q 'Shell.*mcsh'; then
    printf 'sysinfo behaved differently with the variable set:\n%s\n' "$out"
    fail=1
fi

exit $fail
