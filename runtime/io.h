#ifndef META_C_IO_H
#define META_C_IO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char*   data;
    int64_t len;
} MetaCString;

void io_print_int(int64_t val);
void io_print_float(double val);
void io_print_string(const char* data, int64_t len);
void io_print_char(char val);
void io_print_bool(uint8_t val);
void io_print_newline(void);
void io_printf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // META_C_IO_H
     // META_C_IO_H
