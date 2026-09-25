#!/bin/sh
# t020_sysinfo.sh — the `sysinfo' builtin (tc.fetch.c).
#
# The panel is assembled from ordinary file and system calls, with no external
# program involved, and a field that cannot be read is left out rather than
# guessed.  What is checked here is the contract around it: the rows that are
# always obtainable are present, the configuration file and the custom logo do
# what they say, a bad setting is reported rather than silently ignored, and no
# terminal escape sequence is emitted when the output is not a terminal.
#
# Many of the collectors cannot be exercised on every machine — a container has
# no DMI tables, no DRM connectors, no battery and often no swap — and the
# "omit what cannot be read" rule means their absence is correct behaviour, not
# a failure.  Those rows are therefore checked for shape when present and not
# demanded when absent.

. ./lib_pty.sh

fail=0

CFG=$(mktemp -d "${TMPDIR:-/tmp}/t020.XXXXXX") || exit 1
trap 'rm -rf "$CFG"; pty_cleanup 2>/dev/null' EXIT INT TERM
mkdir -p "$CFG/mcsh"

# Every invocation below points XDG_CONFIG_HOME at $CFG, so the developer's own
# ~/.config/mcsh/sysinfo.conf can never change the result of this test.  A
# helper, because forgetting it on one case would make that case unreliable.
run() {
    XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c "$*" 2>&1
}

out=$(run 'sysinfo')
status=$?
if [ $status -ne 0 ]; then
    printf 'sysinfo exited %d; output: %s\n' "$status" "$out"
    exit 1
fi

# Rows that need nothing but uname(2), the password database and the shell's
# own identity, so they must be present on any system that can run this test.
for want in 'Shell' 'Kernel' 'OS'; do
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

plain=$(run 'sysinfo -n')

# The machine type shares the OS row rather than having one of its own, which
# is what every panel of this kind does and what the row is for: which system,
# built for what.
machine=$(uname -m)
osrow=$(printf '%s\n' "$plain" | sed -n 's/^OS  *//p')
case "$osrow" in
    *"$machine") ;;
    *) printf 'the OS row does not end with the machine type %s: [%s]\n' \
           "$machine" "$osrow"
       fail=1 ;;
esac

# Rows composed from more than one formatted fragment must not be truncated.
# fetch_duration() appends by advancing over what xsnprintf() reports it wrote,
# and doprnt() (tc.printf.c) used to under-count every %u/%o/%x/%p conversion
# by its number of digits, so "1 hour, 46 mins" came out as "1 hou, 46 mins".
# The unit words are spelled out in full here so any recurrence is caught.
# Extracted from the logo-less panel, so the rows start at column 0.
up=$(printf '%s\n' "$plain" | sed -n 's/^Uptime  *//p')
if [ -n "$up" ]; then
    if ! printf '%s\n' "$up" | grep -qE \
        '^[0-9]+ (secs|min|mins|hour|hours|day|days)(, [0-9]+ (min|mins|hour|hours))*$'
    then
        printf 'the Uptime row is malformed: [%s]\n' "$up"
        fail=1
    fi
fi

# The three usage rows are built from three fragments and a percentage each.
# Disk carries the filesystem type as well, when /proc/self/mounts names one.
size='[0-9]+(\.[0-9])? (B|KiB|MiB|GiB|TiB|PiB)'
for row in Memory Swap; do
    val=$(printf '%s\n' "$plain" | sed -n "s/^$row  *//p")
    [ -n "$val" ] || continue
    if ! printf '%s\n' "$val" | grep -qE "^$size / $size \([0-9]+%\)\$"; then
        printf 'the %s row is malformed: [%s]\n' "$row" "$val"
        fail=1
    fi
done
val=$(printf '%s\n' "$plain" | sed -n 's/^Disk (\/)  *//p')
if [ -n "$val" ]; then
    if ! printf '%s\n' "$val" | grep -qE \
        "^$size / $size \([0-9]+%\)( - [A-Za-z0-9_.-]+)?\$"
    then
        printf 'the Disk row is malformed: [%s]\n' "$val"
        fail=1
    fi
fi

# The CPU row must not state the clock twice.  Intel writes the nominal clock
# into the model string itself ("... CPU E5-2690 v4 @ 2.60GHz"), and appending
# the one read from cpufreq to that produced "... @ 2.60GHz (8) @ 2.59 GHz".
cpu=$(printf '%s\n' "$plain" | sed -n 's/^CPU  *//p')
if [ -n "$cpu" ]; then
    ats=$(printf '%s\n' "$cpu" | tr -cd '@' | wc -c)
    if [ "$ats" -gt 1 ]; then
        printf 'the CPU row states the clock more than once: [%s]\n' "$cpu"
        fail=1
    fi
fi

# --- the field table --------------------------------------------------------
# -l is the only place the `show' and `hide' field names are enumerated, so it
# has to agree with what those keys actually accept: every name it prints must
# be usable, and a name it does not print must be rejected.
fields=$(run 'sysinfo -l')
if [ -z "$fields" ]; then
    printf 'sysinfo -l printed nothing\n'
    fail=1
fi
for f in $fields; do
    printf 'show = %s\n' "$f" > "$CFG/mcsh/sysinfo.conf"
    err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1 >/dev/null)
    if [ -n "$err" ]; then
        printf 'sysinfo -l lists "%s" but `show = %s'"'"' is refused: %s\n' \
            "$f" "$f" "$err"
        fail=1
    fi
done
# A name that is not a field is reported, in `hide' as well as in `show'.
# `hide = palette' is the case that makes this matter: `palette' is a real
# configuration key, just not a field, so silently ignoring it would leave the
# panel drawing the palette with no explanation.
for key in show hide; do
    printf '%s = nosuchfield\n' "$key" > "$CFG/mcsh/sysinfo.conf"
    err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1 >/dev/null)
    case "$err" in
        *"nosuchfield"*"not a field"*) ;;
        *) printf 'an unknown field name in `%s'"'"' was not reported: [%s]\n' \
               "$key" "$err"
           fail=1 ;;
    esac
done
printf 'hide = palette\n' > "$CFG/mcsh/sysinfo.conf"
err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1 >/dev/null)
case "$err" in
    *palette*"not a field"*) ;;
    *) printf '`hide = palette'"'"' was silently ignored: [%s]\n' "$err"
       fail=1 ;;
esac

# --- the configuration file -------------------------------------------------
# `show' is a layout as well as a filter: the fields come out in the order it
# names them, which is not the table order.
cat > "$CFG/mcsh/sysinfo.conf" <<'EOF'
# a comment, and a blank line, are both ignored
logo = none
show = shell, os
label_width = 14
separator = ": "
palette = off
EOF
conf=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
if [ "$(printf '%s\n' "$conf" | wc -l)" != 2 ]; then
    printf '`show = shell, os'"'"' produced %s rows, not 2:\n%s\n' \
        "$(printf '%s\n' "$conf" | wc -l)" "$conf"
    fail=1
fi
if [ "$(printf '%s\n' "$conf" | sed -n 1p | cut -d: -f1 | sed 's/ *$//')" \
     != Shell ]
then
    printf '`show'"'"' did not put shell first:\n%s\n' "$conf"
    fail=1
fi
# label_width and separator: the label is padded to 14 columns and then ": ".
if ! printf '%s\n' "$conf" | grep -q '^Shell         : mcsh'; then
    printf 'label_width/separator were not applied:\n%s\n' "$conf"
    fail=1
fi
# `logo = none' means no logo, and the built-in one is recognisable.
if printf '%s\n' "$conf" | grep -q '___'; then
    printf '`logo = none'"'"' still drew a logo:\n%s\n' "$conf"
    fail=1
fi
# Whitespace around a value is stripped, so a value that has to begin or end
# with a space is written in double quotes and one layer comes off.  This is the
# only way to give `separator' a trailing space, and the manual and dot.mcshrc
# both now say so, which is why it is pinned here.
printf 'logo = none\nshow = shell\nseparator = ": "\n' \
    > "$CFG/mcsh/sysinfo.conf"
quoted=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
case "$quoted" in
    *": mcsh"*) ;;
    *) printf 'a quoted separator lost its trailing space: [%s]\n' "$quoted"
       fail=1 ;;
esac
printf 'logo = none\nshow = shell\nseparator = : \n' \
    > "$CFG/mcsh/sysinfo.conf"
bare=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
case "$bare" in
    *":mcsh"*) ;;
    *) printf 'an unquoted separator kept trailing whitespace, so the quoting '
       printf 'rule the manual states is wrong: [%s]\n' "$bare"
       fail=1 ;;
esac

# The configuration directory is $XDG_CONFIG_HOME/mcsh only when that variable
# is an absolute path; a relative value is ignored and $HOME/.config is used, as
# the XDG Base Directory Specification requires.  Without this the fallback
# would be untested in either direction: every other case here sets an absolute
# XDG_CONFIG_HOME.
xhome=$(mktemp -d "${TMPDIR:-/tmp}/t020h.XXXXXX") || exit 1
mkdir -p "$xhome/.config/mcsh"
printf 'logo = none\nshow = shell\nlabel_width = 20\n' \
    > "$xhome/.config/mcsh/sysinfo.conf"
rel=$(HOME="$xhome" XDG_CONFIG_HOME=relative/path "$MCSH" -f -c 'sysinfo' \
      2>/dev/null)
if ! printf '%s\n' "$rel" | grep -q '^Shell                mcsh'; then
    printf 'a relative XDG_CONFIG_HOME did not fall back to $HOME/.config:\n'
    printf '%s\n' "$rel"
    fail=1
fi
# And an absolute one does take precedence over $HOME.
printf 'logo = none\nshow = shell\nlabel_width = 30\n' \
    > "$CFG/mcsh/sysinfo.conf"
abs=$(HOME="$xhome" XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
if ! printf '%s\n' "$abs" | grep -q '^Shell                          mcsh'; then
    printf 'an absolute XDG_CONFIG_HOME did not override $HOME/.config:\n'
    printf '%s\n' "$abs"
    fail=1
fi
rm -rf "$xhome"

# `hide' wins over `show', so the two keys need no precedence rule.
printf 'logo = none\nshow = shell, os\nhide = os\n' > "$CFG/mcsh/sysinfo.conf"
conf=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
if printf '%s\n' "$conf" | grep -q '^OS'; then
    printf '`hide'"'"' did not override `show'"'"':\n%s\n' "$conf"
    fail=1
fi

# A setting that cannot be applied is named, with its file and line, on the
# diagnostic output — and does not stop the panel or change what reaches
# standard output.  A configuration file whose typos are silently ignored is a
# configuration file you cannot debug.
cat > "$CFG/mcsh/sysinfo.conf" <<'EOF'
logo = none
show = shell
this line has no equals sign
label_width = 900
palette = maybe
nosuchkey = 1
EOF
err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>&1 >/dev/null)
n=$(printf '%s\n' "$err" | grep -c '^sysinfo: ')
if [ "$n" != 4 ]; then
    printf 'expected 4 configuration diagnostics, got %s:\n%s\n' "$n" "$err"
    fail=1
fi
for lineno in 3 4 5 6; do
    if ! printf '%s\n' "$err" | grep -q ":$lineno: "; then
        printf 'no diagnostic names line %s:\n%s\n' "$lineno" "$err"
        fail=1
    fi
done
good=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
if ! printf '%s\n' "$good" | grep -q '^Shell.*mcsh'; then
    printf 'a bad setting suppressed the panel itself:\n%s\n' "$good"
    fail=1
fi

# A colour key takes SGR parameters, which are digits and ';' and nothing else
# (ECMA-48 5.4).  Anything else is refused, which is what stops a
# configuration file from clearing the screen or moving the cursor: the value
# is interpolated straight into a CSI sequence.
printf 'label_color = 32;1H\n' > "$CFG/mcsh/sysinfo.conf"
err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1 >/dev/null)
case "$err" in
    *'bad setting'*) ;;
    *) printf 'a colour value with a non-digit was accepted: [%s]\n' "$err"
       fail=1 ;;
esac

# --- the custom logo --------------------------------------------------------
# Any text file will do, and the columns to its right stay aligned because the
# width of each row is measured, not assumed: the rows below are 4, 2 and 5
# columns wide, the last of them using two double-width characters, so a
# byte-count or a character-count would both get the alignment wrong.
printf 'AAAA\nBB\nCCCCC\n' > "$CFG/mcsh/logo.txt"
printf 'logo = %s/mcsh/logo.txt\nshow = os, shell, kernel\n' "$CFG" \
    > "$CFG/mcsh/sysinfo.conf"
logo=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
if ! printf '%s\n' "$logo" | grep -q '^AAAA  '; then
    printf 'the custom logo was not drawn:\n%s\n' "$logo"
    fail=1
fi
if printf '%s\n' "$logo" | grep -q '___'; then
    printf 'the built-in logo was drawn instead of the custom one:\n%s\n' \
        "$logo"
    fail=1
fi
# Every row's value must start in the same column.
cols=$(printf '%s\n' "$logo" | sed -n 's/\(.*\)\(OS\|Shell\|Kernel\) .*/\1/p' |
       awk '{print length($0)}' | sort -u | wc -l)
if [ "$cols" != 1 ]; then
    printf 'the rows beside the custom logo are not aligned:\n%s\n' "$logo"
    fail=1
fi

# A row wider in bytes than in columns has to be measured in columns.  The
# width comes from the shell's own model, NLSStringWidth() (tc.nls.c), so it
# is the locale that decides what a byte sequence is worth - which means this
# case needs a UTF-8 locale and is skipped without one.  Under LANG=C those
# bytes are not characters at all and the shell counts each of them as the two
# columns it would take to display it escaped, which is the honest answer for
# that locale and not the one being checked here.
#
# The discriminator is the padding on the row past the bottom of the logo: the
# logo below is two rows of four columns, "AAAA" in four bytes and two
# double-width characters in six, so a correct measurement pads that row to
# four columns and a byte count pads it to six.
utf8=
for l in C.UTF-8 C.utf8 en_US.UTF-8; do
    if locale -a 2>/dev/null | grep -qx "$l"; then
        utf8=$l
        break
    fi
done
if [ -n "$utf8" ]; then
    printf 'AAAA\n\346\274\242\345\255\227\n' > "$CFG/mcsh/logo.txt"
    printf 'logo = %s/mcsh/logo.txt\nshow = os, shell, kernel\n' "$CFG" \
        > "$CFG/mcsh/sysinfo.conf"
    wide=$(XDG_CONFIG_HOME="$CFG" LANG="$utf8" LC_ALL="$utf8" \
           "$MCSH" -f -c 'sysinfo' 2>/dev/null)
    pad=$(printf '%s\n' "$wide" | sed -n 's/Kernel .*//p' | awk '{print length($0)}')
    # four columns of logo plus the two-column gap between logo and rows.
    if [ "$pad" != 6 ]; then
        printf 'a double-width logo row was measured in bytes: the row past '
        printf 'the logo is padded to %s columns, expected 6\n%s\n' \
            "$pad" "$wide"
        fail=1
    fi
fi

# A logo that cannot be read is reported, and the panel falls back to the
# built-in rather than losing the logo silently.
printf 'logo = %s/mcsh/nosuchlogo.txt\nshow = shell\n' "$CFG" \
    > "$CFG/mcsh/sysinfo.conf"
err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>&1 >/dev/null)
case "$err" in
    *'cannot read logo'*) ;;
    *) printf 'an unreadable logo was not reported: [%s]\n' "$err"
       fail=1 ;;
esac
if ! XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null |
     grep -q '___'
then
    printf 'an unreadable logo did not fall back to the built-in one\n'
    fail=1
fi

# A logo that carries its own escape sequences keeps them, and they are still
# counted as zero columns wide.  This is the one place a control character is
# passed through on purpose: it is the user's own file.
printf '\033[35mXX\033[0m\nYYYY\n' > "$CFG/mcsh/logo.txt"
printf 'logo = %s/mcsh/logo.txt\nshow = shell\n' "$CFG" \
    > "$CFG/mcsh/sysinfo.conf"
esc=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -c' 2>/dev/null |
      od -An -tx1 | tr ' ' '\n' | grep -c '^1b$')
if [ "$esc" -lt 2 ]; then
    printf "a logo's own escape sequences did not survive (%s ESC bytes)\n" \
        "$esc"
    fail=1
fi
rm -f "$CFG/mcsh/sysinfo.conf"

# --- escapes and the terminal ----------------------------------------------
# Not a terminal: no colour, no escapes.  A captured or redirected panel has
# to be plain text.
if printf '%s\n' "$out" | grep -q "$(printf '\033')"; then
    printf 'sysinfo emitted an escape sequence when stdout was not a tty\n'
    fail=1
fi

# -c overrides that, which is what makes the colour path testable at all
# without a pty; -C is its opposite and is checked on a pty further down.
n=$(run 'sysinfo -n -c' | od -An -tx1 | tr ' ' '\n' | grep -c '^1b$')
if [ "$n" -lt 2 ]; then
    printf 'sysinfo -c produced no escape sequences (%s ESC bytes)\n' "$n"
    fail=1
fi

# A value collected from outside the shell must not be able to put a control
# character in the output.  fetch_print() writes with output_raw set so the
# panel's own SGR sequences survive, which would otherwise let $TERM through
# verbatim - into a redirected file as readily as onto a terminal.
# Counting escape bytes alone would pass for the wrong reason if the panel
# failed, or left the Terminal row out: the count would be 0 either way.  So
# check the exit status and that the injected value really did reach the row,
# in its sanitised form, and only then that no escape byte survived.
#
# $TERM is what the Terminal row falls back to when no ancestor process is a
# terminal emulator this shell recognises, which is the case under the test
# runner: the parent chain is /bin/sh all the way up.
inj=$(printf 'xterm\033[31mINJECTED')
injout=$(XDG_CONFIG_HOME="$CFG" TERM="$inj" "$MCSH" -f -c 'sysinfo -n' 2>&1)
status=$?
if [ $status -ne 0 ]; then
    printf 'sysinfo exited %d with an escape byte in $TERM; output: %s\n' \
        "$status" "$injout"
    fail=1
elif ! printf '%s\n' "$injout" | grep -qF 'xterm?[31mINJECTED'; then
    printf 'the injected $TERM did not reach the Terminal row in sanitised '
    printf 'form, so the escape-byte check below would prove nothing:\n%s\n' \
        "$(printf '%s\n' "$injout" | sed -n 's/^Terminal  *//p')"
    fail=1
else
    n=$(printf '%s\n' "$injout" | od -An -tx1 | tr ' ' '\n' | grep -c '^1b$')
    if [ "$n" != 0 ]; then
        printf 'an escape byte from $TERM reached the panel (%s found)\n' "$n"
        fail=1
    fi
fi

# -n drops the logo.  The logo's first row is blank but for the ascender of
# the "h", so the reliable marker is the backslash-and-underscore body, which
# appears in no field value.
withlogo=$(run 'sysinfo')
nologo=$(run 'sysinfo -n')
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
    out=$(run "$bad")
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

# Several flags in one invocation.  The builtin table in sh.init.c caps the
# argument count, and it was set to one: `sysinfo -n -c' was rejected as too
# many arguments before dosysinfo() ever saw it.
out=$(run 'sysinfo -n -C')
case "$out" in
    *'Too many'*) printf 'sysinfo -n -C was rejected: %s\n' "$out"; fail=1 ;;
esac

# The variable is what turns the start-up panel on; it must not, by itself,
# make the builtin behave differently.
out=$(run 'set sysinfo; sysinfo -n')
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
    greet_session() (
        unset COLORTERM
        TERM=dumb; export TERM
        HOME=$PTY_DIR; export HOME
        XDG_CONFIG_HOME=$CFG; export XDG_CONFIG_HOME
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

    color_session() {
        ( unset COLORTERM
          TERM=xterm-256color; export TERM
          HOME=$PTY_DIR; export HOME
          XDG_CONFIG_HOME=$CFG; export XDG_CONFIG_HOME
          PTY_SHELL_ARGS='-f -i'
          pty_run 100 30 2>/dev/null )
    }

    tty_esc=$(printf 'sysinfo -n\n' | color_session | esc_count)
    if [ "$tty_esc" -lt 2 ]; then
        printf 'the panel emitted %s escape bytes on a colour terminal; the '\
            "$tty_esc"
        printf 'SGR sequences are not reaching it\n'
        fail=1
    fi

    # -C suppresses that on the very same terminal.  The shell itself emits
    # escapes for its prompt and for the line editor, so the comparison is
    # against the same session running the panel with colour, not against
    # zero.
    off_esc=$(printf 'sysinfo -n -C\n' | color_session | esc_count)
    if [ "$off_esc" -ge "$tty_esc" ]; then
        printf 'sysinfo -C emitted %s escape bytes against %s with colour\n' \
            "$off_esc" "$tty_esc"
        fail=1
    fi

    # `color = off' in the configuration file does the same thing.
    printf 'color = off\n' > "$CFG/mcsh/sysinfo.conf"
    conf_esc=$(printf 'sysinfo -n\n' | color_session | esc_count)
    if [ "$conf_esc" -ge "$tty_esc" ]; then
        printf '`color = off'"'"' emitted %s escape bytes against %s\n' \
            "$conf_esc" "$tty_esc"
        fail=1
    fi
    rm -f "$CFG/mcsh/sysinfo.conf"

    printf 'sysinfo -n > %s/redir.out\n' "$PTY_DIR" | color_session >/dev/null
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
