/*
 * sh.cworkflow.c: C workflow builtin functions
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
#include "tc.h"
#include "tw.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

static Char STRcw_dashdash[] = { '-', '-', '\0' };
static Char STRcw_clean[] = { '-', '-', 'c', 'l', 'e', 'a', 'n', '\0' };
static Char STRcw_verbose[] = { '-', 'v', '\0' };
static Char STRcw_mcsh_cc[] = { 'm', 'c', 's', 'h', '_', 'c', 'c', '\0' };
static Char STRcw_mcsh_cache_dir[] = {
    'm', 'c', 's', 'h', '_', 'c', 'a', 'c', 'h', 'e', '_', 'd', 'i', 'r',
    '\0'
};

typedef struct {
    uint8_t data[64];
    uint32_t state[8];
    uint64_t bitlen;
    size_t datalen;
} cw_sha256_ctx_t;

static const uint32_t cw_sha256_k[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static uint32_t
cw_sha256_rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32U - n));
}

static uint32_t
cw_sha256_ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

static uint32_t
cw_sha256_maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t
cw_sha256_ep0(uint32_t x)
{
    return cw_sha256_rotr(x, 2U) ^ cw_sha256_rotr(x, 13U) ^
	cw_sha256_rotr(x, 22U);
}

static uint32_t
cw_sha256_ep1(uint32_t x)
{
    return cw_sha256_rotr(x, 6U) ^ cw_sha256_rotr(x, 11U) ^
	cw_sha256_rotr(x, 25U);
}

static uint32_t
cw_sha256_sig0(uint32_t x)
{
    return cw_sha256_rotr(x, 7U) ^ cw_sha256_rotr(x, 18U) ^ (x >> 3U);
}

static uint32_t
cw_sha256_sig1(uint32_t x)
{
    return cw_sha256_rotr(x, 17U) ^ cw_sha256_rotr(x, 19U) ^ (x >> 10U);
}

static void
cw_sha256_transform(cw_sha256_ctx_t *ctx, const uint8_t data[64])
{
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t t1, t2, m[64];
    int i, j;

    for (i = 0, j = 0; i < 16; i++, j += 4) {
	m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) |
	    ((uint32_t)data[j + 2] << 8) | (uint32_t)data[j + 3];
    }
    for (; i < 64; i++)
	m[i] = cw_sha256_sig1(m[i - 2]) + m[i - 7] + cw_sha256_sig0(m[i - 15]) +
	    m[i - 16];

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; i++) {
	t1 = h + cw_sha256_ep1(e) + cw_sha256_ch(e, f, g) + cw_sha256_k[i] + m[i];
	t2 = cw_sha256_ep0(a) + cw_sha256_maj(a, b, c);
	h = g;
	g = f;
	f = e;
	e = d + t1;
	d = c;
	c = b;
	b = a;
	a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void
cw_sha256_init(cw_sha256_ctx_t *ctx)
{
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667U;
    ctx->state[1] = 0xbb67ae85U;
    ctx->state[2] = 0x3c6ef372U;
    ctx->state[3] = 0xa54ff53aU;
    ctx->state[4] = 0x510e527fU;
    ctx->state[5] = 0x9b05688cU;
    ctx->state[6] = 0x1f83d9abU;
    ctx->state[7] = 0x5be0cd19U;
}

static void
cw_sha256_update(cw_sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++) {
	ctx->data[ctx->datalen] = data[i];
	ctx->datalen++;
	if (ctx->datalen == 64) {
	    cw_sha256_transform(ctx, ctx->data);
	    ctx->bitlen += 512;
	    ctx->datalen = 0;
	}
    }
}

static void
cw_sha256_final(cw_sha256_ctx_t *ctx, uint8_t out[32])
{
    uint32_t i;

    i = (uint32_t)ctx->datalen;

    if (ctx->datalen < 56) {
	ctx->data[i++] = 0x80;
	while (i < 56)
	    ctx->data[i++] = 0x00;
    }
    else {
	ctx->data[i++] = 0x80;
	while (i < 64)
	    ctx->data[i++] = 0x00;
	cw_sha256_transform(ctx, ctx->data);
	memset(ctx->data, 0, 56);
    }

    ctx->bitlen += (uint64_t)ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    cw_sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; i++) {
	out[i] = (uint8_t)((ctx->state[0] >> (24 - i * 8)) & 0xff);
	out[i + 4] = (uint8_t)((ctx->state[1] >> (24 - i * 8)) & 0xff);
	out[i + 8] = (uint8_t)((ctx->state[2] >> (24 - i * 8)) & 0xff);
	out[i + 12] = (uint8_t)((ctx->state[3] >> (24 - i * 8)) & 0xff);
	out[i + 16] = (uint8_t)((ctx->state[4] >> (24 - i * 8)) & 0xff);
	out[i + 20] = (uint8_t)((ctx->state[5] >> (24 - i * 8)) & 0xff);
	out[i + 24] = (uint8_t)((ctx->state[6] >> (24 - i * 8)) & 0xff);
	out[i + 28] = (uint8_t)((ctx->state[7] >> (24 - i * 8)) & 0xff);
    }
}

static int
cw_sha256_file(const char *path, uint8_t out[32])
{
    cw_sha256_ctx_t ctx;
    uint8_t buf[65536];
    ssize_t nread;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd == -1)
	return -1;

    cw_sha256_init(&ctx);
    while ((nread = read(fd, buf, sizeof(buf))) > 0)
	cw_sha256_update(&ctx, buf, (size_t)nread);

    if (close(fd) == -1 && nread >= 0)
	nread = -1;
    if (nread < 0)
	return -1;

    cw_sha256_final(&ctx, out);
    return 0;
}

static void
cw_sha256_hex(const uint8_t in[32], char out[65])
{
    static const char hex[] = "0123456789abcdef";
    size_t i;

    for (i = 0; i < 32; i++) {
	out[i * 2] = hex[in[i] >> 4];
	out[i * 2 + 1] = hex[in[i] & 0x0f];
    }
    out[64] = '\0';
}

typedef enum {
    CW_TARGET_FILE = 1,
    CW_TARGET_DIR = 2
} cw_target_kind_t;

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} cw_str_list_t;

typedef struct {
    Char *target_word;
    Char **program_args;
    char *target_path;
    cw_target_kind_t target_kind;
    int clean_requested;
    int verbose;
} cw_run_request_t;

typedef struct {
    char cc_path[MAXPATHLEN];
    char cache_root[MAXPATHLEN];
    char objects_dir[MAXPATHLEN];
    char binaries_dir[MAXPATHLEN];
    char binary_path[MAXPATHLEN];
    uint8_t cc_hash[32];
    uint8_t project_hash[32];
    cw_str_list_t sources;
    cw_str_list_t fingerprints;
    cw_str_list_t objects;
} cw_run_state_t;

static void
cw_diagf(const char *fmt, ...)
{
    va_list ap;
    char *msg;
    int fd;
    size_t len;

    va_start(ap, fmt);
    msg = xvasprintf(fmt, ap);
    va_end(ap);
    if (msg == NULL)
	return;

    fd = didfds ? 2 : SHDIAG;
    len = strlen(msg);
    if (len > 0)
	(void)xwrite(fd, msg, len);
    xfree(msg);
}

static void
cw_set_status_code(int code)
{
    setstrstatus(putn((tcsh_number_t)code));
}

static void
cw_str_list_init(cw_str_list_t *list)
{
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

static int
cw_str_list_push(cw_str_list_t *list, const char *value)
{
    char **next_items;
    size_t new_cap;

    if (list->len == list->cap) {
	new_cap = (list->cap == 0) ? 8 : list->cap * 2;
	next_items = xrealloc(list->items, new_cap * sizeof(*next_items));
	list->items = next_items;
	list->cap = new_cap;
    }
    list->items[list->len++] = strsave(value);
    return 0;
}

static int
cw_str_cmp(const void *lhs, const void *rhs)
{
    const char *const *a = lhs;
    const char *const *b = rhs;
    return strcmp(*a, *b);
}

static void
cw_str_list_sort(cw_str_list_t *list)
{
    if (list->len > 1)
	qsort(list->items, list->len, sizeof(*list->items), cw_str_cmp);
}

static void
cw_str_list_free(cw_str_list_t *list)
{
    size_t i;

    for (i = 0; i < list->len; i++)
	xfree(list->items[i]);
    xfree(list->items);
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

static void
cw_run_request_cleanup(cw_run_request_t *req)
{
    if (req->target_path != NULL) {
	xfree(req->target_path);
	req->target_path = NULL;
    }
}

static void
cw_run_state_cleanup(cw_run_state_t *st)
{
    cw_str_list_free(&st->sources);
    cw_str_list_free(&st->fingerprints);
    cw_str_list_free(&st->objects);
}

static int
cw_has_suffix(const char *s, const char *suffix)
{
    size_t slen, suflen;

    slen = strlen(s);
    suflen = strlen(suffix);
    if (slen < suflen)
	return 0;
    return strcmp(s + slen - suflen, suffix) == 0;
}

static int
cw_wait_for_child(pid_t pid)
{
    int status;
    pid_t wpid;

    while ((wpid = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
	(void)handle_pending_signals();
    if (wpid == -1)
	return -1;
    if (WIFEXITED(status))
	return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
	return 128 + WTERMSIG(status);
    return -1;
}

static int
cw_is_regular_file(const char *path)
{
    struct stat st;

    if (stat(path, &st) == -1)
	return 0;
    return S_ISREG(st.st_mode) ? 1 : 0;
}

static int
cw_is_executable_file(const char *path)
{
    struct stat st;

    if (stat(path, &st) == -1)
	return 0;
    return S_ISREG(st.st_mode) && access(path, X_OK) == 0;
}

static int
cw_resolve_on_path(const char *cmd, char *out, size_t out_len)
{
    struct varent *pathv;
    Char **pv;
    char candidate[MAXPATHLEN];
    const char *dir;
    char *env_path;
    char *path_copy;
    char *seg;
    char *next;
    int n;

    if (strchr(cmd, '/') != NULL) {
	if (!cw_is_executable_file(cmd))
	    return -1;
	n = xsnprintf(out, out_len, "%s", cmd);
	return (n >= 0 && (size_t)n < out_len) ? 0 : -1;
    }

    pathv = adrof(STRpath);
    if (pathv && pathv->vec) {
	for (pv = pathv->vec; *pv; pv++) {
	    dir = short2str(*pv);
	    if (dir == NULL)
		continue;
	    if (dir[0] == '\0' || (dir[0] == '.' && dir[1] == '\0'))
		n = xsnprintf(candidate, sizeof(candidate), "%s", cmd);
	    else
		n = xsnprintf(candidate, sizeof(candidate), "%s/%s", dir, cmd);
	    if (n < 0 || (size_t)n >= sizeof(candidate))
		continue;
	    if (!cw_is_executable_file(candidate))
		continue;
	    n = xsnprintf(out, out_len, "%s", candidate);
	    return (n >= 0 && (size_t)n < out_len) ? 0 : -1;
	}
    }

    env_path = getenv("PATH");
    if (env_path == NULL || *env_path == '\0')
	return -1;

    path_copy = strsave(env_path);
    if (path_copy == NULL)
	return -1;
    seg = path_copy;
    for (;;) {
	next = strchr(seg, PATHSEP);
	if (next != NULL)
	    *next = '\0';

	if (*seg == '\0' || (seg[0] == '.' && seg[1] == '\0'))
	    n = xsnprintf(candidate, sizeof(candidate), "%s", cmd);
	else
	    n = xsnprintf(candidate, sizeof(candidate), "%s/%s", seg, cmd);
	if (n >= 0 && (size_t)n < sizeof(candidate) &&
	    cw_is_executable_file(candidate)) {
	    n = xsnprintf(out, out_len, "%s", candidate);
	    xfree(path_copy);
	    return (n >= 0 && (size_t)n < out_len) ? 0 : -1;
	}

	if (next == NULL)
	    break;
	seg = next + 1;
    }
    xfree(path_copy);
    return -1;
}

static int
cw_ci_contains(const char *haystack, const char *needle)
{
    size_t i, j, hlen, nlen;

    hlen = strlen(haystack);
    nlen = strlen(needle);
    if (nlen == 0 || hlen < nlen)
	return 0;

    for (i = 0; i + nlen <= hlen; i++) {
	for (j = 0; j < nlen; j++) {
	    unsigned char hc = (unsigned char)haystack[i + j];
	    unsigned char nc = (unsigned char)needle[j];
	    if (tolower(hc) != tolower(nc))
		break;
	}
	if (j == nlen)
	    return 1;
    }
    return 0;
}

static int
cw_capture_command_output(const char *cmd, char *const argv[],
    char *out, size_t out_len)
{
    int fds[2];
    pid_t pid;
    int rc;
    ssize_t nread;
    size_t used;
    char discard[256];

    if (out_len == 0)
	return -1;
    out[0] = '\0';

    if (pipe(fds) == -1)
	return -1;

    pid = fork();
    if (pid == -1) {
	xclose(fds[0]);
	xclose(fds[1]);
	return -1;
    }

    if (pid == 0) {
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGTERM, SIG_DFL);
	xclose(fds[0]);
	(void)dup2(fds[1], 1);
	(void)dup2(fds[1], 2);
	if (fds[1] != 1 && fds[1] != 2)
	    xclose(fds[1]);
	execv(cmd, argv);
	_exit(127);
    }

    xclose(fds[1]);
    used = 0;
    for (;;) {
	char *dst;
	size_t cap;

	if (used + 1 < out_len) {
	    dst = out + used;
	    cap = out_len - used - 1;
	}
	else {
	    dst = discard;
	    cap = sizeof(discard);
	}

	nread = read(fds[0], dst, cap);
	if (nread == 0)
	    break;
	if (nread < 0) {
	    if (errno == EINTR) {
		(void)handle_pending_signals();
		continue;
	    }
	    break;
	}
	if (dst == out + used)
	    used += (size_t)nread;
    }
    out[used] = '\0';
    xclose(fds[0]);

    rc = cw_wait_for_child(pid);
    return rc;
}

static int
cw_toolchain_is_clang_family(const char *cc_path)
{
    const char *base;
    char version_out[2048];
    char *const argv[] = {
	(char *)(intptr_t)cc_path,
	"--version",
	NULL
    };

    base = strrchr(cc_path, '/');
    base = base ? base + 1 : cc_path;
    if (cw_ci_contains(base, "clang"))
	return 1;

    if (cw_capture_command_output(cc_path, argv, version_out,
	sizeof(version_out)) < 0)
	return 0;
    return cw_ci_contains(version_out, "clang");
}

static char *
cw_get_shell_word(Char *name)
{
    struct varent *vp;

    vp = adrof(name);
    if (vp == NULL || vp->vec == NULL || vp->vec[0] == NULL ||
	vp->vec[0][0] == '\0')
	return NULL;
    return strsave(short2str(vp->vec[0]));
}

static int
cw_find_toolchain(char *cc_path, size_t cc_path_len)
{
    char *user_cc;

    user_cc = cw_get_shell_word(STRcw_mcsh_cc);
    if (user_cc != NULL) {
	if (cw_resolve_on_path(user_cc, cc_path, cc_path_len) == 0 &&
	    cw_toolchain_is_clang_family(cc_path)) {
	    xfree(user_cc);
	    return 0;
	}
	cw_diagf("run: compiler '%s' from $mcsh_cc is not a clang-family compiler.\n"
	    "mcsh run requires clang-family toolchains to keep the runtime BSD/Apache-2.0 aligned.\n",
	    user_cc);
	xfree(user_cc);
	return -1;
    }

    if (cw_resolve_on_path("clang", cc_path, cc_path_len) == 0 &&
	cw_toolchain_is_clang_family(cc_path))
	return 0;
    if (cw_resolve_on_path("cc", cc_path, cc_path_len) == 0 &&
	cw_toolchain_is_clang_family(cc_path))
	return 0;
    return -1;
}

static int
cw_mkdir_p(const char *path)
{
    char tmp[MAXPATHLEN];
    char *p;
    size_t len;
    int n;

    n = xsnprintf(tmp, sizeof(tmp), "%s", path);
    if (n < 0 || (size_t)n >= sizeof(tmp))
	return -1;

    len = strlen(tmp);
    if (len == 0)
	return -1;
    if (len > 1 && tmp[len - 1] == '/')
	tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
	if (*p != '/')
	    continue;
	*p = '\0';
	if (mkdir(tmp, 0700) == -1 && errno != EEXIST)
	    return -1;
	*p = '/';
    }
    if (mkdir(tmp, 0700) == -1 && errno != EEXIST)
	return -1;
    return 0;
}

static int
cw_get_cache_root(char *out, size_t out_len)
{
    char *cache_dir;
    char *home;
    const char *env_home;
    int n;

    cache_dir = cw_get_shell_word(STRcw_mcsh_cache_dir);
    if (cache_dir != NULL) {
	n = xsnprintf(out, out_len, "%s", cache_dir);
	xfree(cache_dir);
	return (n >= 0 && (size_t)n < out_len) ? 0 : -1;
    }

    home = cw_get_shell_word(STRhome);
    env_home = getenv("HOME");
    if (home == NULL && env_home != NULL && *env_home != '\0')
	home = strsave(env_home);
    if (home == NULL)
	return -1;

    n = xsnprintf(out, out_len, "%s/.mcsh_cache/cworkflow", home);
    xfree(home);
    return (n >= 0 && (size_t)n < out_len) ? 0 : -1;
}

static int
cw_should_skip_dir_name(const char *name)
{
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
	return 1;
    if (strcmp(name, ".git") == 0 || strcmp(name, ".mcsh_cache") == 0)
	return 1;
    return 0;
}

static int
cw_collect_project_files(const char *root, cw_str_list_t *sources,
    cw_str_list_t *fingerprints)
{
    DIR *dirp;
    struct dirent *de;

    dirp = opendir(root);
    if (dirp == NULL)
	return -1;

    while ((de = readdir(dirp)) != NULL) {
	char path[MAXPATHLEN];
	struct stat st;
	int n;

	if (cw_should_skip_dir_name(de->d_name))
	    continue;

	n = xsnprintf(path, sizeof(path), "%s/%s", root, de->d_name);
	if (n < 0 || (size_t)n >= sizeof(path)) {
	    xclosedir(dirp);
	    return -1;
	}

	if (lstat(path, &st) == -1)
	    continue;

	if (S_ISDIR(st.st_mode)) {
	    if (cw_collect_project_files(path, sources, fingerprints) != 0) {
		xclosedir(dirp);
		return -1;
	    }
	    continue;
	}

	if (!S_ISREG(st.st_mode))
	    continue;

	if (cw_has_suffix(path, ".c")) {
	    (void)cw_str_list_push(sources, path);
	    (void)cw_str_list_push(fingerprints, path);
	}
	else if (cw_has_suffix(path, ".h")) {
	    (void)cw_str_list_push(fingerprints, path);
	}
    }

    xclosedir(dirp);
    return 0;
}

static int
cw_hash_file_set(const cw_str_list_t *files, uint8_t out[32])
{
    cw_sha256_ctx_t ctx;
    uint8_t file_hash[32];
    size_t i;
    char nul = '\0';

    cw_sha256_init(&ctx);
    for (i = 0; i < files->len; i++) {
	if (cw_sha256_file(files->items[i], file_hash) != 0)
	    return -1;
	cw_sha256_update(&ctx, (const uint8_t *)files->items[i],
	    strlen(files->items[i]));
	cw_sha256_update(&ctx, (const uint8_t *)&nul, 1);
	cw_sha256_update(&ctx, file_hash, sizeof(file_hash));
    }
    cw_sha256_final(&ctx, out);
    return 0;
}

static int
cw_hash_toolchain(const char *cc_path, uint8_t out[32])
{
    cw_sha256_ctx_t ctx;

    if (cw_sha256_file(cc_path, out) == 0)
	return 0;

    cw_sha256_init(&ctx);
    cw_sha256_update(&ctx, (const uint8_t *)cc_path, strlen(cc_path));
    cw_sha256_final(&ctx, out);
    return 0;
}

static int
cw_compute_object_path(const cw_run_state_t *st, const char *source_path,
    char *out, size_t out_len)
{
    cw_sha256_ctx_t ctx;
    uint8_t source_hash[32], object_hash[32];
    char object_hex[65];
    int n;

    if (cw_sha256_file(source_path, source_hash) != 0)
	return -1;

    cw_sha256_init(&ctx);
    cw_sha256_update(&ctx, (const uint8_t *)"obj", 3);
    cw_sha256_update(&ctx, (const uint8_t *)source_path, strlen(source_path));
    cw_sha256_update(&ctx, source_hash, sizeof(source_hash));
    cw_sha256_update(&ctx, st->project_hash, sizeof(st->project_hash));
    cw_sha256_update(&ctx, st->cc_hash, sizeof(st->cc_hash));
    cw_sha256_final(&ctx, object_hash);

    cw_sha256_hex(object_hash, object_hex);
    n = xsnprintf(out, out_len, "%s/%s.o", st->objects_dir, object_hex);
    return (n >= 0 && (size_t)n < out_len) ? 0 : -1;
}

static int
cw_compute_binary_path(cw_run_state_t *st)
{
    cw_sha256_ctx_t ctx;
    uint8_t binary_hash[32];
    char binary_hex[65];
    int n;

    cw_sha256_init(&ctx);
    cw_sha256_update(&ctx, (const uint8_t *)"bin", 3);
    cw_sha256_update(&ctx, st->project_hash, sizeof(st->project_hash));
    cw_sha256_update(&ctx, st->cc_hash, sizeof(st->cc_hash));
    cw_sha256_final(&ctx, binary_hash);

    cw_sha256_hex(binary_hash, binary_hex);
    n = xsnprintf(st->binary_path, sizeof(st->binary_path), "%s/%s",
	st->binaries_dir, binary_hex);
    return (n >= 0 && (size_t)n < sizeof(st->binary_path)) ? 0 : -1;
}

static int
cw_compile_object(const char *cc_path, const char *source_path,
    const char *object_path, int verbose)
{
    pid_t pid;
    int status;
    char *const argv[] = {
	(char *)(intptr_t)cc_path,
	"-c",
	(char *)(intptr_t)source_path,
	"-o",
	(char *)(intptr_t)object_path,
	NULL
    };

    if (verbose)
	cw_diagf("[run] %s -c %s -o %s\n", cc_path, source_path, object_path);

    pid = fork();
    if (pid == -1)
	return -1;
    if (pid == 0) {
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGTERM, SIG_DFL);
	execv(cc_path, argv);
	_exit(127);
    }
    status = cw_wait_for_child(pid);
    return status;
}

static int
cw_link_binary(const char *cc_path, const cw_str_list_t *objects,
    const char *binary_path, int verbose)
{
    char **argv;
    size_t i;
    size_t argc;
    pid_t pid;
    int status;

    argc = objects->len + 4;
    argv = xcalloc(argc, sizeof(*argv));
    argv[0] = (char *)(intptr_t)cc_path;
    for (i = 0; i < objects->len; i++)
	argv[i + 1] = objects->items[i];
    argv[objects->len + 1] = "-o";
    argv[objects->len + 2] = (char *)(intptr_t)binary_path;
    argv[objects->len + 3] = NULL;

    if (verbose)
	cw_diagf("[run] %s <objects:%d> -o %s\n", cc_path,
	    (int)objects->len, binary_path);

    pid = fork();
    if (pid == -1) {
	xfree(argv);
	return -1;
    }
    if (pid == 0) {
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGTERM, SIG_DFL);
	execv(cc_path, argv);
	_exit(127);
    }
    status = cw_wait_for_child(pid);
    xfree(argv);
    return status;
}

/*
 * cw_execute_binary — fork and exec the compiled binary.
 *
 * Returns the child's exit status (0–255, or 128+sig).
 * Returns -1 on fork or pipe-setup failure (errno set).
 * Returns -2 if execv itself failed (errno set to the exec error).
 * Returns -3 if the exec-detection pipe could not transfer a complete
 *   errno value (partial read/write or read error); the exec outcome
 *   is then ambiguous and cannot be distinguished from a normal exit 127.
 *
 * A FD_CLOEXEC pipe distinguishes exec failure (child writes errno before
 * _exit) from a legitimate user-program exit code of 127.  Both the child
 * write and the parent read retry on EINTR and accumulate partial transfers
 * to guarantee the full sizeof(int) is moved atomically at the semantic level.
 */
static int
cw_execute_binary(const char *binary_path, char **argv)
{
    pid_t pid;
    int status;
    int execpipe[2];
    int exec_errno;
    ssize_t n;

    if (pipe(execpipe) == -1)
	return -1;
    if (fcntl(execpipe[1], F_SETFD, FD_CLOEXEC) == -1) {
	(void)close(execpipe[0]);
	(void)close(execpipe[1]);
	return -1;
    }

    pid = fork();
    if (pid == -1) {
	(void)close(execpipe[0]);
	(void)close(execpipe[1]);
	return -1;
    }
    if (pid == 0) {
	const char *wp;
	size_t wrem;
	ssize_t nw;

	(void)close(execpipe[0]);
	(void)signal(SIGINT, SIG_DFL);
	(void)signal(SIGQUIT, SIG_DFL);
	(void)signal(SIGTERM, SIG_DFL);
	execv(binary_path, argv);
	/* execv failed: write errno through the pipe before exiting. */
	exec_errno = errno;
	wp = (const char *)&exec_errno;
	wrem = sizeof(exec_errno);
	while (wrem > 0) {
	    nw = write(execpipe[1], wp, wrem);
	    if (nw < 0) {
		if (errno == EINTR)
		    continue;
		break;	/* write error: best effort only */
	    }
	    wp += nw;
	    wrem -= (size_t)nw;
	}
	_exit(127);
    }

    /* Parent: drain the pipe into exec_errno with EINTR retry. */
    (void)close(execpipe[1]);
    {
	char *rp = (char *)&exec_errno;
	size_t rrem = sizeof(exec_errno);
	ssize_t nr;
	n = 0;
	while (rrem > 0) {
	    nr = read(execpipe[0], rp, rrem);
	    if (nr == 0)
		break;	/* EOF: exec succeeded, pipe closed by FD_CLOEXEC */
	    if (nr < 0) {
		if (errno == EINTR)
		    continue;
		n = -1;	/* read error: flag as pipe failure */
		break;
	    }
	    n += nr;
	    rp += nr;
	    rrem -= (size_t)nr;
	}
    }
    (void)close(execpipe[0]);
    status = cw_wait_for_child(pid);

    if (n == (ssize_t)sizeof(exec_errno)) {
	/* exec failed: restore the exec errno for the caller's strerror(). */
	errno = exec_errno;
	return -2;
    }
    if (n != 0) {
	/* Partial transfer or read error: pipe state is ambiguous. */
	return -3;
    }
    return status;
}

static char **
cw_build_program_argv(const char *binary_path, Char **prog_args)
{
    int argc, i;
    char **argv;

    argc = 1;
    if (prog_args != NULL) {
	for (i = 0; prog_args[i] != NULL; i++)
	    argc++;
    }
    argv = xcalloc((size_t)argc + 1U, sizeof(*argv));
    argv[0] = strsave(binary_path);
    if (prog_args != NULL) {
	for (i = 0; prog_args[i] != NULL; i++)
	    argv[i + 1] = strsave(short2str(prog_args[i]));
    }
    argv[argc] = NULL;
    return argv;
}

static void
cw_free_program_argv(char **argv)
{
    int i;

    if (argv == NULL)
	return;
    for (i = 0; argv[i] != NULL; i++)
	xfree(argv[i]);
    xfree(argv);
}

static int
cw_run_parse_request(Char **v, cw_run_request_t *req)
{
    int i;

    memset(req, 0, sizeof(*req));

    for (i = 1; v[i] != NULL; i++) {
	if (req->target_word == NULL) {
	    if (eq(v[i], STRcw_clean)) {
		req->clean_requested = 1;
		continue;
	    }
	    if (eq(v[i], STRcw_verbose)) {
		req->verbose = 1;
		continue;
	    }
	    if (eq(v[i], STRcw_dashdash)) {
		cw_diagf("run: missing argument.\n"
		    "Usage: run <file.c|target> [-- program-args...]\n");
		return 1;
	    }
	    if (v[i][0] == '-') {
		cw_diagf("run: unknown option '%s'.\n", short2str(v[i]));
		return 1;
	    }
	    req->target_word = v[i];
	    continue;
	}

	if (eq(v[i], STRcw_dashdash))
	    req->program_args = &v[i + 1];
	else
	    req->program_args = &v[i];
	return 0;
    }

    if (req->target_word == NULL) {
	cw_diagf("run: missing argument.\n"
	    "Usage: run <file.c|target> [-- program-args...]\n");
	return 1;
    }
    return 0;
}

static int
cw_run_validate_request(cw_run_request_t *req)
{
    struct stat st;

    req->target_path = strsave(short2str(req->target_word));
    if (req->target_path == NULL)
	return 1;

    if (stat(req->target_path, &st) == -1) {
	cw_diagf("run: '%s': file not found.\n", req->target_path);
	return 1;
    }

    if (S_ISDIR(st.st_mode)) {
	req->target_kind = CW_TARGET_DIR;
	return 0;
    }
    if (!S_ISREG(st.st_mode)) {
	cw_diagf("run: '%s': unsupported target type.\n", req->target_path);
	return 1;
    }

    if (cw_has_suffix(req->target_path, ".mcsh")) {
	cw_diagf("run: '%s' is a shell script, not a C file.\n"
	    "Shell scripts execute directly - no 'run' needed:\n"
	    "  mcsh %s\n"
	    "  ./%s  (if executable)\n"
	    "'run' is for C files only.\n",
	    req->target_path, req->target_path, req->target_path);
	return 1;
    }
    if (!cw_has_suffix(req->target_path, ".c")) {
	cw_diagf("run: '%s' is not a C source file or project directory.\n"
	    "'run' supports <file.c> or <directory>.\n", req->target_path);
	return 1;
    }

    req->target_kind = CW_TARGET_FILE;
    return 0;
}

static int
cw_run_prepare_state(const cw_run_request_t *req, cw_run_state_t *st)
{
    char canonical[MAXPATHLEN];
    const char *fp_target;
    size_t i;
    int n;

    memset(st, 0, sizeof(*st));
    cw_str_list_init(&st->sources);
    cw_str_list_init(&st->fingerprints);
    cw_str_list_init(&st->objects);

    if (cw_find_toolchain(st->cc_path, sizeof(st->cc_path)) != 0) {
	cw_diagf("run: no C compiler found. Install clang or cc and ensure it is in $PATH.\n");
	return 5;
    }
    (void)cw_hash_toolchain(st->cc_path, st->cc_hash);

    /*
     * Canonicalize the target path so that file.c, ./file.c, and
     * /absolute/file.c that refer to the same file produce identical
     * fingerprints.  Fall back to the original path if realpath(3) fails
     * (e.g. the file was removed between validate and prepare).
     */
    fp_target = (realpath(req->target_path, canonical) != NULL)
	? canonical : req->target_path;

    if (req->target_kind == CW_TARGET_FILE) {
	(void)cw_str_list_push(&st->sources, fp_target);
	(void)cw_str_list_push(&st->fingerprints, fp_target);
    }
    else {
	if (cw_collect_project_files(fp_target, &st->sources,
		&st->fingerprints) != 0) {
	    cw_diagf("run: failed to scan project '%s'.\n", req->target_path);
	    return 1;
	}
    }

    cw_str_list_sort(&st->sources);
    cw_str_list_sort(&st->fingerprints);
    if (st->sources.len == 0) {
	cw_diagf("run: '%s' has no C sources to build.\n", req->target_path);
	return 1;
    }

    if (cw_hash_file_set(&st->fingerprints, st->project_hash) != 0) {
	cw_diagf("run: failed to hash project inputs under '%s'.\n",
	    req->target_path);
	return 1;
    }

    if (cw_get_cache_root(st->cache_root, sizeof(st->cache_root)) != 0) {
	cw_diagf("run: unable to determine cache directory (set $home or $mcsh_cache_dir).\n");
	return 1;
    }
    n = xsnprintf(st->objects_dir, sizeof(st->objects_dir), "%s/objects",
	st->cache_root);
    if (n < 0 || (size_t)n >= sizeof(st->objects_dir)) {
	cw_diagf("run: cache path is too long.\n");
	return 1;
    }
    n = xsnprintf(st->binaries_dir, sizeof(st->binaries_dir), "%s/binaries",
	st->cache_root);
    if (n < 0 || (size_t)n >= sizeof(st->binaries_dir)) {
	cw_diagf("run: cache path is too long.\n");
	return 1;
    }
    if (cw_mkdir_p(st->objects_dir) != 0 || cw_mkdir_p(st->binaries_dir) != 0) {
	cw_diagf("run: failed to create cache directories under '%s': %s\n",
	    st->cache_root, strerror(errno));
	return 1;
    }

    for (i = 0; i < st->sources.len; i++) {
	char object_path[MAXPATHLEN];
	if (cw_compute_object_path(st, st->sources.items[i], object_path,
		sizeof(object_path)) != 0) {
	    cw_diagf("run: failed to compute object path for '%s'.\n",
		st->sources.items[i]);
	    return 1;
	}
	(void)cw_str_list_push(&st->objects, object_path);
    }
    if (cw_compute_binary_path(st) != 0) {
	cw_diagf("run: failed to compute binary cache path.\n");
	return 1;
    }

    if (req->clean_requested) {
	(void)unlink(st->binary_path);
	for (i = 0; i < st->objects.len; i++)
	    (void)unlink(st->objects.items[i]);
    }

    return 0;
}

static int
cw_run_compile_stage(const cw_run_request_t *req, const cw_run_state_t *st)
{
    size_t i;
    int status;

    for (i = 0; i < st->sources.len; i++) {
	if (cw_is_regular_file(st->objects.items[i]))
	    continue;
	status = cw_compile_object(st->cc_path, st->sources.items[i],
	    st->objects.items[i], req->verbose);
	if (status == -1 || status == 127) {
	    cw_diagf("run: failed to execute compiler '%s'.\n", st->cc_path);
	    return 5;
	}
	if (status != 0)
	    return 3;
    }
    return 0;
}

static int
cw_run_build_stage(const cw_run_request_t *req, const cw_run_state_t *st)
{
    int status;

    if (cw_is_executable_file(st->binary_path))
	return 0;

    status = cw_link_binary(st->cc_path, &st->objects, st->binary_path,
	req->verbose);
    if (status == -1 || status == 127) {
	cw_diagf("run: failed to execute linker '%s'.\n", st->cc_path);
	return 5;
    }
    if (status != 0)
	return 3;
    return 0;
}

static int
cw_run_execute_stage(const cw_run_request_t *req, const cw_run_state_t *st)
{
    int run_status;
    char **run_argv;

    run_argv = cw_build_program_argv(st->binary_path, req->program_args);
    run_status = cw_execute_binary(st->binary_path, run_argv);
    cw_free_program_argv(run_argv);

    if (run_status == -1) {
	cw_diagf("run: fork failed: %s\n", strerror(errno));
	return 127;
    }
    if (run_status == -2) {
	cw_diagf("run: exec failed: %s\n", strerror(errno));
	return 127;
    }
    if (run_status == -3) {
	cw_diagf("run: internal error: exec pipe failed\n");
	return 127;
    }
    return run_status;
}

/*ARGSUSED*/
void
dorun(Char **v, struct command *c)
{
    cw_run_request_t req;
    cw_run_state_t st;
    int status;

    USE(c);
    setname("run");
    memset(&req, 0, sizeof(req));
    memset(&st, 0, sizeof(st));

    status = cw_run_parse_request(v, &req);
    if (status == 0)
	status = cw_run_validate_request(&req);
    if (status == 0)
	status = cw_run_prepare_state(&req, &st);
    if (status == 0)
	status = cw_run_compile_stage(&req, &st);
    if (status == 0)
	status = cw_run_build_stage(&req, &st);
    if (status == 0)
	status = cw_run_execute_stage(&req, &st);

    cw_set_status_code(status);
    cw_run_state_cleanup(&st);
    cw_run_request_cleanup(&req);
}
