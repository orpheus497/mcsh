#!/bin/sh
# mklogo.sh: build tc.fetch.logo.h from the `sysinfo' logo art under logos/.
# Usage: sh mklogo.sh [-o outfile] [srcdir]
#
#	-o outfile	where to write the header; `-' means standard output.
#			The default is <srcdir>/tc.fetch.logo.h.
#	srcdir		source tree to read logos/ from.  The default is the
#			directory this script lives in, so the script works
#			when invoked by an absolute path from anywhere.
#
# This is the gethost/host.defs arrangement - a generator beside its data,
# producing C that nobody edits by hand - with one deliberate difference:
# tc.fetch.logo.h is committed rather than remade by make.  The art changes
# about never, and when it does change the three tiers want looking at by eye
# (the mid tier is hand-tuned, not computed); committing the header keeps all
# of that out of every contributor's build, where it would otherwise mean a
# new Makefile rule and an awk run on every build host.  Re-run this script
# when you touch logos/*.txt, and commit what it writes.
#
# The art is restricted to the five-level density ramp
#
#	space (empty) < `.' < `-' < `+' < `#' (solid)
#
# and this script refuses anything else on purpose.  The panel scales a logo
# by max-pooling over exactly those five characters, so a sixth character
# would not merely look different - it would be read as empty and dropped
# from every scaled rendering.  That is a bug far better failed on here, with
# the art in front of you, than discovered in a narrow terminal later.

prog=mklogo.sh
out=
srcdir=

usage() {
    printf 'usage: sh %s [-o outfile] [srcdir]\n' "$prog" >&2
    exit 2
}

die() {
    printf '%s: %s\n' "$prog" "$1" >&2
    exit 1
}

while [ $# -gt 0 ]; do
    case $1 in
    -o)
	shift
	[ $# -gt 0 ] || usage
	out=$1
	;;
    -h|--help)
	usage
	;;
    -*)
	printf '%s: unknown option %s\n' "$prog" "$1" >&2
	usage
	;;
    *)
	[ -z "$srcdir" ] || usage
	srcdir=$1
	;;
    esac
    shift
done

[ -n "$srcdir" ] || srcdir=$(dirname "$0")
[ -n "$out" ] || out=$srcdir/tc.fetch.logo.h

#
# Where the art comes from.
#
# logos/mcsh.txt is the drawing itself, 58 rows of 100 columns of ink, and it
# is the only file to edit to change what the panel draws.
#
# logos/mcsh-mid.txt is that same drawing at 29 rows of 48 columns, and it is
# not a scaler's output: it was derived from one and then gone over by hand,
# because a strict 2x downscale of a line drawing of single-column strokes
# makes mistakes that no choice of filter fixes.  Its provenance is recorded
# here rather than at the head of the art file because that file has to stay
# art and nothing else - the `logo' configuration key feeds a file of exactly
# this shape to fetch_logo_load() at run time, which would draw a comment as
# part of the picture.  48 columns rather than 50 because the two rightmost
# columns of the 50-column grid it was drawn on hold no ink, and the width
# this file claims is the column budget the panel reserves for it.
#
# It started as the max-pool downscale, which is what fetch_logo_scale()
# itself computes at this size.  Max-pool emits the block maximum, so `output
# level >= t' is exactly `some source cell >= t', and its silhouette is
# therefore right at every contrast level at once: measured against the
# full-size art it was exact on per-row and per-column extent, kept all 15 of
# the drawing's strokes unbroken, and held 79% of the spiral's scanline
# crossings - against 44% for box-averaging, which also drops 4 strokes
# outright.  Starting from the scaler's own output has a second benefit: the
# committed mid tier and what the scaler would produce at this size are the
# same image, so the hand pass is a strict improvement over the fallback and
# a later change in the scaler shows up as a diff here rather than as a
# surprise in a narrow terminal.
#
# What max-pool gets wrong is weight.  It drew 52% of the inked cells solid
# where the art draws 28%, because any 2x2 block holding one solid stroke
# comes out wholly solid.  So the hand pass is all lightening, never
# darkening, and it went after three things:
#
#	- The 63 cells drawn two or more ramp levels above their source
#	  block's mean - the outer rims and the cap's shoulders, where a
#	  single-column stroke in an otherwise empty block was promoted to
#	  `#' - stepped down to that mean.
#
#	- The slabs.  Inside a horizontal run of four or more `#' cells, the
#	  cells whose block was not actually solid stepped down one level to
#	  `+'.  This is where the drawing read as blobs rather than strokes -
#	  20 runs of five or more cells in the body against the art's 7 - and
#	  it breaks them at the columns where the art really does have
#	  separate strokes, because those are the columns with the lower
#	  block means.  Runs of one or two cells are strokes and were left
#	  alone, which is what keeps this from becoming the grey smear that
#	  box-averaging produces.
#
#	- A `+' whose block held no solid ink at all is shading rather than a
#	  stroke, and went to `-'.
#
# One guard overrides all three: a cell whose block holds a `#' in the same
# column in every one of its source rows is a stroke crossing the whole cell
# vertically, and is never lightened.  At 2x such a stroke's block mean is
# only 2.0, because the column beside it is empty, so every mean-based rule
# wants to lighten it - and lightening it is exactly what turns a continuous
# hairline into a dashed one.  The chamber wall down the drawing's lower
# middle is eight source rows of one such stroke, and was coming out as three
# fading dashes before the guard went in.  A stroke solid across the block's
# columns instead gets no protection: that one is a half-height horizontal
# feature, and the cells either side of it in the same row carry the line on.
#
# Eight sites were then drawn by hand, because no rule about block means could
# have reached them.  Coordinates are 0-indexed on the 29x48 grid.
#
#	row 0, the apex.  It pools the art's 5-column tip with the
#	17-column bar under it, so max-pool drew a flat top edge 18 source
#	columns wide - 3.6x the real apex.  Redrawn from the tip's source row
#	alone, which costs nothing, because row 1's extent already contains
#	the bar's.
#
#	row 3, the cap's right fringe - five separate single strokes fused
#	into one slab.  The three with the lowest block means step down, so
#	it reads as a fringe again rather than an edge.
#
#	row 5, the band that detaches the cap from the body.  This row pools a
#	solid 47-column bar with the art's `+++-+##+----------#######-' band
#	and the bar won everywhere, leaving cap and body with no boundary at
#	all.  Drawn from the band's source row alone; the last column keeps
#	the bar's `+' so the row's extent does not move.
#
#	rows 18-26, the right-hand arm.  The art has two one-column hairlines
#	with a single `-' column between them; at 2x the pair lands in
#	adjacent output columns and max-pool welded it into a 2-cell bar - the
#	worst area inflation in the drawing, +24% to +52% per column.  The
#	leftmost `#' of each row is the left hairline, which walks 41, 41,
#	41, 40, 40, 40, 40, 39, 39, and is kept; everything else in the row
#	steps down, so the arm reads as one clean stroke with its shadow.
#	Two hairlines one column apart cannot be drawn with a gap between
#	them in half the columns: that is a resolution limit, not a choice of
#	filter, and no candidate filter recovered it without inventing ink.
#
#	(12,35), emptied.  The point where the spiral tightens is 3/4 blank in
#	the art, and leaving the cell inked filled the last countable void.
#
#	(27,39), kept solid at `#'.  Its block mean is 3.00, enough to carry
#	the bottom-right flourish's second accent.
#
#	row 28, the closing baseline, drawn by hand.  The art's last row is a
#	detached light rule, and pooling it with the row above drew one solid
#	bar across the foot of the drawing; by hand it keeps the flourish's
#	solid-light-solid shape.
#
#	(11,4), put back to `#'.  The slab rule took out the whole run at
#	cols 4-8, which left the outer rim - a stroke that defines the
#	silhouette and walks (10,5) (11,4) (12,3) - with a soft spot in the
#	middle of it.
#
# What came out, measured against the full-size art: row-extent error 36
# source columns over 29 rows, against max-pool's 26, and all of that
# difference is the deliberate apex; 12 of 15 strokes still drawn at `+' or
# better, 2 broken; 78% of the spiral's crossings; 3 slab runs in the body
# against the art's 7; and an ink histogram of . 7 / - 25 / + 32 / # 36
# against the art's . 14 / - 26 / + 32 / # 28.  Distance between those two
# histograms came down from max-pool's 0.171 to 0.073, and the share of cells
# at either end of the ramp - which is what makes a drawing read as a drawing
# rather than as a wash - landed at 43.1% against the art's 42.4%.
#
# The wordmark tier has no file under logos/; see wordmark() below for why.
#
full=$srcdir/logos/mcsh.txt
mid=$srcdir/logos/mcsh-mid.txt

#
# Both tiers are required.  A header that silently came out with one array
# missing would fail the build a long way from here, with a message about an
# undeclared identifier rather than about a missing file, so say it now.
#
for f in "$full" "$mid"; do
    [ -r "$f" ] || die "cannot read \`$f' - pass srcdir, or create the art"
done

#
# The buffers the panel keeps a logo in, read out of tc.fetch.c rather than
# written down here.  array() refuses art that will not fit them, and a limit
# copied into this script would be a limit that silently stopped matching the
# one the panel enforces.  The pattern is anchored on the `#define' and takes
# the first field after the name, so the comment that follows each macro is
# not part of the number.
#
limit() {
    [ -r "$srcdir/tc.fetch.c" ] ||
	die "cannot read \`$srcdir/tc.fetch.c' - pass srcdir"
    v=$(sed -n 's/^#define[ 	]*'"$1"'[ 	]*\([0-9][0-9]*\).*/\1/p' \
	    "$srcdir/tc.fetch.c" | sed -n 1p)
    [ -n "$v" ] || die "cannot find \`$1' in \`$srcdir/tc.fetch.c'"
    printf '%s\n' "$v"
}

FETCH_LOGO_ROWS=$(limit FETCH_LOGO_ROWS) || exit 1
FETCH_LOGO_COLS=$(limit FETCH_LOGO_COLS) || exit 1

#
# LC_ALL=C because the bracket expression is matched against bytes: in a
# multibyte locale a stray 8-bit byte is not a character at all and grep may
# decline to look at it.  A CR left behind by a CRLF line ending trips this
# check too, which is the intent - it would otherwise end up inside a C
# string literal and be printed to the terminal.
#
for f in "$full" "$mid"; do
    bad=$(LC_ALL=C grep -n '[^ .+#-]' "$f" | head -1)
    [ -z "$bad" ] && continue
    die "$f:${bad%%:*}: character off the density ramp \` .-+#'"
done

#
# banner - the head of the generated file.
#
banner() {
    cat <<'EOF'
/* Do not edit this file, mklogo.sh creates it */
/*
 * tc.fetch.logo.h: the logos the `sysinfo' panel draws, compiled in.
 *
 * Generated from the art under logos/ by mklogo.sh.  Edit the art, re-run
 *
 *	sh mklogo.sh
 *
 * and commit the result.  Do not edit this file: the next run of the script
 * would throw the edit away.
 *
 * Three fixed sizes are compiled in rather than one size scaled at run time,
 * because this art is a line drawing of single-column strokes rather than
 * filled regions, and scaling it is lossy in a way worth controlling by
 * hand.  Measured against the full-size art, max-pooling over the ramp holds
 * up to about 66x38, is blobby but recognizable at 50x29, is marginal by
 * 46x24, and is mush at 40x20 and below.  (Box-averaging is worse at every
 * size: it turns a one-column stroke into a grey smear.)  So the full tier is
 * the art verbatim, the mid tier is hand-tuned at the size real terminals
 * actually land on instead of being left to the scaler, and anything
 * narrower than that gets the wordmark, which is type and does not survive
 * resampling at all.
 *
 * Compiled in rather than installed as data files because the panel has to
 * work on a machine where nothing was installed beside the binary, and a
 * logo that could not be found would mean no logo at all.  Anyone who wants
 * their own art still has the `logo' configuration key, which reads any file.
 *
 * The art is 7-bit ASCII on the five-level ramp ` .-+#', space being the
 * empty level, for the same reason the wordmark below is: box-drawing or
 * block-element characters would be mojibake in a non-UTF-8 locale and on a
 * terminal without the glyphs, and the panel has to stay legible on a serial
 * console.
 *
 * `?' is escaped in the rows below because `??' followed by one of =/()!<>-
 * is a trigraph (C89 5.2.1.1), and a C89 compiler replaces it inside a
 * string literal.  No ramp character is a `?', so this never fires today;
 * the escaper does not rely on that remaining true.
 *
 * <stddef.h> is included for the NULL that terminates each array below.
 * tc.fetch.c has sh.h in front of this header and would not need it, but a
 * generated header that cannot be compiled on its own cannot be checked on
 * its own either, and the check
 *
 *	printf '#include "tc.fetch.logo.h"\nint main(void){return 0;}\n' \
 *	    > t.c && cc -I. -c t.c
 *
 * is the cheap way to find out that a run of mklogo.sh produced something a
 * compiler will not read.  Including a standard header twice, and in any
 * order, is explicitly harmless (C89 4.1.2), so this costs the real build
 * nothing.
 */
#include <stddef.h>
EOF
}

#
# array <file> <C name> <what> <quoted source name> - emit one C array.
#
# The escaping is done one character at a time rather than with gsub(), whose
# replacement text treats `&' and `\' specially in ways implementations have
# historically disagreed about.  The art files cannot contain a backslash or
# a double quote today - the ramp check above refuses them - but this is the
# one place where getting it wrong would produce a header that compiles and
# draws the wrong thing, so it does not depend on the check.
#
# $AWK is honoured, unquoted so that it can carry options, for the machine
# whose awk turns out to be the one with the bug.  The program itself is
# POSIX awk and nothing more.
#
# Wholly blank leading and trailing rows are trimmed, interior blank rows are
# kept: the blank rows inside the drawing are part of it, while the ones
# around the edge are just how the file was saved, and they would otherwise
# push the logo down the panel and pad the row block out to the right.
# Trailing spaces within a row are kept, so the art stays the rectangle it
# was drawn as and the column budget the panel computes from it is the
# footprint the art claims.
#
# Art bigger than the buffers it is loaded into is refused here, for the same
# reason art off the ramp is: tc.fetch.c keeps a logo in
# FETCH_LOGO_ROWS x FETCH_LOGO_COLS and cuts anything past that at run time,
# so a header generated from art that had outgrown them would compile, draw
# the top of the picture, and say nothing about the rest.  The limits are
# passed in rather than written into the awk program so that there is one
# place to change when the macros change, and the message names the macros so
# that the two can be found from each other.
#
array() {
    ${AWK:-awk} -v name="$2" -v what="$3" -v src="$4" \
		-v maxrows="$FETCH_LOGO_ROWS" -v maxcols="$FETCH_LOGO_COLS" '
	{
	    rows[NR] = $0
	    n = NR
	}
	END {
	    first = 1
	    last = n
	    while (first <= last && rows[first] ~ /^ *$/)
		first++
	    while (last >= first && rows[last] ~ /^ *$/)
		last--
	    if (last < first) {
		print "mklogo.sh: " src " holds no art at all" | "cat 1>&2"
		exit 1
	    }
	    w = 0
	    for (i = first; i <= last; i++)
		if (length(rows[i]) > w)
		    w = length(rows[i])
	    if (last - first + 1 > maxrows) {
		printf "mklogo.sh: %s: %d rows of art, past the %d of " \
		       "FETCH_LOGO_ROWS\n", src, last - first + 1, maxrows \
		    | "cat 1>&2"
		exit 1
	    }
	    if (w > maxcols - 1) {
		printf "mklogo.sh: %s: rows of %d columns, past the %d of " \
		       "FETCH_LOGO_COLS less its NUL\n", src, w, maxcols - 1 \
		    | "cat 1>&2"
		exit 1
	    }
	    printf "/*\n * %s: %d rows of %d columns, from %s.\n */\n", \
		   what, last - first + 1, w, src
	    printf "static const char * const %s[] = {\n", name
	    for (i = first; i <= last; i++)
		printf "    \"%s\",\n", esc(rows[i])
	    printf "    NULL\n};\n"
	}
	function esc(s,    out, k, c) {
	    out = ""
	    for (k = 1; k <= length(s); k++) {
		c = substr(s, k, 1)
		if (c == "\\" || c == "\"" || c == "?")
		    out = out "\\"
		out = out c
	    }
	    return out
	}
    ' "$1"
}

#
# wordmark - the smallest tier.
#
# This one lives in the script rather than under logos/ because it is not art
# on the ramp at all: it is type, drawn out of the characters a C compiler
# cares about, and it is the tier that must never be scaled.  Keeping it here
# keeps it out of reach of the ramp check and the scaler both, and the rows
# are emitted exactly as they stood in tc.fetch.c - escaped backslashes and
# all - so that moving them here cannot have changed what is drawn.
#
wordmark() {
    cat <<'EOF'
/*
 * The wordmark tier: 5 rows of 28 columns, for a terminal too narrow for a
 * drawing.
 */
static const char * const fetch_logo_wordmark[] = {
    "                      _     ",
    "  _ __ ___   ___ ___ | |__  ",
    " | '_ ` _ \\ / __/ __|| '_ \\ ",
    " | | | | | | (__\\__ \\| | | |",
    " |_| |_| |_|\\___|___/|_| |_|",
    NULL
};
EOF
}

#
# emit - the whole header.  Every step is checked: a function here is a
# pipeline of several commands, and the status of the last of them would say
# nothing about an awk that died on the way.
#
emit() {
    banner || return 1
    printf '\n'
    array "$full" fetch_logo_full \
	  "The full tier" "\`logos/mcsh.txt'" || return 1
    printf '\n'
    array "$mid" fetch_logo_mid \
	  "The mid tier, hand-tuned" "\`logos/mcsh-mid.txt'" || return 1
    printf '\n'
    wordmark || return 1
}

if [ "x$out" = "x-" ]; then
    emit || die "generation failed"
    exit 0
fi

#
# Written to a temporary file and moved into place only if it differs, the
# way the Makefile treats tc.defs.c: a header whose timestamp moved for no
# reason costs everyone who includes it a rebuild.
#
tmp=$out.tmp
trap 'rm -f "$tmp"' 0 1 2 3 13 15
emit > "$tmp" || die "generation failed, \`$out' left alone"
if [ -f "$out" ] && cmp -s "$tmp" "$out"; then
    rm -f "$tmp"
    printf '%s unchanged.\n' "$out"
else
    mv -f "$tmp" "$out" || die "cannot write \`$out'"
    printf '%s recreated.\n' "$out"
fi
