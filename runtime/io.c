#include "io.h"
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

// ─── Type-specific integer prints ─────────────────────────────
// ─── Prints de inteiro especificos por tipo ────────────────────

void io_print_u8(uint8_t v)     { printf("%" PRIu8 "\n", v); }
void io_print_u16(uint16_t v)   { printf("%" PRIu16 "\n", v); }
void io_print_u32(uint32_t v)   { printf("%" PRIu32 "\n", v); }
void io_print_u64(uint64_t v)   { printf("%" PRIu64 "\n", v); }
void io_print_i8(int8_t v)      { printf("%" PRId8 "\n", v); }
void io_print_i16(int16_t v)    { printf("%" PRId16 "\n", v); }
void io_print_i32(int32_t v)    { printf("%" PRId32 "\n", v); }
void io_print_i64(int64_t v)    { printf("%" PRId64 "\n", v); }
void io_print_usize(size_t v)   { printf("%zu\n", v); }
void io_print_isize(ptrdiff_t v){ printf("%td\n", v); }

// ─── Type-specific float prints ───────────────────────────────
// ─── Prints de ponto flutuante especificos por tipo ────────────

void io_print_f32(float v)      { printf("%f\n", (double)v); }
void io_print_f64(double v)     { printf("%f\n", v); }

// ─── Generic prints (kept for backward compat) ────────────────
// ─── Prints genericos (mantidos pra compatibilidade) ───────────

void io_print_int(int64_t v)    { io_print_i64(v); }
void io_print_float(double v)   { io_print_f64(v); }

void io_print_string(const char* data, size_t len) {
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
