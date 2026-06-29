#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void git_append_status(const char *dir, char *branch, size_t branchsz) {
    char cmd[1024];
    FILE *fp;
    char line[256];
    int staged = 0, modified = 0, untracked = 0;
    int ahead = 0, behind = 0;
    size_t len;
    
    if (strchr(dir, '\'')) return;
    
    snprintf(cmd, sizeof(cmd), "cd '%s' && git status --porcelain -b 2>/dev/null", dir);
    fp = popen(cmd, "r");
    if (!fp) return;
    
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' && line[1] == '#') {
            char *p = strchr(line, '[');
            if (p) {
                char *a = strstr(p, "ahead ");
                char *b = strstr(p, "behind ");
                if (a) ahead = atoi(a + 6);
                if (b) behind = atoi(b + 7);
            }
        } else if (line[0] == '?' && line[1] == '?') {
            untracked++;
        } else {
            if (line[0] != ' ' && line[0] != '?' && line[0] != '#') staged++;
            if (line[1] != ' ' && line[1] != '?' && line[1] != '#') modified++;
        }
    }
    pclose(fp);

    len = strlen(branch);
    if (staged > 0 && len < branchsz - 1) {
        snprintf(branch + len, branchsz - len, " +");
        len = strlen(branch);
    }
    if (modified > 0 && len < branchsz - 1) {
        snprintf(branch + len, branchsz - len, " *");
        len = strlen(branch);
    }
    if (untracked > 0 && len < branchsz - 1) {
        snprintf(branch + len, branchsz - len, " ?");
        len = strlen(branch);
    }
    if (ahead > 0 && len < branchsz - 1) {
        snprintf(branch + len, branchsz - len, " \xE2\x86\x91%d", ahead); /* ↑ */
        len = strlen(branch);
    }
    if (behind > 0 && len < branchsz - 1) {
        snprintf(branch + len, branchsz - len, " \xE2\x86\x93%d", behind); /* ↓ */
        len = strlen(branch);
    }
}

int main() {
    char branch[256] = "main";
    git_append_status(".", branch, sizeof(branch));
    printf("Result: '%s'\n", branch);
    return 0;
}
