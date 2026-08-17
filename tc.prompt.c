/*
 * tc.prompt.c: Prompt printing stuff
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
#include "tw.h"
#include <stdio.h>

/*
 * kfk 21oct1983 -- add @ (time) and / ($cwd) in prompt.
 * PWP 4/27/87 -- rearange for tcsh.
 * mrdch@com.tau.edu.il 6/26/89 - added ~, T and .# - rearanged to switch()
 *                 instead of if/elseif
 * Luke Mewburn
 *	6-Sep-91	changed date format
 *	16-Feb-94	rewrote directory prompt code, added $ellipsis
 *	29-Dec-96	added rprompt support
 */

#define GIT_POLL_INTERVAL 2  /* seconds between filesystem mtime polls */
#define GIT_SHORT_SHA_LEN 7  /* abbreviated object name length for detached HEAD */
#define GIT_HEAD_MAX	  256  /* enough for "ref: refs/heads/<name>" */

static const char   *month_list[12];
static const char   *day_list[7];

void
dateinit(void)
{
#ifdef notyet
  int i;

  setlocale(LC_TIME, "");

  for (i = 0; i < 12; i++)
      xfree((ptr_t) month_list[i]);
  month_list[0] = strsave(_time_info->abbrev_month[0]);
  month_list[1] = strsave(_time_info->abbrev_month[1]);
  month_list[2] = strsave(_time_info->abbrev_month[2]);
  month_list[3] = strsave(_time_info->abbrev_month[3]);
  month_list[4] = strsave(_time_info->abbrev_month[4]);
  month_list[5] = strsave(_time_info->abbrev_month[5]);
  month_list[6] = strsave(_time_info->abbrev_month[6]);
  month_list[7] = strsave(_time_info->abbrev_month[7]);
  month_list[8] = strsave(_time_info->abbrev_month[8]);
  month_list[9] = strsave(_time_info->abbrev_month[9]);
  month_list[10] = strsave(_time_info->abbrev_month[10]);
  month_list[11] = strsave(_time_info->abbrev_month[11]);

  for (i = 0; i < 7; i++)
      xfree((ptr_t) day_list[i]);
  day_list[0] = strsave(_time_info->abbrev_wkday[0]);
  day_list[1] = strsave(_time_info->abbrev_wkday[1]);
  day_list[2] = strsave(_time_info->abbrev_wkday[2]);
  day_list[3] = strsave(_time_info->abbrev_wkday[3]);
  day_list[4] = strsave(_time_info->abbrev_wkday[4]);
  day_list[5] = strsave(_time_info->abbrev_wkday[5]);
  day_list[6] = strsave(_time_info->abbrev_wkday[6]);
#else
  month_list[0] = "Jan";
  month_list[1] = "Feb";
  month_list[2] = "Mar";
  month_list[3] = "Apr";
  month_list[4] = "May";
  month_list[5] = "Jun";
  month_list[6] = "Jul";
  month_list[7] = "Aug";
  month_list[8] = "Sep";
  month_list[9] = "Oct";
  month_list[10] = "Nov";
  month_list[11] = "Dec";

  day_list[0] = "Sun";
  day_list[1] = "Mon";
  day_list[2] = "Tue";
  day_list[3] = "Wed";
  day_list[4] = "Thu";
  day_list[5] = "Fri";
  day_list[6] = "Sat";
#endif
}

void
printprompt(int promptno, const char *str)
{
    static  const Char *ocp = NULL;
    static  const char *ostr = NULL;
    time_t  lclock = time(NULL);
    const Char *cp;

    switch (promptno) {
    default:
    case 0:
	cp = varval(STRprompt);
	break;
    case 1:
	cp = varval(STRprompt2);
	break;
    case 2:
	cp = varval(STRprompt3);
	break;
    case 3:
	if (ocp != NULL) {
	    cp = ocp;
	    str = ostr;
	}
	else
	    cp = varval(STRprompt);
	break;
    }

    if (promptno < 2) {
	ocp = cp;
	ostr = str;
    }

    xfree(Prompt);
    Prompt = NULL;
    Prompt = tprintf(FMT_PROMPT, cp, str, lclock, NULL);
    if (!editing) {
	for (cp = Prompt; *cp ; )
	    (void) putwraw(*cp++);
	SetAttributes(0);
	flush();
    }

    xfree(RPrompt);
    RPrompt = NULL;
    if (promptno == 0) {	/* determine rprompt if using main prompt */
	cp = varval(STRrprompt);
	RPrompt = tprintf(FMT_PROMPT, cp, NULL, lclock, NULL);
				/* if not editing, put rprompt after prompt */
	if (!editing && RPrompt[0] != '\0') {
	    for (cp = RPrompt; *cp ; )
		(void) putwraw(*cp++);
	    SetAttributes(0);
	    putraw(' ');
	    flush();
	}
    }
}

static void
tprintf_append_mbs(struct Strbuf *buf, const char *mbs, Char attributes)
{
    while (*mbs != 0) {
	Char wc;

	mbs += one_mbtowc(&wc, mbs, MB_LEN_MAX);
	Strbuf_append1(buf, wc | attributes);
    }
}

static int
strip_trailing_newline(char *buf, size_t bufsize, size_t *len_out)
{
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
	buf[--len] = '\0';
    } else if (len == bufsize - 1) {
	/* Line was truncated by fgets */
	return -1;
    }
    if (len_out)
	*len_out = len;
    return 0;
}

/*
 * git_poll_interval - seconds to wait between filesystem staleness polls.
 * Overridable at run time with $GIT_POLL_INTERVAL; a malformed, negative or
 * out-of-range value falls back to the compiled-in default.
 */
static int
git_poll_interval(void)
{
    const char *ev = getenv("GIT_POLL_INTERVAL");
    char *end;
    long v;

    if (ev == NULL || *ev == '\0')
	return GIT_POLL_INTERVAL;

    errno = 0;
    v = strtol(ev, &end, 10);
    if (errno != 0 || end == ev || *end != '\0' || v < 0 || v > INT_MAX)
	return GIT_POLL_INTERVAL;

    return (int) v;
}

/*
 * git_read_state - capture the two signals that decide whether the cached git
 * information is still current.
 *
 * head is filled with the literal contents of gitdir/HEAD (an empty string if
 * it cannot be read).  HEAD is a ~41 byte file, so reading it costs about what
 * stat()ing it does and is exact: st_mtime has one-second granularity, and two
 * HEAD writes inside the same second - scripted checkouts, a TUI git client,
 * rebase stepping through commits - left the cache permanently stale.
 *
 * marker_mtime is the newest mtime of any in-progress operation marker.  It is
 * tracked separately from HEAD so that a live MERGE_HEAD, whose mtime is
 * unrelated to HEAD's, does not force a refresh on every prompt.  Second
 * granularity is tolerable here because these files are only probed for
 * existence; a same-second create/delete pair still changes HEAD or the branch.
 *
 * gitdir is the resolved git directory - what git_get_info() reported, not
 * "$cwd/.git" - so the markers are found from anywhere inside the worktree.
 */
static void
git_read_state(const char *gitdir, char *head, size_t headsz,
	       time_t *marker_mtime)
{
    /* One entry per state git_get_info() can report, so that entering or
     * leaving any of them is noticed.  Directories are watched alongside the
     * files inside them because a state can begin or end without any watched
     * file's own mtime changing. */
    static const char * const markers[] = {
	"MERGE_HEAD",
	"CHERRY_PICK_HEAD",
	"REVERT_HEAD",
	"BISECT_LOG",
	"REBASE_HEAD",
	"rebase-merge",
	"rebase-merge/head-name",
	"rebase-apply",
	NULL
    };
    char path[MAXPATHLEN];
    struct stat st;
    const char * const *mp;
    char *tail;
    size_t remain;
    int len, tlen;

    if (headsz > 0)
	head[0] = '\0';
    *marker_mtime = 0;

    if (gitdir == NULL || *gitdir == '\0')
	return;

    /* Format the gitdir prefix once; only the trailing component varies. */
    len = xsnprintf(path, sizeof(path), "%s/", gitdir);
    if (len < 0 || (size_t) len >= sizeof(path))
	return;

    tail = path + len;
    remain = sizeof(path) - len;

    tlen = xsnprintf(tail, remain, "%s", "HEAD");
    if (tlen >= 0 && (size_t) tlen < remain && headsz > 0) {
	FILE *hf = fopen(path, "r");

	if (hf != NULL) {
	    if (fgets(head, (int) headsz, hf) == NULL)
		head[0] = '\0';
	    fclose(hf);
	}
    }

    for (mp = markers; *mp != NULL; mp++) {
	/* Skip rather than stat a truncated path, which would name a
	 * different file than intended. */
	tlen = xsnprintf(tail, remain, "%s", *mp);
	if (tlen < 0 || (size_t) tlen >= remain)
	    continue;
	if (stat(path, &st) == 0 && st.st_mtime > *marker_mtime)
	    *marker_mtime = st.st_mtime;
    }
}

/*
 * git_get_info - fill branch (up to branchsz-1 bytes) and op (up to opsz-1
 * bytes) for the git worktree that contains dir.  Returns 1 on success, 0 if
 * dir is not inside a git worktree.  Both buffers are always NUL-terminated.
 *
 * On success the resolved git directory is also written to gitdirout (up to
 * gitdirsz-1 bytes).  Callers need it to watch HEAD and the operation markers
 * for changes: dir may be any subdirectory of the worktree, and for linked
 * worktrees and submodules the git directory is not "$dir/.git" at all.
 *
 * op is the empty string when no special operation is in progress, or one of:
 * MERGING, REBASING, REBASING-i, AM, CHERRY-PICKING, REVERTING, BISECTING,
 * DETACHED.
 *
 * Detection is done by walking up the directory tree reading plain files; no
 * subprocesses are spawned.
 */
static int
git_get_info(const char *dir, char *branch, size_t branchsz,
	     char *op, size_t opsz, char *gitdirout, size_t gitdirsz)
{
    char path[MAXPATHLEN];
    char gitdir[MAXPATHLEN];
    char *p;
    FILE *fp;
    size_t n;
    int found = 0;
    int detached = 0;

    if (gitdirout != NULL && gitdirsz > 0)
	gitdirout[0] = '\0';

    if (dir == NULL || *dir == '\0')
	return 0;

    /* Walk up, looking for .git */
    n = strlen(dir);
    if (n >= sizeof(gitdir))
	n = sizeof(gitdir) - 1;
    memcpy(gitdir, dir, n);
    gitdir[n] = '\0';

    for (;;) {
	int plen, blen;

	/* Try .git - may be a directory (normal repo) or a file (linked
	 * worktree or submodule). */
	plen = xsnprintf(path, sizeof(path), "%s/.git", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(path)) {
	    struct stat st;

	    if (stat(path, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
		    /* Normal repo: .git/HEAD */
		    char head[MAXPATHLEN];
		    int hlen = xsnprintf(head, sizeof(head), "%s/.git/HEAD",
					 gitdir);

		    if (hlen >= 0 && (size_t) hlen < sizeof(head) &&
			access(head, R_OK) == 0) {
			found = 1;
			break;
		    }
		}
		else if (S_ISREG(st.st_mode)) {
		    /* Linked worktree or submodule: .git is a file whose
		     * first line reads "gitdir: <path>". */
		    FILE *gf = fopen(path, "r");

		    if (gf != NULL) {
			char line[MAXPATHLEN];
			int resolved_ok = 0;

			if (fgets(line, sizeof(line), gf) != NULL &&
			    strncmp(line, "gitdir: ", 8) == 0) {
			    char resolved[MAXPATHLEN];
			    char *target = line + 8;
			    size_t llen = strlen(target);
			    int len;

			    while (llen > 0 && (target[llen - 1] == '\n' ||
						target[llen - 1] == '\r'))
				target[--llen] = '\0';

			    if (target[0] == '/')
				len = xsnprintf(resolved, sizeof(resolved),
						"%s", target);
			    else
				len = xsnprintf(resolved, sizeof(resolved),
						"%s/%s", gitdir, target);

			    if (len >= 0 && (size_t) len < sizeof(resolved)) {
				int glen = xsnprintf(gitdir, sizeof(gitdir),
						     "%s", resolved);

				if (glen >= 0 && (size_t) glen < sizeof(gitdir))
				    resolved_ok = 1;
			    }
			}
			fclose(gf);
			if (resolved_ok) {
			    /* gitdir already points at the real git dir */
			    found = 1;
			    goto git_found;
			}
		    }
		}
	    }
	}

	/* Try bare repo: HEAD and config directly in this directory. */
	blen = xsnprintf(path, sizeof(path), "%s/HEAD", gitdir);
	if (blen >= 0 && (size_t) blen < sizeof(path)) {
	    char cfg[MAXPATHLEN];
	    int clen = xsnprintf(cfg, sizeof(cfg), "%s/config", gitdir);

	    if (clen >= 0 && (size_t) clen < sizeof(cfg) &&
		access(cfg, R_OK) == 0 && access(path, R_OK) == 0) {
		FILE *hf = fopen(path, "r");

		if (hf != NULL) {
		    char line[256];
		    int looks_like_head = 0;

		    /* Check it looks like a bare repo HEAD */
		    if (fgets(line, sizeof(line), hf) != NULL &&
			(strncmp(line, "ref: ", 5) == 0 ||
			 (strlen(line) >= 40 &&
			  strspn(line, "0123456789abcdef") >= 40)))
			looks_like_head = 1;
		    fclose(hf);
		    if (looks_like_head) {
			/* Bare repo: gitdir already points at the repo dir */
			found = 2;
			break;
		    }
		}
	    }
	}

	/* Go up one level */
	p = strrchr(gitdir, '/');
	if (p == NULL || p == gitdir)
	    break;
	*p = '\0';
    }

    if (!found)
	return 0;

    /* Build the .git directory path.  found == 2 means gitdir already points
     * at the bare repo directory. */
    if (found == 1) {
	char tmp[MAXPATHLEN];
	int tlen = xsnprintf(tmp, sizeof(tmp), "%s/.git", gitdir);

	if (tlen < 0 || (size_t) tlen >= sizeof(tmp))
	    return 0;
	xsnprintf(gitdir, sizeof(gitdir), "%s", tmp);
    }

git_found:
    /* Read HEAD */
    {
	int plen = xsnprintf(path, sizeof(path), "%s/HEAD", gitdir);

	if (plen < 0 || (size_t) plen >= sizeof(path))
	    return 0;
    }
    fp = fopen(path, "r");
    if (fp == NULL)
	return 0;

    branch[0] = '\0';
    if (fgets(path, sizeof(path), fp) != NULL) {
	size_t len;

	if (strip_trailing_newline(path, sizeof(path), &len) < 0) {
	    fclose(fp);
	    return 0;
	}
	if (strncmp(path, "ref: refs/heads/", 16) == 0) {
	    int blen = xsnprintf(branch, branchsz, "%s", path + 16);

	    if (blen < 0 || (size_t) blen >= branchsz) {
		fclose(fp);
		return 0;
	    }
	}
	else if (strncmp(path, "ref: ", 5) == 0) {
	    int blen = xsnprintf(branch, branchsz, "%s", path + 5);

	    if (blen < 0 || (size_t) blen >= branchsz) {
		fclose(fp);
		return 0;
	    }
	}
	else if (len >= GIT_SHORT_SHA_LEN) {
	    /* Detached HEAD: show the abbreviated object name.  xsnprintf()
	     * does not implement "%.*s" precision - a '.' straight after '%'
	     * is consumed as a zero-pad flag - so truncate explicitly rather
	     * than printing the full 40-character object name. */
	    if (branchsz < GIT_SHORT_SHA_LEN + 1) {
		fclose(fp);
		return 0;
	    }
	    memcpy(branch, path, GIT_SHORT_SHA_LEN);
	    branch[GIT_SHORT_SHA_LEN] = '\0';
	    detached = 1;
	}
    }
    fclose(fp);

    if (branch[0] == '\0')
	return 0;

    if (gitdirout != NULL && gitdirsz > 0)
	xsnprintf(gitdirout, gitdirsz, "%s", gitdir);

    /* Detect operation state.  A detached HEAD is reported only when no more
     * specific operation is in progress - rebase and bisect both detach. */
    op[0] = '\0';
    if (detached)
	xsnprintf(op, opsz, "DETACHED");

    {
	char probe[MAXPATHLEN];
	int plen;

	/* MERGE */
	plen = xsnprintf(probe, sizeof(probe), "%s/MERGE_HEAD", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(probe) &&
	    access(probe, F_OK) == 0) {
	    xsnprintf(op, opsz, "MERGING");
	    return 1;
	}

	/* REBASE (interactive) */
	plen = xsnprintf(probe, sizeof(probe), "%s/rebase-merge", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(probe) &&
	    access(probe, F_OK) == 0) {
	    char rbranch[256];
	    FILE *rf;
	    int rplen;

	    rplen = xsnprintf(probe, sizeof(probe), "%s/rebase-merge/head-name",
			      gitdir);
	    rf = (rplen >= 0 && (size_t) rplen < sizeof(probe))
		? fopen(probe, "r") : NULL;
	    if (rf != NULL) {
		if (fgets(rbranch, sizeof(rbranch), rf) != NULL &&
		    strip_trailing_newline(rbranch, sizeof(rbranch), NULL) == 0) {
		    if (strncmp(rbranch, "refs/heads/", 11) == 0)
			xsnprintf(branch, branchsz, "%s", rbranch + 11);
		    else
			xsnprintf(branch, branchsz, "%s", rbranch);
		}
		fclose(rf);
	    }
	    xsnprintf(op, opsz, "REBASING-i");
	    return 1;
	}

	/* REBASE (am/apply) */
	plen = xsnprintf(probe, sizeof(probe), "%s/rebase-apply", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(probe) &&
	    access(probe, F_OK) == 0) {
	    int rplen = xsnprintf(probe, sizeof(probe),
				  "%s/rebase-apply/rebasing", gitdir);

	    if (rplen >= 0 && (size_t) rplen < sizeof(probe) &&
		access(probe, F_OK) == 0)
		xsnprintf(op, opsz, "REBASING");
	    else
		xsnprintf(op, opsz, "AM");
	    return 1;
	}

	/* CHERRY-PICK */
	plen = xsnprintf(probe, sizeof(probe), "%s/CHERRY_PICK_HEAD", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(probe) &&
	    access(probe, F_OK) == 0) {
	    xsnprintf(op, opsz, "CHERRY-PICKING");
	    return 1;
	}

	/* REVERT */
	plen = xsnprintf(probe, sizeof(probe), "%s/REVERT_HEAD", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(probe) &&
	    access(probe, F_OK) == 0) {
	    xsnprintf(op, opsz, "REVERTING");
	    return 1;
	}

	/* BISECT */
	plen = xsnprintf(probe, sizeof(probe), "%s/BISECT_LOG", gitdir);
	if (plen >= 0 && (size_t) plen < sizeof(probe) &&
	    access(probe, F_OK) == 0) {
	    xsnprintf(op, opsz, "BISECTING");
	    return 1;
	}
    }
    return 1;
}

Char *
tprintf(int what, const Char *fmt, const char *str, time_t tim, ptr_t info)
{
    struct Strbuf buf = Strbuf_INIT;
    Char   *z, *q;
    Char    attributes = 0;
    static int print_prompt_did_ding = 0;
    char *cz;

    Char *p;
    const Char *cp = fmt;
    Char Scp;
    struct tm *t = localtime(&tim);

			/* prompt stuff */
    static Char *olduser = NULL;
    int updirs;
    size_t pdirs;

    /* git info cache.  git_oldcwd holds a copy of the cwd rather than a
     * pointer into the variable table, so the key stays valid and comparable
     * after the variable is reassigned or freed. */
    static char git_oldcwd[MAXPATHLEN];
    static char git_gitdir[MAXPATHLEN];	/* resolved git dir for git_oldcwd */
    static char git_branch[256];
    static char git_op[64];
    static char git_head[GIT_HEAD_MAX];	/* literal contents of gitdir/HEAD */
    static int  git_valid = -1;
    static time_t git_marker_mtime = 0;
    static time_t git_last_stattime = 0; /* wall-clock of last mtime poll */

    cleanup_push(&buf, Strbuf_cleanup);
    for (; *cp; cp++) {
	if ((*cp == '%') && ! (cp[1] == '\0')) {
	    cp++;
	    switch (*cp) {
	    case 'R':
		if (what == FMT_HISTORY) {
		    cz = fmthist('R', info);
		    tprintf_append_mbs(&buf, cz, attributes);
		    xfree(cz);
		} else {
		    if (str != NULL)
			tprintf_append_mbs(&buf, str, attributes);
		}
		break;
	    case '#':
#ifdef __CYGWIN__
		/* Check for being member of the Administrators group */
		{
			gid_t grps[NGROUPS_MAX];
			int grp, gcnt;

			gcnt = getgroups(NGROUPS_MAX, grps);
# define DOMAIN_GROUP_RID_ADMINS 544
			for (grp = 0; grp < gcnt; ++grp)
				if (grps[grp] == DOMAIN_GROUP_RID_ADMINS)
					break;
			Scp = (grp < gcnt) ? PRCHROOT : PRCH;
		}
#else
		Scp = (uid == 0 || euid == 0) ? PRCHROOT : PRCH;
#endif
		if (Scp != '\0')
		    Strbuf_append1(&buf, attributes | Scp);
		break;
	    case '!':
	    case 'h':
		switch (what) {
		case FMT_HISTORY:
		    cz = fmthist('h', info);
		    break;
		case FMT_SCHED:
		    cz = xasprintf("%d", *(int *)info);
		    break;
		default:
		    cz = xasprintf("%d", eventno + 1);
		    break;
		}
		tprintf_append_mbs(&buf, cz, attributes);
		xfree(cz);
		break;
	    case 'T':		/* 24 hour format	 */
	    case '@':
	    case 't':		/* 12 hour am/pm format */
	    case 'p':		/* With seconds	*/
	    case 'P':
		{
		    char    ampm = 'a';
		    int     hr = t->tm_hour;

		    /* addition by Hans J. Albertsson */
		    /* and another adapted from Justin Bur */
		    if (adrof(STRampm) || (*cp != 'T' && *cp != 'P')) {
			if (hr >= 12) {
			    if (hr > 12)
				hr -= 12;
			    ampm = 'p';
			}
			else if (hr == 0)
			    hr = 12;
		    }		/* else do a 24 hour clock */

		    /* "DING!" stuff by Hans also */
		    if (t->tm_min || print_prompt_did_ding ||
			what != FMT_PROMPT || adrof(STRnoding)) {
			if (t->tm_min)
			    print_prompt_did_ding = 0;
			/*
			 * Pad hour to 2 characters if padhour is set,
			 * by ADAM David Alan Martin
			 */
			p = Itoa(hr, adrof(STRpadhour) ? 2 : 0, attributes);
			Strbuf_append(&buf, p);
			xfree(p);
			Strbuf_append1(&buf, attributes | ':');
			p = Itoa(t->tm_min, 2, attributes);
			Strbuf_append(&buf, p);
			xfree(p);
			if (*cp == 'p' || *cp == 'P') {
			    Strbuf_append1(&buf, attributes | ':');
			    p = Itoa(t->tm_sec, 2, attributes);
			    Strbuf_append(&buf, p);
			    xfree(p);
			}
			if (adrof(STRampm) || (*cp != 'T' && *cp != 'P')) {
			    Strbuf_append1(&buf, attributes | ampm);
			    Strbuf_append1(&buf, attributes | 'm');
			}
		    }
		    else {	/* we need to ding */
			size_t i;

			for (i = 0; STRDING[i] != 0; i++)
			    Strbuf_append1(&buf, attributes | STRDING[i]);
			print_prompt_did_ding = 1;
		    }
		}
		break;

	    case 'M':
#ifndef HAVENOUTMP
		if (what == FMT_WHO)
		    cz = who_info(info, 'M');
		else
#endif /* HAVENOUTMP */
		    cz = getenv("HOST");
		/*
		 * Bug pointed out by Laurent Dami <dami@cui.unige.ch>: don't
		 * derefrence that NULL (if HOST is not set)...
		 */
		if (cz != NULL)
		    tprintf_append_mbs(&buf, cz, attributes);
		if (what == FMT_WHO)
		    xfree(cz);
		break;

	    case 'm': {
		char *scz = NULL;
#ifndef HAVENOUTMP
		if (what == FMT_WHO)
		    scz = cz = who_info(info, 'm');
		else
#endif /* HAVENOUTMP */
		    cz = getenv("HOST");

		if (cz != NULL)
		    while (*cz != 0 && (what == FMT_WHO || *cz != '.')) {
			Char wc;

			cz += one_mbtowc(&wc, cz, MB_LEN_MAX);
			Strbuf_append1(&buf, wc | attributes);
		    }
		if (scz)
		    xfree(scz);
		break;
	    }

	    case '~':
	    case '/':
	    case '.':
	    case 'c':
	    case 'C':
		Scp = *cp;
		if (Scp == 'c')		/* store format type (c == .) */
		    Scp = '.';
		if ((z = varval(STRcwd)) == STRNULL)
		    break;		/* no cwd, so don't do anything */

			/* show ~ whenever possible - a la dirs */
		if (Scp == '~' || Scp == '.' ) {
		    static Char *olddir = NULL;

		    if (tlength == 0 || olddir != z) {
			olddir = z;		/* have we changed dir? */
			olduser = getusername(&olddir);
		    }
		    if (olduser)
			z = olddir;
		}
		updirs = pdirs = 0;

			/* option to determine fixed # of dirs from path */
		if (Scp == '.' || Scp == 'C') {
		    int skip;
		    q = z;
		    while (*z)				/* calc # of /'s */
			if (*z++ == '/')
			    updirs++;

		    if ((Scp == 'C' && *q != '/'))
			updirs++;

		    if (cp[1] == '0') {			/* print <x> or ...  */
			pdirs = 1;
			cp++;
		    }
		    if (cp[1] >= '1' && cp[1] <= '9') {	/* calc # to skip  */
			skip = cp[1] - '0';
			cp++;
		    }
		    else
			skip = 1;

		    updirs -= skip;
		    while (skip-- > 0) {
			while ((z > q) && (*z != '/'))
			    z--;			/* back up */
			if (skip && z > q)
			    z--;
		    }
		    if (*z == '/' && z != q)
			z++;
		} /* . || C */

							/* print ~[user] */
		if ((olduser) && ((Scp == '~') ||
		     (Scp == '.' && (pdirs || (!pdirs && updirs <= 0))) )) {
		    Strbuf_append1(&buf, attributes | '~');
		    for (q = olduser; *q; q++)
			Strbuf_append1(&buf, attributes | *q);
		}

			/* RWM - tell you how many dirs we've ignored */
			/*       and add '/' at front of this         */
		if (updirs > 0 && pdirs) {
		    if (adrof(STRellipsis)) {
			Strbuf_append1(&buf, attributes | '.');
			Strbuf_append1(&buf, attributes | '.');
			Strbuf_append1(&buf, attributes | '.');
		    } else {
			Strbuf_append1(&buf, attributes | '/');
			Strbuf_append1(&buf, attributes | '<');
			if (updirs > 9) {
			    Strbuf_append1(&buf, attributes | '9');
			    Strbuf_append1(&buf, attributes | '+');
			} else
			    Strbuf_append1(&buf, attributes | ('0' + updirs));
			Strbuf_append1(&buf, attributes | '>');
		    }
		}

		while (*z)
		    Strbuf_append1(&buf, attributes | *z++);
		break;

	    case 'n':
#ifndef HAVENOUTMP
		if (what == FMT_WHO) {
		    cz = who_info(info, 'n');
		    tprintf_append_mbs(&buf, cz, attributes);
		    xfree(cz);
		}
		else
#endif /* HAVENOUTMP */
		{
		    if ((z = varval(STRuser)) != STRNULL)
			while (*z)
			    Strbuf_append1(&buf, attributes | *z++);
		}
		break;
	    case 'N':
		if ((z = varval(STReuser)) != STRNULL)
		    while (*z)
			Strbuf_append1(&buf, attributes | *z++);
		break;
	    case 'l':
#ifndef HAVENOUTMP
		if (what == FMT_WHO) {
		    cz = who_info(info, 'l');
		    tprintf_append_mbs(&buf, cz, attributes);
		    xfree(cz);
		}
		else
#endif /* HAVENOUTMP */
		{
		    if ((z = varval(STRtty)) != STRNULL)
			while (*z)
			    Strbuf_append1(&buf, attributes | *z++);
		}
		break;
	    case 'd':
		tprintf_append_mbs(&buf, day_list[t->tm_wday], attributes);
		break;
	    case 'D':
		p = Itoa(t->tm_mday, 2, attributes);
		Strbuf_append(&buf, p);
		xfree(p);
		break;
	    case 'w':
		tprintf_append_mbs(&buf, month_list[t->tm_mon], attributes);
		break;
	    case 'W':
		p = Itoa(t->tm_mon + 1, 2, attributes);
		Strbuf_append(&buf, p);
		xfree(p);
		break;
	    case 'y':
		p = Itoa(t->tm_year % 100, 2, attributes);
		Strbuf_append(&buf, p);
		xfree(p);
		break;
	    case 'Y':
		p = Itoa(t->tm_year + 1900, 4, attributes);
		Strbuf_append(&buf, p);
		xfree(p);
		break;
	    case 'S':		/* start standout */
		attributes |= STANDOUT;
		break;
	    case 'B':		/* start bold */
		attributes |= BOLD;
		break;
	    case 'U':		/* start underline */
		attributes |= UNDER;
		break;
	    case 's':		/* end standout */
		attributes &= ~STANDOUT;
		break;
	    case 'b':		/* end bold */
		attributes &= ~BOLD;
		break;
	    case 'u':		/* end underline */
		attributes &= ~UNDER;
		break;
	    case 'L':
		ClearToBottom();
		break;

	    case 'j':
		{
		    int njobs = 0;
		    struct process *pp;

		    for (pp = proclist.p_next; pp; pp = pp->p_next) {
			if (pp->p_procid == pp->p_jobid) {
			    struct process *mp = pp;
			    do {
				if (mp->p_flags & (PRUNNING | PSTOPPED)) {
				    njobs++;
				    break;
				}
				mp = mp->p_friends;
			    } while (mp != pp);
			}
		    }
		    p = Itoa(njobs, 1, attributes);
		    Strbuf_append(&buf, p);
		    xfree(p);
		    break;
		}
	    case '?':
		if ((z = varval(STRstatus)) != STRNULL)
		    while (*z)
			Strbuf_append1(&buf, attributes | *z++);
		break;
	    case 'g':
	    case 'G':
		if (what == FMT_PROMPT) {
		    Char *gcwd = varval(STRcwd);
		    char mbcwd[MAXPATHLEN];
		    int need_refresh;
		    int clen;

		    if (gcwd == STRNULL)
			break;

		    /* short2str() hands back a single static buffer that the
		     * next call overwrites, so take a copy up front. */
		    clen = xsnprintf(mbcwd, sizeof(mbcwd), "%s", short2str(gcwd));
		    if (clen < 0 || (size_t) clen >= sizeof(mbcwd))
			break;

		    need_refresh = (git_valid < 0 ||
				    strcmp(git_oldcwd, mbcwd) != 0);

		    if (!need_refresh) {
			/* Throttle stat() calls: poll the filesystem at most
			 * once every GIT_POLL_INTERVAL seconds.  A cwd or
			 * validity change bypasses the throttle. */
			time_t now = time(NULL);

			if (now - git_last_stattime >= git_poll_interval()) {
			    git_last_stattime = now;
			    if (git_valid) {
				char head[GIT_HEAD_MAX];
				time_t marker_mtime;

				/* Watch the resolved git directory, not
				 * "$cwd/.git": the latter exists only at the
				 * root of a non-worktree checkout, so watching
				 * it left the cache permanently stale in every
				 * subdirectory and in linked worktrees. */
				git_read_state(git_gitdir, head, sizeof(head),
					       &marker_mtime);
				if (strcmp(head, git_head) != 0 ||
				    marker_mtime != git_marker_mtime)
				    need_refresh = 1;
			    }
			    else {
				/* Not a repo last time; a cheap probe picks up
				 * a fresh "git init" in this directory. */
				char probe[MAXPATHLEN];
				struct stat st;
				int plen = xsnprintf(probe, sizeof(probe),
						     "%s/.git", mbcwd);

				if (plen >= 0 && (size_t) plen < sizeof(probe) &&
				    stat(probe, &st) == 0)
				    need_refresh = 1;
			    }
			}
		    }

		    if (need_refresh) {
			git_valid = git_get_info(mbcwd,
			    git_branch, sizeof(git_branch),
			    git_op, sizeof(git_op),
			    git_gitdir, sizeof(git_gitdir));
			if (!git_valid)
			    git_gitdir[0] = '\0';
			git_read_state(git_gitdir, git_head, sizeof(git_head),
				       &git_marker_mtime);
			xsnprintf(git_oldcwd, sizeof(git_oldcwd), "%s", mbcwd);
			/* A refresh is itself a filesystem read, so restart
			 * the throttle window from here; otherwise the first
			 * staleness poll always fired regardless of the
			 * configured interval. */
			git_last_stattime = time(NULL);
		    }

		    if (!git_valid)
			break;

		    tprintf_append_mbs(&buf, git_branch, attributes);
		    if (*cp == 'G' && git_op[0] != '\0') {
			tprintf_append_mbs(&buf, "|", attributes);
			tprintf_append_mbs(&buf, git_op, attributes);
		    }
		}
		break;
	    case '$':
		expdollar(&buf, &cp, attributes);
		/* cp should point the last char of current % sequence */
		cp--;
		break;
	    case '%':
		Strbuf_append1(&buf, attributes | '%');
		break;
	    case '{':		/* literal characters start */
#if LITERAL == 0
		/*
		 * No literal capability, so skip all chars in the literal
		 * string
		 */
		while (*cp != '\0' && (cp[-1] != '%' || *cp != '}'))
		    cp++;
#endif				/* LITERAL == 0 */
		attributes |= LITERAL;
		break;
	    case '}':		/* literal characters end */
		attributes &= ~LITERAL;
		break;
	    default:
#ifndef HAVENOUTMP
		if (*cp == 'a' && what == FMT_WHO) {
		    cz = who_info(info, 'a');
		    tprintf_append_mbs(&buf, cz, attributes);
		    xfree(cz);
		}
		else
#endif /* HAVENOUTMP */
		{
		    Strbuf_append1(&buf, attributes | '%');
		    Strbuf_append1(&buf, attributes | *cp);
		}
		break;
	    }
	}
	else if (*cp == '\\' || *cp == '^')
	    Strbuf_append1(&buf, attributes | parseescape(&cp, TRUE));
	else if (*cp == HIST) {	/* EGS: handle '!'s in prompts */
	    if (what == FMT_HISTORY)
		cz = fmthist('h', info);
	    else
		cz = xasprintf("%d", eventno + 1);
	    tprintf_append_mbs(&buf, cz, attributes);
	    xfree(cz);
	}
	else
	    Strbuf_append1(&buf, attributes | *cp); /* normal character */
    }
    cleanup_ignore(&buf);
    cleanup_until(&buf);
    return Strbuf_finish(&buf);
}

int
expdollar(struct Strbuf *buf, const Char **srcp, Char attr)
{
    struct varent *vp;
    const Char *src = *srcp;
    Char *var, *val;
    size_t i;
    int curly = 0;

    /* found a variable, expand it */
    var = xmalloc((Strlen(src) + 1) * sizeof (*var));
    for (i = 0; ; i++) {
	var[i] = *++src & TRIM;
	if (i == 0 && var[i] == '{') {
	    curly = 1;
	    var[i] = *++src & TRIM;
	}
	if (!alnum(var[i]) && var[i] != '_') {

	    var[i] = '\0';
	    break;
	}
    }
    if (curly && (*src & TRIM) == '}')
	src++;

    vp = adrof(var);
    if (vp && vp->vec) {
	for (i = 0; vp->vec[i] != NULL; i++) {
	    for (val = vp->vec[i]; *val; val++)
		if (*val != '\n' && *val != '\r')
		    Strbuf_append1(buf, *val | attr);
	    if (vp->vec[i+1])
		Strbuf_append1(buf, ' ' | attr);
	}
    }
    else {
	val = (!vp) ? tgetenv(var) : NULL;
	if (val) {
	    for (; *val; val++)
		if (*val != '\n' && *val != '\r')
		    Strbuf_append1(buf, *val | attr);
	} else {
	    *srcp = src;
	    xfree(var);
	    return 0;
	}
    }

    *srcp = src;
    xfree(var);
    return 1;
}
