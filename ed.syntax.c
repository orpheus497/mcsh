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
#include <pwd.h>

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
    /* SYN_ALIAS    */ { 34, 1 },   /* bold blue    */
    /* SYN_FUNCTION */ { 35, 1 },   /* bold magenta */
    /* SYN_OPTION   */ { 36, 0 },   /* cyan         */
    /* SYN_PATH     */ { 34, 0 },   /* blue         */
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

    /* Anything carrying a '/' names a file directly - absolute, "./x", or a
     * plain relative path like "build/tool".  Only bare names are searched
     * along $PATH; previously "build/tool" was appended to each $PATH entry
     * and so never resolved. */
    if (word[0] == '/' || word[0] == '.' || strchr(word, '/') != NULL) {
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
    for (; *table; table++) {
	size_t tl = strlen(*table);
	if (tl == len && strncmp(*table, word, len) == 0)
	    return 1;
    }
    return 0;
}

/*
 * kw_takes_expr - keywords followed by "( ... )" that is an expression or a
 * word list rather than a command list: `if (x == 1)', `while (1)',
 * `foreach i (a b c)', `switch ($x)'.
 *
 * A bare "( ... )" without one of these in front is a subshell, whose first
 * word really is a command, so the two cases have to be told apart before
 * deciding whether "(" opens command position.
 */
static int
kw_takes_expr(const char *word, size_t len)
{
    static const char * const kws[] = {
	"if", "while", "foreach", "switch", NULL
    };

    return in_table(kws, word, len);
}

/*
 * Commands that run another command given as their arguments.  After one of
 * these the following word is still in command position, so `sudo ls' colours
 * `ls' too instead of leaving it an anonymous argument.
 */
static const char * const cmd_wrappers[] = {
    "sudo", "doas", "env", "nohup", "nice", "time", "command", "exec",
    "xargs", "setsid", "stdbuf", "timeout", "ionice", "chrt", "proxychains",
    NULL
};

/*
 * path_exists - does word name something on the filesystem?
 *
 * A leading ~ or ~user is expanded first so that "~/bin" and "~root/x" are
 * recognised.  Uses lstat(), so a dangling symlink still counts as present -
 * the point is "this name exists", not "it resolves".
 */
static int
path_exists(const char *word)
{
    char buf[MAXPATHLEN];
    struct stat st;

    if (word == NULL || *word == '\0')
	return 0;

    if (word[0] == '~') {
	const char *rest = strchr(word, '/');
	const char *home = NULL;

	if (word[1] == '\0' || word[1] == '/') {
	    home = getenv("HOME");
	    rest = (word[1] == '/') ? word + 1 : "";
	} else {
	    /* ~user[/...] */
	    char user[128];
	    size_t ulen = (rest != NULL) ? (size_t)(rest - word - 1)
					 : strlen(word) - 1;
	    struct passwd *pw;

	    if (ulen >= sizeof(user))
		return 0;
	    memcpy(user, word + 1, ulen);
	    user[ulen] = '\0';
	    pw = getpwnam(user);
	    if (pw == NULL)
		return 0;
	    home = pw->pw_dir;
	    if (rest == NULL)
		rest = "";
	}
	if (home == NULL)
	    return 0;
	{
	    int len = xsnprintf(buf, sizeof(buf), "%s%s", home, rest);

	    if (len < 0 || len >= (int)sizeof(buf))
		return 0;
	}
	return lstat(buf, &st) == 0;
    }

    return lstat(word, &st) == 0;
}

/*
 * classify_argument - decide the token for a word that is not in command
 * position.  Deliberately conservative: a word is only coloured when it is
 * unambiguously an option, a glob, or a name that exists on disk.  Anything
 * else stays SYN_NORMAL rather than guessing, so ordinary arguments do not
 * light up.
 *
 * stat_budget bounds the number of filesystem probes per line so that a
 * pathological command line cannot turn every keystroke into hundreds of
 * lstat() calls.
 */
static SynToken
classify_argument(const char *word, size_t len, int *stat_budget)
{
    if (len == 0)
	return SYN_NORMAL;

    /* Options: -v, --verbose, -- .  A bare "-" is conventionally stdin. */
    if (word[0] == '-' && len > 1)
	return SYN_OPTION;

    /* Unquoted glob metacharacters.  A backslash-escaped one ("echo \*") is
     * a literal character, not a wildcard, so skip whatever it protects. */
    {
	size_t k;

	for (k = 0; k < len; k++) {
	    if (word[k] == '\\' && k + 1 < len) {
		k++;
		continue;
	    }
	    if (word[k] == '*' || word[k] == '?' || word[k] == '[')
		return SYN_OPERATOR;
	}
    }

    /* Only probe things that actually look like filenames: an explicit path,
     * or a ~ expansion.  Probing every bare word would stat the cwd for
     * things like "install" or "-j4". */
    if (word[0] == '~' || word[0] == '/' || strchr(word, '/') != NULL) {
	if (*stat_budget > 0) {
	    (*stat_budget)--;
	    if (path_exists(word))
		return SYN_PATH;
	}
    }

    return SYN_NORMAL;
}

/*
 * classify_command - decide the token for a word appearing in command
 * position.  The order mirrors what the shell itself would actually run:
 * keywords and builtins first, then aliases and functions (which shadow
 * anything on $PATH), and only then the $PATH lookup.
 *
 * adrof1() is a read-only lookup over the existing variable tables, so this
 * stays allocation-free and cannot mutate shell state.
 */
static SynToken
classify_command(const char *word, size_t len)
{
    if (in_table(keywords, word, len))
	return SYN_KEYWORD;
    if (in_table(builtins, word, len))
	return SYN_BUILTIN;
    /* Functions are probed before aliases: declaring a function also
     * installs an alias shim ("name -> (function name !*)") that dispatches
     * to it, so an alias lookup alone would report every function as a
     * plain alias. */
    if (adrof1(str2short(word), &functions) != NULL)
	return SYN_FUNCTION;
    if (adrof1(str2short(word), &aliases) != NULL)
	return SYN_ALIAS;
    if (cmd_on_path(word))
	return SYN_CMD_OK;
    return SYN_CMD_BAD;
}

/*
 * is_modifier - one of the csh ":" variable/history modifier letters
 * (:h head, :t tail, :r root, :e extension, :u upper, :l lower, :s subst,
 * :q quote, :x quote-words, and the :g / :a repeat prefixes).
 */
static int
is_modifier(int c)
{
    return c == 'h' || c == 't' || c == 'r' || c == 'e' || c == 'u' ||
	   c == 'l' || c == 's' || c == 'q' || c == 'x' || c == 'g' ||
	   c == 'a' || c == 'p';
}

/*
 * word_to_mbs - copy buf[start..end) out as a NUL-terminated multibyte string.
 * Returns its byte length, or -1 if it does not fit.
 *
 * Characters are encoded with one_wctomb() rather than narrowed with
 * (char)(c & CHAR): truncating a wide character to its low byte produced a
 * corrupted name, so any command or path containing a non-ASCII character was
 * looked up wrong and always classified as "not found".
 */
static int
word_to_mbs(const Char *buf, ptrdiff_t start, ptrdiff_t end,
	    char *out, size_t outsz)
{
    size_t n = 0;
    ptrdiff_t i;

    for (i = start; i < end; i++) {
	char tmp[MB_LEN_MAX];
	int w = one_wctomb(tmp, buf[i] & CHAR);

	if (w <= 0 || n + (size_t) w >= outsz)
	    return -1;
	memcpy(out + n, tmp, (size_t) w);
	n += (size_t) w;
    }
    out[n] = '\0';
    return (int) n;
}

/*
 * flush_word - classify and colour the word buf[word_start..word_end).
 *
 * at_cmd says the word sits in command position; first_word distinguishes the
 * genuine head of a pipeline segment from a position reached through a wrapper
 * such as `sudo'.  Only a genuine head is allowed to render as "command not
 * found": after a wrapper the parse is a guess (`sudo -u root ls' puts `root'
 * in command position), so an unresolved word is left plain rather than shown
 * as an error.
 *
 * Returns non-zero when the following word should also be treated as a
 * command, i.e. this word was a wrapper or an option to one.  *expr_kw is set
 * when the word is a keyword whose following "( ... )" holds an expression
 * rather than commands.
 */
static int
flush_word(const Char *buf, ptrdiff_t word_start, ptrdiff_t word_end,
	   int at_cmd, int first_word, int *stat_budget, int *expr_kw)
{
    char word[MAXPATHLEN];
    int wlen = word_to_mbs(buf, word_start, word_end, word, sizeof(word));
    size_t n;
    SynToken tok;

    if (wlen <= 0)
	return 0;		/* unencodable or too long: leave it plain */
    n = (size_t) wlen;

    if (at_cmd) {
	/* An option in command position belongs to the wrapper we came
	 * through (`sudo -E ls'); colour it and keep looking. */
	if (word[0] == '-' && n > 1) {
	    memset(SyntaxColor + word_start, SYN_OPTION,
		   (size_t)(word_end - word_start));
	    return 1;
	}
	tok = classify_command(word, n);
	if (tok == SYN_CMD_BAD && !first_word)
	    tok = SYN_NORMAL;
	/* Set only from a command word, and left alone otherwise: `foreach'
	 * has a variable name between the keyword and its "( ... )", so
	 * clearing this on every word would lose the flag before the paren
	 * is reached. */
	if (expr_kw != NULL)
	    *expr_kw = kw_takes_expr(word, n);
	if (tok != SYN_NORMAL)
	    memset(SyntaxColor + word_start, tok,
		   (size_t)(word_end - word_start));
	return in_table(cmd_wrappers, word, n);
    }

    tok = classify_argument(word, n, stat_budget);
    if (tok != SYN_NORMAL)
	memset(SyntaxColor + word_start, tok,
	       (size_t)(word_end - word_start));
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
    int first_word = 1;    /* this command word heads the pipeline segment */
    int stat_budget = 64;  /* cap filesystem probes per rescan */
    int expr_kw = 0;       /* last command word was if/while/foreach/switch */
    int expr_depth = 0;    /* inside such a keyword's ( ... ) */

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
		       ch == '_') {
		/* ordinary identifier character: stay in variable mode */
		SyntaxColor[i] = SYN_VARIABLE;
	    } else if (ch == '?' || ch == '#') {
		/* $?var / $#var — single-char modifier prefix; absorb and
		 * keep state so the trailing name is also coloured. */
		SyntaxColor[i] = SYN_VARIABLE;
	    } else if (ch == '$' || ch == '!' || ch == '<') {
		/* $$, $!, $< — single-character special variables */
		SyntaxColor[i] = SYN_VARIABLE;
		state = ST_NORMAL;
	    } else if (ch == '[') {
		/* $argv[1], $x[2-3] — subscript belongs to the reference */
		SyntaxColor[i] = SYN_VARIABLE;
		while (i + 1 < len) {
		    SyntaxColor[++i] = SYN_VARIABLE;
		    if ((int)(buf[i] & CHAR) == ']')
			break;
		}
	    } else if (ch == ':' && i + 1 < len &&
		       is_modifier((int)(buf[i + 1] & CHAR))) {
		/* $x:h, $x:t, $x:gr — modifiers belong to the reference */
		SyntaxColor[i] = SYN_VARIABLE;
		SyntaxColor[++i] = SYN_VARIABLE;
		/* 'g' and 'a' are prefixes: :gh, :as */
		if (i + 1 < len && is_modifier((int)(buf[i + 1] & CHAR)))
		    SyntaxColor[++i] = SYN_VARIABLE;
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

	/* History reference: !! !$ !* !^ !:n !n !-n !string !{...} .
	 * Not "!=", which is the inequality operator, and not a bare '!'. */
	if (ch == '!' && i + 1 < len) {
	    int nc = (int)(buf[i + 1] & CHAR);

	    if (nc == '=') {
		SyntaxColor[i] = SYN_OPERATOR;
		SyntaxColor[++i] = SYN_OPERATOR;
		continue;
	    }
	    if (nc == '?') {
		/* !?string? - search history for a line containing string.
		 * The closing '?' is optional at end of word. */
		SyntaxColor[i] = SYN_VARIABLE;
		SyntaxColor[++i] = SYN_VARIABLE;
		while (i + 1 < len) {
		    SyntaxColor[++i] = SYN_VARIABLE;
		    if ((int)(buf[i] & CHAR) == '?')
			break;
		}
		/* A history reference is a complete word: give command position
		 * the same handoff flush_word() gives a wrapper's argument, so
		 * what follows is not also read as a command. */
		if (at_cmd) {
		    at_cmd = 0;
		    first_word = 0;
		}
		continue;
	    }
	    if (nc == '!' || nc == '$' || nc == '*' || nc == '^' ||
		nc == ':' || nc == '-' || nc == '{' || nc == '#' ||
		(nc >= '0' && nc <= '9') ||
		(nc >= 'a' && nc <= 'z') || (nc >= 'A' && nc <= 'Z')) {
		SyntaxColor[i] = SYN_VARIABLE;
		i++;
		SyntaxColor[i] = SYN_VARIABLE;
		/* absorb the rest of the event/word designator */
		while (i + 1 < len) {
		    int c2 = (int)(buf[i + 1] & CHAR);

		    if ((c2 >= 'a' && c2 <= 'z') || (c2 >= 'A' && c2 <= 'Z') ||
			(c2 >= '0' && c2 <= '9') || c2 == '_' || c2 == ':' ||
			c2 == '$' || c2 == '^' || c2 == '*' || c2 == '-' ||
			c2 == '}')
			SyntaxColor[++i] = SYN_VARIABLE;
		    else
			break;
		}
		if (at_cmd) {
		    at_cmd = 0;
		    first_word = 0;
		}
		continue;
	    }
	}

	/* Assignment.  Only when it is not inside a word, so that
	 * "set x = 5" marks the operator while "--opt=value" is left as a
	 * single argument. */
	if (ch == '=' && !in_word) {
	    SyntaxColor[i] = SYN_OPERATOR;
	    continue;
	}

	/* Operators / word separators */
	if (ch == '|' || ch == ';' || ch == '&' || ch == '(' ||
	    ch == ')' || ch == '\n') {
	    if (in_word) {
		(void) flush_word(buf, word_start, i, at_cmd, first_word,
				  &stat_budget, &expr_kw);
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

	    /* "(" opens command position only for a subshell.  After
	     * if/while/foreach/switch it opens an expression or word list, and
	     * it is the word after the matching ")" that is the command. */
	    if (ch == '(') {
		if (expr_kw || expr_depth > 0) {
		    expr_depth++;
		    at_cmd = 0;
		    first_word = 0;
		} else {
		    at_cmd = 1;
		    first_word = 1;
		}
		expr_kw = 0;
	    } else if (ch == ')') {
		if (expr_depth > 0) {
		    expr_depth--;
		    /* the command follows the closing paren */
		    at_cmd = (expr_depth == 0);
		    first_word = at_cmd;
		} else {
		    at_cmd = 0;
		    first_word = 0;
		}
	    } else if (expr_depth == 0) {
		/* Inside an expression's "( ... )" these are boolean operators
		 * on values, not command separators: "if (1 && 0) echo ok"
		 * must not send "0" to classify_command() as a fake command. */
		at_cmd = 1;
		first_word = 1;
		expr_kw = 0;
	    }
	    continue;
	}

	/* Redirection */
	if (ch == '>' || ch == '<') {
	    int opener = ch;

	    if (in_word) {
		/* A word ending right at a redirection operator ("cat foo>bar") was
		 * dropped without ever being classified, leaving it plain instead of
		 * a path/option/glob like any other argument. */
		(void) flush_word(buf, word_start, i, at_cmd, first_word,
				  &stat_budget, &expr_kw);
		in_word = 0;
	    }
	    SyntaxColor[i] = SYN_OPERATOR;
	    /* >> >>! >>& >& >| >! < << */
	    while (i + 1 < len) {
		int nc = (int)(buf[i+1] & CHAR);
		if (nc == '>' || nc == '<' || nc == '&' || nc == '-' ||
		    (opener == '>' && (nc == '!' || nc == '|')))
		    SyntaxColor[++i] = SYN_OPERATOR;
		else
		    break;
	    }
	    at_cmd = 0; /* after redirection, next word is not a command */
	    first_word = 0;
	    continue;
	}

	/* Whitespace — word boundary */
	if (ch == ' ' || ch == '\t') {
	    if (in_word) {
		/* A wrapper (sudo, env, ...) keeps the next word in command
		 * position, but only the genuine head of the segment may be
		 * reported as "command not found". */
		int again = flush_word(buf, word_start, i, at_cmd, first_word,
				       &stat_budget, &expr_kw);
		if (at_cmd) {
		    at_cmd = again;
		    first_word = 0;
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
    if (in_word && state == ST_NORMAL)
	(void) flush_word(buf, word_start, len, at_cmd, first_word,
			  &stat_budget, &expr_kw);

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
