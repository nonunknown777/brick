#include "io.h"
#include <stdio.h>
#include <stdarg.h>

void io_print_int(int64_t v) {
    printf("%lld\n", (long long)v);
}

void io_print_float(double v) {
    printf("%f\n", v);
}

void io_print_string(const char* data, int64_t len) {
    printf("%.*s\n", (int)len, data);
}

void io_print_bool(uint8_t v) {
    printf("%s\n", v ? "true" : "false");
}

void io_print_char(char v) {
    printf("%c\n", v);
}

void io_print_newline(void) {
    printf("\n");
}

void io_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
