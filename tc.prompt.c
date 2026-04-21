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

/*
 * git_get_info - fill branch (up to branchsz-1 bytes) and op (up to opsz-1
 * bytes) for the git worktree that contains dir.  Returns 1 on success, 0 if
 * dir is not inside a git worktree.  Both buffers are always NUL-terminated.
 *
 * op is empty string when no special operation is in progress, or one of:
 * MERGING, REBASING, REBASING-i, REBASING-m, CHERRY-PICKING, REVERTING,
 * BISECTING.
 *
 * Detection is done by walking up the directory tree reading plain files; no
 * subprocesses are spawned.
 */
static int
git_get_info(const char *dir, char *branch, size_t branchsz,
	     char *op, size_t opsz)
{
    char path[MAXPATHLEN];
    char gitdir[MAXPATHLEN];
    char *p;
    FILE *fp;
    size_t n;
    int found = 0;

    if (!dir || !*dir)
	return 0;

    /* Walk up, looking for .git */
    n = strlen(dir);
    if (n >= sizeof(gitdir))
	n = sizeof(gitdir) - 1;
    memcpy(gitdir, dir, n + 1);

    for (;;) {
	/* Try .git/HEAD */
	if ((size_t)snprintf(path, sizeof(path), "%s/.git/HEAD", gitdir)
		< sizeof(path)) {
	    if (access(path, R_OK) == 0) {
		found = 1;
		break;
	    }
	}
	/* Try bare repo: HEAD directly */
	if ((size_t)snprintf(path, sizeof(path), "%s/HEAD", gitdir)
		< sizeof(path)) {
	    char cfg[MAXPATHLEN];
	    if ((size_t)snprintf(cfg, sizeof(cfg), "%s/config", gitdir)
		    < sizeof(cfg) && access(cfg, R_OK) == 0
		    && access(path, R_OK) == 0) {
		/* Check it looks like a bare repo HEAD */
		FILE *hf = fopen(path, "r");
		if (hf) {
		    char line[256];
		    if (fgets(line, sizeof(line), hf)) {
			if (strncmp(line, "ref: ", 5) == 0 ||
			    (strlen(line) >= 40 &&
			     strspn(line, "0123456789abcdef") >= 40)) {
			    fclose(hf);
			    /* Rewrite path to be used below without /.git prefix */
			    snprintf(gitdir, sizeof(gitdir), "%s", gitdir);
			    /* Adjust: bare repo uses gitdir itself as git dir */
			    found = 2;
			    break;
			}
		    }
		    fclose(hf);
		}
	    }
	}
	/* Go up one level */
	p = strrchr(gitdir, '/');
	if (!p || p == gitdir)
	    break;
	*p = '\0';
    }

    if (!found)
	return 0;

    /* Build the .git directory path */
    if (found == 1) {
	char tmp[MAXPATHLEN];
	snprintf(tmp, sizeof(tmp), "%s/.git", gitdir);
	memcpy(gitdir, tmp, sizeof(gitdir));
    }
    /* found == 2: gitdir already points at the bare repo dir */

    /* Read HEAD */
    snprintf(path, sizeof(path), "%s/HEAD", gitdir);
    fp = fopen(path, "r");
    if (!fp)
	return 0;
    branch[0] = '\0';
    if (fgets(path, sizeof(path), fp)) {
	/* Strip trailing newline */
	size_t len = strlen(path);
	if (len > 0 && path[len - 1] == '\n')
	    path[--len] = '\0';
	if (strncmp(path, "ref: refs/heads/", 16) == 0) {
	    strncpy(branch, path + 16, branchsz - 1);
	    branch[branchsz - 1] = '\0';
	} else if (strncmp(path, "ref: ", 5) == 0) {
	    strncpy(branch, path + 5, branchsz - 1);
	    branch[branchsz - 1] = '\0';
	} else if (len >= 7) {
	    /* Detached HEAD: show first 7 hex chars */
	    strncpy(branch, path, 7);
	    branch[7] = '\0';
	}
    }
    fclose(fp);

    if (!branch[0])
	return 0;

    /* Detect operation state */
    op[0] = '\0';
    {
	char probe[MAXPATHLEN];
	/* MERGE */
	snprintf(probe, sizeof(probe), "%s/MERGE_HEAD", gitdir);
	if (access(probe, F_OK) == 0) {
	    strncpy(op, "MERGING", opsz - 1);
	    op[opsz - 1] = '\0';
	    return 1;
	}
	/* REBASE (interactive) */
	snprintf(probe, sizeof(probe), "%s/rebase-merge", gitdir);
	if (access(probe, F_OK) == 0) {
	    char rbranch[256];
	    FILE *rf;
	    snprintf(probe, sizeof(probe), "%s/rebase-merge/head-name", gitdir);
	    rf = fopen(probe, "r");
	    if (rf) {
		if (fgets(rbranch, sizeof(rbranch), rf)) {
		    size_t rlen = strlen(rbranch);
		    if (rlen && rbranch[rlen-1] == '\n') rbranch[--rlen] = '\0';
		    if (strncmp(rbranch, "refs/heads/", 11) == 0)
			strncpy(branch, rbranch + 11, branchsz - 1);
		    else
			strncpy(branch, rbranch, branchsz - 1);
		    branch[branchsz - 1] = '\0';
		}
		fclose(rf);
	    }
	    strncpy(op, "REBASING-i", opsz - 1);
	    op[opsz - 1] = '\0';
	    return 1;
	}
	/* REBASE (am/apply) */
	snprintf(probe, sizeof(probe), "%s/rebase-apply", gitdir);
	if (access(probe, F_OK) == 0) {
	    snprintf(probe, sizeof(probe), "%s/rebase-apply/rebasing", gitdir);
	    if (access(probe, F_OK) == 0)
		strncpy(op, "REBASING", opsz - 1);
	    else
		strncpy(op, "AM", opsz - 1);
	    op[opsz - 1] = '\0';
	    return 1;
	}
	/* CHERRY-PICK */
	snprintf(probe, sizeof(probe), "%s/CHERRY_PICK_HEAD", gitdir);
	if (access(probe, F_OK) == 0) {
	    strncpy(op, "CHERRY-PICKING", opsz - 1);
	    op[opsz - 1] = '\0';
	    return 1;
	}
	/* REVERT */
	snprintf(probe, sizeof(probe), "%s/REVERT_HEAD", gitdir);
	if (access(probe, F_OK) == 0) {
	    strncpy(op, "REVERTING", opsz - 1);
	    op[opsz - 1] = '\0';
	    return 1;
	}
	/* BISECT */
	snprintf(probe, sizeof(probe), "%s/BISECT_LOG", gitdir);
	if (access(probe, F_OK) == 0) {
	    strncpy(op, "BISECTING", opsz - 1);
	    op[opsz - 1] = '\0';
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

		/* git info cache */
    static Char *git_oldcwd = NULL;
    static char git_branch[256];
    static char git_op[64];
    static int  git_valid = -1;

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
		    if (gcwd == STRNULL)
			break;
		    if (git_oldcwd != gcwd || git_valid < 0) {
			git_oldcwd = gcwd;
			git_valid = git_get_info(short2str(gcwd),
			    git_branch, sizeof(git_branch),
			    git_op, sizeof(git_op));
		    }
		    if (!git_valid)
			break;
		    {
			const char *s;
			for (s = git_branch; *s; s++)
			    Strbuf_append1(&buf, attributes | (unsigned char)*s);
			if (*cp == 'G' && git_op[0]) {
			    Strbuf_append1(&buf, attributes | '|');
			    for (s = git_op; *s; s++)
				Strbuf_append1(&buf, attributes | (unsigned char)*s);
			}
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
