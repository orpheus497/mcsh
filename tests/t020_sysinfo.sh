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

# --- which layout did the panel choose? -------------------------------------
# Nothing below looks for a piece of the drawing.  This test used to grep for
# three underscores in five places, which were part of the body of the old
# wordmark, so what it actually tested was the art: redrawing the logo broke
# it, and a redrawn logo is not a regression.  What the panel promises is a
# layout, and the layout is legible in the shape of the output on its own.
#
#   panel_layout OUT LABEL
#       prints `KIND COLUMN' for the row block that LABEL's row belongs to:
#
#       side    the rows are indented, so a logo is drawn to their left, and
#               COLUMN is that logo's width plus the two-column gap
#       stack   the rows are flush left but lines come before them, so a logo
#               is drawn above them
#       none    the rows are flush left and begin on the first line: no logo
#       missing there is no such row at all, which is a failure in itself
#
# COLUMN is read from where LABEL sits on its own row, because beside a logo
# every row is pushed right by the same amount, and that amount is the one
# thing the three layouts disagree about.  Telling a logo line from a panel
# line needs no more than whether it holds a letter or a digit: every field
# row holds one in its label, and the only art ever drawn above the rows is
# the built-in wordmark, which is punctuation throughout.  A logo of the
# user's own is never stacked (tc.fetch.c, fetch_logo_layout()), so the one
# case where the art could hold a letter is a case where COLUMN settles it.
panel_layout() {
    printf '%s\n' "$1" |
    awk -v lab="$2" '
        BEGIN { col = -1 }
        /[0-9A-Za-z]/ { if (text == 0) text = NR }
        col < 0 {
            p = index($0, lab)
            if (p > 0 && (p == 1 || substr($0, p - 1, 1) == " ") &&
                substr($0, p + length(lab), 1) == " ")
                col = p - 1
        }
        END {
            if (col < 0)
                print "missing -1"
            else if (col > 0)
                print "side " col
            else if (text > 1)
                print "stack " col
            else
                print "none " col
        }'
}

#   panel_labels OUT COLUMN
#       prints the label of every field row of OUT, one per line, reading each
#       line from COLUMN on so that it works beside a logo as well as beneath
#       one.  A label is cut off by the two columns that always separate it
#       from its value - `label_width' is 9 and the widest label is the eight
#       of `Disk (/)', then the separator - and a line whose first character
#       at COLUMN is neither a letter nor a digit is not a field row at all,
#       which is what leaves out the logo, the blank line under a stacked one
#       and the rule beneath the title.
panel_labels() {
    printf '%s\n' "$1" |
    awk -v col="$2" '
        {
            s = substr($0, col + 1)
            p = index(s, "  ")
            if (p > 1 && substr(s, 1, 1) ~ /[0-9A-Za-z]/)
                print substr(s, 1, p - 1)
        }'
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

# The Memory row needs both figures.  An available figure that could not be
# read used to be taken as zero, so the row reported all of memory in use -
# "X / X (100%)", which FreeBSD, with no _SC_AVPHYS_PAGES in its sysconf(),
# showed on every machine.  A real reading can be 100% under memory pressure,
# so the percentage proves nothing; the case is reproduced with controlled
# input instead: a /proc/meminfo of the test's own, bind-mounted over the real
# one in a private user and mount namespace that nothing else sees.  Without
# one of those - no unshare(1), unprivileged namespaces disabled - the check
# is skipped, and says so on the diagnostic output; on a system with no
# /proc/meminfo at all it does not apply, and stays quiet.
#
# With both figures the row is exactly what they make, which also shows the
# panel read the controlled file; without the available one there is no row.
meminfo_panel() {
    printf '%b' "$1" > "$CFG/meminfo"
    unshare --user --map-root-user --mount sh -c '
        mount --bind "$1" /proc/meminfo && cmp -s "$1" /proc/meminfo &&
        echo MOUNTED && XDG_CONFIG_HOME="$2" "$3" -f -c "sysinfo -n"
    ' sh "$CFG/meminfo" "$CFG" "$MCSH" 2>/dev/null
}
meminfo_skipped() {
    printf 't020: skipped the Memory row check: no private /proc/meminfo could '
    printf 'be mounted (it needs unshare(1) and unprivileged user namespaces)\n'
} >&2
if [ -r /proc/meminfo ]; then
    both=$(meminfo_panel 'MemTotal: 16000000 kB\nMemAvailable: 4000000 kB\n')
    case "$both" in
        MOUNTED*)
            mem=$(printf '%s\n' "$both" | sed -n 's/^Memory  *//p')
            if [ "$mem" != '11.4 GiB / 15.2 GiB (75%)' ]; then
                printf 'from a MemTotal of 16000000 kB and a MemAvailable of '
                printf '4000000 kB, the Memory row is [%s]\n' "$mem"
                fail=1
            fi
            noavail=$(meminfo_panel 'MemTotal: 16000000 kB\n')
            case "$noavail" in
                MOUNTED*)
                    if printf '%s\n' "$noavail" | grep -q '^Memory'; then
                        printf 'a Memory row with no available figure: [%s]\n' \
                            "$(printf '%s\n' "$noavail" |
                               sed -n 's/^Memory  *//p')"
                        fail=1
                    fi ;;
                *) meminfo_skipped ;;
            esac ;;
        *) meminfo_skipped ;;
    esac
fi

# /var/db/pkg is FreeBSD pkg(8)'s database directory, and pkgsrc's and
# OpenBSD's, as well as Portage's: a Portage count belongs only on a system
# that has Portage.
if [ ! -d /etc/portage ]; then
    case "$plain" in
        *'(portage)'*)
            printf 'a Portage count was reported without /etc/portage: [%s]\n' \
                "$(printf '%s\n' "$plain" | sed -n 's/^Packages  *//p')"
            fail=1 ;;
    esac
fi

# rpm's and pkg(8)'s databases are SQLite, read only by a shell built with it,
# which says so in $version.  Built with it, the rpm count is the one
# `rpm -qa' gives, gpg-pubkey entries included; built without it, no count
# that only a database could give may appear at all.
case $("$MCSH" -f -c 'echo $version' 2>/dev/null) in
    *sqlite*)
        if command -v rpm >/dev/null 2>&1 &&
           { [ -f /usr/lib/sysimage/rpm/rpmdb.sqlite ] ||
             [ -f /var/lib/rpm/rpmdb.sqlite ]; }; then
            want=$(rpm -qa 2>/dev/null | wc -l | tr -d ' ')
            got=$(printf '%s\n' "$plain" | sed -n 's/^Packages  *//p' |
                  tr ',' '\n' | sed -n 's/^ *\([0-9][0-9]*\) (rpm)$/\1/p')
            if [ "$got" != "$want" ]; then
                printf 'the rpm count is [%s], but rpm -qa lists %s\n' \
                    "$got" "$want"
                fail=1
            fi
        fi ;;
    *)
        case "$plain" in
            *'(rpm)'*|*'(pkg)'*|*'(ports)'*|*'(manual)'*)
                printf 'a database count appeared without SQLite support: [%s]\n' \
                    "$(printf '%s\n' "$plain" | sed -n 's/^Packages  *//p')"
                fail=1 ;;
        esac ;;
esac

# libsqlite3 is loaded with dlopen(3) when the panel counts packages, never
# linked in: a shell that needed it to start would not start at all - as a
# login shell, too - once the library was removed.  So the binary must not
# name it as a dependency, and with the library hidden the shell must still
# start and draw the panel, less the counts only the library can give.  The
# second half needs a private mount namespace, as the Memory check does, and
# the library's paths from ldconfig(8) - every one it reports for
# libsqlite3.so.0 and libsqlite3.so, since a 64-bit system can list a 32-bit
# copy too, and each is checked to be empty before the shell runs; without
# them it is skipped, and says so.
case $("$MCSH" -f -c 'echo $version' 2>/dev/null) in
    *sqlite*)
        if command -v readelf >/dev/null 2>&1 &&
           readelf -d "$MCSH" 2>/dev/null | grep -q 'NEEDED.*libsqlite3'; then
            printf 'the shell is linked with libsqlite3; it must load it with '
            printf 'dlopen(3) instead\n'
            fail=1
        fi
        libs=$({ ldconfig -p || /sbin/ldconfig -p; } 2>/dev/null |
               sed -n 's/.*libsqlite3\.so\(\.0\)\{0,1\} .*=> //p' | sort -u)
        hidden=
        if [ -n "$libs" ] && command -v unshare >/dev/null 2>&1; then
            hidden=$(unshare --user --map-root-user --mount sh -c '
                printf "%s\n" "$1" | while IFS= read -r lib; do
                    mount --bind /dev/null "$lib" && ! [ -s "$lib" ] || exit 1
                done && echo MOUNTED &&
                XDG_CONFIG_HOME="$2" "$3" -f -c "sysinfo -n"
            ' sh "$libs" "$CFG" "$MCSH" 2>/dev/null)
        fi
        case "$hidden" in
            MOUNTED*)
                if ! printf '%s\n' "$hidden" | grep -q '^Shell.*mcsh'; then
                    printf 'with libsqlite3 gone, the panel was not drawn:\n%s\n' \
                        "$hidden"
                    fail=1
                elif printf '%s\n' "$hidden" |
                     grep -qE '\((rpm|pkg|ports|manual)\)'; then
                    printf 'a database count appeared with libsqlite3 gone: [%s]\n' \
                        "$(printf '%s\n' "$hidden" | sed -n 's/^Packages  *//p')"
                    fail=1
                fi ;;
            *)
                { printf 't020: skipped the missing-libsqlite3 check: the library '
                  printf 'could not be hidden (it needs ldconfig -p, unshare(1) '
                  printf 'and unprivileged user namespaces)\n'; } >&2 ;;
        esac ;;
esac

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
# `logo = none' means no logo: the two rows above begin on the first line of
# the output and in its first column, so there is nothing above them and
# nothing to their left for a logo to be in.
lay=$(panel_layout "$conf" Shell)
if [ "$lay" != "none 0" ]; then
    printf '`logo = none'"'"' still drew a logo (%s):\n%s\n' "$lay" "$conf"
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

# Each of the four keys the responsive logo added validates its value, and
# none of them is a key that quietly ignores what it cannot read.  A size that
# is not one of the five names, a count that is not a number, a negative cap
# and a boolean that is neither: each has to produce the same complaint as its
# neighbours above, naming the key and the line.  Spelled out one key at a
# time rather than folded into the six-line file further up, because what is
# being pinned is the validation of each key and a count of diagnostics would
# pass with three of the four accepting anything.
for bad in 'logo_size = bogus' 'logo_width = -3' 'logo_height = abc' \
	   'truncate = maybe'
do
    printf '%s\n' "$bad" > "$CFG/mcsh/sysinfo.conf"
    err=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1 >/dev/null)
    case "$err" in
	*"bad setting \`$bad'"*) ;;
	*) printf '`%s'"'"' was not refused: [%s]\n' "$bad" "$err"
	   fail=1 ;;
    esac
done

# --- the custom logo --------------------------------------------------------
# Any text file will do, and the columns to its right stay aligned because the
# width of each row is measured, not assumed: the rows below are 4, 2 and 5
# columns wide, the last of them using two double-width characters, so a
# byte-count or a character-count would both get the alignment wrong.
#
# `show' is pinned to two short rows, not because the alignment check below
# needs more than two but because the layout check does: a captured panel is
# laid out for 80 columns, and with the full field set the row block on a
# machine whose GPU has a long name takes enough of those 80 that the logo is
# held to what the rows can be squeezed into instead of being drawn as it was
# written.  `shell' and `kernel' are bounded by the kernel release on every
# machine, which leaves the five columns below a statement about the logo.
printf 'AAAA\nBB\nCCCCC\n' > "$CFG/mcsh/logo.txt"
printf 'logo = %s/mcsh/logo.txt\nshow = shell, kernel\n' "$CFG" \
    > "$CFG/mcsh/sysinfo.conf"
logo=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
if ! printf '%s\n' "$logo" | grep -q '^AAAA  '; then
    printf 'the custom logo was not drawn:\n%s\n' "$logo"
    fail=1
fi
# ...and the built-in art was not drawn in its place.  The rows are beside the
# logo, not under it - a logo of the user's own is never stacked - and they are
# indented by its own width: five columns, the widest of the three rows above,
# plus the two-column gap.  No tier of the built-in art is five columns wide,
# so this says `the custom logo and nothing else' without the test having to
# know what either of them looks like.
lay=$(panel_layout "$logo" Shell)
if [ "$lay" != "side 7" ]; then
    printf 'the built-in logo was drawn instead of the custom one (%s):\n%s\n' \
        "$lay" "$logo"
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
# The file named above does not exist, so any logo the panel draws now is the
# built-in one; which tier it is and whether it is beside the rows or above
# them is the terminal's business and not this case's, so anything but "no
# logo" will do here.
back=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo' 2>/dev/null)
lay=$(panel_layout "$back" Shell)
case "$lay" in
    side*|stack*) ;;
    *) printf 'an unreadable logo did not fall back to the built-in one '
       printf '(%s):\n%s\n' "$lay" "$back"
       fail=1 ;;
esac

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

# --- a panel that is not going to a terminal --------------------------------
# A captured or redirected panel is laid out for 80 by 24 and is the same
# panel every time, whatever window the shell happens to be in and whatever
# the termcap entry says.  That is a promise the manual makes, and it was
# false while the layout fell back to T_Cols and T_Lines: ChangeSize()
# (ed.screen.c) keeps those in step with the live window on every SIGWINCH, so
# an interactive shell redirecting its panel produced a different file at
# every width.  This test cannot resize the window it is run from, so what it
# pins instead is the consequence - that nothing about the destination's width
# reaches the content.
#
# A long $TERM is the lever.  The Terminal row carries it verbatim, and at 106
# characters it is wider than the 80 columns the panel is laid out for, so a
# width-dependent panel would have to cut it: cutting is for a terminal's
# right margin, and a file has no right margin to defend.  Before this, 80
# columns were enforced in the file too, and anyone piping the panel into a
# bug report, a log or awk got their longest values elided.
longterm=xterm-$(awk 'BEGIN { while (i++ < 100) printf "A" }')
for trunc in on off; do
    printf 'show = terminal\ntruncate = %s\n' "$trunc" \
        > "$CFG/mcsh/sysinfo.conf"
    term_row=$(XDG_CONFIG_HOME="$CFG" TERM="$longterm" "$MCSH" -f -c \
        'sysinfo -n' 2>/dev/null | sed -n 's/^Terminal  *//p')
    if [ "$term_row" != "$longterm" ]; then
        printf 'with `truncate = %s'"'"' a captured panel cut a value that ' \
            "$trunc"
        printf 'nothing was going to wrap:\nwanted [%s]\n   got [%s]\n' \
            "$longterm" "$term_row"
        fail=1
    fi
done
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

# -n drops the logo, and a panel that has room for one has one.  Which one,
# and where, is the terminal's business: a redirected panel is laid out for
# 80x24, where the tiers that go beside the rows do not fit and the wordmark
# is stacked above them instead, and the same shell on a wider window puts a
# tier to their left.  Both are a logo; neither is checked for here beyond
# that.
#
# `show' is pinned because 24 lines is the other half of "room".  Stacked, the
# wordmark's five rows are added to the panel rather than riding beside the
# fields, so the full field set - 26 rows on an ordinary desktop - leaves no
# room above them in a 24-line window and the logo is correctly declined; the
# fields are never dropped to make room for art.  That rule is t022's
# business, and pinning the field set here keeps this case about -n.
printf 'show = os, shell, kernel\n' > "$CFG/mcsh/sysinfo.conf"
withlogo=$(run 'sysinfo')
nologo=$(run 'sysinfo -n')
lay=$(panel_layout "$withlogo" Shell)
case "$lay" in
    side*|stack*) ;;
    *) printf 'the default panel is missing its logo (%s):\n%s\n' \
           "$lay" "$withlogo"
       fail=1 ;;
esac
lay_n=$(panel_layout "$nologo" Shell)
if [ "$lay_n" != "none 0" ]; then
    printf 'sysinfo -n still drew the logo (%s):\n%s\n' "$lay_n" "$nologo"
    fail=1
fi
# Dropping the logo must not drop a row.  Not a line count: the logo is drawn
# above the rows as readily as beside them, and stacked it adds its own lines
# to the panel without taking anything away from it, so the two panels differ
# in height for a reason that has nothing to do with the rows.  What has to be
# the same is which fields are drawn, and only the labels are compared,
# because the two panels come from two runs and a value that moves between
# them - the uptime, the load average - would differ in a correct shell.
at=${lay#* }
[ "$at" -lt 0 ] && at=0   # a panel with no row to find, reported above
w=$(panel_labels "$withlogo" "$at")
n=$(panel_labels "$nologo" 0)
if [ "$w" != "$n" ]; then
    printf 'sysinfo -n changed which rows are drawn:\n'
    printf 'with a logo: %s\n' "$(printf '%s\n' "$w" | tr '\n' ' ')"
    printf 'without one: %s\n' "$(printf '%s\n' "$n" | tr '\n' ' ')"
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
