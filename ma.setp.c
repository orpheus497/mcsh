/*
 * Copyright (c) 1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND CARNEGIE MELLON UNIVERSITY
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT
 * SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Users of this software agree to return to Carnegie Mellon any
 * improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 * Export of this software is permitted only after complying with the
 * regulations of the U.S. Deptartment of Commerce relating to the
 * Export of Technical Data.
 */
/*
 *  setpath --- smart interface for setting path variables
 *
 *  usage:	setpath(paths, cmds, localsyspath, dosuffix, printerrors)
 *		char **paths, **cmds, *localsyspath;
 *		int dosuffix, printerrors;
 *
 *  The 'paths' argument is a list of pointers to path lists of the
 *  form "name=value" where name is the name of the path and value
 *  is a colon separated list of directories.  There can never be
 *  more than MAXDIRS (64) directories in a path.
 *
 *  The 'cmds' argument may be a sequence of any of the following:
 *	-r			reset path to default
 *	-i newpath		insert newpath before localsyspath
 *	-ia oldpath newpath	insert newpath after oldpath
 *	-ib oldpath newpath	insert newpath before oldpath
 *	-i# newpath		insert newpath at position #
 *	-d oldpath		delete oldpath
 *	-d#			delete path at position #
 *	-c oldpath newpath	change oldpath to newpath
 *	-c# newpath		change position # to newpath
 *
 *  The "-i newpath" command is equivilent to "-ib 'localsyspath' newpath".
 *
 *  If 'dosuffix' is true, the appropriate suffix will be added to
 *  all command operands for any system path in 'paths'.
 *
 *  Both of the 'paths' and 'cmds' lists are terminated by a NULL
 *  entry.
 *
 *  if 'printerrors' is true, setpath will printf error diagnostics.
 *
 *  WARNING !!!: Under no circumstances should anyone change this
 *  module without fully understanding the impact on the C shell.
 *  The C shell has it's own malloc and printf routines and this
 *  module was carefully written taking that into account.  Do not
 *  use any stdio routines from this module except printf.
 *
 **********************************************************************
 * HISTORY
 *
 * Revision 1.4  90/12/11  17:58:44  mja
 * 	Add copyright/disclaimer for distribution.
 *
 * 05-Jun-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Make all non-entry points static.
 *
 * 30-Apr-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added -r switch to reset paths to their default values.
 *
 * 06-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created from old setpath program for the shell.
 *
 **********************************************************************
 */
#include "sh.h"

#ifdef MACH

#define MAXDIRS 64		/* max directories on a path */
#ifndef NULL
# define NULL 0
#endif

static int npaths;		/* # pathlist arguments */

static struct pelem {
    struct pelem *pnext;	/* pointer to next path */
    char *pname;		/* name of pathlist */
    char *psuf;			/* suffix for pathlist */
    char *pdef;			/* default for pathlist */
    int pdirs;			/* # directories on each pathlist */
    char *pdir[MAXDIRS];	/* directory names for each pathlist */
} *pathhead = NULL;

static struct {
    char *name;
    char *suffix;
    char *defalt;
} syspath[] = {
    "PATH",	"/bin",		":/usr/ucb:/bin:/usr/bin",
    "CPATH",	"/include",	":/usr/include",
    "LPATH",	"/lib",		":/lib:/usr/lib",
    "MPATH",	"/man",		":/usr/man",
    "EPATH",	"/maclib",	"",
    0, 0, 0
};

static int sflag;
static int eflag;

#define INVALID { \
	if (eflag) xprintf(CGETS(10, 1, \
				 "setpath: invalid command '%s'.\n"), cmd); \
	freepaths(); \
	return(-1); \
}

#define TOOFEW { \
	if (eflag) xprintf(CGETS(10, 2, \
		 "setpath: insufficient arguments to command '%s'.\n"), cmd); \
	freepaths(); \
	return(-1); \
}

static int initpaths	(char **);
static void savepaths	(char **);
static void freepaths	(void);
static int tcsh_rcmd	(char *);
static int icmd	(char *, char *);
static int iacmd	(char *, char *);
static int ibcmd	(char *, char *);
static int incmd	(char *, int);
static int insert	(struct pelem *, int, char *);
static void dcmd	(char *);
static void dncmd	(int);
static void delete	(struct pelem *, int);
static int ccmd	(char *, char *);
static int cncmd	(char *, int);
static int change	(struct pelem *, int, char *);
static int locate	(struct pelem *, char *);



int
setpath(char **paths, char **cmds, char *localsyspath, int dosuffix,
	int printerrors)
{
    char *cmd, *cmd1, *cmd2;
    int ncmd;

    sflag = dosuffix;
    eflag = printerrors;
    if (initpaths(paths) < 0)
	return(-1);
    if (npaths == 0)
	return(0);
    for (ncmd = 0; cmd = cmds[ncmd]; ncmd++) {
	if (cmd[0] != '-')
	    INVALID;
	cmd1 = cmds[ncmd+1];
	cmd2 = cmds[ncmd+2];
	switch (cmd[1]) {
	case 'r':
	    if (cmd[2] != '\0')
		INVALID;
	    if (tcsh_rcmd(localsyspath) < 0) { freepaths(); return -1; }
	    break;
	case 'i':
	    if (cmd[2] == '\0') {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		if (icmd(cmd1, localsyspath) < 0) { freepaths(); return -1; }
	    } else if (isdigit(cmd[2])) {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		if (incmd(cmd1, atoi(cmd+2)) < 0) { freepaths(); return -1; }
	    } else if (cmd[3] != '\0' || (cmd[2] != 'a' && cmd[2] != 'b')) {
		INVALID;
	    } else {
		ncmd += 2;
		if (cmd1 == NULL || cmd2 == NULL) TOOFEW;
		if (cmd[2] == 'a') {
		    if (iacmd(cmd1, cmd2) < 0) { freepaths(); return -1; }
		} else {
		    if (ibcmd(cmd1, cmd2) < 0) { freepaths(); return -1; }
		}
	    }
	    break;
	case 'd':
	    if (cmd[2] == '\0') {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		dcmd(cmd1);
	    } else if (isdigit(cmd[2]))
		dncmd(atoi(cmd+2));
	    else {
		INVALID;
	    }
	    break;
	case 'c':
	    if (cmd[2] == '\0') {
		ncmd += 2;
		if (cmd1 == NULL || cmd2 == NULL) TOOFEW;
		if (ccmd(cmd1, cmd2) < 0) { freepaths(); return -1; }
	    } else if (isdigit(cmd[2])) {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		if (cncmd(cmd1, atoi(cmd+2)) < 0) { freepaths(); return -1; }
	    } else {
		INVALID;
	    }
	    break;
	default:
	    INVALID;
	}
    }
    savepaths(paths);
    freepaths();
    return(0);
}

static int
initpaths(char **paths)
{
    char *path, *val, *p, *q;
    int i, done;
    struct pelem *pe, *pathend;

    freepaths();
    for (npaths = 0; path = paths[npaths]; npaths++) {
	val = index(path, '=');
	if (val == NULL) {
	    if (eflag)
		xprintf(CGETS(10, 3,
			      "setpath: value missing in path '%s'\n"), path);
	    freepaths();
	    return(-1);
	}
	*val++ = '\0';
	pe = xmalloc(sizeof(struct pelem));
	setzero(pe, sizeof(struct pelem));
	if (pathhead == NULL)
	    pathhead = pathend = pe;
	else {
	    pathend->pnext = pe;
	    pathend = pe;
	}
	p = strsave(path);
	pe->pname = p;
	pe->psuf = "";
	pe->pdef = "";
	for (i = 0; syspath[i].name; i++)
	    if (strcmp(pe->pname, syspath[i].name) == 0) {
		pe->psuf = syspath[i].suffix;
		pe->pdef = syspath[i].defalt;
		break;
	    }
	q = val;
	for (;;) {
	    q = index(p = q, ':');
	    done = (q == NULL);
	    if (!done)
		*q++ = '\0';
	    p = strsave(p);
	    pe->pdir[pe->pdirs] = p;
	    pe->pdirs++;
	    if (done)
		break;
	}
    }
    return(0);
}

static void
savepaths(char **paths)
{
    char *p, *q;
    int npath, i, len;
    struct pelem *pe;

    for (npath = 0, pe = pathhead; pe; npath++, pe = pe->pnext) {
	len = strlen(pe->pname) + 1;
	if (pe->pdirs == 0)
	    len++;
	else for (i = 0; i < pe->pdirs; i++)
	    len += strlen(pe->pdir[i]) + 1;
	p = xmalloc((unsigned)len);
	paths[npath] = p;
	for (q = pe->pname; *p = *q; p++, q++);
	*p++ = '=';
	if (pe->pdirs != 0) {
	    for (i = 0; i < pe->pdirs; i++) {
		for (q = pe->pdir[i]; *p = *q; p++, q++);
		*p++ = ':';
	    }
	    p--;
	}
	*p = '\0';
    }
}

static void
freepaths(void)
{
    char *p;
    int i;
    struct pelem *pe;

    if (npaths == 0 || pathhead == NULL)
	return;
    while (pe = pathhead) {
	if (pe->pname) {
	    for (i = 0; i < pe->pdirs; i++) {
		if (pe->pdir[i] == NULL)
		    continue;
		p = pe->pdir[i];
		pe->pdir[i] = NULL;
		xfree((ptr_t) p);
	    }
	    pe->pdirs = 0;
	    p = pe->pname;
	    pe->pname = NULL;
	    xfree((ptr_t) p);
	}
	pathhead = pe->pnext;
	xfree((ptr_t) pe);
    }
    npaths = 0;
}

/***********************************************
 ***    R E S E T   A   P A T H N A M E    ***
 ***********************************************/

static int
tcsh_rcmd(char *localsyspath)	/* reset path with localsyspath */
{
    int n, done;
    char *new, *p;
    struct pelem *pe;
    char newbuf[MAXPATHLEN+1];/*FIXBUF*/

    for (pe = pathhead; pe; pe = pe->pnext) {
	new = newbuf;
	if (localsyspath != NULL) {
	    if (xsnprintf(new, sizeof(newbuf), ":%s%s%s", localsyspath, pe->psuf, pe->pdef) < 0) return -1;
	} else {
	    if (xsnprintf(new, sizeof(newbuf), "%s", pe->pdef) < 0) return -1;
	}
	for (n = 0; n < pe->pdirs; n++) {
	    if (pe->pdir[n] == NULL)
		continue;
	    p = pe->pdir[n];
	    pe->pdir[n] = NULL;
	    xfree((ptr_t) p);
	}
	pe->pdirs = 0;
	for (;;) {
	    new = index(p = new, ':');
	    done = (new == NULL);
	    if (!done)
		*new++ = '\0';
	    p = strsave(p);
	    pe->pdir[pe->pdirs] = p;
	    pe->pdirs++;
	    if (done)
		break;
	}
    }
    return 0;
}

/***********************************************
 ***    I N S E R T   A   P A T H N A M E    ***
 ***********************************************/

static int
icmd(char *path, char *localsyspath)	/* insert path before localsyspath */
{
    int n;
    char *new;
    struct pelem *pe;
    char newbuf[MAXPATHLEN+1];/*FIXBUF*/

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (sflag)
	    new = localsyspath;
	else {
	    new = newbuf;
	    if (xsnprintf(new, sizeof(newbuf), "%s%s", localsyspath ? localsyspath : "", pe->psuf) < 0) return -1;
	}
	n = locate(pe, new);
	if (n >= 0) {
	    if (insert(pe, n, path) < 0) return -1;
	} else {
	    if (insert(pe, 0, path) < 0) return -1;
	}
    }
    return 0;
}

static int
iacmd(char *inpath, char *path)	/* insert path after inpath */
{
    int n;
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, inpath);
	if (n >= 0) {
	    if (insert(pe, n + 1, path) < 0) return -1;
	} else
	    xprintf(CGETS(10, 4, "setpath: %s not found in %s\n"),
		    inpath, pe->pname);
    }
    return 0;
}

static int
ibcmd(char *inpath, char *path)	/* insert path before inpath */
{
    int n;
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, inpath);
	if (n >= 0) {
	    if (insert(pe, n, path) < 0) return -1;
	} else
	    xprintf(CGETS(10, 4, "setpath: %s not found in %s\n"),
		    inpath, pe->pname);
    }
    return 0;
}

static int
incmd(char *path, int n)	/* insert path at position n */
{
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (insert(pe, n, path) < 0) return -1;
    }
    return 0;
}

static int
insert(struct pelem *pe, int loc, char *key)
{
    int i;
    char *new;
    char newbuf[2000];/*FIXBUF*/

    if (sflag) {		/* add suffix */
	new = newbuf;
	if (xsnprintf(new, sizeof(newbuf), "%s%s", key ? key : "", pe->psuf) < 0) return -1;
    } else
	new = key;
    new = strsave(new);
    for (i = pe->pdirs; i > loc; --i)
	pe->pdir[i] = pe->pdir[i-1];
    if (loc > pe->pdirs)
	loc = pe->pdirs;
    pe->pdir[loc] = new;
    pe->pdirs++;
    return 0;
}

/***********************************************
 ***    D E L E T E   A   P A T H N A M E    ***
 ***********************************************/

static void
dcmd(char *path)		/* delete path */
{
    int n;
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, path);
	if (n >= 0)
	    delete(pe, n);
	else
	    xprintf(CGETS(10, 4, "setpath: %s not found in %s\n"),
		    path, pe->pname);
    }
}

static void
dncmd(int n)			/* delete at position n */
{
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (n < pe->pdirs)
	    delete(pe, n);
	else
	    xprintf(CGETS(10, 5,
			    "setpath: %d not valid position in %s\n"),
		    n, pe->pname);
    }
}

static void
delete(struct pelem *pe, int n)
{
    int d;

    xfree((ptr_t) (pe->pdir[n]));
    for (d = n; d < pe->pdirs - 1; d++)
	pe->pdir[d] = pe->pdir[d+1];
    --pe->pdirs;
}

/***********************************************
 ***    C H A N G E   A   P A T H N A M E    ***
 ***********************************************/

static int
ccmd(char *inpath, char *path)	/* change inpath to path */
{
    int n;
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, inpath);
	if (n >= 0) {
	    if (change(pe, n, path) < 0) return -1;
	} else
	    xprintf(CGETS(10, 4, "setpath: %s not found in %s\n"),
		    inpath, pe->pname);
    }
    return 0;
}

static int
cncmd(char *path, int n)	/* change at position n to path */
{
    struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (n < pe->pdirs) {
	    if (change(pe, n, path) < 0) return -1;
	} else
	    xprintf(CGETS(10, 5,
			    "setpath: %d not valid position in %s\n"),
		    n, pe->pname);
    }
    return 0;
}

static int
change(struct pelem *pe, int loc, char *key)
{
    char *new;
    char newbuf[MAXPATHLEN+1];/*FIXBUF*/

    if (sflag) {		/* append suffix */
	new = newbuf;
	if (xsnprintf(new, sizeof(newbuf), "%s%s", key ? key : "", pe->psuf) < 0) return -1;
    } else
	new = key;
    new = strsave(new);
    xfree((ptr_t) (pe->pdir[loc]));
    pe->pdir[loc] = new;
    return 0;
}

/***************************************
 ***    F I N D   P A T H N A M E    ***
 ***************************************/

static int
locate(struct pelem *pe, char *key)
{
    int i;
    char *realkey;
    char keybuf[MAXPATHLEN+1];/*FIXBUF*/

    if (sflag) {
	realkey = keybuf;
	if (xsnprintf(realkey, sizeof(keybuf), "%s%s", key ? key : "", pe->psuf) < 0) return -1;
    } else
	realkey = key;
    for (i = 0; i < pe->pdirs; i++)
	if (strcmp(pe->pdir[i], realkey) == 0)
	    break;
    return((i < pe->pdirs) ? i : -1);
}
#endif
