#include <stdio.h>

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

