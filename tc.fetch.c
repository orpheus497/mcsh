/*
 * tc.fetch.c: The `sysinfo' builtin - a system information panel.
 *
 * A compact summary of the machine the shell is running on, printed on
 * demand by the `sysinfo' builtin and, when `set sysinfo' is in effect,
 * once when an interactive shell starts.
 *
 * Everything here is read with ordinary file and system calls: nothing is
 * shelled out to, no external program is required, and no field is guessed.
 * A value that cannot be determined is omitted rather than approximated, so
 * the panel never reports something it has not actually read.
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
 * The logo.  Deliberately 7-bit ASCII: a box-drawing or block-element logo
 * would be mojibake in a non-UTF-8 locale and on a terminal without the
 * glyphs, and the panel has to be legible on a serial console.
 */
static const char * const fetch_logo[] = {
    "                      _     ",
    "  _ __ ___   ___ ___ | |__  ",
    " | '_ ` _ \\ / __/ __|| '_ \\ ",
    " | | | | | | (__\\__ \\| | | |",
    " |_| |_| |_|\\___|___/|_| |_|",
    NULL
};
#define FETCH_LOGO_W	28	/* every logo row is this wide */

#define FETCH_MAX_ROWS	16
#define FETCH_VAL_MAX	192
#define FETCH_LABEL_W	9	/* width the labels are padded to */

static struct fetch_row {
    const char *label;			/* NULL for the header row */
    char	value[FETCH_VAL_MAX];
} fetch_rows[FETCH_MAX_ROWS];
static int fetch_nrows;

/*
 * fetch_add - append one row.  Silently ignores the row if the table is full
 * or the value is empty, which is what makes "omit what cannot be read" a
 * single check at the point of use rather than at every call site.
 */
static void
fetch_add(const char *label, const char *value)
{
    if (fetch_nrows >= FETCH_MAX_ROWS || value == NULL || *value == '\0')
	return;
    fetch_rows[fetch_nrows].label = label;
    (void) xsnprintf(fetch_rows[fetch_nrows].value,
		     sizeof(fetch_rows[0].value), "%s", value);
    fetch_nrows++;
}

/*
 * fetch_trim - strip leading and trailing whitespace, and one layer of
 * surrounding double quotes, in place.  os-release(5) permits the value to be
 * quoted; /proc/meminfo and /proc/cpuinfo pad theirs with tabs and spaces.
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
 * followed by '=' or ':', which covers all three formats the panel reads:
 *
 *   os-release(5)     PRETTY_NAME="Debian GNU/Linux 12 (bookworm)"
 *   proc(5) meminfo   MemTotal:       16318296 kB
 *   proc(5) cpuinfo   model name	: AMD EPYC 7B12
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
 * The individual rows.  Each collector reads what it can and returns; a
 * failure leaves the row out of the table.
 */

static void
fetch_identity(const struct utsname *uts)
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
		     (uts != NULL && uts->nodename[0] != '\0')
		     ? uts->nodename : "localhost");
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
fetch_os(const struct utsname *uts)
{
    char buf[FETCH_VAL_MAX];

    /* os-release(5): /etc/os-release, with /usr/lib/os-release as the
     * vendor-supplied fallback the specification mandates. */
    if (fetch_key("/etc/os-release", "PRETTY_NAME", buf, sizeof(buf)) ||
	fetch_key("/usr/lib/os-release", "PRETTY_NAME", buf, sizeof(buf))) {
	fetch_add("OS", buf);
	return;
    }
    if (uts != NULL) {
	(void) xsnprintf(buf, sizeof(buf), "%s %s", uts->sysname,
			 uts->release);
	fetch_add("OS", buf);
    }
}

static void
fetch_kernel(const struct utsname *uts)
{
    char buf[FETCH_VAL_MAX];

    if (uts == NULL)
	return;
    (void) xsnprintf(buf, sizeof(buf), "%s %s", uts->sysname, uts->release);
    fetch_add("Kernel", buf);
    fetch_add("Arch", uts->machine);
}

static void
fetch_uptime(void)
{
    char buf[FETCH_VAL_MAX];
    char line[128];

    /*
     * /proc/uptime (proc(5)) holds two numbers: seconds since boot, and
     * seconds spent idle.  Only the first is wanted.  There is no portable
     * interface for this - the BSDs expose kern.boottime through sysctl(3),
     * spelled differently on each - so the row is Linux-only and simply
     * absent elsewhere.
     */
    if (!fetch_line("/proc/uptime", line, sizeof(line)))
	return;
    {
	char *end;
	unsigned long secs;

	errno = 0;
	secs = strtoul(line, &end, 10);
	if (errno != 0 || end == line)
	    return;
	fetch_duration(secs, buf, sizeof(buf));
	fetch_add("Uptime", buf);
    }
}

static void
fetch_shell(void)
{
    char buf[FETCH_VAL_MAX];

    (void) xsnprintf(buf, sizeof(buf), "%s %s", MCSH_NAME, MCSH_VERSION);
    fetch_add("Shell", buf);
}

static void
fetch_term(void)
{
    const char *term = getenv("TERM");

    fetch_add("Terminal", (term != NULL) ? term : "");
}

static void
fetch_cpu(const struct utsname *uts)
{
    char buf[FETCH_VAL_MAX];
    char model[FETCH_VAL_MAX];
    long ncpu = -1;

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
	if (uts != NULL)
	    (void) xsnprintf(model, sizeof(model), "%s", uts->machine);
    }
    if (model[0] == '\0')
	return;

    if (ncpu > 0)
	(void) xsnprintf(buf, sizeof(buf), "%s (%ld)", model, ncpu);
    else
	(void) xsnprintf(buf, sizeof(buf), "%s", model);
    fetch_add("CPU", buf);
}

static void
fetch_memory(void)
{
    char buf[FETCH_VAL_MAX];
    char used_s[48], total_s[48];
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
    fetch_size(total - avail, used_s, sizeof(used_s));
    fetch_size(total, total_s, sizeof(total_s));
    (void) xsnprintf(buf, sizeof(buf), "%s / %s", used_s, total_s);
    fetch_add("Memory", buf);
}

static void
fetch_disk(void)
{
#ifdef FETCH_HAVE_STATVFS
    struct statvfs vfs;
    char buf[FETCH_VAL_MAX];
    char used_s[48], total_s[48];
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
    fetch_size(total - avail, used_s, sizeof(used_s));
    fetch_size(total, total_s, sizeof(total_s));
    (void) xsnprintf(buf, sizeof(buf), "%s / %s", used_s, total_s);
    fetch_add("Disk (/)", buf);
#endif /* FETCH_HAVE_STATVFS */
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
 * Colour.  Emitted only when the terminal advertises colour, exactly as the
 * syntax highlighter and the ghost-text renderer decide it (T_CanColor,
 * ed.screen.c), and only when writing to a terminal at all - a redirected
 * `sysinfo > file' must produce plain text.
 *
 * The sequences are ECMA-48 (5th edition) 8.3.117 SGR: 1 BOLD, 22 NORMAL,
 * 36 and 34 foreground cyan and blue, 39 default foreground, and 40..47 /
 * 100..107 for the background swatches in the palette row.
 */
static int fetch_color;		/* set once per run of do_sysinfo() */

static void
fetch_sgr(const char *params)
{
    if (!fetch_color)
	return;
    xprintf("\033[%sm", params);
}

static void
fetch_palette(void)
{
    int i;

    if (!fetch_color)
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
    int nlogo = 0, i, rows;

    while (fetch_logo[nlogo] != NULL)
	nlogo++;

    if (with_logo && T_Cols > 0 && T_Cols < FETCH_LOGO_W + 24)
	with_logo = 0;

    rows = (with_logo && nlogo > fetch_nrows) ? nlogo : fetch_nrows;

    for (i = 0; i < rows; i++) {
	if (with_logo) {
	    if (i < nlogo) {
		fetch_sgr("1;36");
		xprintf("%s", fetch_logo[i]);
		fetch_sgr("22;39");
	    }
	    else
		/* Past the bottom of the logo: pad with plain spaces rather
		 * than an empty coloured run, so the rows below cost no
		 * escape sequences at all. */
		xprintf("%*s", FETCH_LOGO_W, "");
	    xprintf("  ");
	}
	if (i < fetch_nrows) {
	    const struct fetch_row *r = &fetch_rows[i];

	    if (r->label == NULL) {
		/* The header: user@host, then a rule the same width. */
		fetch_sgr("1;36");
		xprintf("%s", r->value);
		fetch_sgr("22;39");
	    }
	    else {
		fetch_sgr("1;34");
		xprintf("%-*s", FETCH_LABEL_W, r->label);
		fetch_sgr("22;39");
		xprintf(" %s", r->value);
	    }
	}
	xprintf("\n");
    }
    fetch_palette();
    flush();
}

/*
 * fetch_collect - fill the row table.  Order is the reading order of the
 * panel, not the cost of the collectors: every one of them is a handful of
 * syscalls.
 */
static void
fetch_collect(void)
{
    struct utsname uts;
    const struct utsname *u = NULL;

    fetch_nrows = 0;
    if (uname(&uts) >= 0)
	u = &uts;

    fetch_identity(u);
    fetch_os(u);
    fetch_kernel(u);
    fetch_uptime();
    fetch_shell();
    fetch_term();
    fetch_cpu(u);
    fetch_memory();
    fetch_disk();
    fetch_load();
}

/*
 * dosysinfo - the `sysinfo' builtin.
 *
 *	sysinfo [-n]
 *
 * -n suppresses the logo.  There are no other options: every row the panel
 * can produce is cheap enough to always produce, and a row that cannot be
 * read is already omitted.
 */
/*ARGSUSED*/
void
dosysinfo(Char **v, struct command *c)
{
    int with_logo = 1;

    USE(c);
    v++;
    while (*v != NULL && (*v)[0] == '-') {
	if (eq(*v, STRmn))
	    with_logo = 0;
	else
	    /* A plain literal, not CGETS(): a one-line usage string does not
	     * justify a new message-catalogue set of its own. */
	    stderror(ERR_NAME | ERR_STRING, "Usage: sysinfo [-n]");
	v++;
    }
    if (*v != NULL)
	stderror(ERR_NAME | ERR_TOOMANY);

    fetch_color = T_CanColor && isoutatty;
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
    fetch_color = T_CanColor && isoutatty;
    fetch_collect();
    fetch_print(1);
}
