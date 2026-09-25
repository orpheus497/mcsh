#!/bin/sh
# t020_sysinfo.sh — the `sysinfo' builtin (tc.fetch.c).
#
# The panel is assembled from ordinary file and system calls, with no external
# program involved, and a field that cannot be read is left out rather than
# guessed.  What is checked here is the contract around it: the rows that are
# always obtainable are present, the flag and error handling behave, and no
# terminal escape sequence is emitted when the output is not a terminal.

. ./lib_pty.sh

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

# Rows composed from more than one formatted fragment must not be truncated.
# fetch_duration() appends by advancing over what xsnprintf() reports it wrote,
# and doprnt() (tc.printf.c) used to under-count every %u/%o/%x/%p conversion
# by its number of digits, so "1 hour, 46 mins" came out as "1 hou, 46 mins".
# The unit words are spelled out in full here so any recurrence is caught.
# Extracted from the logo-less panel, so the rows start at column 0.
plain=$("$MCSH" -f -c 'sysinfo -n' 2>&1)
up=$(printf '%s\n' "$plain" | sed -n 's/^Uptime  *//p')
if [ -n "$up" ]; then
    if ! printf '%s\n' "$up" | grep -qE \
        '^[0-9]+ (secs|min|mins|hour|hours|day|days)(, [0-9]+ (min|mins|hour|hours))*$'
    then
        printf 'the Uptime row is malformed: [%s]\n' "$up"
        fail=1
    fi
fi

# Same for the two size rows, which are built from three fragments each.
for row in Memory 'Disk (\/)'; do
    val=$(printf '%s\n' "$plain" | sed -n "s/^$row  *//p")
    [ -n "$val" ] || continue
    if ! printf '%s\n' "$val" | grep -qE \
        '^[0-9]+(\.[0-9])? (B|KiB|MiB|GiB|TiB|PiB) / [0-9]+(\.[0-9])? (B|KiB|MiB|GiB|TiB|PiB)$'
    then
        printf 'the %s row is malformed: [%s]\n' "$row" "$val"
        fail=1
    fi
done

# Not a terminal: no colour, no escapes.  A captured or redirected panel has
# to be plain text.
if printf '%s\n' "$out" | grep -q "$(printf '\033')"; then
    printf 'sysinfo emitted an escape sequence when stdout was not a tty\n'
    fail=1
fi

# A value collected from outside the shell must not be able to put a control
# character in the output.  fetch_print() writes with output_raw set so the
# panel's own SGR sequences survive, which would otherwise let $TERM through
# verbatim - into a redirected file as readily as onto a terminal.
inj=$(printf 'xterm\033[31mINJECTED')
out=$(TERM="$inj" "$MCSH" -f -c 'sysinfo -n' 2>/dev/null |
      od -An -tx1 | tr ' ' '\n' | grep -c '^1b$')
if [ "$out" != 0 ]; then
    printf 'an escape byte from $TERM reached the panel (%s found)\n' "$out"
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

# --- the start-up panel ------------------------------------------------------
# Everything above runs the builtin with -c, which sets targinp, and sh.c calls
# sysinfo_greeting() only when `intty && !targinp'.  So none of it covers the
# start-up path at all: it would still pass with sysinfo_greeting() deleted.
# This case needs a terminal and a start-up file, so it is skipped rather than
# failed when the pty driver cannot be built.
if pty_setup; then
    trap 'pty_cleanup' EXIT INT TERM

    greet_session() (
        unset COLORTERM
        TERM=dumb; export TERM
        HOME=$PTY_DIR; export HOME
        PTY_SHELL_ARGS=-i        # not -f: ~/.mcshrc has to be read
        printf 'exit\n' | pty_run 100 24 2>/dev/null | tr -d '\r'
    )

    printf 'set prompt="%%%% "\nset sysinfo\n' > "$PTY_DIR/.mcshrc"
    stream=$(greet_session)
    # One panel, and it must precede the first prompt.
    rows=$(printf '%s\n' "$stream" | grep -c 'Shell.*mcsh')
    if [ "$rows" != 1 ]; then
        printf 'expected exactly one start-up panel, counted %s:\n%s\n' \
            "$rows" "$stream"
        fail=1
    fi
    first=$(printf '%s\n' "$stream" | grep -n -e 'Shell.*mcsh' -e '^%' |
            head -1)
    case "$first" in
        *Shell*) ;;
        *) printf 'the panel did not precede the first prompt: %s\n' "$first"
           fail=1 ;;
    esac

    # And with the variable unset there must be no panel at all.
    printf 'set prompt="%%%% "\n' > "$PTY_DIR/.mcshrc"
    stream=$(greet_session)
    if printf '%s\n' "$stream" | grep -q 'Shell.*mcsh'; then
        printf 'a panel was printed without `set sysinfo'"'"':\n%s\n' "$stream"
        fail=1
    fi

    # --- colour reaches a terminal, and only a terminal ---------------------
    # Two separate failures hid behind each other here.  xputchar() rewrites a
    # control character as "^X" unless output_raw is set, so the panel's SGR
    # sequences were arriving as the literal text "^[[1;36m" and no colour was
    # ever produced; and the tty test was plain isoutatty, which describes
    # SHOUT and not the descriptor a redirection put in place, so a redirected
    # panel still tried to colour itself.  Counting real ESC bytes (0x1b) is
    # the only way to tell those apart - rendering them for display makes the
    # literal and the real form look identical.
    esc_count() { od -An -tx1 | tr ' ' '\n' | grep -c '^1b$'; }

    tty_esc=$(printf 'sysinfo -n\n' |
              ( unset COLORTERM
                TERM=xterm-256color; export TERM
                HOME=$PTY_DIR; export HOME
                PTY_SHELL_ARGS='-f -i'
                pty_run 100 30 2>/dev/null ) | esc_count)
    if [ "$tty_esc" -lt 2 ]; then
        printf 'the panel emitted %s escape bytes on a colour terminal; the '\
            "$tty_esc"
        printf 'SGR sequences are not reaching it\n'
        fail=1
    fi

    printf 'sysinfo -n > %s/redir.out\n' "$PTY_DIR" |
        ( unset COLORTERM
          TERM=xterm-256color; export TERM
          HOME=$PTY_DIR; export HOME
          PTY_SHELL_ARGS='-f -i'
          pty_run 100 30 2>/dev/null ) >/dev/null
    if [ -s "$PTY_DIR/redir.out" ]; then
        n=$(esc_count < "$PTY_DIR/redir.out")
        if [ "$n" != 0 ]; then
            printf 'a panel redirected to a file from an interactive shell '
            printf 'contains %s escape bytes\n' "$n"
            fail=1
        fi
        if grep -q '\^\[' "$PTY_DIR/redir.out"; then
            printf 'a redirected panel contains the literal text "^[":\n'
            head -3 "$PTY_DIR/redir.out"
            fail=1
        fi
    else
        printf 'the redirected panel produced no output at all\n'
        fail=1
    fi
fi

exit $fail
