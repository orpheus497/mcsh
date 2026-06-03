#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

// Dummy representations
typedef int Char;

#define Strrchr(s, c) my_Strrchr(s, c)
#define Strsave(s) my_Strsave(s)

Char *my_Strrchr(Char *s, int c) {
    Char *last = NULL;
    while (*s) {
        if (*s == c) last = s;
        s++;
    }
    return last;
}

Char *my_Strsave(const Char *s) {
    int len = 0;
    while (s[len]) len++;
    Char *ret = malloc((len + 1) * sizeof(Char));
    for (int i = 0; i <= len; i++) ret[i] = s[i];
    return ret;
}

char *short2str(const Char *src) {
    static char *sdst = NULL;
    static size_t dstsize = 0;
    char *dst, *edst;

    if (src == NULL)
        return (NULL);

    if (sdst == NULL) {
        dstsize = 128;
        sdst = malloc((dstsize + 0) * sizeof(char));
    }
    dst = sdst;
    edst = &dst[dstsize];
    while (*src) {
        // Simulating the work done by one_wctomb
        char buf[8];
        int wlen = 1;
        buf[0] = (char)*src;
        for (int k = 0; k < wlen; k++) {
            *dst++ = buf[k];
        }
        src++;
        if (dst >= edst) {
            char *wdst = dst;
            char *wedst = edst;

            dstsize *= 2;
            sdst = realloc(sdst, (dstsize + 0) * sizeof(char));
            edst = &sdst[dstsize];
            dst = &sdst[wdst - wedst + dstsize / 2];
        }
    }
    *dst = '\0';
    return (sdst);
}

char *strsave(const char *s) {
    return strdup(s);
}

// We mock stat to return -1 sometimes or 0, simulating a deep directory structure search.
int my_stat(const char *path, struct stat *buf) {
    // just simulate some work
    if (path[0] == '\0') return -1;
    buf->st_ino = 1; // dummy
    return 0; // return 0 so it continues searching backwards till it breaks or finishes
}


void test_original(const Char *cp) {
    struct stat statbuf;
    Char *p2, *copy, *sp;
    int found = 0;

    p2 = copy = Strsave(cp);
    while (*p2 && my_stat(short2str(p2), &statbuf) != -1) {
        if (statbuf.st_ino == 12345) { // Dummy
            found = 1;
            break;
        }
        if ((sp = Strrchr(p2, '/')) != NULL)
            *sp = '\0';
        else
            break;
    }
    free(copy);
}

void test_optimized(const Char *cp) {
    struct stat statbuf;
    Char *p2, *copy;
    char *narrow_path, *narrow_sp;
    int found = 0;

    p2 = copy = Strsave(cp);
    narrow_path = strsave(short2str(p2));

    while (*narrow_path && my_stat(narrow_path, &statbuf) != -1) {
        if (statbuf.st_ino == 12345) { // Dummy
            found = 1;
            break;
        }
        if ((narrow_sp = strrchr(narrow_path, '/')) != NULL) {
            *narrow_sp = '\0';
            p2[narrow_sp - narrow_path] = '\0';
        } else {
            *p2 = '\0';
            break;
        }
    }
    free(narrow_path);
    free(copy);
}

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

int main() {
    Char path[4000];
    for(int i=0; i<3999; i++) path[i] = (i%2 == 0) ? 'a' : '/';
    path[3999] = '\0';

    double start, end;

    start = get_time();
    for(int i=0; i<10000; i++) {
        test_original(path);
    }
    end = get_time();
    double orig_time = end - start;
    printf("Original logic time: %f seconds\n", orig_time);

    start = get_time();
    for(int i=0; i<10000; i++) {
        test_optimized(path);
    }
    end = get_time();
    double opt_time = end - start;
    printf("Optimized logic time: %f seconds\n", opt_time);

    if (orig_time > opt_time) {
        printf("Improvement: %.2f%%\n", (orig_time - opt_time) / orig_time * 100);
    } else {
        printf("Slight performance difference or noise in benchmark.\n");
    }

    return 0;
}
