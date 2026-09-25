/*
 * ptydrive.c - drive an interactive program through a pseudo-terminal and
 * dump every byte it writes.
 *
 * The line editor's display code only runs when the shell's input is a
 * terminal, so the ghost-text renderer, the syntax highlighter and the prompt
 * escapes cannot be exercised by a plain `mcsh -c'.  This helper supplies the
 * terminal.  It is built on demand by the tests that need it, and uses only
 * interfaces specified by POSIX.1-2001: posix_openpt(), grantpt(), unlockpt(),
 * ptsname(), setsid(), tcsetattr() and select().  TIOCSWINSZ and TIOCSCTTY are
 * not in POSIX; both are guarded.
 *
 *	ptydrive [-c cols] [-r rows] [-d ms] [-s ms] prog [args ...]
 *
 * Keystrokes are read from standard input and written to the program one byte
 * at a time, -d milliseconds apart, so that the program sees them as separate
 * key presses rather than one paste.  Everything the program writes is copied
 * to standard output verbatim.  After the input is exhausted the driver waits
 * -s milliseconds, sends ^C and ^D, and exits when the program does.
 */
#define _XOPEN_SOURCE 600
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#ifdef __linux__
# include <sys/ioctl.h>
#endif
#if defined(TIOCSWINSZ) || defined(TIOCSCTTY)
# include <sys/ioctl.h>
#endif

static int
pump(int fd, long ms)
{
    struct timeval tv;
    fd_set rf;
    char buf[8192];
    ssize_t n;
    long left = ms;

    while (left > 0) {
	FD_ZERO(&rf);
	FD_SET(fd, &rf);
	tv.tv_sec = 0;
	tv.tv_usec = 20000;	/* 20 ms slices */
	if (select(fd + 1, &rf, NULL, NULL, &tv) > 0) {
	    n = read(fd, buf, sizeof(buf));
	    if (n <= 0)
		return -1;	/* the child closed the terminal */
	    if (fwrite(buf, 1, (size_t) n, stdout) != (size_t) n)
		return -1;
	}
	left -= 20;
    }
    return 0;
}

int
main(int argc, char **argv)
{
    int master;
    int cols = 80, rows = 24;
    long delay = 60, settle = 400;
    char *slavename;
    char *keys = NULL;
    size_t nkeys = 0, cap = 0;
    pid_t pid;
    size_t i;
    int ai = 1;

    /*
     * Options are parsed by hand rather than with getopt(3): POSIX getopt
     * stops at the first operand, GNU getopt permutes past it unless it is
     * asked not to, and the operand here is a command with options of its own
     * ("mcsh -f -i").  Four flags do not justify depending on which getopt
     * the host has.
     */
    while (ai < argc && argv[ai][0] == '-' && argv[ai][1] != '\0' &&
	   argv[ai][2] == '\0' && strchr("crds", argv[ai][1]) != NULL) {
	if (ai + 1 >= argc) {
	    fprintf(stderr, "ptydrive: -%c needs a value\n", argv[ai][1]);
	    return 2;
	}
	switch (argv[ai][1]) {
	case 'c': cols = atoi(argv[ai + 1]); break;
	case 'r': rows = atoi(argv[ai + 1]); break;
	case 'd': delay = atol(argv[ai + 1]); break;
	case 's': settle = atol(argv[ai + 1]); break;
	}
	ai += 2;
    }
    if (ai >= argc) {
	fprintf(stderr, "usage: ptydrive [-c cols] [-r rows] [-d ms] "
			"[-s ms] prog [args ...]\n");
	return 2;
    }

    /* Slurp the keystrokes before forking: the child must not inherit a
     * half-read standard input. */
    for (;;) {
	ssize_t n;

	if (nkeys == cap) {
	    char *p;

	    cap = cap ? cap * 2 : 4096;
	    p = realloc(keys, cap);
	    if (p == NULL) {
		free(keys);
		fprintf(stderr, "ptydrive: out of memory\n");
		return 2;
	    }
	    keys = p;
	}
	n = read(STDIN_FILENO, keys + nkeys, cap - nkeys);
	if (n < 0) {
	    if (errno == EINTR)
		continue;
	    break;
	}
	if (n == 0)
	    break;
	nkeys += (size_t) n;
    }

    master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0 || grantpt(master) < 0 || unlockpt(master) < 0 ||
	(slavename = ptsname(master)) == NULL) {
	fprintf(stderr, "ptydrive: cannot allocate a pseudo-terminal: %s\n",
		strerror(errno));
	free(keys);
	return 77;		/* the suite's "skip" status */
    }

#ifdef TIOCSWINSZ
    {
	struct winsize ws;

	memset(&ws, 0, sizeof(ws));
	ws.ws_row = (unsigned short) rows;
	ws.ws_col = (unsigned short) cols;
	(void) ioctl(master, TIOCSWINSZ, &ws);
    }
#endif

    pid = fork();
    if (pid < 0) {
	fprintf(stderr, "ptydrive: fork: %s\n", strerror(errno));
	free(keys);
	return 2;
    }
    if (pid == 0) {
	int slave;

	(void) close(master);
	if (setsid() < 0)
	    _exit(127);
	slave = open(slavename, O_RDWR);
	if (slave < 0)
	    _exit(127);
#ifdef TIOCSCTTY
	(void) ioctl(slave, TIOCSCTTY, 0);
#endif
	if (dup2(slave, STDIN_FILENO) < 0 || dup2(slave, STDOUT_FILENO) < 0 ||
	    dup2(slave, STDERR_FILENO) < 0)
	    _exit(127);
	if (slave > STDERR_FILENO)
	    (void) close(slave);
	execvp(argv[ai], &argv[ai]);
	_exit(127);
    }

    (void) pump(master, 800);	/* let the shell start up and prompt */

    for (i = 0; i < nkeys; i++) {
	if (write(master, keys + i, 1) != 1)
	    break;
	if (pump(master, delay) < 0)
	    break;
    }
    free(keys);

    (void) pump(master, settle);
    (void) write(master, "\003", 1);	/* ^C: abandon any partial line */
    (void) pump(master, 200);
    (void) write(master, "\004", 1);	/* ^D: end of input */
    (void) pump(master, 400);

    (void) close(master);
    (void) waitpid(pid, NULL, 0);
    (void) fflush(stdout);
    return 0;
}
