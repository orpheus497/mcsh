/*
 * ed.syntax.c: Interactive syntax highlighting for mcsh.
 *
 * syntax_colorize() rescans InputBuf on every buffer mutation (when
 * `set syntax` is active) and populates SyntaxColor[], a parallel byte
 * array that the virtual-display pipeline consults at render time.
 *
 * Design constraints:
 *  - No allocation, no stderror(), no shell state mutation.
 *  - O(n) in line length; safe to call on every keystroke.
 *  - Correct for csh/tcsh syntax: quoting, variable expansion, operators,
 *    keywords, builtins, and $PATH command lookup with a tiny LRU cache.
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
#include "ed.syntax.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Public state                                                         */
/* ------------------------------------------------------------------ */

uint8_t SyntaxColor[INBUFSIZE];

/*
 * Default colour palette.  fg values are raw ANSI codes (30-37 standard,
 * 90-97 bright).  0 means "use terminal default".
 */
SynColor SynPalette[SYN__MAX] = {
    /* SYN_NORMAL   */ { 0,  0 },
    /* SYN_KEYWORD  */ { 36, 1 },   /* bold cyan    */
    /* SYN_BUILTIN  */ { 32, 1 },   /* bold green   */
    /* SYN_CMD_OK   */ { 32, 0 },   /* green        */
    /* SYN_CMD_BAD  */ { 31, 1 },   /* bold red     */
    /* SYN_OPERATOR */ { 33, 0 },   /* yellow       */
    /* SYN_VARIABLE */ { 35, 0 },   /* magenta      */
    /* SYN_DQUOTE   */ { 33, 0 },   /* yellow       */
    /* SYN_SQUOTE   */ { 33, 0 },   /* yellow       */
    /* SYN_BACKTICK */ { 36, 0 },   /* cyan         */
    /* SYN_COMMENT  */ { 90, 0 },   /* bright black / dim gray */
    /* SYN_ERROR    */ { 31, 1 },   /* bold red     */
};

/* ------------------------------------------------------------------ */
/* csh/tcsh keyword and builtin tables                                  */
/* ------------------------------------------------------------------ */

static const char * const keywords[] = {
    "if", "else", "endif", "then",
    "while", "end",
    "foreach", "in",
    "switch", "case", "default", "breaksw", "endsw",
    "repeat",
    "break", "continue",
    "goto",
    "exit",
    "return",
    NULL
};

static const char * const builtins[] = {
    "alias", "unalias",
    "bg", "fg", "jobs", "stop", "notify",
    "cd", "chdir", "pushd", "popd", "dirs",
    "echo", "printf",
    "eval",
    "exec",
    "exit",
    "function",
    "glob",
    "hashstat", "rehash", "unhash",
    "history", "hup",
    "kill",
    "limit", "unlimit",
    "login", "logout",
    "ls-F",
    "nice",
    "nohup",
    "onintr",
    "printenv",
    "pwd",
    "read",
    "sched",
    "set", "unset", "setenv", "unsetenv",
    "settc", "setty",
    "shift",
    "source",
    "suspend",
    "time",
    "umask",
    "wait",
    "which",
    NULL
};

/* ------------------------------------------------------------------ */
/* $PATH command-existence LRU cache                                    */
/* ------------------------------------------------------------------ */

#define CMD_CACHE_SIZE   64
#define CMD_CACHE_NAMELEN 64

typedef struct {
    char     name[CMD_CACHE_NAMELEN];
    int      found;   /* 1 = on path, 0 = not found, -1 = empty/unused */
    unsigned age;     /* logical clock: higher = more recently used */
} CmdCacheEntry;

static CmdCacheEntry cmd_cache[CMD_CACHE_SIZE];
static int      cmd_cache_init = 0;
static unsigned cmd_cache_clock = 0; /* monotone tick, wraps harmlessly */

static void
cache_init(void)
{
    int i;
    for (i = 0; i < CMD_CACHE_SIZE; i++) {
	cmd_cache[i].found = -1;
	cmd_cache[i].age   = 0;
    }
    cmd_cache_clock = 0;
    cmd_cache_init  = 1;
}

void
syntax_cache_clear(void)
{
    cache_init();
}

static int
cache_lookup(const char *name)
{
    int i;
    if (!cmd_cache_init) cache_init();
    for (i = 0; i < CMD_CACHE_SIZE; i++) {
	if (cmd_cache[i].found >= 0 &&
	    strncmp(cmd_cache[i].name, name, CMD_CACHE_NAMELEN - 1) == 0) {
	    /* LRU: refresh age on hit */
	    cmd_cache[i].age = ++cmd_cache_clock;
	    return cmd_cache[i].found;
	}
    }
    return -1;
}

static void
cache_store(const char *name, int found)
{
    int i, victim = -1;
    unsigned oldest_age;
    if (!cmd_cache_init) cache_init();

    /* First pass: prefer an empty slot. */
    for (i = 0; i < CMD_CACHE_SIZE; i++) {
	if (cmd_cache[i].found < 0) {
	    victim = i;
	    break;
	}
    }

    /* Second pass: if no empty slot, evict the least-recently-used entry. */
    if (victim < 0) {
	oldest_age = cmd_cache[0].age;
	victim = 0;
	for (i = 1; i < CMD_CACHE_SIZE; i++) {
	    if (cmd_cache[i].age < oldest_age) {
		oldest_age = cmd_cache[i].age;
		victim = i;
	    }
	}
    }

    strncpy(cmd_cache[victim].name, name, CMD_CACHE_NAMELEN - 1);
    cmd_cache[victim].name[CMD_CACHE_NAMELEN - 1] = '\0';
    cmd_cache[victim].found = found;
    cmd_cache[victim].age   = ++cmd_cache_clock;
}

/*
 * Look up whether `word' (plain ASCII, no quoting) is executable on $PATH.
 * Returns 1 if found, 0 if not.  Uses the cache to avoid repeated stat(2).
 * Absolute/relative paths are checked directly.
 */
static int
cmd_on_path(const char *word)
{
    char path[1024];
    const char *pathenv;
    const char *p, *q;
    size_t dlen, wlen;
    struct stat st;
    int cached;

    if (!word || !word[0])
	return 0;

    cached = cache_lookup(word);
    if (cached >= 0)
	return cached;

    /* absolute or relative path: check directly */
    if (word[0] == '/' || word[0] == '.') {
	int ok = (stat(word, &st) == 0 &&
		  S_ISREG(st.st_mode) && access(word, X_OK) == 0);
	cache_store(word, ok);
	return ok;
    }

    pathenv = getenv("PATH");
    if (!pathenv) {
	cache_store(word, 0);
	return 0;
    }

    wlen = strlen(word);
    p = pathenv;
    while (p && *p) {
	q = strchr(p, ':');
	dlen = q ? (size_t)(q - p) : strlen(p);
	if (dlen + 1 + wlen + 1 < sizeof(path)) {
	    if (dlen == 0) {
		/* empty component = current dir */
		snprintf(path, sizeof(path), "./%s", word);
	    } else {
		memcpy(path, p, dlen);
		path[dlen] = '/';
		memcpy(path + dlen + 1, word, wlen);
		path[dlen + 1 + wlen] = '\0';
	    }
	    if (stat(path, &st) == 0 && S_ISREG(st.st_mode) &&
		access(path, X_OK) == 0) {
		cache_store(word, 1);
		return 1;
	    }
	}
	p = q ? q + 1 : NULL;
    }
    cache_store(word, 0);
    return 0;
}

/* ------------------------------------------------------------------ */
/* String table helpers                                                 */
/* ------------------------------------------------------------------ */

static int
in_table(const char * const *table, const char *word, size_t len)
{
    size_t i;
    for (; *table; table++) {
	size_t tl = strlen(*table);
	if (tl == len && strncmp(*table, word, len) == 0)
	    return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Tokenizer state                                                      */
/* ------------------------------------------------------------------ */

typedef enum {
    ST_NORMAL = 0,
    ST_DQUOTE,
    ST_SQUOTE,
    ST_BACKTICK,
    ST_COMMENT,
    ST_VARIABLE,
    ST_BRACE_VAR   /* ${…} */
} TokState;

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void
syntax_clear(void)
{
    memset(SyntaxColor, SYN_NORMAL, sizeof(SyntaxColor));
}

/*
 * syntax_colorize — rescan InputBuf and populate SyntaxColor[].
 *
 * The scanner is a single-pass state machine.  It tracks:
 *   - current quoting state (ST_NORMAL / ST_DQUOTE / ST_SQUOTE / …)
 *   - whether we are at the start of a command word (at_cmd = 1)
 *   - word boundaries so we can classify the first word per pipeline
 *     segment as keyword / builtin / cmd-ok / cmd-bad
 *
 * We emit colour for each character individually so that partial words
 * during typing get coloured correctly.
 */
void
syntax_colorize(void)
{
    const Char *buf = InputBuf;
    const Char *end = LastChar;
    ptrdiff_t len = end - buf;
    ptrdiff_t i;
    TokState state = ST_NORMAL;
    int at_cmd = 1;        /* next non-space word is the command */
    int in_word = 0;       /* currently inside a word */
    ptrdiff_t word_start = 0;
    int brace_depth = 0;   /* for ${…} */
    char wordbuf[256];

    if (len <= 0) {
	syntax_clear();
	return;
    }

    /* Zero out the region we'll colour */
    memset(SyntaxColor, SYN_NORMAL, (size_t)len);

    /* ---- pass: classify every character ---- */
    for (i = 0; i < len; i++) {
	Char raw = buf[i];
	int  ch  = (int)(raw & CHAR);

	switch (state) {

	/* -------------------------------------------------- */
	case ST_COMMENT:
	    SyntaxColor[i] = SYN_COMMENT;
	    continue;

	/* -------------------------------------------------- */
	case ST_SQUOTE:
	    SyntaxColor[i] = SYN_SQUOTE;
	    if (ch == '\'') {
		state = ST_NORMAL;
		in_word = 0;
	    }
	    continue;

	/* -------------------------------------------------- */
	case ST_DQUOTE:
	    SyntaxColor[i] = SYN_DQUOTE;
	    if (ch == '\\' && i + 1 < len) {
		i++;
		SyntaxColor[i] = SYN_DQUOTE;
	    } else if (ch == '"') {
		state = ST_NORMAL;
		in_word = 0;
	    } else if (ch == '$') {
		/* variable inside dquote: colour it magenta */
		SyntaxColor[i] = SYN_VARIABLE;
	    }
	    continue;

	/* -------------------------------------------------- */
	case ST_BACKTICK:
	    SyntaxColor[i] = SYN_BACKTICK;
	    if (ch == '`') {
		state = ST_NORMAL;
		in_word = 0;
	    }
	    continue;

	/* -------------------------------------------------- */
	case ST_VARIABLE:
	    if (ch == '{') {
		SyntaxColor[i] = SYN_VARIABLE;
		state = ST_BRACE_VAR;
		brace_depth = 1;
	    } else if ((ch >= 'a' && ch <= 'z') ||
		       (ch >= 'A' && ch <= 'Z') ||
		       (ch >= '0' && ch <= '9') ||
		       ch == '_' || ch == '?' || ch == '#' ||
		       ch == '$' || ch == '!' || ch == '<') {
		SyntaxColor[i] = SYN_VARIABLE;
		/* '?' and '#' are single-char special-variable prefixes;
		 * keep state as ST_VARIABLE so the following alphanumeric
		 * characters remain part of the variable name (e.g. $?path). */
		if (ch == '?' || ch == '#') {
		    /* stay in ST_VARIABLE to absorb trailing name chars */
		} else if (!((buf[i] & CHAR) >= 'a' && (buf[i] & CHAR) <= 'z') &&
		    !((buf[i] & CHAR) >= 'A' && (buf[i] & CHAR) <= 'Z') &&
		    !((buf[i] & CHAR) >= '0' && (buf[i] & CHAR) <= '9') &&
		    (buf[i] & CHAR) != '_')
		    state = ST_NORMAL;
	    } else {
		state = ST_NORMAL;
		/* reprocess this char in normal mode */
		i--;
	    }
	    continue;

	/* -------------------------------------------------- */
	case ST_BRACE_VAR:
	    SyntaxColor[i] = SYN_VARIABLE;
	    if (ch == '{') brace_depth++;
	    else if (ch == '}') {
		brace_depth--;
		if (brace_depth == 0)
		    state = ST_NORMAL;
	    }
	    continue;

	/* -------------------------------------------------- */
	case ST_NORMAL:
	    break;
	}

	/* ---- ST_NORMAL processing ---- */

	/* Escape: next char is literal */
	if (ch == '\\' && i + 1 < len) {
	    SyntaxColor[i] = SYN_NORMAL;
	    i++;
	    SyntaxColor[i] = SYN_NORMAL;
	    continue;
	}

	/* Comment */
	if (ch == '#' && !in_word) {
	    state = ST_COMMENT;
	    SyntaxColor[i] = SYN_COMMENT;
	    continue;
	}

	/* Quote starts */
	if (ch == '\'') {
	    /* flush any open word */
	    if (in_word) {
		in_word = 0;
	    }
	    state = ST_SQUOTE;
	    SyntaxColor[i] = SYN_SQUOTE;
	    continue;
	}
	if (ch == '"') {
	    if (in_word) in_word = 0;
	    state = ST_DQUOTE;
	    SyntaxColor[i] = SYN_DQUOTE;
	    continue;
	}
	if (ch == '`') {
	    if (in_word) in_word = 0;
	    state = ST_BACKTICK;
	    SyntaxColor[i] = SYN_BACKTICK;
	    continue;
	}

	/* Variable expansion */
	if (ch == '$') {
	    state = ST_VARIABLE;
	    SyntaxColor[i] = SYN_VARIABLE;
	    continue;
	}

	/* Operators / word separators */
	if (ch == '|' || ch == ';' || ch == '&' || ch == '(' ||
	    ch == ')' || ch == '\n') {
	    if (in_word) {
		/* classify the word we just closed */
		size_t wlen = (size_t)(i - word_start);
		if (wlen < sizeof(wordbuf) - 1) {
		    size_t wi;
		    for (wi = 0; wi < wlen; wi++)
			wordbuf[wi] = (char)(buf[word_start + wi] & CHAR);
		    wordbuf[wlen] = '\0';
		    SynToken tok;
		    if (!at_cmd)
			tok = SYN_NORMAL;
		    else if (in_table(keywords, wordbuf, wlen))
			tok = SYN_KEYWORD;
		    else if (in_table(builtins, wordbuf, wlen))
			tok = SYN_BUILTIN;
		    else if (cmd_on_path(wordbuf))
			tok = SYN_CMD_OK;
		    else
			tok = SYN_CMD_BAD;
		    if (at_cmd) {
			ptrdiff_t wi2;
			for (wi2 = word_start; wi2 < i; wi2++)
			    SyntaxColor[wi2] = (uint8_t)tok;
		    }
		}
		in_word = 0;
	    }

	    /* double-char operators */
	    if (ch == '|' && i + 1 < len && (buf[i+1] & CHAR) == '|') {
		SyntaxColor[i] = SYN_OPERATOR;
		SyntaxColor[++i] = SYN_OPERATOR;
	    } else if (ch == '&' && i + 1 < len && (buf[i+1] & CHAR) == '&') {
		SyntaxColor[i] = SYN_OPERATOR;
		SyntaxColor[++i] = SYN_OPERATOR;
	    } else {
		SyntaxColor[i] = SYN_OPERATOR;
	    }

	    at_cmd = (ch != ')');
	    continue;
	}

	/* Redirection */
	if (ch == '>' || ch == '<') {
	    if (in_word) in_word = 0;
	    SyntaxColor[i] = SYN_OPERATOR;
	    /* >>  >&  >>&  etc. */
	    while (i + 1 < len) {
		int nc = (int)(buf[i+1] & CHAR);
		if (nc == '>' || nc == '&' || nc == '-')
		    SyntaxColor[++i] = SYN_OPERATOR;
		else
		    break;
	    }
	    at_cmd = 0; /* after redirection, next word is not a command */
	    continue;
	}

	/* Whitespace — word boundary */
	if (ch == ' ' || ch == '\t') {
	    if (in_word) {
		/* classify the word we just finished */
		size_t wlen = (size_t)(i - word_start);
		if (wlen < sizeof(wordbuf) - 1) {
		    size_t wi;
		    for (wi = 0; wi < wlen; wi++)
			wordbuf[wi] = (char)(buf[word_start + wi] & CHAR);
		    wordbuf[wlen] = '\0';
		    if (at_cmd) {
			SynToken tok;
			if (in_table(keywords, wordbuf, wlen))
			    tok = SYN_KEYWORD;
			else if (in_table(builtins, wordbuf, wlen))
			    tok = SYN_BUILTIN;
			else if (cmd_on_path(wordbuf))
			    tok = SYN_CMD_OK;
			else
			    tok = SYN_CMD_BAD;
			ptrdiff_t wi2;
			for (wi2 = word_start; wi2 < i; wi2++)
			    SyntaxColor[wi2] = (uint8_t)tok;
			at_cmd = 0;
		    }
		}
		in_word = 0;
	    }
	    SyntaxColor[i] = SYN_NORMAL;
	    continue;
	}

	/* Regular character — accumulate word */
	if (!in_word) {
	    in_word = 1;
	    word_start = i;
	}
	/* Leave SyntaxColor[i] = SYN_NORMAL for now; we'll back-fill
	 * when we detect the word boundary above. */
    }

    /* Flush any open word at end of buffer */
    if (in_word && state == ST_NORMAL) {
	size_t wlen = (size_t)(len - word_start);
	if (wlen < sizeof(wordbuf) - 1) {
	    size_t wi;
	    for (wi = 0; wi < wlen; wi++)
		wordbuf[wi] = (char)(buf[word_start + wi] & CHAR);
	    wordbuf[wlen] = '\0';
	    if (at_cmd) {
		SynToken tok;
		if (in_table(keywords, wordbuf, wlen))
		    tok = SYN_KEYWORD;
		else if (in_table(builtins, wordbuf, wlen))
		    tok = SYN_BUILTIN;
		else if (cmd_on_path(wordbuf))
		    tok = SYN_CMD_OK;
		else
		    tok = SYN_CMD_BAD;
		ptrdiff_t wi2;
		for (wi2 = word_start; wi2 < len; wi2++)
		    SyntaxColor[wi2] = (uint8_t)tok;
	    }
	}
    }

    /* Mark unterminated quotes as errors */
    if (state == ST_SQUOTE || state == ST_DQUOTE ||
	state == ST_BACKTICK || state == ST_BRACE_VAR) {
	for (i = 0; i < len; i++) {
	    if ((SynToken)SyntaxColor[i] == SYN_SQUOTE ||
		(SynToken)SyntaxColor[i] == SYN_DQUOTE ||
		(SynToken)SyntaxColor[i] == SYN_BACKTICK ||
		(SynToken)SyntaxColor[i] == SYN_VARIABLE)
		SyntaxColor[i] = SYN_ERROR;
	}
    }
}
