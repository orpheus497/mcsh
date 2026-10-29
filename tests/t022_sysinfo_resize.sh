#!/bin/sh
# t022_sysinfo_resize.sh — `sysinfo' lays its panel out for the window it is
# really in (sysinfo.c: fetch_termsize, fetch_logo_layout, fetch_logo_scale).
#
# The logo used to be all or nothing: one piece of art, drawn beside the rows
# when the shell believed there was room for it and dropped altogether when it
# believed there was not — and what it believed came from T_Cols, the termcap
# figure, which is the 80 that GetTermCaps() substitutes for an entry without
# `co' however wide the window is.  An ordinary 80-column terminal therefore
# lost the logo for good, since 80 less the 72 columns the rows need less the
# two-column gap leaves six.
#
# What replaces it is three tiers and two placements, chosen from the size the
# kernel reports: the full art beside the rows where it fits, a smaller tier
# scaled to the room there is, the wordmark stacked above the rows where
# neither will go, and no logo at all below that.  Checked here are the
# properties that make that a feature rather than a lottery, and not one of
# them is a check on the drawing:
#
#   1. a wide window gets a large logo beside the rows, and a narrower one a
#      logo smaller on both axes — so the scaler scales, rather than only
#      dropping to the next tier down;
#   2. one factor is applied to both axes, so a scaled logo keeps the shape of
#      the art it came from;
#   3. an 80-column window gets the wordmark above the rows, which is the case
#      that used to lose the logo entirely;
#   4. a window too narrow for any logo still draws every field row: dropping
#      the logo must never drop data;
#   5. no line is wider than the window, and a value that had to be cut says so
#      with an ellipsis instead of simply ending;
#   6. the logo never makes the panel taller than a window that could have
#      held it — including stacked, where the logo's rows are added to the
#      panel rather than riding beside the fields, so what has to fit is the
#      whole block.  The qualifier carries the rest of the rule: where the
#      fields alone already overflow the window, no choice the layout makes
#      can keep that reserve, so the art is drawn rather than withheld for a
#      window that had already lost it;
#   7. each of the four configuration keys that steer all this does what it
#      says: `logo_size' names a tier, `logo_width' and `logo_height' cap it
#      (and decline a tier they are smaller than, rather than squashing it),
#      and `truncate = off' leaves a value whole.
#
# Where a measurement is taken at an odd-looking size, the size is the point.
# Truncation is measured at 20 columns rather than at 80 because at 80 the
# logo is stacked, which leaves the rows the whole width and so leaves nothing
# to cut; the case that does cut at 80 is a side-by-side tier with a narrow
# remainder, and `logo_size = mid' there is exactly that.  The stacked-height
# cases are stated against the height of the fields on their own, measured on
# the machine running them rather than assumed, and straddle the point where
# five more rows of art stop fitting above them - a field set small enough
# that the fields do fit, since it is only then that the reserve is a reserve
# the layout can keep.
#
# The panel comes from `mcsh -f -c sysinfo' under the driver rather than from
# an interactive session.  The shell writes to the pseudo-terminal either way,
# so GetSize() reports the window each case is about; what the non-interactive
# form adds is that the capture holds the panel and nothing else — no prompt,
# no echoed command line — so every line in it is a line whose width means
# something.
#
# Skipped when the driver cannot be built or no pseudo-terminal is available.

. ./lib_pty.sh

pty_setup || { echo "no pseudo-terminal available"; exit 77; }

CFG=$(mktemp -d "${TMPDIR:-/tmp}/t022.XXXXXX") || exit 1
trap 'rm -rf "$CFG"; pty_cleanup' EXIT INT TERM
mkdir -p "$CFG/mcsh"

fail=0

# panel COLS ROWS - the panel as it is drawn in a COLS x ROWS window.
#
# TERM=dumb and `color = off' between them leave the capture in plain text:
# every measurement below is a count of columns, and an escape sequence is
# bytes that take none of them, so a coloured panel would be measured wrong in
# a way that looked like a layout bug.
#
# The driver ends every session with a ^C and then a ^D, so that a case which
# left a partial line behind cannot hang the shell.  The line discipline echoes
# that ^C back as the two characters `^' and `C', on the end of the stream and
# with no newline after them, so they are taken off the last line and a last
# line left blank is dropped with them.
panel() (
    unset COLORTERM
    TERM=dumb; export TERM
    XDG_CONFIG_HOME=$CFG; export XDG_CONFIG_HOME
    PTY_SHELL_ARGS='-f -c sysinfo'
    pty_run "$1" "$2" </dev/null 2>/dev/null |
    tr -d '\r' | sed -e '$s/\^[CD]*$//' |
    awk '{ l[NR] = $0 }
         END {
             n = NR
             while (n > 0 && l[n] ~ /^ *$/) n--
             for (i = 1; i <= n; i++) print l[i]
         }'
)

# measure OUT - describe the shape of a panel, setting five variables:
#
#   col     the column the block of rows begins in, read from where the label
#           `Shell' sits on its own row.  Beside a logo every row is pushed
#           right by the logo's width and the two-column gap, so a positive
#           column means a logo to the left of the rows and a zero one means
#           the rows are flush left, with the logo above them or absent.  -1
#           says there is no such row at all, which is a failure in itself.
#   logow   the width of the logo block, and
#   logoh   its height, in rows: beside the rows, the columns before them and
#           the last line of the capture that puts ink in any of them; above
#           the rows, the lines before the first line that holds a letter or a
#           digit, since every field row holds one in its label and the art
#           drawn above the rows is punctuation throughout.  Both are 0 when
#           there is no logo.
#   plines  the height of the whole panel, and
#   pwidth  the width of its widest line.
#
# Nothing here reads the art.  A test that knows what the logo looks like
# fails when the logo is redrawn, and a redrawn logo is not a regression; what
# the panel promises is a shape, and the shape is in the output on its own.
measure() {
    set -- $(printf '%s\n' "$1" | awk -v lab=Shell '
        { l[NR] = $0; if (length($0) > maxw) maxw = length($0) }
        END {
            col = -1
            for (i = 1; i <= NR; i++) {
                p = index(l[i], lab)
                if (p > 0 && (p == 1 || substr(l[i], p - 1, 1) == " ") &&
                    substr(l[i], p + length(lab), 1) == " ") {
                    col = p - 1
                    break
                }
            }
            if (col > 0) {
                w = col - 2
                for (i = 1; i <= NR; i++)
                    if (substr(l[i], 1, col - 2) ~ /[^ ]/)
                        h = i
            }
            else if (col == 0) {
                for (first = 1; first <= NR; first++)
                    if (l[first] ~ /[0-9A-Za-z]/)
                        break
                for (i = 1; i < first; i++) {
                    s = l[i]
                    sub(/ +$/, "", s)
                    if (length(s) > w)
                        w = length(s)
                    if (s != "")
                        h = i
                }
            }
            print col " " (w + 0) " " (h + 0) " " NR " " (maxw + 0)
        }')
    col=$1
    logow=$2
    logoh=$3
    plines=$4
    pwidth=$5
}

# shape COLS ROWS - capture a panel and measure it in one step, keeping $out
# for a failure message to quote.
shape() {
    out=$(panel "$1" "$2")
    measure "$out"
    if [ "$col" = -1 ]; then
        printf 'no Shell row in the %sx%s panel, so there is nothing to ' \
            "$1" "$2"
        printf 'measure:\n%s\n' "$out"
        fail=1
        return 1
    fi
    return 0
}

# shaped - what the last measurement found, as one line for a message.
shaped() {
    printf 'rows at column %s, logo %sx%s, panel %s lines by %s columns' \
        "$col" "$logow" "$logoh" "$plines" "$pwidth"
}

# The art is 100 columns by 58 rows, and fetch_logo_scale() divides both axes
# by one integer, so whatever size it comes out at its width-to-height ratio is
# still about 100:58.  A tenth either way is the slack the two ceilings leave:
# at a factor of three, 100x58 becomes 34x20, which is 1.70 against 1.724.
aspect_off() {
    d=$(( $1 * 58 - $2 * 100 ))
    [ "$d" -lt 0 ] && d=$(( -d ))
    [ "$d" -gt $(( $2 * 10 )) ]
}

# --- a window the full art fits in -----------------------------------------
# One field row, so the width the rows need is the same 20 columns on every
# machine and the tier each case below gets is the tier the geometry asks for,
# not a consequence of how long this machine's GPU happens to be called.
printf 'show = shell\npalette = off\ncolor = off\n' > "$CFG/mcsh/sysinfo.conf"

if shape 200 60; then
    full_w=$logow
    full_h=$logoh
    if [ "$col" -le 0 ]; then
        printf 'at 200x60 the logo is not beside the rows (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    fi
    # The sizes, not the content: the full tier is the only one of the three
    # that is anywhere near this big, and a tier that had been scaled down
    # would not be.
    if [ "$logow" -le 60 ] || [ "$logoh" -le 40 ]; then
        printf 'at 200x60 the largest tier was not drawn (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    fi
    if aspect_off "$logow" "$logoh"; then
        printf 'the logo at 200x60 is %sx%s, which is not the shape of the ' \
            "$logow" "$logoh"
        printf 'art it is drawn from\n'
        fail=1
    fi
    if [ "$plines" -gt 60 ] || [ "$pwidth" -gt 200 ]; then
        printf 'the panel does not fit its 200x60 window (%s)\n' "$(shaped)"
        fail=1
    fi
    if [ "$logoh" -gt 58 ]; then
        printf 'the logo at 200x60 is %s rows tall, past the two rows kept ' \
            "$logoh"
        printf 'back for the prompt\n'
        fail=1
    fi
fi

# --- a narrower window gets a smaller logo, not the same one or none --------
# This is the case the whole design turns on.  Dropping straight from the full
# tier to the wordmark would pass a check that only asked for "some logo", so
# what is asked for here is a logo smaller on both axes than the one above and
# still beside the rows.
if shape 160 50; then
    if [ "$col" -le 0 ]; then
        printf 'at 160x50 the logo is not beside the rows (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    fi
    # Only the comparison needs the case above to have got that far; the rest
    # of this one stands on its own.
    if [ -n "$full_w" ] &&
       { [ "$logow" -ge "$full_w" ] || [ "$logoh" -ge "$full_h" ]; }
    then
        printf 'the logo did not shrink for a narrower window: %sx%s at ' \
            "$full_w" "$full_h"
        printf '200x60 against %sx%s at 160x50\n' "$logow" "$logoh"
        fail=1
    fi
    if aspect_off "$logow" "$logoh"; then
        printf 'the logo at 160x50 is %sx%s, which is not the shape of the ' \
            "$logow" "$logoh"
        printf 'art it is drawn from: one factor is not being applied to both '
        printf 'axes\n'
        fail=1
    fi
    if [ "$plines" -gt 50 ] || [ "$pwidth" -gt 160 ]; then
        printf 'the panel does not fit its 160x50 window (%s)\n' "$(shaped)"
        fail=1
    fi
    if [ "$logoh" -gt 48 ]; then
        printf 'the logo at 160x50 is %s rows tall, past the two rows kept ' \
            "$logoh"
        printf 'back for the prompt\n'
        fail=1
    fi
fi

# --- the 80-column window, which is the regression --------------------------
# Eighty columns less the rows less the gap is not enough for any tier to go
# beside them, and before this that meant no logo at all.  Stacked above the
# rows the logo is not competing with them for columns, so it is drawn: the
# rows stay flush left and the logo is in the lines above them.
if shape 80 24; then
    if [ "$col" != 0 ]; then
        printf 'at 80x24 the logo took columns from the rows (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    elif [ "$logoh" -lt 1 ]; then
        printf 'at 80x24 no logo was drawn at all; this is the window that '
        printf 'used to lose it (%s):\n%s\n' "$(shaped)" "$out"
        fail=1
    fi
    if [ "$pwidth" -gt 80 ]; then
        printf 'the panel is %s columns wide in an 80-column window\n' "$pwidth"
        fail=1
    fi
    if [ "$logoh" -gt 22 ]; then
        printf 'the stacked logo is %s rows tall, past the two rows kept ' \
            "$logoh"
        printf 'back for the prompt\n'
        fail=1
    fi
fi

# --- too narrow for anything -----------------------------------------------
# Below the wordmark's own width there is nothing left to draw, and the panel
# says so by drawing the rows alone rather than by drawing a logo that has
# gone to mush.
if shape 20 24; then
    if [ "$col" != 0 ] || [ "$logoh" != 0 ]; then
        printf 'at 20x24 a logo was drawn anyway (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    fi
    if [ "$pwidth" -gt 20 ]; then
        printf 'the panel is %s columns wide in a 20-column window:\n%s\n' \
            "$pwidth" "$out"
        fail=1
    fi
fi

# --- a window wide enough and nowhere near tall enough ----------------------
# The budget at 200 columns would hold the full art twice over; what it will
# not hold is 58 rows in 30 lines.  A tier that does not fit the height has to
# fall through to the next one down, or the panel scrolls its own first rows
# off the screen as it draws them.
if shape 200 30; then
    if [ "$logoh" -gt 28 ]; then
        printf 'at 200x30 a %s-row logo was drawn in a 30-line window (%s):\n' \
            "$logoh" "$(shaped)"
        printf '%s\n' "$out"
        fail=1
    fi
    if [ "$logoh" -lt 1 ]; then
        printf 'at 200x30 the logo was dropped rather than reduced (%s)\n' \
            "$(shaped)"
        fail=1
    fi
fi

# --- one factor, both axes --------------------------------------------------
# The tiers are checked in at the sizes they were drawn at, so the cases above
# measure the scaler only where it had something to do.  `logo_size = full'
# with `logo_width' under the art's own width is scaling and nothing else: the
# art has to come back a third of the size on both axes, and a width that had
# been scaled on its own would leave the height at 58.
printf 'show = shell\npalette = off\ncolor = off\n' > "$CFG/mcsh/sysinfo.conf"
printf 'logo_size = full\nlogo_width = 40\n' >> "$CFG/mcsh/sysinfo.conf"
if shape 200 60; then
    if [ "$logow" -gt 40 ]; then
        printf '`logo_width = 40'"'"' was not obeyed: the logo is %s columns ' \
            "$logow"
        printf 'wide\n'
        fail=1
    fi
    if [ "$logow" -lt 2 ] || [ "$logoh" -lt 2 ]; then
        printf 'the scaled logo came out %sx%s, which is not a drawing\n' \
            "$logow" "$logoh"
        fail=1
    elif aspect_off "$logow" "$logoh"; then
        printf 'scaled to %s columns the logo came out %s rows tall; 100x58 ' \
            "$logow" "$logoh"
        printf 'scaled by one factor on both axes would not\n'
        fail=1
    fi
fi

# --- dropping the logo must not drop a row ---------------------------------
# The whole panel this time, not one row of it: a window too narrow for a logo
# is still a window the panel has to tell the truth in, and the fields it draws
# there are the fields it draws anywhere.  The labels are compared and not the
# text, because the two panels come from two runs and a value that moves
# between them - the uptime, the load average - differs in a correct shell.
printf 'palette = off\ncolor = off\n' > "$CFG/mcsh/sysinfo.conf"
labels() {
    printf '%s\n' "$1" |
    awk -v col="$2" '
        {
            s = substr($0, col + 1)
            p = index(s, "  ")
            if (p > 1 && substr(s, 1, 1) ~ /[0-9A-Za-z]/)
                print substr(s, 1, p - 1)
        }'
}

if shape 200 60; then
    wide_labels=$(labels "$out" "$col")
    # Which tier this one gets is this machine's business: the rows are the
    # real ones here, and a machine whose GPU has a long enough name takes
    # columns off the budget that a machine with a short one leaves for the
    # art.  What has to be true of all of them is that a 200x60 window gets a
    # logo somewhere.
    if [ "$col" -le 0 ] && [ "$logoh" -lt 1 ]; then
        printf 'the default panel at 200x60 has no logo at all (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    fi
    if [ "$plines" -gt 60 ]; then
        printf 'the default panel is %s lines in a 60-line window\n' "$plines"
        fail=1
    fi
    if [ -z "$wide_labels" ]; then
        printf 'no field rows were found beside the logo at 200x60:\n%s\n' \
            "$out"
        fail=1
    fi
fi

if shape 20 24; then
    narrow_labels=$(labels "$out" 0)
    if [ "$logoh" != 0 ] || [ "$col" != 0 ]; then
        printf 'the default panel at 20x24 still drew a logo (%s):\n%s\n' \
            "$(shaped)" "$out"
        fail=1
    fi
    if [ -n "$wide_labels" ] && [ "$narrow_labels" != "$wide_labels" ]; then
        printf 'the panel at 20x24 does not draw the same rows as at 200x60:\n'
        printf 'at 200x60: %s\n' "$(printf '%s\n' "$wide_labels" | tr '\n' ' ')"
        printf 'at 20x24:  %s\n' \
            "$(printf '%s\n' "$narrow_labels" | tr '\n' ' ')"
        fail=1
    fi
    # Every line inside the window, and a value that had to give way says so.
    # Without the marker a cut value reads as the value itself, which is the
    # one thing this panel may not do: a truncated kernel release looks like a
    # kernel release.
    if [ "$pwidth" -gt 20 ]; then
        printf 'a line of the panel is %s columns wide in a 20-column ' \
            "$pwidth"
        printf 'window:\n%s\n' "$out"
        fail=1
    fi
    if ! printf '%s\n' "$out" | grep -q '\.\.\.$'; then
        printf 'no value in a 20-column panel was marked as cut:\n%s\n' "$out"
        fail=1
    fi
fi

# --- `truncate = off' leaves the value whole --------------------------------
# The case above with the key the other way round, so that what it measures is
# the key and not the width.  Off, nothing is cut: the panel is as wide as its
# widest value and the terminal is left to wrap it, which is the documented
# cost of the setting.  The default field set again, because `show = shell'
# has no value long enough to be cut at any width.
printf 'palette = off\ncolor = off\ntruncate = off\n' > "$CFG/mcsh/sysinfo.conf"
if shape 20 24; then
    if [ "$pwidth" -le 20 ]; then
        printf '`truncate = off'"'"' still held the panel to the 20-column '
        printf 'window (%s):\n%s\n' "$(shaped)" "$out"
        fail=1
    fi
    if printf '%s\n' "$out" | grep -q '\.\.\.$'; then
        printf '`truncate = off'"'"' cut a value anyway:\n%s\n' "$out"
        fail=1
    fi
fi

# --- and the cut that bites at 80 columns ----------------------------------
# Stacked, the logo leaves the rows the whole width, so the 80-column case
# above has nothing to cut.  A side-by-side tier with a narrow remainder is
# what does: `logo_size = mid' is an instruction, so the 48-column tier is
# drawn beside the rows whatever the budget says, the rows are left what is
# over, and the values that will not fit are cut rather than wrapped.  Forty
# lines rather than twenty-four so that the tier fits the height and the
# measurement is about the width.
printf 'palette = off\ncolor = off\nlogo_size = mid\n' > "$CFG/mcsh/sysinfo.conf"
if shape 80 40; then
    if [ "$col" -le 0 ]; then
        printf '`logo_size = mid'"'"' did not put the tier beside the rows '
        printf '(%s):\n%s\n' "$(shaped)" "$out"
        fail=1
    fi
    if [ "$pwidth" -gt 80 ]; then
        printf 'the panel is %s columns wide in an 80-column window, so the ' \
            "$pwidth"
        printf 'rows were not cut to fit beside the tier:\n%s\n' "$out"
        fail=1
    fi
    if ! printf '%s\n' "$out" | grep -q '\.\.\.$'; then
        printf 'no value was marked as cut although the rows were left %s ' \
            "$(( 80 - col ))"
        printf 'columns:\n%s\n' "$out"
        fail=1
    fi
fi

# --- the stacked logo is part of the panel's height -------------------------
# This is the half of the height rule that is easy to get wrong.  Beside the
# rows the logo rides alongside them and costs the panel no lines it was not
# going to have; stacked, its rows are added to them.  So what has to fit is
# the whole block: the art, the blank line under it, and every field row.
# This is what the wordmark costs, and the window it is drawn in has to be
# able to afford all of it - a window that cannot has to be left the fields,
# since the fields are never dropped to make room for art and the panel would
# otherwise scroll its own first rows off the screen.
#
# Both directions are checked, because dropping the logo in both would pass a
# check that only asked for the first.
#
# The height of the fields on their own is this machine's business - how many
# of them can be read here is not fixed - so it is measured rather than
# assumed, and the two cases below are stated against it.  `logo = none'
# rather than `sysinfo -n' so that the field set is identical in all three
# captures.
printf 'show = os, shell, kernel\npalette = off\ncolor = off\nlogo = none\n' \
    > "$CFG/mcsh/sysinfo.conf"
bare=$(panel 80 24)
measure "$bare"
bare_lines=$plines

printf 'show = os, shell, kernel\npalette = off\ncolor = off\n' \
    > "$CFG/mcsh/sysinfo.conf"
tight=$(( bare_lines + 3 ))		# three short of the wordmark's six
roomy=$(( bare_lines + 8 ))		# two over it, for the prompt
if shape 80 "$tight"; then
    if [ "$logoh" != 0 ] || [ "$col" != 0 ]; then
        printf 'the default panel in an %s-line window was given a logo it ' \
            "$tight"
        printf 'has no room for: the fields alone are %s lines (%s):\n%s\n' \
            "$bare_lines" "$(shaped)" "$out"
        fail=1
    fi
    if [ "$plines" -gt "$bare_lines" ]; then
        printf 'the declined logo cost the panel %s lines anyway (%s)\n' \
            "$(( plines - bare_lines ))" "$(shaped)"
        fail=1
    fi
fi
if shape 80 "$roomy"; then
    if [ "$logoh" -lt 1 ] || [ "$col" != 0 ]; then
        printf 'an %s-line window has room for the stacked wordmark above ' \
            "$roomy"
        printf '%s lines of fields and did not get it (%s):\n%s\n' \
            "$bare_lines" "$(shaped)" "$out"
        fail=1
    fi
    if [ "$plines" -gt "$roomy" ]; then
        printf 'the stacked logo made the panel %s lines in an %s-line ' \
            "$plines" "$roomy"
        printf 'window (%s)\n' "$(shaped)"
        fail=1
    fi
fi

# --- `logo_size' names the tier --------------------------------------------
# One field row again, so the budget is the same on every machine and each
# answer below is the key's doing.  The sizes are compared against the tiers'
# own dimensions - 100x58, 48x29, 28x5 - and a tier is identified by being
# near its own width and nowhere near the others'.
printf 'show = shell\npalette = off\ncolor = off\nlogo_size = mid\n' \
    > "$CFG/mcsh/sysinfo.conf"
if shape 200 60; then
    if [ "$col" -le 0 ] || [ "$logow" != 48 ]; then
        printf '`logo_size = mid'"'"' did not draw the 48-column tier beside '
        printf 'the rows (%s):\n%s\n' "$(shaped)" "$out"
        fail=1
    fi
fi

printf 'show = shell\npalette = off\ncolor = off\nlogo_size = small\n' \
    > "$CFG/mcsh/sysinfo.conf"
if shape 200 60; then
    if [ "$col" != 0 ] || [ "$logoh" != 5 ]; then
        printf '`logo_size = small'"'"' did not stack the five-row wordmark '
        printf 'above the rows (%s):\n%s\n' "$(shaped)" "$out"
        fail=1
    fi
fi

# `none' is `logo = none' by another name, which the manual says in so many
# words, so the two are compared rather than each being checked for emptiness.
# `show = shell' pins the one value that moves between runs out of the way:
# the shell's version does not change while this test runs, where the uptime
# and the load average do.
printf 'show = shell\npalette = off\ncolor = off\nlogo_size = none\n' \
    > "$CFG/mcsh/sysinfo.conf"
size_none=$(panel 200 60)
printf 'show = shell\npalette = off\ncolor = off\nlogo = none\n' \
    > "$CFG/mcsh/sysinfo.conf"
logo_none=$(panel 200 60)
if [ "$size_none" != "$logo_none" ]; then
    printf '`logo_size = none'"'"' and `logo = none'"'"' drew different '
    printf 'panels:\n[%s]\n[%s]\n' "$size_none" "$logo_none"
    fail=1
fi
measure "$size_none"
if [ "$logow" != 0 ] || [ "$logoh" != 0 ] || [ "$col" != 0 ]; then
    printf '`logo_size = none'"'"' drew a logo (%s):\n%s\n' \
        "$(shaped)" "$size_none"
    fail=1
fi

# --- the caps choose a size as well as scaling one -------------------------
# A cap smaller than a tier declines that tier rather than squashing it, and
# below the wordmark's own size there is nothing left to decline to.  That
# matters most for the wordmark, which is type: scaled to 14 columns it comes
# out as three solid bars, so a cap under its 28 has to leave the panel the
# fields alone.  Checked on both axes, because the two gates are written
# separately and only one of them used to do this.
printf 'show = shell\npalette = off\ncolor = off\nlogo_height = 20\n' \
    > "$CFG/mcsh/sysinfo.conf"
if shape 200 60; then
    if [ "$logoh" -gt 20 ]; then
        printf '`logo_height = 20'"'"' was not obeyed: the logo is %s rows ' \
            "$logoh"
        printf 'tall (%s)\n' "$(shaped)"
        fail=1
    fi
    # 58 rows and 29 rows are both past the cap, so what is left is the
    # wordmark, stacked; a window this wide would otherwise have the full tier.
    if [ "$col" != 0 ] || [ "$logoh" != 5 ]; then
        printf '`logo_height = 20'"'"' did not fall past both side-by-side '
        printf 'tiers (%s):\n%s\n' "$(shaped)" "$out"
        fail=1
    fi
fi

for cap in logo_width logo_height; do
    printf 'show = shell\npalette = off\ncolor = off\n%s = 4\n' "$cap" \
        > "$CFG/mcsh/sysinfo.conf"
    if shape 200 60; then
        if [ "$logow" != 0 ] || [ "$logoh" != 0 ]; then
            printf '`%s = 4'"'"' squashed a logo into the cap instead of ' \
                "$cap"
            printf 'declining it (%s):\n%s\n' "$(shaped)" "$out"
            fail=1
        fi
    fi
done

# --- the palette is two more lines, and they come out of the same budget ----
# fetch_palette() prints its two rows after the row loop, so they are part of
# the panel's height and have to be counted against the window like any other
# part of it.  Colour has to be forced for them to be printed at all, which
# puts escape sequences in the capture - so this case counts lines and
# measures nothing else, since a column counted through an escape sequence is
# a column counted wrong.  The driver is called directly because pty_run()
# splits $PTY_SHELL_ARGS on spaces and the command here has an argument.
#
# Measured at 200x60, which is the window the omission showed in: the full
# tier is 58 rows, so counted without the palette it fits the 58 the two-row
# reserve leaves exactly, and the panel then came to 60 lines in a 60-line
# window with the reserve spent twice.  The assertion is against the reserve
# and not against the window for that reason - 60 lines in 60 does fit, and
# still pushes the prompt off the top, which is what the reserve is for.
printf 'show = shell\npalette = on\ncolor = on\n' > "$CFG/mcsh/sysinfo.conf"
pal_lines=$(
    unset COLORTERM
    TERM=xterm; export TERM
    XDG_CONFIG_HOME=$CFG; export XDG_CONFIG_HOME
    "$PTY_BIN" -c 200 -r 60 "$MCSH" -f -c 'sysinfo -c' </dev/null 2>/dev/null |
    tr -d '\r' | sed -e '$s/\^[CD]*$//' |
    awk '{ l[NR] = $0 }
         END {
             n = NR
             while (n > 0 && l[n] == "") n--
             print n
         }'
)
if [ "$pal_lines" -gt 58 ]; then
    printf 'with the palette on the panel is %s lines in a 60-line window, ' \
        "$pal_lines"
    printf 'past the two rows kept back for the prompt: the two rows the '
    printf 'palette prints after the fields are not being counted\n'
    fail=1
fi

# --- art that cannot be scaled must not cost the fields their data ---------
# fetch_logo_scale() refuses to resample art carrying escape sequences: the
# bytes of a sequence are not a picture and a block average of them is
# nonsense.  So that one kind of logo can still be wider than the window after
# the scaler has had it, which is the one way `fetch_logo_w' gets past the
# room fetch_logo_layout() allows it.
#
# Beside the rows that was ruinous.  The row block is cut to the columns left
# over, those columns were negative, every width clamped to zero, and the
# panel came out as the art followed by blank lines - no labels, no values,
# the whole point of the panel gone.  Drawing the art above the rows instead
# costs them no columns at all, so the fields are whole and only the art
# wraps, which is what the manual says happens to a logo the window cannot
# hold.
#
# The assertion is about the data, not the placement: a field row has to be
# there and it has to still have its value.  `color = off' does not silence a
# logo file's own escapes - they are the user's bytes and are passed through -
# so this is also the one case here whose capture is not plain text, and it is
# measured by what it still says rather than by where it sits.
awk 'BEGIN {
         row = sprintf("\033[35m%s\033[0m", sprintf("%*s", 100, ""))
         gsub(/ /, "#", row)
         for (i = 0; i < 12; i++) print row
     }' > "$CFG/mcsh/wide-colour.txt"
printf 'logo = %s/mcsh/wide-colour.txt\ncolor = off\n' "$CFG" \
    > "$CFG/mcsh/sysinfo.conf"
esc=$(panel 80 40)
escval=$(printf '%s\n' "$esc" | sed -n 's/.*Shell  *\([^ ].*\)$/\1/p')
if [ -z "$escval" ]; then
    printf 'a 100-column escape-coloured logo in an 80-column window emptied '
    printf 'the field rows: no Shell value survived:\n%s\n' "$esc"
    fail=1
fi
if ! printf '%s\n' "$esc" | grep -q 'Kernel'; then
    printf 'a 100-column escape-coloured logo in an 80-column window cost the '
    printf 'panel its Kernel row entirely:\n%s\n' "$esc"
    fail=1
fi
# ...and above them rather than beside them, which is what leaves the fields
# their columns in the first place.  Drawn beside them the rows survive only
# because nothing can be cut to a negative width; they are still pushed a
# hundred columns to the right of an 80-column window, so every one of them
# wraps into the art.  Flush left is the fix; whole-but-wrapped was the
# symptom's other half.
measure "$esc"
if [ "$col" != 0 ]; then
    printf 'a logo too wide to scale was drawn beside the rows (column %s) '
    printf 'instead of above them, so every field row wraps into it:\n%s\n' \
        "$col" "$esc"
    fail=1
fi
rm -f "$CFG/mcsh/wide-colour.txt"

# --- the size is asked of the terminal the panel goes to -------------------
# fetch_termsize() puts its ioctl to `didfds ? 1 : SHOUT', the same descriptor
# fetch_out_isatty() tests, and deliberately not to SHIN the way GetSize()
# (ed.screen.c) does.  SHIN is the shell's input, and in script mode that is
# the script: the ioctl failed on a regular file, the termcap 80 was used, and
# a script run from a wide terminal drew no logo on a window with room for the
# full art.
#
# Stated as an equality rather than against a tier, so it holds on any machine
# and survives the art being redrawn: one window, one panel, however the
# command reached the shell.
printf 'show = os, shell, kernel\npalette = off\ncolor = off\n' \
    > "$CFG/mcsh/sysinfo.conf"
printf 'sysinfo\n' > "$CFG/mcsh/si.csh"
dash_c=$(panel 200 60)
script_mode=$(
    unset COLORTERM
    TERM=dumb; export TERM
    XDG_CONFIG_HOME=$CFG; export XDG_CONFIG_HOME
    PTY_SHELL_ARGS="-f $CFG/mcsh/si.csh"
    pty_run 200 60 </dev/null 2>/dev/null |
    tr -d '\r' | sed -e '$s/\^[CD]*$//' |
    awk '{ l[NR] = $0 }
         END {
             n = NR
             while (n > 0 && l[n] ~ /^ *$/) n--
             for (i = 1; i <= n; i++) print l[i]
         }'
)
measure "$dash_c"; c_col=$col; c_logow=$logow
measure "$script_mode"; s_col=$col; s_logow=$logow
if [ "$s_col" != "$c_col" ] || [ "$s_logow" != "$c_logow" ]; then
    printf 'the same window laid the panel out differently depending on how '
    printf 'the command reached the shell: `-c'"'"' put the rows at column %s '
    printf 'with a %s-column logo, a script at column %s with %s - so the '
    printf 'size was asked of the script rather than of the terminal\n' \
        "$c_col" "$c_logow" "$s_col" "$s_logow"
    fail=1
fi
rm -f "$CFG/mcsh/si.csh"

exit $fail
