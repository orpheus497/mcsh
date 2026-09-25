/*
 * tc.fetch.c: The `sysinfo' builtin - a system information panel.
 *
 * A summary of the machine the shell is running on, printed on demand by the
 * `sysinfo' builtin and, when `set sysinfo' is in effect, once when an
 * interactive shell starts.
 *
 * Everything here is read with ordinary file and system calls: nothing is
 * shelled out to, no external program is required, and no field is guessed.
 * A value that cannot be determined is omitted rather than approximated, so
 * the panel never reports something it has not actually read.
 *
 * The panel is configurable through
 *
 *	$XDG_CONFIG_HOME/mcsh/sysinfo.conf	(else ~/.config/mcsh/sysinfo.conf)
 *
 * which selects the logo, the colours, the field list and the layout; see
 * fetch_conf_set() for the full key list.  The logo may be any text file,
 * and one named after the distribution's os-release(5) ID is picked up
 * automatically from ~/.config/mcsh/logos/.
 */
/*-
 * Copyright (c) 2026 The mcsh Contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "sh.h"
#include "ed.h"
#include "tc.h"
#include "patchlevel.h"

#include <stdio.h>
#include <sys/utsname.h>

/*
 * statvfs(3) is specified by POSIX.1-2001 (IEEE Std 1003.1-2001,
 * <sys/statvfs.h>), but this shell still builds for systems older than that
 * standard (see system/: hpux8, bsdreno, irix, ...).  Rather than add a
 * configure probe for one optional line of the panel, the filesystem row is
 * compiled in only for the platforms where the header is known to exist, and
 * omitted everywhere else.
 */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__) || \
    defined(__sun) || defined(_AIX) || defined(__CYGWIN__) || \
    defined(__HAIKU__) || defined(__GNU__)
# define FETCH_HAVE_STATVFS	1
# include <sys/statvfs.h>
#endif

/*
 * getifaddrs(3) is not in POSIX at all - it is a BSD interface that glibc,
 * macOS and Solaris 11 also provide - and configure does not probe for it
 * either.  Same treatment as statvfs(3): an explicit list of the platforms
 * where <ifaddrs.h> is known to exist, and no Local IP row anywhere else.
 */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
# define FETCH_HAVE_IFADDRS	1
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <ifaddrs.h>
# include <net/if.h>
#endif

#define FETCH_MAX_ROWS	64	/* hard ceiling on panel rows */
#define FETCH_VAL_MAX	192	/* longest value kept, in bytes */
#define FETCH_PATH_MAX	1024	/* longest path this file ever builds */
#define FETCH_LOGO_ROWS	64	/* hard ceiling on logo rows */
#define FETCH_LOGO_COLS	512	/* longest logo row kept, in bytes */

/*
 * The built-in logo.  Deliberately 7-bit ASCII: a box-drawing or
 * block-element logo would be mojibake in a non-UTF-8 locale and on a
 * terminal without the glyphs, and the panel has to be legible on a serial
 * console.  Anyone who wants better has the `logo' configuration key.
 */
static const char * const fetch_builtin_logo[] = {
    "                      _     ",
    "  _ __ ___   ___ ___ | |__  ",
    " | '_ ` _ \\ / __/ __|| '_ \\ ",
    " | | | | | | (__\\__ \\| | | |",
    " |_| |_| |_|\\___|___/|_| |_|",
    NULL
};

static struct fetch_row {
    const char *label;			/* NULL for the header rows */
    char	value[FETCH_VAL_MAX];
} fetch_rows[FETCH_MAX_ROWS];
static int fetch_nrows;

/* The logo actually in use, built-in or loaded, one row per line. */
static char fetch_logo[FETCH_LOGO_ROWS][FETCH_LOGO_COLS];
static int fetch_logo_rows;
static int fetch_logo_w;		/* display width of the widest row */

/*
 * The configuration, as read from sysinfo.conf.  Every member is set to its
 * default by fetch_conf_defaults() before the file is read, so a missing or
 * unreadable file simply leaves the defaults in place.
 */
static struct fetch_conf {
    char logo[FETCH_PATH_MAX];	/* path, "none", or "" for the built-in */
    char logo_color[32];	/* SGR parameters, "" for none */
    char label_color[32];
    char title_color[32];
    char separator[16];		/* printed between label and value */
    char show[512];		/* comma list: these fields, in this order */
    char hide[512];		/* comma list: all fields but these */
    int  label_width;		/* label is padded to this many columns */
    int  palette;		/* draw the two colour-swatch rows */
    int  color;			/* colour at all (overridden by -C/-c) */
} fetch_conf;

static int fetch_color;		/* colour this run: conf, tty and terminal */

/*
 * fetch_add - append one row.  Silently ignores the row if the table is full
 * or the value is empty, which is what makes "omit what cannot be read" a
 * single check at the point of use rather than at every call site.
 */
static void
fetch_add(const char *label, const char *value)
{
    char *p;

    if (fetch_nrows >= FETCH_MAX_ROWS || value == NULL || *value == '\0')
	return;
    fetch_rows[fetch_nrows].label = label;
    (void) xsnprintf(fetch_rows[fetch_nrows].value,
		     sizeof(fetch_rows[0].value), "%s", value);
    /*
     * Every value here comes from outside the shell - $TERM, os-release,
     * /proc, the password database - and fetch_print() writes with output_raw
     * set so that its own SGR sequences survive xputchar().  A control
     * character inside a value would therefore reach the terminal, or a
     * redirected file, exactly as it stands: TERM='xterm\033[31mINJECTED' put a
     * live escape sequence in the panel.  Replaced rather than dropped, so the
     * value's length still tells you something was there.
     */
    for (p = fetch_rows[fetch_nrows].value; *p != '\0'; p++)
	if (iscntrl((unsigned char) *p))
	    *p = '?';
    fetch_nrows++;
}

/*
 * fetch_trim - strip leading and trailing whitespace, and one layer of
 * surrounding double quotes, in place.  os-release(5) permits the value to be
 * quoted; /proc/meminfo and /proc/cpuinfo pad theirs with tabs and spaces;
 * the GTK settings files quote some values and not others.
 */
static void
fetch_trim(char *s)
{
    char *p = s;
    size_t len;

    while (*p == ' ' || *p == '\t')
	p++;
    len = strlen(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t' ||
		       p[len - 1] == '\n' || p[len - 1] == '\r'))
	p[--len] = '\0';
    if (len >= 2 && p[0] == '"' && p[len - 1] == '"') {
	p[len - 1] = '\0';
	p++;
	len -= 2;
    }
    if (p != s)
	memmove(s, p, len + 1);
}

/*
 * fetch_line - copy the first line of path into buf.  Returns 1 on success.
 */
static int
fetch_line(const char *path, char *buf, size_t bufsz)
{
    FILE *fp = fopen(path, "r");
    int ok = 0;

    if (fp == NULL)
	return 0;
    if (fgets(buf, (int) bufsz, fp) != NULL) {
	fetch_trim(buf);
	ok = (*buf != '\0');
    }
    (void) fclose(fp);
    return ok;
}

/*
 * fetch_key - find the first line of path whose key part is exactly key, and
 * copy its value into buf.  The key is terminated by optional whitespace
 * followed by '=' or ':', which covers every format the panel reads:
 *
 *   os-release(5)     PRETTY_NAME="Debian GNU/Linux 12 (bookworm)"
 *   proc(5) meminfo   MemTotal:       16318296 kB
 *   proc(5) cpuinfo   model name	: AMD EPYC 7B12
 *   GTK settings.ini  gtk-theme-name=Adwaita-dark
 *
 * Returns 1 on success, 0 if the file or the key is absent.
 */
static int
fetch_key(const char *path, const char *key, char *buf, size_t bufsz)
{
    FILE *fp;
    char line[512];
    size_t keylen = strlen(key);
    int ok = 0;

    fp = fopen(path, "r");
    if (fp == NULL)
	return 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
	char *p;

	if (strncmp(line, key, keylen) != 0)
	    continue;
	p = line + keylen;
	while (*p == ' ' || *p == '\t')
	    p++;
	if (*p != '=' && *p != ':')
	    continue;		/* "MemTotalFoo:" is not "MemTotal" */
	p++;
	(void) xsnprintf(buf, bufsz, "%s", p);
	fetch_trim(buf);
	ok = (*buf != '\0');
	break;
    }
    (void) fclose(fp);
    return ok;
}

/*
 * fetch_size - render a byte count as a binary-prefixed size with one
 * fractional digit, e.g. "15.6 GiB".  The prefixes are the binary ones
 * (KiB, MiB, GiB ... = powers of 1024, as standardised by IEC 80000-13), not
 * the decimal SI ones, so the number means exactly what it says.
 *
 * Integer arithmetic only: the shell's own xsnprintf() (tc.printf.c) is a
 * minimal implementation with no floating-point conversion at all.
 */
static void
fetch_size(unsigned long long bytes, char *buf, size_t bufsz)
{
    static const char * const unit[] = { "B", "KiB", "MiB", "GiB", "TiB",
					 "PiB" };
    unsigned long long scale = 1;
    int u = 0;

    while (u + 1 < (int) (sizeof(unit) / sizeof(unit[0])) &&
	   bytes / scale >= 1024) {
	scale *= 1024;
	u++;
    }
    /*
     * Printed as unsigned long, not unsigned long long: after scaling, the
     * whole part is below 1024 for every unit but the last and the fractional
     * part is a single digit, so both always fit.  The shell's xsnprintf()
     * (tc.printf.c) only reads a long long argument for "%llu" when the build
     * defines HAVE_LONG_LONG, and there is no reason to depend on that here.
     */
    if (u == 0)
	(void) xsnprintf(buf, bufsz, "%lu B", (unsigned long) bytes);
    else {
	unsigned long whole = (unsigned long) (bytes / scale);
	unsigned long frac = (unsigned long) (((bytes % scale) * 10) / scale);

	(void) xsnprintf(buf, bufsz, "%lu.%lu %s", whole, frac, unit[u]);
    }
}

/*
 * fetch_usage - "1.2 GiB / 15.6 GiB (8%)".  The percentage is of the total,
 * rounded towards zero; total == 0 is the caller's problem, not this one's.
 */
static void
fetch_usage(unsigned long long used, unsigned long long total,
	    char *buf, size_t bufsz)
{
    char used_s[48], total_s[48];

    fetch_size(used, used_s, sizeof(used_s));
    fetch_size(total, total_s, sizeof(total_s));
    (void) xsnprintf(buf, bufsz, "%s / %s (%lu%%)", used_s, total_s,
		     (unsigned long) ((used * 100) / total));
}

/*
 * fetch_duration - render a number of seconds as "1 day, 2 hours, 3 mins",
 * dropping the units that are zero, and never printing seconds unless the
 * whole duration is under a minute.
 */
static void
fetch_duration(unsigned long secs, char *buf, size_t bufsz)
{
    unsigned long d = secs / 86400;
    unsigned long h = (secs % 86400) / 3600;
    unsigned long m = (secs % 3600) / 60;
    size_t n = 0;
    int w;

    buf[0] = '\0';
    if (d > 0) {
	w = xsnprintf(buf + n, bufsz - n, "%lu day%s", d, d == 1 ? "" : "s");
	if (w < 0 || (size_t) w >= bufsz - n)
	    return;
	n += w;
    }
    if (h > 0) {
	w = xsnprintf(buf + n, bufsz - n, "%s%lu hour%s", n ? ", " : "",
		      h, h == 1 ? "" : "s");
	if (w < 0 || (size_t) w >= bufsz - n)
	    return;
	n += w;
    }
    if (m > 0 || n == 0) {
	if (n == 0 && d == 0 && h == 0 && m == 0)
	    (void) xsnprintf(buf, bufsz, "%lu secs", secs);
	else
	    (void) xsnprintf(buf + n, bufsz - n, "%s%lu min%s", n ? ", " : "",
			     m, m == 1 ? "" : "s");
    }
}

/*
 * fetch_env - getenv() that treats an empty value as absent.  Every
 * environment-derived row in this file wants that: DE="" means no desktop,
 * not a desktop whose name is the empty string.
 */
static const char *
fetch_env(const char *name)
{
    const char *v = getenv(name);

    return (v != NULL && *v != '\0') ? v : NULL;
}

/*
 * fetch_home - the home directory the configuration lives under.  $HOME
 * first, as every other user-configuration lookup in the shell does, then
 * the password database for a shell started without one.
 */
static const char *
fetch_home(void)
{
    const char *home = fetch_env("HOME");
    struct passwd *pw;

    if (home != NULL)
	return home;
    pw = xgetpwuid(getuid());
    return (pw != NULL && pw->pw_dir[0] != '\0') ? pw->pw_dir : NULL;
}

/*
 * fetch_confdir - the mcsh configuration directory, per the XDG Base
 * Directory Specification: $XDG_CONFIG_HOME if set to an absolute path,
 * otherwise $HOME/.config.  Returns 0 if neither can be determined.
 */
static int
fetch_confdir(char *buf, size_t bufsz)
{
    const char *x = fetch_env("XDG_CONFIG_HOME");
    const char *home;

    if (x != NULL && x[0] == '/') {
	(void) xsnprintf(buf, bufsz, "%s/mcsh", x);
	return 1;
    }
    home = fetch_home();
    if (home == NULL)
	return 0;
    (void) xsnprintf(buf, bufsz, "%s/.config/mcsh", home);
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * Configuration.
 *
 * sysinfo.conf is a flat list of `key = value' lines.  A '#' begins a comment
 * that runs to the end of the line, blank lines are ignored, and whitespace
 * around both key and value is stripped.  There are no sections, no
 * continuations and no includes: this is a dozen settings, not a language.
 * ---------------------------------------------------------------------------
 */

static void
fetch_conf_defaults(void)
{
    fetch_conf.logo[0] = '\0';
    (void) xsnprintf(fetch_conf.logo_color, sizeof(fetch_conf.logo_color),
		     "1;36");
    (void) xsnprintf(fetch_conf.label_color, sizeof(fetch_conf.label_color),
		     "1;34");
    (void) xsnprintf(fetch_conf.title_color, sizeof(fetch_conf.title_color),
		     "1;36");
    (void) xsnprintf(fetch_conf.separator, sizeof(fetch_conf.separator), " ");
    fetch_conf.show[0] = '\0';
    fetch_conf.hide[0] = '\0';
    fetch_conf.label_width = 9;
    fetch_conf.palette = 1;
    fetch_conf.color = 1;
}

/*
 * fetch_conf_bool - "on", "yes", "true" and "1" are true; "off", "no",
 * "false" and "0" are false.  Returns -1 for anything else so the caller can
 * report it rather than silently picking one.
 */
static int
fetch_conf_bool(const char *v)
{
    if (strcmp(v, "on") == 0 || strcmp(v, "yes") == 0 ||
	strcmp(v, "true") == 0 || strcmp(v, "1") == 0)
	return 1;
    if (strcmp(v, "off") == 0 || strcmp(v, "no") == 0 ||
	strcmp(v, "false") == 0 || strcmp(v, "0") == 0)
	return 0;
    return -1;
}

/*
 * fetch_conf_sgr - accept an SGR parameter string.
 *
 * The value is interpolated straight into an ECMA-48 CSI sequence, so it is
 * restricted to what can appear between CSI and the final 'm': digits and
 * the ';' separator (ECMA-48 5th edition, 5.4 - parameter bytes are 03/00 to
 * 03/09 and 03/11).  Anything else is refused, which is what stops a
 * configuration file from moving the cursor, clearing the screen or opening a
 * device-control string.  The empty value means "no colour for this element".
 */
static int
fetch_conf_sgr(const char *v)
{
    const char *p;

    for (p = v; *p != '\0'; p++)
	if (!isdigit((unsigned char) *p) && *p != ';')
	    return 0;
    return 1;
}

/*
 * fetch_conf_set - apply one key/value pair.  Returns 0 and leaves the
 * setting alone if the key is unknown or the value is not acceptable, so the
 * caller can name the offending line.
 *
 * The keys:
 *
 *   logo         path of a text file to draw down the left, or `none'.
 *                Unset: ~/.config/mcsh/logos/$ID.txt if it exists, where $ID
 *                is the os-release(5) ID, else the built-in logo.
 *   logo_color   SGR parameters for the logo, e.g. "1;35".  Ignored for a
 *                logo row that carries escape sequences of its own.
 *   label_color  SGR parameters for the field labels.
 *   title_color  SGR parameters for the user@host row and its rule.
 *   separator    text between the padded label and the value.  Default " ";
 *                ": " and " -> " are the other obvious ones.
 *   label_width  columns the label is padded to.  0 turns padding off.
 *   palette      on/off: the two rows of background-colour swatches.
 *   color        on/off: colour at all.  Forced off when the output is not a
 *                terminal or the terminal does not advertise colour.
 *   show         comma-separated field names.  Only these are collected, in
 *                exactly this order.
 *   hide         comma-separated field names to leave out.  Applied after
 *                `show', so hiding a field that `show' names drops it.
 *
 * `sysinfo -l' lists the field names.
 */
static int
fetch_conf_set(const char *key, const char *val)
{
    int b;

    if (strcmp(key, "logo") == 0) {
	(void) xsnprintf(fetch_conf.logo, sizeof(fetch_conf.logo), "%s", val);
	return 1;
    }
    if (strcmp(key, "logo_color") == 0) {
	if (!fetch_conf_sgr(val))
	    return 0;
	(void) xsnprintf(fetch_conf.logo_color,
			 sizeof(fetch_conf.logo_color), "%s", val);
	return 1;
    }
    if (strcmp(key, "label_color") == 0) {
	if (!fetch_conf_sgr(val))
	    return 0;
	(void) xsnprintf(fetch_conf.label_color,
			 sizeof(fetch_conf.label_color), "%s", val);
	return 1;
    }
    if (strcmp(key, "title_color") == 0) {
	if (!fetch_conf_sgr(val))
	    return 0;
	(void) xsnprintf(fetch_conf.title_color,
			 sizeof(fetch_conf.title_color), "%s", val);
	return 1;
    }
    if (strcmp(key, "separator") == 0) {
	(void) xsnprintf(fetch_conf.separator, sizeof(fetch_conf.separator),
			 "%s", val);
	return 1;
    }
    if (strcmp(key, "label_width") == 0) {
	char *end;
	long n;

	errno = 0;
	n = strtol(val, &end, 10);
	if (errno != 0 || end == val || *end != '\0' || n < 0 || n > 64)
	    return 0;
	fetch_conf.label_width = (int) n;
	return 1;
    }
    if (strcmp(key, "palette") == 0) {
	if ((b = fetch_conf_bool(val)) < 0)
	    return 0;
	fetch_conf.palette = b;
	return 1;
    }
    if (strcmp(key, "color") == 0 || strcmp(key, "colour") == 0) {
	if ((b = fetch_conf_bool(val)) < 0)
	    return 0;
	fetch_conf.color = b;
	return 1;
    }
    if (strcmp(key, "show") == 0) {
	(void) xsnprintf(fetch_conf.show, sizeof(fetch_conf.show), "%s", val);
	return 1;
    }
    if (strcmp(key, "hide") == 0) {
	(void) xsnprintf(fetch_conf.hide, sizeof(fetch_conf.hide), "%s", val);
	return 1;
    }
    return 0;
}

/*
 * fetch_warn_file, fetch_warn_line - one diagnostic line, on the
 * diagnostic output.
 *
 * Not stderror(): that longjmps out of the builtin, and a typo in a
 * configuration file must not stop the panel - let alone stop an interactive
 * shell from finishing its start-up.  Not plain xprintf() either, because
 * that would put the complaint in the panel itself, and in the file when the
 * panel is redirected.  Setting haderr makes flush() pick descriptor 2 /
 * SHDIAG instead of 1 / SHOUT, which is the same mechanism sh.exec.c and
 * tc.func.c use to write a warning without raising an error.
 *
 * Deliberately not variadic: doprnt() (tc.printf.c) is the shell's own
 * minimal printf and has no vprintf entry point, so a wrapper would have to
 * duplicate it.  Two fixed shapes cover every complaint this file makes.
 */
static void
fetch_warn_file(const char *path, const char *msg)
{
    int oldhaderr = haderr;

    flush();			/* do not mix with whatever is buffered */
    haderr = 1;
    xprintf("sysinfo: %s: %s\n", path, msg);
    flush();
    haderr = oldhaderr;
}

static void
fetch_warn_line(const char *path, int lineno, const char *msg)
{
    int oldhaderr = haderr;

    flush();
    haderr = 1;
    xprintf("sysinfo: %s:%d: %s\n", path, lineno, msg);
    flush();
    haderr = oldhaderr;
}

/*
 * fetch_conf_read - read path, applying every line.  A line that cannot be
 * applied is reported on the diagnostic output with its file and line number:
 * a configuration file whose typos are silently ignored is a configuration
 * file you cannot debug.  Returns 1 if the file was opened at all.
 */
static int
fetch_conf_read(const char *path)
{
    FILE *fp = fopen(path, "r");
    char line[1024];
    int lineno = 0;

    if (fp == NULL)
	return 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
	char *hash, *eq, *key, *val;

	lineno++;
	if ((hash = strchr(line, '#')) != NULL)
	    *hash = '\0';
	if ((eq = strchr(line, '=')) == NULL) {
	    fetch_trim(line);
	    if (line[0] != '\0')
		fetch_warn_line(path, lineno, "not a `key = value' line");
	    continue;
	}
	*eq = '\0';
	key = line;
	val = eq + 1;
	fetch_trim(key);
	fetch_trim(val);
	if (key[0] == '\0')
	    continue;
	if (!fetch_conf_set(key, val)) {
	    char msg[256];

	    (void) xsnprintf(msg, sizeof(msg), "bad setting `%s = %s'",
			     key, val);
	    fetch_warn_line(path, lineno, msg);
	}
    }
    (void) fclose(fp);
    return 1;
}

/*
 * ---------------------------------------------------------------------------
 * The logo.
 * ---------------------------------------------------------------------------
 */

/*
 * fetch_dwidth - how many columns a logo row occupies.
 *
 * Escape sequences are skipped, because a logo file is free to colour itself
 * and those bytes take no space on the screen.  Two forms are recognised, as
 * they are the only two a text file realistically carries: a CSI sequence
 * (ECMA-48 5.4: ESC '[', parameter and intermediate bytes, then a final byte
 * in the range 04/00 to 07/14), and an OSC string (ECMA-48 8.3.89: ESC ']'
 * up to BEL or ST).  Everything else after an ESC is taken as a two-character
 * escape sequence.
 *
 * What survives is measured with the shell's own width model, NLSStringWidth()
 * (tc.nls.c), so a double-width glyph in a logo counts for the two columns it
 * really takes and the rows beside it stay aligned.
 */
static int
fetch_dwidth(const char *s)
{
    char plain[FETCH_LOGO_COLS];
    size_t n = 0;
    const char *p = s;

    while (*p != '\0' && n + 1 < sizeof(plain)) {
	if (*p != '\033') {
	    plain[n++] = *p++;
	    continue;
	}
	p++;				/* the ESC itself */
	if (*p == '[') {
	    p++;
	    while (*p != '\0' && (unsigned char) *p < 0x40)
		p++;			/* parameter and intermediate bytes */
	    if (*p != '\0')
		p++;			/* the final byte */
	}
	else if (*p == ']') {
	    p++;
	    while (*p != '\0' && *p != '\007') {
		if (*p == '\033' && p[1] == '\\') {
		    p++;
		    break;
		}
		p++;
	    }
	    if (*p != '\0')
		p++;
	}
	else if (*p != '\0')
	    p++;
    }
    plain[n] = '\0';
    return NLSStringWidth(str2short(plain));
}

/*
 * fetch_logo_load - fill fetch_logo[] from a file.  Returns 0 if the file
 * cannot be read, leaving the logo untouched.
 *
 * Tabs are expanded to the next multiple of eight and trailing newlines are
 * dropped; nothing else in the row is altered.  In particular escape
 * sequences are passed through, which is the point: this is the user's own
 * file, and refusing them would rule out every coloured logo there is.  (A
 * value read from the system is a different matter - see fetch_add().)
 */
static int
fetch_logo_load(const char *path)
{
    FILE *fp = fopen(path, "r");
    char line[FETCH_LOGO_COLS * 2];
    int rows = 0;

    if (fp == NULL)
	return 0;
    while (rows < FETCH_LOGO_ROWS && fgets(line, sizeof(line), fp) != NULL) {
	char *dst = fetch_logo[rows];
	size_t n = 0, col = 0;
	char *p;

	for (p = line; *p != '\0' && n + 1 < FETCH_LOGO_COLS; p++) {
	    if (*p == '\n' || *p == '\r')
		break;
	    if (*p == '\t') {
		size_t pad = 8 - (col % 8);

		while (pad-- > 0 && n + 1 < FETCH_LOGO_COLS) {
		    dst[n++] = ' ';
		    col++;
		}
		continue;
	    }
	    dst[n++] = *p;
	    /* Only counted for the tab stops; the real width comes from
	     * fetch_dwidth() below. */
	    if ((*p & 0xC0) != 0x80)
		col++;
	}
	dst[n] = '\0';
	rows++;
    }
    (void) fclose(fp);
    if (rows == 0)
	return 0;
    fetch_logo_rows = rows;
    return 1;
}

/*
 * fetch_logo_builtin - fill fetch_logo[] from fetch_builtin_logo[].
 */
static void
fetch_logo_builtin(void)
{
    int i;

    for (i = 0; i < FETCH_LOGO_ROWS && fetch_builtin_logo[i] != NULL; i++)
	(void) xsnprintf(fetch_logo[i], FETCH_LOGO_COLS, "%s",
			 fetch_builtin_logo[i]);
    fetch_logo_rows = i;
}

/*
 * fetch_logo_select - decide which logo to draw, in this order:
 *
 *   1. `logo = none'                    no logo at all
 *   2. `logo = /path/to/file'           that file
 *   3. ~/.config/mcsh/logos/$ID.txt     the distribution's own, if the user
 *                                       has put one there, named after the
 *                                       os-release(5) ID field
 *   4. the built-in
 *
 * There is deliberately no bundled per-distribution logo set: the shell would
 * have to ship and maintain a hundred pieces of ASCII art it cannot verify,
 * and an out-of-date one is worse than none.  The lookup in 3 makes adding
 * one a matter of dropping a text file in place.
 */
static void
fetch_logo_select(void)
{
    char path[FETCH_PATH_MAX];
    char id[128];

    fetch_logo_rows = 0;
    fetch_logo_w = 0;

    if (strcmp(fetch_conf.logo, "none") == 0)
	return;
    if (fetch_conf.logo[0] != '\0') {
	if (!fetch_logo_load(fetch_conf.logo)) {
	    fetch_warn_file(fetch_conf.logo, "cannot read logo");
	    fetch_logo_builtin();
	}
    }
    else {
	char dir[FETCH_PATH_MAX];
	int got = 0;

	/*
	 * A '/' in the ID would let os-release(5) name a path outside the
	 * logo directory, so the field is used only when it is a bare name.
	 */
	if ((fetch_key("/etc/os-release", "ID", id, sizeof(id)) ||
	     fetch_key("/usr/lib/os-release", "ID", id, sizeof(id))) &&
	    strchr(id, '/') == NULL && fetch_confdir(dir, sizeof(dir))) {
	    (void) xsnprintf(path, sizeof(path), "%s/logos/%s.txt", dir, id);
	    got = fetch_logo_load(path);
	}
	if (!got)
	    fetch_logo_builtin();
    }

    {
	int i;

	for (i = 0; i < fetch_logo_rows; i++) {
	    int w = fetch_dwidth(fetch_logo[i]);

	    if (w > fetch_logo_w)
		fetch_logo_w = w;
	}
    }
}

/*
 * ---------------------------------------------------------------------------
 * The collectors.  Each reads what it can and calls fetch_add(); a failure
 * leaves the row out of the table entirely.
 *
 * uname(2) is called once by fetch_collect() and kept here, so that every
 * collector can have the same void signature and go straight into the field
 * table below.
 * ---------------------------------------------------------------------------
 */
static struct utsname fetch_uts;
static int fetch_have_uts;

static void
fetch_identity(void)
{
    char buf[FETCH_VAL_MAX];
    struct passwd *pw;
    const char *user = NULL;

    /* $user is what the shell itself resolved at startup; fall back to the
     * password database for a shell started with -f in an odd environment. */
    if (varval(STRuser) != STRNULL)
	user = short2str(varval(STRuser));
    if (user == NULL || *user == '\0') {
	pw = xgetpwuid(getuid());
	user = (pw != NULL) ? pw->pw_name : NULL;
    }
    if (user == NULL || *user == '\0')
	return;

    (void) xsnprintf(buf, sizeof(buf), "%s@%s", user,
		     (fetch_have_uts && fetch_uts.nodename[0] != '\0')
		     ? fetch_uts.nodename : "localhost");
    fetch_add(NULL, buf);

    /* A rule the same width as the header, as every panel of this kind has
     * had since screenfetch: it separates the identity from the facts. */
    {
	size_t n = strlen(buf);
	char rule[FETCH_VAL_MAX];

	if (n >= sizeof(rule))
	    n = sizeof(rule) - 1;
	memset(rule, '-', n);
	rule[n] = '\0';
	fetch_add(NULL, rule);
    }
}

static void
fetch_os(void)
{
    char buf[FETCH_VAL_MAX];
    char name[FETCH_VAL_MAX];

    /* os-release(5): /etc/os-release, with /usr/lib/os-release as the
     * vendor-supplied fallback the specification mandates. */
    if (!fetch_key("/etc/os-release", "PRETTY_NAME", name, sizeof(name)) &&
	!fetch_key("/usr/lib/os-release", "PRETTY_NAME", name, sizeof(name))) {
	if (!fetch_have_uts)
	    return;
	(void) xsnprintf(name, sizeof(name), "%s %s", fetch_uts.sysname,
			 fetch_uts.release);
    }
    /* The machine type belongs on this row, not on one of its own: what a
     * person wants to know is "which system, built for what". */
    if (fetch_have_uts && fetch_uts.machine[0] != '\0')
	(void) xsnprintf(buf, sizeof(buf), "%s %s", name, fetch_uts.machine);
    else
	(void) xsnprintf(buf, sizeof(buf), "%s", name);
    fetch_add("OS", buf);
}

/*
 * fetch_host - the machine itself.
 *
 * On a PC this is SMBIOS/DMI, which Linux exports one field per file under
 * /sys/class/dmi/id (Linux Documentation/ABI/testing/sysfs-class-dmi-id);
 * product_name and product_version together are what the vendor calls the
 * model.  On a board with no firmware tables it is the device tree's `model'
 * property instead, which the kernel exports at
 * /sys/firmware/devicetree/base/model as a NUL-terminated string.
 *
 * Both are readable by any user on the systems that have them and absent on
 * the systems that do not, so there is nothing to fall back to: a virtual
 * machine without DMI simply has no Host row.
 */
static void
fetch_host(void)
{
    char buf[FETCH_VAL_MAX];
    char name[FETCH_VAL_MAX], version[FETCH_VAL_MAX];

    if (fetch_line("/sys/class/dmi/id/product_name", name, sizeof(name))) {
	if (fetch_line("/sys/class/dmi/id/product_version", version,
		       sizeof(version)))
	    (void) xsnprintf(buf, sizeof(buf), "%s %s", name, version);
	else
	    (void) xsnprintf(buf, sizeof(buf), "%s", name);
	fetch_add("Host", buf);
	return;
    }
    if (fetch_line("/sys/firmware/devicetree/base/model", name, sizeof(name)))
	fetch_add("Host", name);
}

static void
fetch_kernel(void)
{
    char buf[FETCH_VAL_MAX];

    if (!fetch_have_uts)
	return;
    (void) xsnprintf(buf, sizeof(buf), "%s %s", fetch_uts.sysname,
		     fetch_uts.release);
    fetch_add("Kernel", buf);
}

static void
fetch_uptime(void)
{
    char buf[FETCH_VAL_MAX];
    char line[128];
    char *end;
    unsigned long secs;

    /*
     * /proc/uptime (proc(5)) holds two numbers: seconds since boot, and
     * seconds spent idle.  Only the first is wanted.  There is no portable
     * interface for this - the BSDs expose kern.boottime through sysctl(3),
     * spelled differently on each - so the row is Linux-only and simply
     * absent elsewhere.
     */
    if (!fetch_line("/proc/uptime", line, sizeof(line)))
	return;
    errno = 0;
    secs = strtoul(line, &end, 10);
    if (errno != 0 || end == line)
	return;
    fetch_duration(secs, buf, sizeof(buf));
    fetch_add("Uptime", buf);
}

/*
 * fetch_dircount - how many entries a directory has, ignoring "." and ".."
 * and, when subdirs_only is set, anything that is not itself a directory.
 * Returns -1 if the directory cannot be opened.
 *
 * d_type is not used: it is not in POSIX and several filesystems report
 * DT_UNKNOWN for everything.  lstat(2) on each entry is the portable answer
 * and these directories have hundreds of entries, not millions.
 */
static long
fetch_dircount(const char *path, int subdirs_only)
{
    DIR *dp = opendir(path);
    struct dirent *de;
    long n = 0;

    if (dp == NULL)
	return -1;
    while ((de = readdir(dp)) != NULL) {
	if (de->d_name[0] == '.' &&
	    (de->d_name[1] == '\0' ||
	     (de->d_name[1] == '.' && de->d_name[2] == '\0')))
	    continue;
	if (subdirs_only) {
	    char sub[FETCH_PATH_MAX];
	    struct stat st;

	    (void) xsnprintf(sub, sizeof(sub), "%s/%s", path, de->d_name);
	    if (lstat(sub, &st) != 0 || !S_ISDIR(st.st_mode))
		continue;
	}
	n++;
    }
    (void) closedir(dp);
    return n;
}

/*
 * fetch_count_lines - how many lines of path begin with prefix, or, when
 * value is non-NULL, how many are exactly "prefix value" once the whitespace
 * after the prefix is collapsed.  Returns -1 if the file cannot be read.
 */
static long
fetch_count_lines(const char *path, const char *prefix, const char *value)
{
    FILE *fp = fopen(path, "r");
    char line[1024];
    size_t plen = strlen(prefix);
    long n = 0;

    if (fp == NULL)
	return -1;
    while (fgets(line, sizeof(line), fp) != NULL) {
	if (strncmp(line, prefix, plen) != 0)
	    continue;
	if (value != NULL) {
	    char *p = line + plen;

	    while (*p == ' ' || *p == '\t')
		p++;
	    fetch_trim(p);
	    if (strcmp(p, value) != 0)
		continue;
	}
	n++;
    }
    (void) fclose(fp);
    return n;
}

/*
 * fetch_portage_count - Portage records one directory per installed package,
 * two levels deep: /var/db/pkg/<category>/<package>-<version>.  So the count
 * is the sum of the subdirectory counts of every category directory.
 */
static long
fetch_portage_count(void)
{
    DIR *dp = opendir("/var/db/pkg");
    struct dirent *de;
    long n = 0;

    if (dp == NULL)
	return -1;
    while ((de = readdir(dp)) != NULL) {
	char sub[FETCH_PATH_MAX];
	long k;

	if (de->d_name[0] == '.')
	    continue;
	(void) xsnprintf(sub, sizeof(sub), "/var/db/pkg/%s", de->d_name);
	if ((k = fetch_dircount(sub, 1)) > 0)
	    n += k;
    }
    (void) closedir(dp);
    return n;
}

/*
 * fetch_packages - how many packages each package manager on this system
 * believes it has installed, read from that manager's own on-disk database.
 *
 * Only managers whose database is a plain text file or a directory of one
 * entry per package can be counted this way, which is the constraint that
 * decides the list:
 *
 *   dpkg      /var/lib/dpkg/status, deb-control(5) stanzas.  A package is
 *             installed when its Status field's third word is "installed";
 *             the first two vary ("install ok", "hold ok", "deinstall ok"),
 *             and "deinstall ok config-files" is a package that is gone but
 *             has left its configuration behind, so the third word is what
 *             has to be tested.
 *   pacman    one directory per package under /var/lib/pacman/local.
 *   apk       /lib/apk/db/installed, one "P:<name>" line per package.
 *   flatpak   one directory per application under /var/lib/flatpak/app.
 *   portage   /var/db/pkg/<category>/<package>, see above.
 *
 * rpm is deliberately absent: its database is a Berkeley DB or sqlite file
 * whose format is librpm's business, and guessing at it would be exactly the
 * kind of unverifiable reading this file refuses to do.  The same goes for
 * anything else that keeps its inventory in a binary index.
 */
static void
fetch_packages(void)
{
    static const struct {
	const char *name;
	const char *path;
	const char *prefix;	/* NULL: count directory entries instead */
	const char *value;
    } src[] = {
	{ "dpkg",    "/var/lib/dpkg/status",    "Status:", "install ok installed" },
	{ "pacman",  "/var/lib/pacman/local",   NULL,      NULL },
	{ "apk",     "/lib/apk/db/installed",   "P:",      NULL },
	{ "flatpak", "/var/lib/flatpak/app",    NULL,      NULL },
    };
    char buf[FETCH_VAL_MAX];
    size_t n = 0;
    unsigned i;
    int w;

    buf[0] = '\0';
    for (i = 0; i < sizeof(src) / sizeof(src[0]); i++) {
	long k;

	if (src[i].prefix != NULL)
	    k = fetch_count_lines(src[i].path, src[i].prefix, src[i].value);
	else
	    k = fetch_dircount(src[i].path, 1);
	if (k <= 0)
	    continue;
	w = xsnprintf(buf + n, sizeof(buf) - n, "%s%ld (%s)",
		      n ? ", " : "", k, src[i].name);
	if (w < 0 || (size_t) w >= sizeof(buf) - n)
	    break;
	n += w;
    }
    {
	long k = fetch_portage_count();

	if (k > 0) {
	    w = xsnprintf(buf + n, sizeof(buf) - n, "%s%ld (portage)",
			  n ? ", " : "", k);
	    if (w > 0 && (size_t) w < sizeof(buf) - n)
		n += w;
	}
    }
    fetch_add("Packages", buf);
}

static void
fetch_shell(void)
{
    char buf[FETCH_VAL_MAX];

    (void) xsnprintf(buf, sizeof(buf), "%s %s", MCSH_NAME, MCSH_VERSION);
    fetch_add("Shell", buf);
}

/*
 * fetch_display - the connected outputs and the mode each is running.
 *
 * Linux exports one directory per DRM connector under /sys/class/drm; the
 * "status" file reads "connected" or "disconnected", and "modes" lists the
 * modes the connector reports, most preferred first, one "WIDTHxHEIGHT" per
 * line (Linux Documentation/gpu/drm-kms.rst, "Connector Attributes").
 *
 * The refresh rate is deliberately not reported.  It is not in sysfs at all:
 * reading it means opening the DRM device and issuing DRM_IOCTL_MODE_GETCRTC,
 * which needs the render or master node and a struct definition from
 * <drm/drm_mode.h> - a kernel-ABI dependency far out of proportion to one
 * number on one row.  A resolution that is read is better than a rate that is
 * assumed.
 */
static void
fetch_display(void)
{
    DIR *dp = opendir("/sys/class/drm");
    struct dirent *de;

    if (dp == NULL)
	return;
    while ((de = readdir(dp)) != NULL) {
	char path[FETCH_PATH_MAX];
	char status[64], mode[64], buf[FETCH_VAL_MAX];

	if (de->d_name[0] == '.')
	    continue;
	(void) xsnprintf(path, sizeof(path), "/sys/class/drm/%s/status",
			 de->d_name);
	if (!fetch_line(path, status, sizeof(status)) ||
	    strcmp(status, "connected") != 0)
	    continue;
	(void) xsnprintf(path, sizeof(path), "/sys/class/drm/%s/modes",
			 de->d_name);
	if (!fetch_line(path, mode, sizeof(mode)))
	    continue;
	/*
	 * The connector directory is named "card0-DP-1"; the part after the
	 * first '-' is the connector name the user sees in every other tool.
	 */
	{
	    const char *conn = strchr(de->d_name, '-');

	    conn = (conn != NULL && conn[1] != '\0') ? conn + 1 : de->d_name;
	    (void) xsnprintf(buf, sizeof(buf), "%s (%s)", mode, conn);
	}
	fetch_add("Display", buf);
    }
    (void) closedir(dp);
}

/*
 * fetch_de - the desktop environment, from the environment variables the
 * XDG Desktop Entry Specification defines for it.
 *
 * $XDG_CURRENT_DESKTOP is a colon-separated list, most specific first
 * ("Unity:GNOME"), so only the first element is used.  $DESKTOP_SESSION is
 * the older variable the display managers still set.  The session type comes
 * from $XDG_SESSION_TYPE, which logind sets to "wayland", "x11" or "tty".
 *
 * Nothing here asks a running process anything, so there is no version
 * number: that would mean a D-Bus round trip or running the desktop's own
 * binary, and this file does neither.
 */
static void
fetch_de(void)
{
    const char *de = fetch_env("XDG_CURRENT_DESKTOP");
    const char *type = fetch_env("XDG_SESSION_TYPE");
    char name[FETCH_VAL_MAX], buf[FETCH_VAL_MAX];
    char *colon;

    if (de == NULL)
	de = fetch_env("DESKTOP_SESSION");
    if (de == NULL)
	return;
    (void) xsnprintf(name, sizeof(name), "%s", de);
    if ((colon = strchr(name, ':')) != NULL)
	*colon = '\0';
    if (name[0] == '\0')
	return;
    if (type != NULL)
	(void) xsnprintf(buf, sizeof(buf), "%s (%s)", name, type);
    else
	(void) xsnprintf(buf, sizeof(buf), "%s", name);
    fetch_add("DE", buf);
}

/*
 * fetch_wm - the window manager, from the socket or session variable the
 * compositor itself puts in the environment.
 *
 * Every entry here is a variable the named compositor is documented to set
 * for its children, so a match is a fact about the running session and not a
 * guess.  $KDE_FULL_SESSION and the GNOME desktop name are one step weaker -
 * they identify the session, and the window manager is then the one that
 * session is defined to use - which is why they come last and why nothing
 * else is inferred this way.
 */
static void
fetch_wm(void)
{
    static const struct {
	const char *var;
	const char *wm;
    } marker[] = {
	{ "HYPRLAND_INSTANCE_SIGNATURE", "Hyprland" },
	{ "SWAYSOCK",                    "sway" },
	{ "I3SOCK",                      "i3" },
	{ "KDE_FULL_SESSION",            "KWin" },
    };
    const char *de;
    unsigned i;

    for (i = 0; i < sizeof(marker) / sizeof(marker[0]); i++)
	if (fetch_env(marker[i].var) != NULL) {
	    fetch_add("WM", marker[i].wm);
	    return;
	}
    de = fetch_env("XDG_CURRENT_DESKTOP");
    if (de != NULL && strstr(de, "GNOME") != NULL)
	fetch_add("WM", "Mutter");
}

/*
 * fetch_theme - the GTK theme, icon theme, font and cursor theme.
 *
 * These live in GTK's own settings files, which are key files: a "[Settings]"
 * header followed by "key=value" lines (GTK 4 documentation, "GtkSettings" /
 * settings.ini).  GTK 4's file is read first, then GTK 3's, then the GTK 2
 * ~/.gtkrc-2.0, which uses the same key names with the value quoted.
 *
 * Qt is not read: its theme is set by a platform theme plugin whose
 * configuration is the plugin's own (qt5ct.conf, kdeglobals, or nothing at
 * all when the platform theme is gtk), and there is no file that means "the
 * Qt theme" the way settings.ini means "the GTK theme".
 *
 * There is likewise no "WM Theme" row: a window manager's decoration theme is
 * the window manager's private configuration - a KWin config group, a Mutter
 * GSettings key, a Hyprland config file - with no shared location to read.
 */
static void
fetch_theme(void)
{
    static const struct {
	const char *key;	/* the key, in all three file formats */
	const char *label;
    } want[] = {
	{ "gtk-theme-name",        "Theme" },
	{ "gtk-icon-theme-name",   "Icons" },
	{ "gtk-font-name",         "Font" },
	{ "gtk-cursor-theme-name", "Cursor" },
    };
    char dir[FETCH_PATH_MAX];
    char cand[3][FETCH_PATH_MAX];
    const char *home;
    const char *x;
    int ncand = 0, i;
    unsigned k;

    x = fetch_env("XDG_CONFIG_HOME");
    home = fetch_home();
    if (x != NULL && x[0] == '/')
	(void) xsnprintf(dir, sizeof(dir), "%s", x);
    else if (home != NULL)
	(void) xsnprintf(dir, sizeof(dir), "%s/.config", home);
    else
	dir[0] = '\0';

    if (dir[0] != '\0') {
	(void) xsnprintf(cand[ncand++], FETCH_PATH_MAX,
			 "%s/gtk-4.0/settings.ini", dir);
	(void) xsnprintf(cand[ncand++], FETCH_PATH_MAX,
			 "%s/gtk-3.0/settings.ini", dir);
    }
    if (home != NULL)
	(void) xsnprintf(cand[ncand++], FETCH_PATH_MAX, "%s/.gtkrc-2.0", home);

    for (k = 0; k < sizeof(want) / sizeof(want[0]); k++) {
	char buf[FETCH_VAL_MAX];

	for (i = 0; i < ncand; i++)
	    if (fetch_key(cand[i], want[k].key, buf, sizeof(buf))) {
		fetch_add(want[k].label, buf);
		break;
	    }
    }
}

/*
 * fetch_terminal - which terminal emulator this shell is talking to.
 *
 * $TERM_PROGRAM is authoritative where it exists, because the emulator set it
 * itself, and $TERM_PROGRAM_VERSION goes with it.  Where it does not, the
 * emulator is this shell's ancestor, so the process tree is walked upwards
 * reading /proc/<pid>/stat (proc(5)): field 2 is the executable name in
 * parentheses and field 4 is the parent pid.  The walk stops at the first
 * ancestor whose name is a terminal emulator this table knows.
 *
 * Matching against a list, rather than taking whatever the first non-shell
 * ancestor happens to be, is the difference between a fact and a guess: under
 * `make', `script' or a CI runner the parent chain contains no emulator at
 * all, and the honest answer there is the terminal *type* from $TERM, which
 * is what the fallback reports.
 *
 * The font the emulator draws with is not reported: it is in each emulator's
 * own configuration file, in its own format, at a path only that emulator
 * knows - and nothing publishes it to the programs running inside.
 */
static void
fetch_terminal(void)
{
    static const char * const emulator[] = {
	"alacritty", "foot", "kitty", "wezterm-gui", "wezterm", "ghostty",
	"xterm", "urxvt", "rxvt", "st", "eterm", "Eterm", "mlterm",
	"gnome-terminal-", "gnome-terminal", "konsole", "xfce4-terminal",
	"lxterminal", "mate-terminal", "terminator", "tilix", "deepin-terminal",
	"qterminal", "sakura", "termite", "contour", "rio", "zutty",
	"screen", "tmux", "tmux: server", NULL
    };
    const char *tp = fetch_env("TERM_PROGRAM");
    char buf[FETCH_VAL_MAX];
    pid_t pid = getppid();
    int hops;

    if (tp != NULL) {
	const char *ver = fetch_env("TERM_PROGRAM_VERSION");

	if (ver != NULL)
	    (void) xsnprintf(buf, sizeof(buf), "%s %s", tp, ver);
	else
	    (void) xsnprintf(buf, sizeof(buf), "%s", tp);
	fetch_add("Terminal", buf);
	return;
    }

    /* Bounded: a cycle in the parent chain cannot happen, but a bound costs
     * nothing and the walk must not depend on that being true. */
    for (hops = 0; hops < 32 && pid > 1; hops++) {
	char path[FETCH_PATH_MAX];
	char line[512];
	char comm[128];
	char *open_paren, *close_paren, *p;
	unsigned i;
	long ppid = 0;

	(void) xsnprintf(path, sizeof(path), "/proc/%lu/stat",
			 (unsigned long) pid);
	if (!fetch_line(path, line, sizeof(line)))
	    break;
	/*
	 * The name is in parentheses and may itself contain spaces and even
	 * ')', so it is delimited by the *last* ')' in the line, as proc(5)
	 * requires every reader of this file to do.
	 */
	open_paren = strchr(line, '(');
	close_paren = strrchr(line, ')');
	if (open_paren == NULL || close_paren == NULL || close_paren <= open_paren)
	    break;
	*close_paren = '\0';
	(void) xsnprintf(comm, sizeof(comm), "%s", open_paren + 1);

	/* Field 4, the parent pid, is the second field after the ')'. */
	p = close_paren + 1;
	while (*p == ' ')
	    p++;
	while (*p != '\0' && *p != ' ')	/* field 3: the state character */
	    p++;
	ppid = strtol(p, NULL, 10);

	for (i = 0; emulator[i] != NULL; i++)
	    if (strcmp(comm, emulator[i]) == 0) {
		fetch_add("Terminal", comm);
		return;
	    }
	if (ppid <= 1)
	    break;
	pid = (pid_t) ppid;
    }

    /* No emulator in the chain: report the terminal type instead. */
    {
	const char *term = fetch_env("TERM");

	fetch_add("Terminal", (term != NULL) ? term : "");
    }
}

/*
 * fetch_cpu - the part, how many cores are online, and the clock it can reach.
 *
 * The maximum frequency comes from cpufreq's own sysfs attribute,
 * /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq, in kHz (Linux
 * Documentation/admin-guide/pm/cpufreq.rst).  Where cpufreq is not built or
 * not driving the CPU - inside most virtual machines, for one - there is no
 * such file, and the "cpu MHz" line of /proc/cpuinfo is used instead.  That
 * line is the *current* frequency of that one core, not its maximum, so it is
 * only a floor; it is used because it is the only figure such a system
 * publishes, and it is what every other tool reports there too.
 */
static void
fetch_cpu(void)
{
    char buf[FETCH_VAL_MAX];
    char model[FETCH_VAL_MAX];
    char line[128];
    char ghz[32];
    long ncpu = -1;
    unsigned long khz = 0;

#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    /*
     * proc(5) /proc/cpuinfo is architecture-dependent: x86 names the part in
     * "model name", 64-bit ARM has no model line at all and identifies the
     * board in "Hardware", and several others use "cpu model" or "cpu".  Each
     * is tried in turn; if none is present the row still reports the core
     * count against the uname(2) machine type, which is always available.
     */
    model[0] = '\0';
    if (!fetch_key("/proc/cpuinfo", "model name", model, sizeof(model)) &&
	!fetch_key("/proc/cpuinfo", "cpu model", model, sizeof(model)) &&
	!fetch_key("/proc/cpuinfo", "Hardware", model, sizeof(model)) &&
	!fetch_key("/proc/cpuinfo", "Model", model, sizeof(model)) &&
	!fetch_key("/proc/cpuinfo", "cpu", model, sizeof(model))) {
	if (fetch_have_uts)
	    (void) xsnprintf(model, sizeof(model), "%s", fetch_uts.machine);
    }
    if (model[0] == '\0')
	return;

    if (fetch_line("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
		   line, sizeof(line)))
	khz = strtoul(line, NULL, 10);
    else if (fetch_key("/proc/cpuinfo", "cpu MHz", line, sizeof(line)))
	/* "2100.000": the integer part is the MHz, and strtoul() stops at
	 * the '.' of its own accord. */
	khz = strtoul(line, NULL, 10) * 1000UL;

    ghz[0] = '\0';
    if (khz >= 10000) {		/* below 10 MHz it is not a clock, it is noise */
	char *at;

	(void) xsnprintf(ghz, sizeof(ghz), " @ %lu.%02lu GHz",
			 khz / 1000000UL, (khz % 1000000UL) / 10000UL);
	/*
	 * Intel writes the nominal clock into the model string itself -
	 * "Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz" - so appending the
	 * measured one to it says the clock twice.  The suffix is dropped
	 * only when there is a read frequency to put in its place; with no
	 * cpufreq attribute and no "cpu MHz" line it is the only clock the
	 * system states, and it stays.
	 */
	if ((at = strstr(model, " @ ")) != NULL)
	    *at = '\0';
    }

    if (ncpu > 0)
	(void) xsnprintf(buf, sizeof(buf), "%s (%ld)%s", model, ncpu, ghz);
    else
	(void) xsnprintf(buf, sizeof(buf), "%s%s", model, ghz);
    fetch_add("CPU", buf);
}

/*
 * fetch_pciids - look a vendor and device ID up in the hwdata pci.ids file.
 *
 * The format is fixed-shape and documented by its own header and by
 * pci.ids(5): a vendor line begins in column 1 with the four-digit hex vendor
 * ID, two spaces, then the vendor name; each of its device lines begins with
 * one tab, then the four-digit hex device ID, two spaces, then the device
 * name; subsystem lines begin with two tabs and are skipped.  Comments begin
 * with '#'.
 *
 * Both names are optional in the output: a system without hwdata installed
 * still gets a GPU row, built from whatever fetch_gpu() can say without this.
 */
static void
fetch_pciids(unsigned vendor, unsigned device, char *vname, size_t vsz,
	     char *dname, size_t dsz)
{
    static const char * const path[] = {
	"/usr/share/hwdata/pci.ids",
	"/usr/share/misc/pci.ids",
	"/usr/share/pci.ids",
	"/var/lib/pciutils/pci.ids",
	NULL
    };
    char want_v[8], want_d[8];
    int i;

    vname[0] = dname[0] = '\0';
    (void) xsnprintf(want_v, sizeof(want_v), "%04x", vendor);
    (void) xsnprintf(want_d, sizeof(want_d), "%04x", device);

    for (i = 0; path[i] != NULL; i++) {
	FILE *fp = fopen(path[i], "r");
	char line[512];
	int in_vendor = 0;

	if (fp == NULL)
	    continue;
	while (fgets(line, sizeof(line), fp) != NULL) {
	    if (line[0] == '#' || line[0] == '\n')
		continue;
	    if (line[0] != '\t') {
		if (in_vendor)
		    break;		/* past our vendor's block */
		if (strncmp(line, want_v, 4) == 0 && line[4] == ' ') {
		    char *p = line + 4;

		    while (*p == ' ')
			p++;
		    (void) xsnprintf(vname, vsz, "%s", p);
		    fetch_trim(vname);
		    in_vendor = 1;
		}
		continue;
	    }
	    if (!in_vendor || line[1] == '\t')
		continue;		/* another vendor's, or a subsystem */
	    if (strncmp(line + 1, want_d, 4) == 0 && line[5] == ' ') {
		char *p = line + 5;

		while (*p == ' ')
		    p++;
		(void) xsnprintf(dname, dsz, "%s", p);
		fetch_trim(dname);
		break;
	    }
	}
	(void) fclose(fp);
	if (vname[0] != '\0')
	    return;
    }
}

/*
 * fetch_gpu - every PCI display controller on the machine.
 *
 * Linux exports one directory per PCI function under /sys/bus/pci/devices,
 * each with "class", "vendor" and "device" files holding the configuration
 * space values as "0x" hex (Linux Documentation/ABI/testing/sysfs-bus-pci).
 * The class is a 24-bit value whose top byte is the base class, and base
 * class 0x03 is "Display controller" (PCI Code and ID Assignment
 * Specification, appendix D).
 *
 * The name is resolved through pci.ids when hwdata is installed, and falls
 * back to the vendor's name from the small table below plus the raw device ID
 * when it is not.  A device whose vendor is not even in that table is still
 * reported, by its numbers: those were read, so they are facts.
 *
 * Nothing is labelled "[Discrete]" or "[Integrated]".  There is no sysfs
 * attribute for it; every tool that prints it is pattern-matching the device
 * name, and a wrong label on a row is worse than no label.
 */
static void
fetch_gpu(void)
{
    static const struct {
	unsigned id;
	const char *name;
    } known[] = {
	{ 0x8086, "Intel" },	{ 0x10de, "NVIDIA" },	{ 0x1002, "AMD" },
	{ 0x1022, "AMD" },	{ 0x1af4, "Red Hat" },	{ 0x1b36, "Red Hat" },
	{ 0x15ad, "VMware" },	{ 0x1234, "Bochs" },	{ 0x1013, "Cirrus" },
	{ 0x80ee, "VirtualBox" },
    };
    DIR *dp = opendir("/sys/bus/pci/devices");
    struct dirent *de;

    if (dp == NULL)
	return;
    while ((de = readdir(dp)) != NULL) {
	char path[FETCH_PATH_MAX];
	char line[64];
	char vname[FETCH_VAL_MAX], dname[FETCH_VAL_MAX], buf[FETCH_VAL_MAX];
	unsigned long class_code, vendor, device;
	unsigned i;

	if (de->d_name[0] == '.')
	    continue;
	(void) xsnprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class",
			 de->d_name);
	if (!fetch_line(path, line, sizeof(line)))
	    continue;
	class_code = strtoul(line, NULL, 16);
	if ((class_code >> 16) != 0x03)
	    continue;

	(void) xsnprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/vendor",
			 de->d_name);
	if (!fetch_line(path, line, sizeof(line)))
	    continue;
	vendor = strtoul(line, NULL, 16);
	(void) xsnprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/device",
			 de->d_name);
	if (!fetch_line(path, line, sizeof(line)))
	    continue;
	device = strtoul(line, NULL, 16);

	fetch_pciids((unsigned) vendor, (unsigned) device,
		     vname, sizeof(vname), dname, sizeof(dname));
	if (vname[0] == '\0')
	    for (i = 0; i < sizeof(known) / sizeof(known[0]); i++)
		if (known[i].id == (unsigned) vendor) {
		    (void) xsnprintf(vname, sizeof(vname), "%s",
				     known[i].name);
		    break;
		}
	if (dname[0] != '\0' && vname[0] != '\0')
	    (void) xsnprintf(buf, sizeof(buf), "%s %s", vname, dname);
	else if (vname[0] != '\0')
	    (void) xsnprintf(buf, sizeof(buf), "%s device %04lx", vname,
			     device);
	else
	    (void) xsnprintf(buf, sizeof(buf), "PCI %04lx:%04lx", vendor,
			     device);
	fetch_add("GPU", buf);
    }
    (void) closedir(dp);
}

static void
fetch_memory(void)
{
    char buf[FETCH_VAL_MAX];
    char line[128];
    unsigned long long total = 0, avail = 0;

    /*
     * /proc/meminfo (proc(5)) is preferred over sysconf(_SC_AVPHYS_PAGES):
     * MemAvailable is the kernel's own estimate of how much memory can be
     * given to new work without swapping, which is the number a person wants,
     * whereas free pages alone read as almost nothing on a healthy system
     * whose spare memory is all page cache.  MemFree is the fallback for
     * kernels too old to publish MemAvailable.
     */
    if (fetch_key("/proc/meminfo", "MemTotal", line, sizeof(line))) {
	total = strtoull(line, NULL, 10) * 1024ULL;	/* always kB */
	if (fetch_key("/proc/meminfo", "MemAvailable", line, sizeof(line)))
	    avail = strtoull(line, NULL, 10) * 1024ULL;
	else if (fetch_key("/proc/meminfo", "MemFree", line, sizeof(line)))
	    avail = strtoull(line, NULL, 10) * 1024ULL;
    }
#if defined(HAVE_SYSCONF) && defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    if (total == 0) {
	long pages = sysconf(_SC_PHYS_PAGES);
	long pgsz = sysconf(_SC_PAGESIZE);

	if (pages > 0 && pgsz > 0) {
	    total = (unsigned long long) pages * (unsigned long long) pgsz;
# ifdef _SC_AVPHYS_PAGES
	    {
		long freep = sysconf(_SC_AVPHYS_PAGES);

		if (freep > 0)
		    avail = (unsigned long long) freep *
			    (unsigned long long) pgsz;
	    }
# endif
	}
    }
#endif
    if (total == 0)
	return;

    if (avail > total)
	avail = total;
    fetch_usage(total - avail, total, buf, sizeof(buf));
    fetch_add("Memory", buf);
}

/*
 * fetch_swap - swap in use, from /proc/meminfo's SwapTotal and SwapFree
 * (proc(5)).  A machine with no swap configured has SwapTotal 0, and gets no
 * row at all rather than a row reading "0 B / 0 B": there is nothing there to
 * report on.
 */
static void
fetch_swap(void)
{
    char buf[FETCH_VAL_MAX];
    char line[128];
    unsigned long long total = 0, freed = 0;

    if (!fetch_key("/proc/meminfo", "SwapTotal", line, sizeof(line)))
	return;
    total = strtoull(line, NULL, 10) * 1024ULL;
    if (total == 0)
	return;
    if (fetch_key("/proc/meminfo", "SwapFree", line, sizeof(line)))
	freed = strtoull(line, NULL, 10) * 1024ULL;
    if (freed > total)
	freed = total;
    fetch_usage(total - freed, total, buf, sizeof(buf));
    fetch_add("Swap", buf);
}

/*
 * fetch_fstype - the filesystem type mounted on mp, from /proc/self/mounts.
 *
 * The file is in fstab(5) field order: device, mount point, type, options,
 * dump, pass.  The mount point is escaped octally for the four characters
 * that would otherwise break the field split (space, tab, newline and
 * backslash, as "\040", "\011", "\012" and "\134"), which is why the
 * comparison is against the escaped form of what the caller asked for - and
 * why the only caller asks about "/", which has no escapable character in it.
 */
static int
fetch_fstype(const char *mp, char *buf, size_t bufsz)
{
    FILE *fp = fopen("/proc/self/mounts", "r");
    char line[1024];
    int ok = 0;

    if (fp == NULL)
	return 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
	char *dev, *point, *type, *p = line;

	dev = p;
	while (*p != '\0' && *p != ' ')
	    p++;
	if (*p == '\0')
	    continue;
	*p++ = '\0';
	point = p;
	while (*p != '\0' && *p != ' ')
	    p++;
	if (*p == '\0')
	    continue;
	*p++ = '\0';
	type = p;
	while (*p != '\0' && *p != ' ' && *p != '\n')
	    p++;
	*p = '\0';
	USE(dev);
	if (strcmp(point, mp) != 0)
	    continue;
	(void) xsnprintf(buf, bufsz, "%s", type);
	ok = (*buf != '\0');
	/* Not a break: a later mount on the same point shadows an earlier
	 * one, so the *last* match is the filesystem actually there. */
    }
    (void) fclose(fp);
    return ok;
}

static void
fetch_disk(void)
{
#ifdef FETCH_HAVE_STATVFS
    struct statvfs vfs;
    char buf[FETCH_VAL_MAX];
    char usage[FETCH_VAL_MAX];
    char fstype[64];
    unsigned long long total, avail, frsize;

    if (statvfs("/", &vfs) != 0)
	return;
    /*
     * struct statvfs (POSIX.1-2001, <sys/statvfs.h>): f_blocks and f_bavail
     * are counted in units of f_frsize, the filesystem's fundamental block
     * size, so the byte figures are the products.  f_bavail rather than
     * f_bfree, so what is reported is what an unprivileged process can
     * actually write.
     */
    frsize = (vfs.f_frsize != 0) ? (unsigned long long) vfs.f_frsize
				 : (unsigned long long) vfs.f_bsize;
    if (frsize == 0 || vfs.f_blocks == 0)
	return;
    total = (unsigned long long) vfs.f_blocks * frsize;
    avail = (unsigned long long) vfs.f_bavail * frsize;
    if (avail > total)
	avail = total;
    fetch_usage(total - avail, total, usage, sizeof(usage));
    if (fetch_fstype("/", fstype, sizeof(fstype)))
	(void) xsnprintf(buf, sizeof(buf), "%s - %s", usage, fstype);
    else
	(void) xsnprintf(buf, sizeof(buf), "%s", usage);
    fetch_add("Disk (/)", buf);
#endif /* FETCH_HAVE_STATVFS */
}

/*
 * fetch_localip - the first non-loopback IPv4 address configured on this
 * machine, with its prefix length and its interface name.
 *
 * getifaddrs(3) returns the whole list; ifa_addr is the address and
 * ifa_netmask the mask, both as a struct sockaddr, and IFF_LOOPBACK marks the
 * loopback interface.  inet_ntop(3) (POSIX.1-2001) renders the address.  The
 * prefix length is the population count of the mask, which is only meaningful
 * for a contiguous mask - every mask in use for the last thirty years - and
 * is therefore counted rather than derived.
 *
 * Only the first is reported.  A machine with several addresses has no way to
 * say which one is "the" local IP, and a panel is not a routing table.
 */
static void
fetch_localip(void)
{
#ifdef FETCH_HAVE_IFADDRS
    struct ifaddrs *list, *ifa;

    if (getifaddrs(&list) != 0)
	return;
    for (ifa = list; ifa != NULL; ifa = ifa->ifa_next) {
	char addr[INET_ADDRSTRLEN];
	char buf[FETCH_VAL_MAX];
	struct sockaddr_in *sin, *mask;
	unsigned long m;
	int bits = 0;

	if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
	    continue;
	if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
	    continue;
	if ((ifa->ifa_flags & IFF_UP) == 0)
	    continue;
	sin = (struct sockaddr_in *) (void *) ifa->ifa_addr;
	if (inet_ntop(AF_INET, &sin->sin_addr, addr, sizeof(addr)) == NULL)
	    continue;

	mask = (struct sockaddr_in *) (void *) ifa->ifa_netmask;
	if (mask != NULL) {
	    for (m = (unsigned long) ntohl(mask->sin_addr.s_addr); m != 0;
		 m <<= 1) {
		if ((m & 0x80000000UL) == 0)
		    break;
		bits++;
	    }
	}
	if (bits > 0)
	    (void) xsnprintf(buf, sizeof(buf), "%s/%d (%s)", addr, bits,
			     ifa->ifa_name != NULL ? ifa->ifa_name : "?");
	else
	    (void) xsnprintf(buf, sizeof(buf), "%s (%s)", addr,
			     ifa->ifa_name != NULL ? ifa->ifa_name : "?");
	fetch_add("Local IP", buf);
	break;
    }
    freeifaddrs(list);
#endif /* FETCH_HAVE_IFADDRS */
}

/*
 * fetch_battery - charge and charging state.
 *
 * Linux's power supply class gives one directory per supply under
 * /sys/class/power_supply, with "type" saying what it is ("Battery",
 * "Mains", ...), "capacity" the charge as a whole percent and "status" one of
 * Charging, Discharging, Full, Not charging or Unknown (Linux
 * Documentation/ABI/testing/sysfs-class-power).  A "Mains" supply whose
 * "online" reads 1 is external power, which is worth saying on the same row.
 */
static void
fetch_battery(void)
{
    DIR *dp = opendir("/sys/class/power_supply");
    struct dirent *de;
    char capacity[32], status[64];
    int have_bat = 0, on_ac = 0;

    if (dp == NULL)
	return;
    capacity[0] = status[0] = '\0';
    while ((de = readdir(dp)) != NULL) {
	char path[FETCH_PATH_MAX];
	char type[64], line[64];

	if (de->d_name[0] == '.')
	    continue;
	(void) xsnprintf(path, sizeof(path), "/sys/class/power_supply/%s/type",
			 de->d_name);
	if (!fetch_line(path, type, sizeof(type)))
	    continue;
	if (strcmp(type, "Mains") == 0) {
	    (void) xsnprintf(path, sizeof(path),
			     "/sys/class/power_supply/%s/online", de->d_name);
	    if (fetch_line(path, line, sizeof(line)) && line[0] == '1')
		on_ac = 1;
	    continue;
	}
	if (strcmp(type, "Battery") != 0 || have_bat)
	    continue;
	(void) xsnprintf(path, sizeof(path),
			 "/sys/class/power_supply/%s/capacity", de->d_name);
	if (!fetch_line(path, capacity, sizeof(capacity)))
	    continue;
	(void) xsnprintf(path, sizeof(path),
			 "/sys/class/power_supply/%s/status", de->d_name);
	if (!fetch_line(path, status, sizeof(status)))
	    status[0] = '\0';
	have_bat = 1;
    }
    (void) closedir(dp);
    if (!have_bat)
	return;
    {
	char buf[FETCH_VAL_MAX];

	if (status[0] != '\0')
	    (void) xsnprintf(buf, sizeof(buf), "%s%% (%s)%s", capacity, status,
			     on_ac ? " [AC connected]" : "");
	else
	    (void) xsnprintf(buf, sizeof(buf), "%s%%%s", capacity,
			     on_ac ? " [AC connected]" : "");
	fetch_add("Battery", buf);
    }
}

/*
 * fetch_locale - the locale this shell is actually running in.
 *
 * The precedence is the one POSIX.1 sets out for the locale environment
 * variables (XBD 8.2 Internationalization Variables): LC_ALL overrides
 * everything, then the individual LC_* categories, then LANG.  LC_CTYPE is
 * the category that decides how this shell's own line editor and width
 * calculations behave, so it is the one reported.
 *
 * With none of them set, the login-time default is read from the file the
 * system keeps it in - /etc/locale.conf on a systemd system (locale.conf(5)),
 * /etc/default/locale on a Debian one - because that is what a fresh login
 * shell would have inherited.
 */
static void
fetch_locale(void)
{
    const char *v;
    char buf[FETCH_VAL_MAX];

    if ((v = fetch_env("LC_ALL")) != NULL || (v = fetch_env("LC_CTYPE")) != NULL ||
	(v = fetch_env("LANG")) != NULL) {
	fetch_add("Locale", v);
	return;
    }
    if (fetch_key("/etc/locale.conf", "LANG", buf, sizeof(buf)) ||
	fetch_key("/etc/default/locale", "LANG", buf, sizeof(buf)))
	fetch_add("Locale", buf);
}

static void
fetch_load(void)
{
    char line[128];
    char buf[FETCH_VAL_MAX];
    int n = 0, field = 0;
    char *p;

    /* proc(5) /proc/loadavg: the first three fields are the 1, 5 and 15
     * minute load averages.  getloadavg(3) is not in POSIX and is not probed
     * by configure, so this row too is Linux-only. */
    if (!fetch_line("/proc/loadavg", line, sizeof(line)))
	return;
    /* Copied by hand rather than with a "%.*s": the shell's minimal
     * xsnprintf() (tc.printf.c) consumes a '.' straight after '%' as a
     * zero-pad flag and has no precision conversion at all. */
    buf[0] = '\0';
    for (p = line; *p != '\0' && field < 3; ) {
	char *start = p;

	while (*p != '\0' && *p != ' ' && *p != '\t')
	    p++;
	if (p != start) {
	    if (field > 0 && (size_t) n + 1 < sizeof(buf))
		buf[n++] = ' ';
	    while (start < p && (size_t) n + 1 < sizeof(buf))
		buf[n++] = *start++;
	    buf[n] = '\0';
	    field++;
	}
	while (*p == ' ' || *p == '\t')
	    p++;
    }
    if (field == 3)
	fetch_add("Load", buf);
}

/*
 * ---------------------------------------------------------------------------
 * The field table.
 *
 * Every row the panel can produce is named here, once, and that name is what
 * `show' and `hide' match and what `sysinfo -l' prints.  Adding a field is
 * adding a collector and one line to this table; nothing else in the file
 * needs to know about it.
 *
 * The order is the reading order of the panel, not the cost of the
 * collectors: every one of them is a handful of syscalls.
 * ---------------------------------------------------------------------------
 */
static const struct fetch_field {
    const char *name;
    void (*collect)(void);
} fetch_fields[] = {
    { "title",    fetch_identity },
    { "os",       fetch_os },
    { "host",     fetch_host },
    { "kernel",   fetch_kernel },
    { "uptime",   fetch_uptime },
    { "packages", fetch_packages },
    { "shell",    fetch_shell },
    { "display",  fetch_display },
    { "de",       fetch_de },
    { "wm",       fetch_wm },
    { "theme",    fetch_theme },
    { "terminal", fetch_terminal },
    { "cpu",      fetch_cpu },
    { "gpu",      fetch_gpu },
    { "memory",   fetch_memory },
    { "swap",     fetch_swap },
    { "disk",     fetch_disk },
    { "localip",  fetch_localip },
    { "battery",  fetch_battery },
    { "locale",   fetch_locale },
    { "load",     fetch_load },
};
#define FETCH_NFIELDS	((int) (sizeof(fetch_fields) / sizeof(fetch_fields[0])))

/*
 * fetch_listed - is name one of the comma-separated entries of list?
 * Whitespace around an entry is ignored, so "os, kernel" works.
 */
static int
fetch_listed(const char *list, const char *name)
{
    const char *p = list;
    size_t want = strlen(name);

    while (*p != '\0') {
	const char *start, *end;

	while (*p == ',' || *p == ' ' || *p == '\t')
	    p++;
	start = p;
	while (*p != '\0' && *p != ',')
	    p++;
	end = p;
	while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
	    end--;
	if ((size_t) (end - start) == want &&
	    strncmp(start, name, want) == 0)
	    return 1;
    }
    return 0;
}

/*
 * fetch_field_by_name - the table entry called name, or NULL.
 */
static const struct fetch_field *
fetch_field_by_name(const char *name, size_t len)
{
    int i;

    for (i = 0; i < FETCH_NFIELDS; i++)
	if (strlen(fetch_fields[i].name) == len &&
	    strncmp(fetch_fields[i].name, name, len) == 0)
	    return &fetch_fields[i];
    return NULL;
}

/*
 * fetch_collect - fill the row table.
 *
 * Without `show', every field in table order.  With it, exactly the fields it
 * names, in exactly the order it names them - so `show' is both a filter and
 * a layout.  `hide' is applied last in both cases, which makes
 * "show = os, kernel" and "hide = everything else" two ways of saying the
 * same thing and lets one override the other without a precedence rule to
 * remember.
 */
/*
 * fetch_check_list - report any name in a comma list that is not a field.
 *
 * Both `show' and `hide' go through this, because a name that is silently
 * ignored is the worst possible outcome: "hide = palette" looks like it should
 * work - `palette' is a real key, just not a field - and without this the
 * panel would simply keep drawing the palette with no explanation.
 */
static void
fetch_check_list(const char *which, const char *list)
{
    const char *p = list;

    while (*p != '\0') {
	const char *start, *end;

	while (*p == ',' || *p == ' ' || *p == '\t')
	    p++;
	start = p;
	while (*p != '\0' && *p != ',')
	    p++;
	end = p;
	while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
	    end--;
	if (end == start)
	    continue;
	if (fetch_field_by_name(start, (size_t) (end - start)) == NULL) {
	    char msg[256], name[64];
	    size_t n = (size_t) (end - start);

	    /* Copied by hand: doprnt() (tc.printf.c) has no precision
	     * conversion, and reads a '.' straight after '%' as a zero-pad
	     * flag, so "%.*s" would not do what it looks like. */
	    if (n >= sizeof(name))
		n = sizeof(name) - 1;
	    memcpy(name, start, n);
	    name[n] = '\0';
	    (void) xsnprintf(msg, sizeof(msg),
			     "`%s' names `%s', which is not a field; "
			     "`sysinfo -l' lists them", which, name);
	    fetch_warn_file("sysinfo.conf", msg);
	}
    }
}

static void
fetch_collect(void)
{
    fetch_nrows = 0;
    fetch_have_uts = (uname(&fetch_uts) >= 0);

    fetch_check_list("show", fetch_conf.show);
    fetch_check_list("hide", fetch_conf.hide);

    if (fetch_conf.show[0] == '\0') {
	int i;

	for (i = 0; i < FETCH_NFIELDS; i++)
	    if (!fetch_listed(fetch_conf.hide, fetch_fields[i].name))
		(*fetch_fields[i].collect)();
	return;
    }

    {
	const char *p = fetch_conf.show;

	while (*p != '\0') {
	    const char *start, *end;
	    const struct fetch_field *f;

	    while (*p == ',' || *p == ' ' || *p == '\t')
		p++;
	    start = p;
	    while (*p != '\0' && *p != ',')
		p++;
	    end = p;
	    while (end > start && (end[-1] == ' ' || end[-1] == '\t'))
		end--;
	    if (end == start)
		continue;
	    /* Already reported by fetch_check_list() above. */
	    f = fetch_field_by_name(start, (size_t) (end - start));
	    if (f == NULL)
		continue;
	    if (!fetch_listed(fetch_conf.hide, f->name))
		(*f->collect)();
	}
    }
}

/*
 * ---------------------------------------------------------------------------
 * Output.
 *
 * Colour is emitted only when the terminal advertises it, exactly as the
 * syntax highlighter and the ghost-text renderer decide it (T_CanColor,
 * ed.screen.c), and only when writing to a terminal at all - a redirected
 * `sysinfo > file' must produce plain text.
 *
 * The sequences are ECMA-48 (5th edition) 8.3.117 SGR: 0 resets every
 * attribute, and 40..47 / 100..107 set the background colours the palette
 * rows show.  What the logo, labels and title use is whatever the
 * configuration says, which is why those are parameters and not literals.
 * ---------------------------------------------------------------------------
 */

/*
 * fetch_out_isatty - is what this panel is about to be written to a terminal?
 *
 * Not plain isoutatty: that describes SHOUT, the shell's own standard output,
 * and an ordinary redirection on the command does not touch it.  doio()
 * (sh.sem.c) redirects descriptor 1, sets is1atty from it and sets didfds, and
 * flush() then writes to descriptor 1 rather than SHOUT - so `sysinfo > file'
 * in an interactive shell had isoutatty still true while the output was going
 * to a file.  This is the same test xputchar() (sh.print.c) makes, for exactly
 * the same reason; stderr is not considered because the panel never goes
 * there.
 */
static int
fetch_out_isatty(void)
{
    return didfds ? is1atty : isoutatty;
}

/* Start a run in the given SGR parameters; "" and no colour are both no-ops. */
static void
fetch_sgr(const char *params)
{
    if (!fetch_color || params == NULL || *params == '\0')
	return;
    xprintf("\033[%sm", params);
}

/* End a run started by fetch_sgr(params).  SGR 0 rather than the individual
 * "off" parameters: the configuration may have asked for any combination of
 * attributes, and 0 is the one reset that undoes all of them. */
static void
fetch_sgr_off(const char *params)
{
    if (!fetch_color || params == NULL || *params == '\0')
	return;
    xprintf("\033[0m");
}

static void
fetch_palette(void)
{
    int i;

    if (!fetch_color || !fetch_conf.palette)
	return;
    for (i = 40; i <= 47; i++)
	xprintf("\033[%dm  ", i);
    xprintf("\033[0m\n");
    for (i = 100; i <= 107; i++)
	xprintf("\033[%dm  ", i);
    xprintf("\033[0m\n");
}

/*
 * fetch_print - emit the panel: the logo down the left, the rows down the
 * right, and the palette underneath.
 *
 * The logo is dropped when the terminal is too narrow to hold it beside the
 * widest plausible value, or when the caller asked for no logo; the rows are
 * never dropped.
 */
static void
fetch_print(int with_logo)
{
    int i, rows;
    int old_output_raw;

    /*
     * xputchar() (sh.print.c) rewrites a control character as "^X" unless
     * output_raw is set, so an SGR sequence printed with xprintf() arrived as
     * the literal text "^[[1;36m" rather than as colour.  The editor's display
     * code avoids this by emitting escapes through putpure(); a builtin that
     * prints whole formatted lines sets this flag instead, the same way
     * `setenv' with no arguments and `history -h' do (sh.func.c, sh.hist.c).
     */
    old_output_raw = output_raw;
    output_raw = 1;
    cleanup_push(&old_output_raw, output_raw_restore);

    if (fetch_logo_rows == 0 || fetch_logo_w == 0)
	with_logo = 0;
    if (with_logo && T_Cols > 0 && T_Cols < fetch_logo_w + 24)
	with_logo = 0;

    rows = (with_logo && fetch_logo_rows > fetch_nrows) ? fetch_logo_rows
							: fetch_nrows;

    for (i = 0; i < rows; i++) {
	if (with_logo) {
	    if (i < fetch_logo_rows) {
		/*
		 * A logo row that carries escape sequences of its own is
		 * printed as it stands: the file has already said what
		 * colour it wants, and wrapping it would either be undone by
		 * its first sequence or would leak past its last one.
		 */
		int own = (strchr(fetch_logo[i], '\033') != NULL);
		int pad = fetch_logo_w - fetch_dwidth(fetch_logo[i]);

		if (!own)
		    fetch_sgr(fetch_conf.logo_color);
		xprintf("%s", fetch_logo[i]);
		if (!own)
		    fetch_sgr_off(fetch_conf.logo_color);
		else if (fetch_color)
		    xprintf("\033[0m");	/* do not let it leak into the rows */
		if (pad > 0)
		    xprintf("%*s", pad, "");
	    }
	    else
		/* Past the bottom of the logo: pad with plain spaces rather
		 * than an empty coloured run, so the rows below cost no
		 * escape sequences at all. */
		xprintf("%*s", fetch_logo_w, "");
	    xprintf("  ");
	}
	if (i < fetch_nrows) {
	    const struct fetch_row *r = &fetch_rows[i];

	    if (r->label == NULL) {
		/* The header: user@host, then a rule the same width. */
		fetch_sgr(fetch_conf.title_color);
		xprintf("%s", r->value);
		fetch_sgr_off(fetch_conf.title_color);
	    }
	    else {
		/*
		 * The label is coloured and the padding after it is not.  With
		 * a foreground colour the difference is invisible, but a
		 * `label_color' that sets a background would otherwise paint a
		 * block of empty columns after every short label.  The labels
		 * are this file's own ASCII literals, so their width is their
		 * length.
		 */
		int pad = fetch_conf.label_width - (int) strlen(r->label);

		fetch_sgr(fetch_conf.label_color);
		xprintf("%s", r->label);
		fetch_sgr_off(fetch_conf.label_color);
		if (pad > 0)
		    xprintf("%*s", pad, "");
		xprintf("%s%s", fetch_conf.separator, r->value);
	    }
	}
	xprintf("\n");
    }
    fetch_palette();
    flush();
    cleanup_until(&old_output_raw);
}

/*
 * fetch_setup - defaults, then the configuration file, then the logo, then
 * the colour decision.  Shared by the builtin and the start-up greeting so
 * that the two cannot drift apart.
 *
 * force_color: 1 to colour regardless, 0 to never colour, -1 to decide from
 * the configuration and the terminal.
 */
static void
fetch_setup(int force_color)
{
    char dir[FETCH_PATH_MAX], path[FETCH_PATH_MAX];

    fetch_conf_defaults();
    if (fetch_confdir(dir, sizeof(dir))) {
	(void) xsnprintf(path, sizeof(path), "%s/sysinfo.conf", dir);
	(void) fetch_conf_read(path);
    }
    fetch_logo_select();

    if (force_color < 0)
	fetch_color = fetch_conf.color && T_CanColor && fetch_out_isatty();
    else
	fetch_color = force_color;
}

/*
 * dosysinfo - the `sysinfo' builtin.
 *
 *	sysinfo [-n] [-l] [-c|-C]
 *
 * -n  no logo.
 * -l  list the field names `show' and `hide' take, one per line, and print
 *     no panel.  Without this the names would be documented only in the
 *     manual, and a configuration key whose values you have to look up
 *     elsewhere is a key people get wrong.
 * -c  colour even if the output is not a terminal.
 * -C  never colour.
 */
/*ARGSUSED*/
void
dosysinfo(Char **v, struct command *c)
{
    int with_logo = 1;
    int force_color = -1;
    int list_only = 0;

    USE(c);
    v++;
    while (*v != NULL && (*v)[0] == '-' && (*v)[1] != '\0') {
	const char *opt = short2str(*v);

	if (strcmp(opt, "-n") == 0)
	    with_logo = 0;
	else if (strcmp(opt, "-l") == 0)
	    list_only = 1;
	else if (strcmp(opt, "-c") == 0)
	    force_color = 1;
	else if (strcmp(opt, "-C") == 0)
	    force_color = 0;
	else
	    /* A plain literal, not CGETS(): a one-line usage string does not
	     * justify a new message-catalogue set of its own. */
	    stderror(ERR_NAME | ERR_STRING, "Usage: sysinfo [-n] [-l] [-c|-C]");
	v++;
    }
    if (*v != NULL)
	stderror(ERR_NAME | ERR_TOOMANY);

    if (list_only) {
	int i;

	for (i = 0; i < FETCH_NFIELDS; i++)
	    xprintf("%s\n", fetch_fields[i].name);
	flush();
	return;
    }

    fetch_setup(force_color);
    fetch_collect();
    fetch_print(with_logo);
}

/*
 * sysinfo_greeting - print the panel once, when an interactive shell starts,
 * if `set sysinfo' is in effect.  Called from sh.c after the start-up files
 * have been read, so that the variable can be set in ~/.mcshrc.
 */
void
sysinfo_greeting(void)
{
    if (adrof(STRsysinfo) == NULL)
	return;
    fetch_setup(-1);
    fetch_collect();
    fetch_print(1);
}
