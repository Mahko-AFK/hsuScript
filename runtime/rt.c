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
    size_t la = a ? strlen(a) : 0;
    size_t lb = b ? strlen(b) : 0;
    char *res = malloc(la + lb + 1);
    if (!res) return NULL;
    if (a) memcpy(res, a, la);
    if (b) memcpy(res + la, b, lb);
    res[la + lb] = '\0';
    return res;
}
