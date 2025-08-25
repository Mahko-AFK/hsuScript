#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runtime.h"

void hsu_print_cstr(const char *s) {
    if (s) {
        puts(s);
    } else {
        puts("(null)");
    }
}

void hsu_print_int(long n) {
    printf("%ld\n", n);
}

char *hsu_concat(const char *a, const char *b) {
    const char *sa = a ? a : "";
    const char *sb = b ? b : "";
    size_t len = strlen(sa) + strlen(sb) + 1;
    char *res = malloc(len);
    if (!res) return NULL;
    strcpy(res, sa);
    strcat(res, sb);
    return res;
}
