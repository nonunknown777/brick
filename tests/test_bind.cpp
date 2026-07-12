// Test file for C header parser (src/bind/bind.cpp)
// Arquivo de teste para o parser de headers C (src/bind/bind.cpp)
//
// Tests are end-to-end: write a C header to temp file, run brick::bind::generate(),
// and verify the generated .brc code against expected patterns.
//
// Os testes sao ponta-a-ponta: escreve um header C em arquivo temporario,
// executa brick::bind::generate(), e verifica o codigo .brc gerado.

#include "../src/bind/bind.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

using namespace brick::bind;

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")" << std::endl; \
        tests_failed++; \
    } else { \
        std::cout << "  [PASS] " << msg << std::endl; \
        tests_passed++; \
    } \
} while(0)

#define CHECK_CONTAINS(haystack, needle, msg) \
    CHECK(std::string(haystack).find(needle) != std::string::npos, msg)

#define CHECK_NOT_CONTAINS(haystack, needle, msg) \
    CHECK(std::string(haystack).find(needle) == std::string::npos, msg)

// ─── Helpers ────────────────────────────────────────────────────────────────

// Fast: parse directly from source string (no temp file I/O)
static Result generate_from_string(const std::string& c_header, const Options& opts = {}) {
    return generate_from_source(c_header, opts);
}

// ────────────────────────────────────────────────────────────────────────────
//  1. Basic function declarations
// ────────────────────────────────────────────────────────────────────────────

void test_func_no_params() {
    std::cout << "=== test_func_no_params ===\n";
    auto r = generate_from_string("void init(void);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "extern fn init(", "function name");
    CHECK_CONTAINS(r.brc_code, "-> void", "return type void");
}

void test_func_empty_parens() {
    std::cout << "=== test_func_empty_parens ===\n";
    auto r = generate_from_string("int get_value();\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "extern fn get_value(", "function name");
    CHECK_CONTAINS(r.brc_code, "-> i32", "return type i32");
}

void test_func_with_params() {
    std::cout << "=== test_func_with_params ===\n";
    auto r = generate_from_string("int add(int a, int b);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "extern fn add(", "function name");
    CHECK_CONTAINS(r.brc_code, "i32 a", "param a");
    CHECK_CONTAINS(r.brc_code, "i32 b", "param b");
    CHECK_CONTAINS(r.brc_code, "-> i32", "return type");
}

void test_func_float_params() {
    std::cout << "=== test_func_float_params ===\n";
    auto r = generate_from_string("float lerp(float a, float b, float t);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "f32 a", "param a f32");
    CHECK_CONTAINS(r.brc_code, "f32 b", "param b f32");
    CHECK_CONTAINS(r.brc_code, "f32 t", "param t f32");
    CHECK_CONTAINS(r.brc_code, "-> f32", "return f32");
}

void test_func_double_params() {
    std::cout << "=== test_func_double_params ===\n";
    auto r = generate_from_string("double sqrt(double x);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "f64 x", "param x f64");
    CHECK_CONTAINS(r.brc_code, "-> f64", "return f64");
}

void test_func_const_char_ptr() {
    std::cout << "=== test_func_const_char_ptr ===\n";
    auto r = generate_from_string("const char* get_string(int id);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "i32 id", "param id");
    CHECK_CONTAINS(r.brc_code, "-> *u8", "return *u8");
}

void test_func_const_char_ptr_param() {
    std::cout << "=== test_func_const_char_ptr_param ===\n";
    auto r = generate_from_string("void log_info(const char* msg);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "*u8 msg", "param msg *u8");
}

void test_func_variadic() {
    std::cout << "=== test_func_variadic ===\n";
    auto r = generate_from_string("int printf(const char* fmt, ...);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "*u8 fmt", "param fmt");
    CHECK_CONTAINS(r.brc_code, "...", "variadic ...");
    CHECK_CONTAINS(r.brc_code, "-> i32", "return i32");
}

void test_func_void_ptr() {
    std::cout << "=== test_func_void_ptr ===\n";
    auto r = generate_from_string("void* memcpy(void* dst, const void* src, unsigned long n);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "*u8 dst", "dst *u8");
    CHECK_CONTAINS(r.brc_code, "*u8 src", "src *u8");
    CHECK_CONTAINS(r.brc_code, "*u8", "return *u8");
}

void test_func_ptr_ptr() {
    std::cout << "=== test_func_ptr_ptr ===\n";
    auto r = generate_from_string("void set_ptr(int** p);\n");
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "**i32 p", "double pointer param");
}

// ────────────────────────────────────────────────────────────────────────────
//  2. Type mapping verification
// ────────────────────────────────────────────────────────────────────────────

void test_stdint_types() {
    std::cout << "=== test_stdint_types ===\n";
    auto r = generate_from_string(
        "uint8_t a(uint8_t x);\n"
        "int8_t b(int8_t x);\n"
        "uint16_t c(uint16_t x);\n"
        "int16_t d(int16_t x);\n"
        "uint32_t e(uint32_t x);\n"
        "int32_t f(int32_t x);\n"
        "uint64_t g(uint64_t x);\n"
        "int64_t h(int64_t x);\n"
        "size_t i(size_t x);\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "u8 x", "uint8_t → u8");
    CHECK_CONTAINS(r.brc_code, "i8 x", "int8_t → i8");
    CHECK_CONTAINS(r.brc_code, "u16 x", "uint16_t → u16");
    CHECK_CONTAINS(r.brc_code, "i16 x", "int16_t → i16");
    CHECK_CONTAINS(r.brc_code, "u32 x", "uint32_t → u32");
    CHECK_CONTAINS(r.brc_code, "i32 x", "int32_t → i32");
    CHECK_CONTAINS(r.brc_code, "u64 x", "uint64_t → u64");
    CHECK_CONTAINS(r.brc_code, "i64 x", "int64_t → i64");
    CHECK_CONTAINS(r.brc_code, "usize x", "size_t → usize");
}

// ────────────────────────────────────────────────────────────────────────────
//  3. Struct parsing
// ────────────────────────────────────────────────────────────────────────────

void test_struct_simple() {
    std::cout << "=== test_struct_simple ===\n";
    auto r = generate_from_string(
        "typedef struct { int x; int y; } Point;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "@packed struct Point {", "struct keyword");
    CHECK_CONTAINS(r.brc_code, "i32 x", "field x");
    CHECK_CONTAINS(r.brc_code, "i32 y", "field y");
}

void test_struct_named() {
    std::cout << "=== test_struct_named ===\n";
    auto r = generate_from_string(
        "typedef struct Vertex { float x; float y; float z; } Vertex;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "struct Vertex", "named struct");
    CHECK_CONTAINS(r.brc_code, "f32 x", "field x");
    CHECK_CONTAINS(r.brc_code, "f32 y", "field y");
    CHECK_CONTAINS(r.brc_code, "f32 z", "field z");
}

void test_struct_mixed_types() {
    std::cout << "=== test_struct_mixed_types ===\n";
    auto r = generate_from_string(
        "typedef struct { int id; float value; char flag; double weight; } Item;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "i32 id", "field id");
    CHECK_CONTAINS(r.brc_code, "f32 value", "field value");
    CHECK_CONTAINS(r.brc_code, "i8 flag", "field flag");
    CHECK_CONTAINS(r.brc_code, "f64 weight", "field weight");
}

void test_struct_forward_decl() {
    std::cout << "=== test_struct_forward_decl ===\n";
    auto r = generate_from_string(
        "typedef struct GLFWwindow GLFWwindow;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "@packed struct GLFWwindow {}", "forward decl as empty struct");
}

void test_struct_uint32_fields() {
    std::cout << "=== test_struct_uint32_fields ===\n";
    auto r = generate_from_string(
        "typedef struct { uint32_t flags; uint32_t color; } Config;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "u32 flags", "field flags");
    CHECK_CONTAINS(r.brc_code, "u32 color", "field color");
}

void test_struct_const_fields() {
    std::cout << "=== test_struct_const_fields ===\n";
    auto r = generate_from_string(
        "typedef struct { const int id; const char* name; } Entry;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "i32 id", "const int field");
    CHECK_CONTAINS(r.brc_code, "*u8 name", "const char* field");
}

void test_struct_bitfields() {
    std::cout << "=== test_struct_bitfields ===\n";
    auto r = generate_from_string(
        "typedef struct { int a : 3; int b : 5; unsigned int c : 1; } Flags;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "struct Flags", "struct name");
    CHECK_CONTAINS(r.brc_code, "i32 a", "field a");
    CHECK_CONTAINS(r.brc_code, "i32 b", "field b");
    CHECK_CONTAINS(r.brc_code, "u32 c", "field c (unsigned)");
}

// ────────────────────────────────────────────────────────────────────────────
//  4. Enum parsing
// ────────────────────────────────────────────────────────────────────────────

void test_enum_simple() {
    std::cout << "=== test_enum_simple ===\n";
    auto r = generate_from_string(
        "typedef enum { NONE = 0, READ = 1, WRITE = 2 } Perms;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "enum Perms", "enum keyword");
    CHECK_CONTAINS(r.brc_code, "NONE = 0", "value NONE=0");
    CHECK_CONTAINS(r.brc_code, "READ = 1", "value READ=1");
    CHECK_CONTAINS(r.brc_code, "WRITE = 2", "value WRITE=2");
}

void test_enum_hex_values() {
    std::cout << "=== test_enum_hex_values ===\n";
    auto r = generate_from_string(
        "typedef enum { NONE = 0, READ = 0x01, WRITE = 0x02, EXEC = 0x04 } Flags;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "enum Flags", "enum keyword");
    CHECK_CONTAINS(r.brc_code, "READ = 0x01", "hex value no double 0x");
    CHECK_CONTAINS(r.brc_code, "WRITE = 0x02", "hex value 0x02");
    CHECK_CONTAINS(r.brc_code, "EXEC = 0x04", "hex value 0x04");
}

void test_enum_auto_increment() {
    std::cout << "=== test_enum_auto_increment ===\n";
    auto r = generate_from_string(
        "typedef enum { A, B, C } Alphabet;\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "A = 0", "auto inc A=0");
    CHECK_CONTAINS(r.brc_code, "B = 1", "auto inc B=1");
    CHECK_CONTAINS(r.brc_code, "C = 2", "auto inc C=2");
}

// ────────────────────────────────────────────────────────────────────────────
//  5. Define parsing
// ────────────────────────────────────────────────────────────────────────────

void test_define_decimal() {
    std::cout << "=== test_define_decimal ===\n";
    auto r = generate_from_string(
        "#define MAX_COUNT 100\n"
        "void test(void);\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "const MAX_COUNT = 100", "decimal const");
}

void test_define_hex() {
    std::cout << "=== test_define_hex ===\n";
    auto r = generate_from_string(
        "#define DEFAULT_COLOR 0xFF00FF\n"
        "void test(void);\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "const DEFAULT_COLOR = 0xFF00FF", "hex const");
}

void test_define_parenthesized_hex() {
    std::cout << "=== test_define_parenthesized_hex ===\n";
    auto r = generate_from_string(
        "#define BG_COLOR (0x443355FF)\n"
        "void test(void);\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "const BG_COLOR = 0x443355FF", "parenthesized hex const");
}

void test_define_bit_shift() {
    std::cout << "=== test_define_bit_shift ===\n";
    auto r = generate_from_string(
        "#define FLAG_BIT (1ULL << 4)\n"
        "void test(void);\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "const FLAG_BIT = 1", "bit-shift extracts left operand");
}

// ────────────────────────────────────────────────────────────────────────────
//  6. Combined / complex headers
// ────────────────────────────────────────────────────────────────────────────

void test_complex_header() {
    std::cout << "=== test_complex_header ===\n";
    std::string header = R"(
#include <stdint.h>

typedef struct { int x; int y; } Point;

typedef enum { NONE = 0, READ = 0x01, WRITE = 0x02, EXEC = 0x04 } Perms;

typedef struct GLFWwindow GLFWwindow;

void init(void);
int add(int a, int b);
const char* get_string(int id);
GLFWwindow* create_window(int w, int h, const char* title);
void variadic_func(const char* fmt, ...);
uint32_t get_flags(void);

#define MAX_COUNT 100
#define DEFAULT_COLOR 0xFF00FF
#define FLAG_READ 0x01
)";

    auto r = generate_from_string(header);
    CHECK(r.success, "complex header parse success");

    // Check struct
    CHECK_CONTAINS(r.brc_code, "struct Point {", "struct Point");
    CHECK_CONTAINS(r.brc_code, "i32 x", "Point.x");
    CHECK_CONTAINS(r.brc_code, "i32 y", "Point.y");

    // Check enum (no double 0x)
    CHECK_CONTAINS(r.brc_code, "READ = 0x01", "enum READ");
    CHECK_CONTAINS(r.brc_code, "WRITE = 0x02", "enum WRITE");

    // Check forward decl
    CHECK_CONTAINS(r.brc_code, "@packed struct GLFWwindow {}", "forward decl");

    // Check functions
    CHECK_CONTAINS(r.brc_code, "extern fn init(", "func init");
    CHECK_CONTAINS(r.brc_code, "extern fn add(", "func add");
    CHECK_CONTAINS(r.brc_code, "extern fn get_string(", "func get_string");
    CHECK_CONTAINS(r.brc_code, "extern fn create_window(", "func create_window");
    CHECK_CONTAINS(r.brc_code, "extern fn variadic_func(", "func variadic_func");
    CHECK_CONTAINS(r.brc_code, "extern fn get_flags(", "func get_flags");

    // Check pointer return
    CHECK_CONTAINS(r.brc_code, "-> *u8", "return *u8 (const char*)");
    CHECK_CONTAINS(r.brc_code, "-> *GLFWwindow", "return *GLFWwindow");

    // Check variadic
    CHECK_CONTAINS(r.brc_code, "...", "variadic ...");

    // Check params
    CHECK_CONTAINS(r.brc_code, "*u8 title", "param title *u8");

    // Check defines
    CHECK_CONTAINS(r.brc_code, "const MAX_COUNT = 100", "define MAX_COUNT");
    CHECK_CONTAINS(r.brc_code, "const DEFAULT_COLOR = 0xFF00FF", "define DEFAULT_COLOR");
    CHECK_CONTAINS(r.brc_code, "const FLAG_READ = 0x1", "define FLAG_READ");
}

void test_glfw_style_header() {
    std::cout << "=== test_glfw_style_header ===\n";
    std::string header = R"(
typedef struct GLFWwindow GLFWwindow;
typedef struct GLFWmonitor GLFWmonitor;
typedef struct GLFWcursor GLFWcursor;

GLFWwindow* glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share);
void glfwDestroyWindow(GLFWwindow* window);
void glfwMakeContextCurrent(GLFWwindow* window);
int glfwWindowShouldClose(GLFWwindow* window);
void glfwSwapBuffers(GLFWwindow* window);
void glfwPollEvents(void);
double glfwGetTime(void);
void glfwSetTime(double time);
const char* glfwGetVersionString(void);
)";

    auto r = generate_from_string(header);
    CHECK(r.success, "glfw-style header parse success");

    // Check forward decls
    CHECK_CONTAINS(r.brc_code, "@packed struct GLFWwindow {}", "GLFWwindow fwd");
    CHECK_CONTAINS(r.brc_code, "@packed struct GLFWmonitor {}", "GLFWmonitor fwd");
    CHECK_CONTAINS(r.brc_code, "@packed struct GLFWcursor {}", "GLFWcursor fwd");

    // Check functions
    CHECK_CONTAINS(r.brc_code, "glfwCreateWindow(", "glfwCreateWindow");
    CHECK_CONTAINS(r.brc_code, "glfwDestroyWindow(", "glfwDestroyWindow");
    CHECK_CONTAINS(r.brc_code, "glfwSwapBuffers(", "glfwSwapBuffers");
    CHECK_CONTAINS(r.brc_code, "glfwGetTime(", "glfwGetTime");
    CHECK_CONTAINS(r.brc_code, "glfwGetVersionString(", "glfwGetVersionString");

    // Check *u8 for const char*
    CHECK_CONTAINS(r.brc_code, "*u8 title", "title as *u8");
    CHECK_CONTAINS(r.brc_code, "-> *u8", "return *u8");

    // Check struct pointer params
    CHECK_CONTAINS(r.brc_code, "*GLFWmonitor monitor", "GLFWmonitor* param");
    CHECK_CONTAINS(r.brc_code, "*GLFWwindow share", "GLFWwindow* param");
    CHECK_CONTAINS(r.brc_code, "*GLFWwindow window", "window param");
    CHECK_CONTAINS(r.brc_code, "f64 time", "time f64 param");

    // Check return types
    CHECK_CONTAINS(r.brc_code, "-> *GLFWwindow", "return *GLFWwindow");
    CHECK_CONTAINS(r.brc_code, "-> f64", "return f64");
    CHECK_CONTAINS(r.brc_code, "-> i32", "return i32");
    CHECK_CONTAINS(r.brc_code, "-> void", "return void");
}

// ────────────────────────────────────────────────────────────────────────────
//  7. Edge cases
// ────────────────────────────────────────────────────────────────────────────

void test_empty_header() {
    std::cout << "=== test_empty_header ===\n";
    auto r = generate_from_string("// just a comment\n");
    CHECK(r.success, "empty header parse success");
    // Should have header comment + include, no functions/structs/enums
    CHECK_CONTAINS(r.brc_code, "Auto-generated Brick bindings", "has header comment");
}

void test_header_with_only_includes() {
    std::cout << "=== test_header_with_only_includes ===\n";
    auto r = generate_from_string("#include <stdio.h>\n#include \"myheader.h\"\n");
    CHECK(r.success, "only includes");
    CHECK_CONTAINS(r.brc_code, "Auto-generated Brick bindings", "has output");
}

void test_func_with_macro_decorator() {
    std::cout << "=== test_func_with_macro_decorator ===\n";
    auto r = generate_from_string(
        "GLFWAPI void glfwInit(void);\n"
        "GLFWAPI void glfwTerminate(void);\n"
    );
    CHECK(r.success, "macro decorator parse");
    CHECK_CONTAINS(r.brc_code, "glfwInit(", "glfwInit");
    CHECK_CONTAINS(r.brc_code, "glfwTerminate(", "glfwTerminate");
}

void test_func_unnamed_params() {
    std::cout << "=== test_func_unnamed_params ===\n";
    auto r = generate_from_string(
        "void draw(int, float);\n"
        "void set(int, int, int);\n"
    );
    CHECK(r.success, "unnamed params parse");
    CHECK_CONTAINS(r.brc_code, "draw(", "draw function");
    CHECK_CONTAINS(r.brc_code, "set(", "set function");
    // Should have i32 and f32 params without names
    CHECK_CONTAINS(r.brc_code, "i32", "i32 type");
    CHECK_CONTAINS(r.brc_code, "f32", "f32 type");
}

void test_unsigned_int_param() {
    std::cout << "=== test_unsigned_int_param ===\n";
    auto r = generate_from_string("void wait(unsigned int ms);\n");
    CHECK(r.success, "unsigned int param");
    CHECK_CONTAINS(r.brc_code, "u32 ms", "unsigned int → u32");
}

void test_unsigned_long_param() {
    std::cout << "=== test_unsigned_long_param ===\n";
    auto r = generate_from_string("unsigned long strlen(const char* s);\n");
    CHECK(r.success, "unsigned long return");
    CHECK_CONTAINS(r.brc_code, "-> u64", "unsigned long → u64");
}

void test_short_param() {
    std::cout << "=== test_short_param ===\n";
    auto r = generate_from_string("short get_value(short v);\n");
    CHECK(r.success, "short param");
    CHECK_CONTAINS(r.brc_code, "i16 v", "short → i16");
}

void test_long_param() {
    std::cout << "=== test_long_param ===\n";
    auto r = generate_from_string("long get_value(long v);\n");
    CHECK(r.success, "long param");
    CHECK_CONTAINS(r.brc_code, "i64 v", "long → i64");
}

void test_unsigned_long_func() {
    std::cout << "=== test_unsigned_long_func ===\n";
    // 'unsigned long' as return type: compound type without 'int' keyword
    auto r = generate_from_string(
        "unsigned long strlen(const char* s);\n"
        "unsigned long long get_big();\n"
        "unsigned short get_word();\n"
        "long long get_llong();\n"
    );
    CHECK(r.success, "parse success");
    CHECK_CONTAINS(r.brc_code, "strlen(", "strlen function");
    CHECK_CONTAINS(r.brc_code, "get_big(", "get_big function");
    CHECK_CONTAINS(r.brc_code, "get_word(", "get_word function");
    CHECK_CONTAINS(r.brc_code, "get_llong(", "get_llong function");
    CHECK_CONTAINS(r.brc_code, "-> u64", "unsigned long -> u64");
}

void test_mixed_qualifiers() {
    std::cout << "=== test_mixed_qualifiers ===\n";
    auto r = generate_from_string(
        "extern void reset(volatile int* ptr);\n"
    );
    CHECK(r.success, "volatile ptr param");
    CHECK_CONTAINS(r.brc_code, "*i32 ptr", "volatile stripped, ptr remains");
}

void test_const_int_ptr() {
    std::cout << "=== test_const_int_ptr ===\n";
    auto r = generate_from_string(
        "void read_only(const int* buffer);\n"
    );
    CHECK(r.success, "const int* param");
    CHECK_CONTAINS(r.brc_code, "*i32 buffer", "const int* → *i32");
}

void test_multiple_typedef_names() {
    std::cout << "=== test_multiple_typedef_names ===\n";
    auto r = generate_from_string(
        "typedef struct { int id; } Item, Item_t;\n"
    );
    CHECK(r.success, "multiple typedef names");
    CHECK_CONTAINS(r.brc_code, "struct Item", "first typedef name used");
}

void test_enum_typedef_with_name_and_alias() {
    std::cout << "=== test_enum_typedef_with_name_and_alias ===\n";
    auto r = generate_from_string(
        "typedef enum Color { RED = 1, GREEN = 2, BLUE = 4 } Color_t;\n"
    );
    CHECK(r.success, "named enum with typedef alias");
    CHECK_CONTAINS(r.brc_code, "RED = 1", "RED value");
    CHECK_CONTAINS(r.brc_code, "GREEN = 2", "GREEN value");
    CHECK_CONTAINS(r.brc_code, "BLUE = 4", "BLUE value");
    // Either Color or Color_t is the name
    bool has_color = r.brc_code.find("enum Color") != std::string::npos ||
                     r.brc_code.find("enum Color_t") != std::string::npos;
    CHECK(has_color, "enum name present");
}

void test_nested_comments() {
    std::cout << "=== test_nested_comments ===\n";
    auto r = generate_from_string(
        "/* block comment */ int get_value(void);\n"
        "// line comment\n"
        "float get_ratio(void);\n"
    );
    CHECK(r.success, "comments handled");
    CHECK_CONTAINS(r.brc_code, "get_value(", "get_value");
    CHECK_CONTAINS(r.brc_code, "get_ratio(", "get_ratio");
}

void test_bool_type() {
    std::cout << "=== test_bool_type ===\n";
    auto r = generate_from_string(
        "#include <stdbool.h>\n"
        "bool is_ready(void);\n"
    );
    CHECK(r.success, "bool type");
    CHECK_CONTAINS(r.brc_code, "-> bool", "return bool");
}

// ────────────────────────────────────────────────────────────────────────────
//  8. Generator output verification
// ────────────────────────────────────────────────────────────────────────────

void test_generator_include_directive() {
    std::cout << "=== test_generator_include_directive ===\n";
    auto r = generate_from_string("void test(void);\n");
    CHECK(r.success, "parse success");
    // Should have include directive with a path
    CHECK_CONTAINS(r.brc_code, "include \"", "include directive");
    CHECK_CONTAINS(r.brc_code, "include \"source.brc\"", "include path matches source");
}

void test_generator_sections_order() {
    std::cout << "=== test_generator_sections_order ===\n";
    auto r = generate_from_string(
        "typedef struct { int x; } Point;\n"
        "typedef enum { A = 1 } MyEnum;\n"
        "#define MAX 100\n"
        "void test(void);\n"
    );
    CHECK(r.success, "parse success");
    // Sections should appear in order: structs, enums, constants, functions
    size_t pos_struct = r.brc_code.find("struct Point");
    size_t pos_enum = r.brc_code.find("enum MyEnum");
    size_t pos_const = r.brc_code.find("const MAX");
    size_t pos_func = r.brc_code.find("test(");
    CHECK(pos_struct != std::string::npos, "struct section present");
    CHECK(pos_enum != std::string::npos, "enum section present");
    CHECK(pos_const != std::string::npos, "constants section present");
    CHECK(pos_func != std::string::npos, "functions section present");
    CHECK(pos_struct < pos_enum, "struct before enum");
    CHECK(pos_enum < pos_func, "enum before function");
}

void test_generator_dedup() {
    std::cout << "=== test_generator_dedup ===\n";
    auto r = generate_from_string(
        "void test(void);\n"
        "void test(void);\n"
        "typedef struct { int x; } Point;\n"
        "typedef struct { int x; } Point;\n"
    );
    CHECK(r.success, "parse success");
    // Should only appear once each
    size_t first = r.brc_code.find("extern fn test(");
    size_t second = r.brc_code.find("extern fn test(", first + 1);
    CHECK(second == std::string::npos, "functions deduplicated");
    first = r.brc_code.find("struct Point");
    second = r.brc_code.find("struct Point", first + 1);
    CHECK(second == std::string::npos, "structs deduplicated");
}

// ────────────────────────────────────────────────────────────────────────────
//  9. Selective generation (Options)
// ────────────────────────────────────────────────────────────────────────────

void test_opts_no_functions() {
    std::cout << "=== test_opts_no_functions ===\n";
    Options opts;
    opts.generate_functions = false;
    auto r = generate_from_string("void test(void);\n", opts);
    CHECK(r.success, "parse success");
    CHECK_NOT_CONTAINS(r.brc_code, "extern fn test", "functions suppressed");
}

void test_opts_no_structs() {
    std::cout << "=== test_opts_no_structs ===\n";
    Options opts;
    opts.generate_structs = false;
    auto r = generate_from_string(
        "typedef struct { int x; } Point;\n"
        "void test(void);\n", opts
    );
    CHECK(r.success, "parse success");
    CHECK_NOT_CONTAINS(r.brc_code, "struct Point", "structs suppressed");
    CHECK_CONTAINS(r.brc_code, "test(", "functions still generated");
}

void test_opts_no_enums() {
    std::cout << "=== test_opts_no_enums ===\n";
    Options opts;
    opts.generate_enums = false;
    auto r = generate_from_string(
        "typedef enum { A = 1 } MyEnum;\n"
        "void test(void);\n", opts
    );
    CHECK(r.success, "parse success");
    CHECK_NOT_CONTAINS(r.brc_code, "enum MyEnum", "enums suppressed");
    CHECK_CONTAINS(r.brc_code, "test(", "functions still generated");
}

void test_opts_no_defines() {
    std::cout << "=== test_opts_no_defines ===\n";
    Options opts;
    opts.generate_defines = false;
    auto r = generate_from_string(
        "#define MAX 100\n"
        "void test(void);\n", opts
    );
    CHECK(r.success, "parse success");
    CHECK_NOT_CONTAINS(r.brc_code, "const MAX", "defines suppressed");
    CHECK_CONTAINS(r.brc_code, "test(", "functions still generated");
}

// ────────────────────────────────────────────────────────────────────────────
//  main
// ────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== Brick Bind Tests ===\n\n";

    std::cout << "--- Basic Functions ---\n";
    test_func_no_params();
    test_func_empty_parens();
    test_func_with_params();
    test_func_float_params();
    test_func_double_params();
    test_func_const_char_ptr();
    test_func_const_char_ptr_param();
    test_func_variadic();
    test_func_void_ptr();
    test_func_ptr_ptr();

    std::cout << "\n--- Type Mapping ---\n";
    test_stdint_types();

    std::cout << "\n--- Structs ---\n";
    test_struct_simple();
    test_struct_named();
    test_struct_mixed_types();
    test_struct_forward_decl();
    test_struct_uint32_fields();
    test_struct_const_fields();
    test_struct_bitfields();

    std::cout << "\n--- Enums ---\n";
    test_enum_simple();
    test_enum_hex_values();
    test_enum_auto_increment();

    std::cout << "\n--- Defines ---\n";
    test_define_decimal();
    test_define_hex();
    test_define_parenthesized_hex();
    test_define_bit_shift();

    std::cout << "\n--- Complex Headers ---\n";
    test_complex_header();
    test_glfw_style_header();

    std::cout << "\n--- Edge Cases ---\n";
    test_empty_header();
    test_header_with_only_includes();
    test_func_with_macro_decorator();
    test_func_unnamed_params();
    test_unsigned_int_param();
    test_unsigned_long_param();
    test_unsigned_long_func();
    test_short_param();
    test_long_param();
    test_mixed_qualifiers();
    test_const_int_ptr();
    test_multiple_typedef_names();
    test_enum_typedef_with_name_and_alias();
    test_nested_comments();
    test_bool_type();

    std::cout << "\n--- Generator Output ---\n";
    test_generator_include_directive();
    test_generator_sections_order();
    test_generator_dedup();

    std::cout << "\n--- Options ---\n";
    test_opts_no_functions();
    test_opts_no_structs();
    test_opts_no_enums();
    test_opts_no_defines();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
