#include <stdio.h>
#include <string.h>

int main() {
    char p2[] = "abc";
    char *sp;
    int count = 0;
    while (*p2 && count < 5) {
        printf("p2: %s\n", p2);
        if ((sp = strrchr(p2, '/')) != NULL)
            *sp = '\0';
        count++;
    }
    return 0;
}
