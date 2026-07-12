#include "bind.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <stack>

namespace brick {
namespace bind {

// ═════════════════════════════════════════════════════════════════════════════
// Lexer — C Tokenizer
// ═════════════════════════════════════════════════════════════════════════════

enum class CTokenType {
    IDENTIFIER,
    NUMBER,
    STRING_LITERAL,
    CHAR_LITERAL,
    PUNCTUATION,
    PREPROCESSOR,   // line starting with #
    ELLIPSIS,       // ...
    END
};

struct CToken {
    CTokenType type;
    std::string text;
    int line = 0;
};

struct CLexer {
    const std::string& src;
    size_t pos = 0;
    int line = 1;
    int saved_line = 1;

    CLexer(const std::string& s) : src(s) {}

    char peek() {
        return pos < src.size() ? src[pos] : '\0';
    }

    char advance() {
        if (pos < src.size()) {
            char c = src[pos++];
            if (c == '\n') line++;
            return c;
        }
        return '\0';
    }

    void skip_line_comment() {
        while (pos < src.size() && src[pos] != '\n') advance();
    }

    void skip_block_comment() {
        while (pos + 1 < src.size()) {
            if (src[pos] == '*' && src[pos + 1] == '/') {
                pos += 2;
                return;
            }
            advance();
        }
        // Unterminated comment — just stop
    }

    void skip_whitespace() {
        while (pos < src.size()) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                advance();
            } else if (c == '/' && pos + 1 < src.size()) {
                if (src[pos + 1] == '/') {
                    skip_line_comment();
                } else if (src[pos + 1] == '*') {
                    pos += 2; // skip /*
                    skip_block_comment();
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }

    // Check if we're looking at a preprocessor directive (# at start of line)
    bool is_preprocessor() {
        size_t p = pos;
        while (p < src.size() && (src[p] == ' ' || src[p] == '\t')) p++;
        return p < src.size() && src[p] == '#';
    }

    CToken next() {
        skip_whitespace();
        if (pos >= src.size()) return {CTokenType::END, "", line};

        char c = src[pos];
        saved_line = line;

        // Preprocessor directive
        if (c == '#' && is_preprocessor()) {
            std::string text;
            while (pos < src.size() && src[pos] != '\n') {
                // Handle line continuation (\ at end of line)
                if (src[pos] == '\\' && pos + 1 < src.size() && src[pos + 1] == '\n') {
                    text += advance(); // backslash
                    text += advance(); // newline
                } else {
                    text += advance();
                }
            }
            return {CTokenType::PREPROCESSOR, text, saved_line};
        }

        // String literal
        if (c == '"') {
            advance(); // skip opening "
            std::string text;
            text += '"';
            while (pos < src.size()) {
                char c2 = advance();
                text += c2;
                if (c2 == '\\' && pos < src.size()) {
                    text += advance(); // escaped char
                } else if (c2 == '"') {
                    break;
                }
            }
            return {CTokenType::STRING_LITERAL, text, saved_line};
        }

        // Char literal
        if (c == '\'') {
            advance(); // skip opening '
            std::string text;
            text += '\'';
            while (pos < src.size()) {
                char c2 = advance();
                text += c2;
                if (c2 == '\\' && pos < src.size()) {
                    text += advance();
                } else if (c2 == '\'') {
                    break;
                }
            }
            return {CTokenType::CHAR_LITERAL, text, saved_line};
        }

        // Ellipsis ...
        if (c == '.' && pos + 2 < src.size() &&
            src[pos + 1] == '.' && src[pos + 2] == '.') {
            pos += 3;
            return {CTokenType::ELLIPSIS, "...", saved_line};
        }

        // Punctuation & operators
        static const char* punct_chars = "(){}[];,:=<>!|&~^%?+-*/.";
        if (std::strchr(punct_chars, c)) {
            std::string text;
            text += advance();

            // Two-character operators: <= >= == != && || << >> += -= *= /= -> // etc.
            if (pos < src.size()) {
                char n = src[pos];
                if ((c == '>' && n == '=') ||
                    (c == '<' && n == '=') ||
                    (c == '=' && n == '=') ||
                    (c == '!' && n == '=') ||
                    (c == '&' && n == '&') ||
                    (c == '|' && n == '|') ||
                    (c == '+' && n == '=') ||
                    (c == '-' && n == '=') ||
                    (c == '*' && n == '=') ||
                    (c == '/' && n == '=') ||
                    (c == '<' && n == '<') ||
                    (c == '>' && n == '>') ||
                    (c == '-' && n == '>') ||
                    (c == ':' && n == '>') ||
                    (c == '+' && n == '+') ||
                    (c == '-' && n == '-')) {
                    text += advance();
                }
            }
            return {CTokenType::PUNCTUATION, text, saved_line};
        }

        // Number: hex (0x...), octal (0...), binary (0b...), decimal, float
        if (std::isdigit(c) || (c == '.' && pos + 1 < src.size() && std::isdigit(src[pos + 1]))) {
            std::string text;

            // Detect hex 0x or 0X
            if (c == '0' && pos + 1 < src.size() && (src[pos + 1] == 'x' || src[pos + 1] == 'X')) {
                text += advance(); // 0
                text += advance(); // x
                while (pos < src.size() && (std::isxdigit(src[pos]) || src[pos] == '\'')) {
                    if (src[pos] != '\'') text += advance();
                    else advance();
                }
            } else {
                while (pos < src.size()) {
                    char nc = src[pos];
                    if (std::isdigit(nc) || nc == '.' || nc == 'e' || nc == 'E' ||
                        nc == 'p' || nc == 'P' || nc == 'x' || nc == 'X' ||
                        nc == '+' || nc == '-' || nc == '\'') {
                        if (nc == '\'') { advance(); continue; } // digit separator
                        text += advance();
                    } else if (nc == 'u' || nc == 'U' || nc == 'l' || nc == 'L' ||
                               nc == 'f' || nc == 'F') {
                        text += advance();
                    } else {
                        break;
                    }
                }
            }
            return {CTokenType::NUMBER, text, saved_line};
        }

        // Identifier or keyword
        if (std::isalpha(c) || c == '_') {
            std::string text;
            while (pos < src.size() && (std::isalnum(src[pos]) || src[pos] == '_')) {
                text += advance();
            }
            return {CTokenType::IDENTIFIER, text, saved_line};
        }

        // Unknown character — just skip
        advance();
        return next();
    }

    // Peek at the next token without consuming it
    CToken peek_token() {
        size_t saved_pos = pos;
        int saved_line = line;
        CToken tok = next();
        pos = saved_pos;
        line = saved_line;
        return tok;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// Type System
// ═════════════════════════════════════════════════════════════════════════════

// A parsed C type (e.g. "const char *" → {base="char", ptr_depth=1, is_const=true})
struct CType {
    std::string base_name;    // e.g. "char", "int", "GLFWwindow", "uint32_t"
    int ptr_depth = 0;        // number of * levels
    bool is_const = false;
    bool is_unsigned = false;
    bool is_volatile = false;

    // Check if this is a "void" type (function returns void)
    bool is_void() const {
        return base_name == "void" && ptr_depth == 0;
    }

    // Check if this is a void pointer (void*)
    bool is_void_ptr() const {
        return base_name == "void" && ptr_depth > 0;
    }

    // Convert to a string representation for the Brick extern fn mapping
    std::string to_brick_type() const {
        // void returns void (no pointer)
        if (base_name == "void") {
            if (ptr_depth > 0) {
                // void* -> *u8 (generic data pointer as u8 pointer)
                return "*u8";
            }
            return "void";
        }

        // Map C base type to Brick type
        std::string brick_base = map_to_brick_base(base_name, is_unsigned);

        // Special case: char* (C string) → *u8, not *i8
        // This matches Brick convention where *u8 becomes char* in generated C
        if (base_name == "char" && ptr_depth > 0 && !is_unsigned) {
            brick_base = "u8";
        }

        // Apply pointer depth
        std::string result = brick_base;
        for (int i = 0; i < ptr_depth; i++) {
            result = "*" + result;
        }
        return result;
    }

    // Map C identifier base name to Brick brick
    static std::string map_to_brick_base(const std::string& cname, bool is_unsigned) {
        // Fixed-width stdint types (most common in bgfx/GLFW)
        static const std::unordered_map<std::string, std::string> stdint_map = {
            {"int8_t", "i8"}, {"uint8_t", "u8"},
            {"int16_t", "i16"}, {"uint16_t", "u16"},
            {"int32_t", "i32"}, {"uint32_t", "u32"},
            {"int64_t", "i64"}, {"uint64_t", "u64"},
            {"size_t", "usize"}, {"ssize_t", "isize"},
            {"ptrdiff_t", "isize"}, {"intptr_t", "isize"},
            {"uintptr_t", "usize"},
            {"float", "f32"}, {"double", "f64"},
            {"char", "i8"},
            {"short", "i16"}, {"int", "i32"}, {"long", "i64"},
            {"bool", "bool"}, {"_Bool", "bool"},
            {"FILE", "u8"}, // FILE* -> *u8
            {"va_list", "u8"},
        };

        // Check stdint types first
        auto it = stdint_map.find(cname);
        if (it != stdint_map.end()) {
            if (is_unsigned) {
                // Handle unsigned variants of signed types
                if (it->second == "i8") return "u8";
                if (it->second == "i16") return "u16";
                if (it->second == "i32") return "u32";
                if (it->second == "i64") return "u64";
            }
            return it->second;
        }

        // "unsigned" alone without suffix means unsigned int -> u32
        if (cname.empty() || cname == "unsigned") {
            return is_unsigned ? "u32" : "i32";
        }

        // If it's a struct name, enum name, or other identifier, pass through
        // These become Brick struct names, which map to themselves
        return cname;
    }

    // Get the base C type name without const/volatile, for struct detection
    std::string get_base_c_name() const {
        return base_name;
    }
};

// Parse a complete C type from tokens.
// Handles: const, volatile, unsigned, signed, *, and compound names like "unsigned long long"
// Returns the parsed type and updates the tokens iterator.
struct TypeParseResult {
    CType type;
    bool success = false;
};

// ═════════════════════════════════════════════════════════════════════════════
//  C Parser — extracts declarations from a C header
// ═════════════════════════════════════════════════════════════════════════════

struct ParamInfo {
    CType type;
    std::string name;
};

struct FunctionInfo {
    CType return_type;
    std::string name;
    std::vector<ParamInfo> params;
    bool is_variadic = false;
};

struct StructField {
    CType type;
    std::string name;
    int bit_width = 0; // 0 means not a bitfield
};

struct StructInfo {
    std::string name;
    std::vector<StructField> fields;
    bool is_forward_decl = false; // just "typedef struct Name Name;" (no body)
};

struct EnumValue {
    std::string name;
    std::string value_str; // string representation (could be hex, decimal, expression)
    bool is_hex = false;
};

struct EnumInfo {
    std::string name;
    std::vector<EnumValue> values;
};

struct DefineInfo {
    std::string name;
    std::string value_str;
    bool is_numeric = false;
    int64_t numeric_value = 0;
    bool is_hex = false;
};

class CBindParser {
public:
    CBindParser(const std::string& source)
        : lexer(source) {}

    // Parse the entire header and collect declarations
    void parse_all() {
        while (true) {
            skip_to_next_declaration();
            auto tok = lexer.next();
            if (tok.type == CTokenType::END) break;
            if (tok.type == CTokenType::PREPROCESSOR) {
                handle_preprocessor(tok.text);
                continue;
            }
            lexer.pos -= (tok.text.size()); // put it back, handle_declaration will re-read
            lexer.line = tok.line;
            handle_declaration();
        }
    }

    // Getters for parsed results
    const std::vector<FunctionInfo>& get_functions() const { return functions_; }
    const std::vector<StructInfo>& get_structs() const { return structs_; }
    const std::vector<EnumInfo>& get_enums() const { return enums_; }
    const std::vector<DefineInfo>& get_defines() const { return defines_; }

private:
    CLexer lexer;
    std::vector<FunctionInfo> functions_;
    std::vector<StructInfo> structs_;
    std::vector<EnumInfo> enums_;
    std::vector<DefineInfo> defines_;

    // Forward declarations we've seen (to avoid duplicate struct emission)
    std::unordered_set<std::string> forward_declared_structs_;

    // Skip tokens until we find something that looks like a declaration start
    void skip_to_next_declaration() {
        while (true) {
            auto tok = lexer.next();
            if (tok.type == CTokenType::END) return;
            if (tok.type == CTokenType::PREPROCESSOR) {
                handle_preprocessor(tok.text);
                continue;
            }
            // Found a potential declaration start — put it back
            lexer.pos -= tok.text.size();
            lexer.line = tok.line;
            return;
        }
    }

    // Handle a preprocessor directive (#include, #define, #if, etc.)
    void handle_preprocessor(const std::string& text) {
        // Trim leading #
        std::string line = text;
        // Remove leading whitespace and #
        size_t start = 0;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) start++;
        if (start < line.size() && line[start] == '#') start++;
        while (start < line.size() && (line[start] == ' ' || line[start] == '\t')) start++;

        if (line.compare(start, 6, "define") == 0 && (line.size() <= start + 6 || std::isspace(line[start + 6]))) {
            // #define
            parse_define(line, start + 6);
        }
        // #include, #if, #ifdef, #endif, etc. — skip silently
    }

    // Parse a #define line
    void parse_define(const std::string& line, size_t pos) {
        // Skip whitespace after "define"
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) pos++;

        // Extract name
        std::string name;
        while (pos < line.size() && (std::isalnum(line[pos]) || line[pos] == '_')) {
            name += line[pos++];
        }
        if (name.empty()) return;

        // Skip whitespace
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) pos++;
        if (pos >= line.size()) return;

        // Extract value — could be number, hex, or expression
        std::string value_str;
        while (pos < line.size() && line[pos] != '\n') value_str += line[pos++];

        // Trim whitespace
        while (!value_str.empty() && (value_str.back() == ' ' || value_str.back() == '\t'))
            value_str.pop_back();

        // Also trim leading whitespace
        size_t vs = 0;
        while (vs < value_str.size() && (value_str[vs] == ' ' || value_str[vs] == '\t')) vs++;
        if (vs > 0) value_str = value_str.substr(vs);

        if (value_str.empty()) return;

        // Check if numeric (parse parenthesized expression and simple bit shifts)
        auto try_parse_numeric = [&](const std::string& s) -> bool {
            std::string clean;
            size_t i = 0;

            // Skip leading (
            if (i < s.size() && s[i] == '(') { i++; }

            // Extract hex or decimal number (ignore trailing junk like ULL, << expr, etc.)
            if (i < s.size() && s[i] == '0' && i + 1 < s.size() &&
                (s[i + 1] == 'x' || s[i + 1] == 'X')) {
                clean = "0x";
                i += 2;
                while (i < s.size() && std::isxdigit(s[i])) {
                    clean += s[i];
                    i++;
                }
                // Handle bit-shift: (1ULL << N) or (0xABCD << N)
                // Extract the left operand only
            } else if (i < s.size() && std::isdigit(s[i])) {
                while (i < s.size() && std::isdigit(s[i])) {
                    clean += s[i];
                    i++;
                }
            } else {
                // Unknown pattern — try whole string trimmed
                clean = s;
            }

            if (clean.empty()) return false;

            try {
                size_t end = 0;
                if (clean.size() > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
                    numeric_value_ = std::stoll(clean, &end, 16);
                    is_hex_ = true;
                } else {
                    numeric_value_ = std::stoll(clean, &end, 10);
                    is_hex_ = false;
                }
                return end > 0;
            } catch (...) {
                return false;
            }
        };

        if (try_parse_numeric(value_str)) {
            DefineInfo di;
            di.name = name;
            di.value_str = value_str;
            di.numeric_value = numeric_value_;
            di.is_numeric = true;
            di.is_hex = is_hex_;
            defines_.push_back(di);
        }
    }

    int64_t numeric_value_ = 0;
    bool is_hex_ = false;

    // ── C Type Parser ───────────────────────────────────────────────────────

    // Parse a C type starting at current lexer position.
    // Returns the parsed type, and the param name (if any).
    // The lexer position is left after the type.
    struct ParsedCType {
        CType type;
        std::string param_name;
        bool is_function_ptr = false;
        std::string fn_ptr_ret_type;
        std::vector<ParamInfo> fn_ptr_params;
    };

    ParsedCType parse_c_type() {
        ParsedCType result;
        CType ctype;
        std::vector<std::string> qualifiers;

        // Collect qualifiers and type keywords
        bool has_type = false;
        bool has_unsigned = false;
        bool long_seen = false;
        bool short_seen = false;
        std::string custom_type_name;

        while (true) {
            auto tok = lexer.next();
            if (tok.type != CTokenType::IDENTIFIER) {
                // If we hit punctuation, this might be pointer stars or paren for fnptr
                if (tok.type == CTokenType::PUNCTUATION && tok.text == "*") {
                    ctype.ptr_depth++;
                    continue;
                }
                if (tok.type == CTokenType::PUNCTUATION && tok.text == "(") {
                    // Could be function pointer: void (*name)(params)
                    // Check if next token is * (pointer to function) or identifier
                    auto next_tok = lexer.peek_token();
                    if (next_tok.type == CTokenType::PUNCTUATION && next_tok.text == "*") {
                        result.is_function_ptr = true;
                        // return type already parsed in ctype
                        result.fn_ptr_ret_type = ctype.to_brick_type();
                        lexer.next(); // consume (
                        lexer.next(); // consume *
                        // Parse name (optional)
                        auto name_tok = lexer.next();
                        if (name_tok.type == CTokenType::IDENTIFIER) {
                            result.param_name = name_tok.text;
                        } else {
                            lexer.pos -= name_tok.text.size();
                        }
                        // Expect )
                        auto close = lexer.next();
                        if (close.text != ")") {
                            // unexpected, but continue
                        }
                        // Expect (
                        auto open2 = lexer.next();
                        if (open2.text == "(") {
                            // Parse params until )
                            int depth = 1;
                            while (depth > 0) {
                                auto ptok = lexer.next();
                                if (ptok.type == CTokenType::END) break;
                                if (ptok.text == "(") depth++;
                                else if (ptok.text == ")") depth--;
                                else if (ptok.text == ",") continue;
                                // For simplicity, don't parse function pointer params in detail
                                // Just collect as opaque
                            }
                        }
                        return result;
                    }
                }
                // Not part of a type — put it back
                lexer.pos -= tok.text.size();
                lexer.line = tok.line;
                break;
            }

            std::string kw = tok.text;

            if (kw == "const") {
                ctype.is_const = true;
            } else if (kw == "volatile") {
                ctype.is_volatile = true;
            } else if (kw == "unsigned") {
                has_unsigned = true;
            } else if (kw == "signed") {
                // signed is default in Brick for integer types; skip
            } else if (kw == "struct" || kw == "union") {
                // Next token should be the struct name
                auto name_tok = lexer.next();
                if (name_tok.type == CTokenType::IDENTIFIER) {
                    custom_type_name = name_tok.text;
                    has_type = true;
                } else {
                    // Anonymous struct/union after name token
                    if (name_tok.text == "{") {
                        // Anonymous struct inside typedef — this is complex
                        // Skip to matching }
                        int brace_depth = 1;
                        while (brace_depth > 0) {
                            auto t = lexer.next();
                            if (t.type == CTokenType::END) break;
                            if (t.text == "{") brace_depth++;
                            if (t.text == "}") brace_depth--;
                        }
                        // Try to get the name after }
                        auto name_tok2 = lexer.next();
                        if (name_tok2.type == CTokenType::IDENTIFIER) {
                            custom_type_name = name_tok2.text;
                            has_type = true;
                        }
                    } else {
                        lexer.pos -= name_tok.text.size();
                        lexer.line = name_tok.line;
                    }
                }
            } else if (kw == "enum") {
                // enum name — skip and check for enum type name
                auto name_tok = lexer.next();
                if (name_tok.type == CTokenType::IDENTIFIER) {
                    custom_type_name = name_tok.text;
                    has_type = true;
                } else {
                    // Anonymous enum — put back
                    lexer.pos -= name_tok.text.size();
                    lexer.line = name_tok.line;
                }
            } else if (kw == "long") {
                long_seen = true;
                // "long" alone is "long int" (i64)
                // "long long" is also i64
            } else if (kw == "short") {
                short_seen = true;
            } else if (kw == "void") {
                ctype.base_name = "void";
                has_type = true;
            } else if (kw == "char") {
                ctype.base_name = "char";
                has_type = true;
            } else if (kw == "int") {
                ctype.base_name = "int";
                has_type = true;
            } else if (kw == "float") {
                ctype.base_name = "float";
                has_type = true;
            } else if (kw == "double") {
                ctype.base_name = "double";
                has_type = true;
            } else if (kw == "bool" || kw == "_Bool") {
                ctype.base_name = "bool";
                has_type = true;
            } else if (kw == "size_t" || kw == "ssize_t" || kw == "ptrdiff_t" ||
                       kw == "intptr_t" || kw == "uintptr_t") {
                ctype.base_name = kw;
                has_type = true;
            } else if (kw == "int8_t" || kw == "uint8_t" || kw == "int16_t" ||
                       kw == "uint16_t" || kw == "int32_t" || kw == "uint32_t" ||
                       kw == "int64_t" || kw == "uint64_t") {
                ctype.base_name = kw;
                has_type = true;
            } else if (kw == "extern" || kw == "static" || kw == "inline") {
                // Storage class specifiers — skip
                continue;
            } else {
                // Unknown identifier after a type qualifier or known type → this is variable name
                // Put it back and stop collecting type tokens
                if (has_type || has_unsigned || long_seen || short_seen) {
                    lexer.pos -= tok.text.size();
                    lexer.line = tok.line;
                    break;
                }
                // Unknown identifier without any type qualifiers → treat as custom type name
                custom_type_name = kw;
                has_type = true;
            }
        }

        // After the type tokens, check for *
        while (true) {
            auto ptok = lexer.peek_token();
            if (ptok.type == CTokenType::PUNCTUATION && ptok.text == "*") {
                lexer.next(); // consume *
                ctype.ptr_depth++;
            } else if (ptok.type == CTokenType::IDENTIFIER && (ptok.text == "const" || ptok.text == "volatile")) {
                lexer.next(); // consume const/volatile after *
            } else {
                break;
            }
        }

        // Peek at possible param/field name, but DON'T consume it.
        // The caller is responsible for consuming the name.
        auto name_tok = lexer.peek_token();
        if (name_tok.type == CTokenType::IDENTIFIER) {
            result.param_name = name_tok.text;
        }

        // Resolve base name
        if (!custom_type_name.empty()) {
            ctype.base_name = custom_type_name;
        } else if (long_seen) {
            ctype.base_name = "long";
        } else if (short_seen) {
            ctype.base_name = "short";
        } else if (has_unsigned && !has_type) {
            ctype.base_name = "unsigned"; // unsigned alone = unsigned int
        } else if (has_type && ctype.base_name.empty()) {
            ctype.base_name = "int"; // fallback
        }

        ctype.is_unsigned = has_unsigned;
        result.type = ctype;
        result.type.is_unsigned = has_unsigned;

        return result;
    }

    // ── Struct Parser ──────────────────────────────────────────────────────

    StructInfo parse_struct_body(const std::string& struct_name) {
        StructInfo si;
        si.name = struct_name;

        // Expect {
        auto open = lexer.next();
        if (open.text != "{") {
            // Forward declaration: struct Name; or typedef struct Name Name;
            si.is_forward_decl = true;
            return si;
        }

        // Parse fields until }
        while (true) {
            auto tok = lexer.peek_token();
            if (tok.type == CTokenType::END) break;
            if (tok.text == "}") {
                lexer.next(); // consume }
                break;
            }

            // Each field: type name [;] or type name : bitwidth;
            // Could start with struct { ... } nested
            auto ptok = lexer.peek_token();
            if (ptok.text == "{") {
                // Anonymous nested struct/union — skip for now (rare in bgfx)
                int depth = 0;
                while (true) {
                    auto t = lexer.next();
                    if (t.type == CTokenType::END) break;
                    if (t.text == "{") depth++;
                    if (t.text == "}") { if (depth == 0) break; depth--; }
                }
                continue;
            }

            auto parsed = parse_c_type();
            StructField field;
            if (parsed.is_function_ptr) {
                field.type.base_name = parsed.fn_ptr_ret_type + "(*)" + std::to_string(parsed.fn_ptr_params.size());
                field.name = parsed.param_name;
                // Function pointer param name was peeked but not consumed; consume it now
                if (!parsed.param_name.empty()) {
                    lexer.next(); // consume the peeked name
                    // Check for array suffix [N] — skip it
                    auto arr_tok = lexer.peek_token();
                    if (arr_tok.type == CTokenType::PUNCTUATION && arr_tok.text == "[") {
                        lexer.next(); // consume [
                        int depth = 1;
                        while (depth > 0 && lexer.pos < lexer.src.size()) {
                            auto t = lexer.next();
                            if (t.type == CTokenType::END) break;
                            if (t.text == "[") depth++;
                            else if (t.text == "]") depth--;
                        }
                    }
                }
            } else {
                field.type = parsed.type;
                field.name = parsed.param_name;
                // Consume the peeked field name
                if (!parsed.param_name.empty()) {
                    lexer.next();
                    // Check for array suffix [N] — skip it
                    auto arr_tok = lexer.peek_token();
                    if (arr_tok.type == CTokenType::PUNCTUATION && arr_tok.text == "[") {
                        lexer.next(); // consume [
                        int depth = 1;
                        while (depth > 0 && lexer.pos < lexer.src.size()) {
                            auto t = lexer.next();
                            if (t.type == CTokenType::END) break;
                            if (t.text == "[") depth++;
                            else if (t.text == "]") depth--;
                        }
                    }
                }
            }

            // Check for bitfield : width
            auto colon_check = lexer.peek_token();
            if (colon_check.text == ":") {
                lexer.next(); // consume :
                auto width_tok = lexer.next();
                if (width_tok.type == CTokenType::NUMBER) {
                    field.bit_width = std::stoi(width_tok.text);
                }
            }

            // Skip to next field or closing }
            auto semi = lexer.next();
            while (semi.text != ";" && semi.text != "}" && semi.type != CTokenType::END) {
                if (semi.text == "}") {
                    lexer.pos -= semi.text.size();
                    break;
                }
                semi = lexer.next();
            }
            if (semi.text == "}") {
                // Don't consume the closing brace
                break;
            }

            if (!field.type.base_name.empty() || !field.name.empty()) {
                si.fields.push_back(field);
            }
        }

        return si;
    }

    // ── Enum Parser ────────────────────────────────────────────────────────

    EnumInfo parse_enum_body(const std::string& enum_name) {
        EnumInfo ei;
        ei.name = enum_name;

        auto open = lexer.next();
        if (open.text != "{") {
            // Forward declaration or typedef — just return empty enum
            lexer.pos -= open.text.size();
            return ei;
        }

        std::string last_value;
        int64_t next_value = 0;

        while (true) {
            auto tok = lexer.next();
            if (tok.type == CTokenType::END) break;
            if (tok.text == "}") break;

            if (tok.type == CTokenType::IDENTIFIER) {
                EnumValue ev;
                ev.name = tok.text;

                // Check for = value
                auto eq = lexer.next();
                if (eq.text == "=") {
                    auto val = lexer.next();
                    if (val.type == CTokenType::NUMBER || val.type == CTokenType::IDENTIFIER ||
                        (val.type == CTokenType::PUNCTUATION && val.text == "(")) {
                        // Could be numeric, another constant name, or parenthesized expression
                if (val.type == CTokenType::NUMBER) {
                    ev.value_str = val.text;
                    // Remove leading 0x if present (we'll add it back in the generator)
                    bool starts_with_hex = (val.text.size() > 2 && val.text[0] == '0' &&
                                           (val.text[1] == 'x' || val.text[1] == 'X'));
                    if (starts_with_hex) {
                        ev.value_str = val.text.substr(2); // strip 0x prefix
                        ev.is_hex = true;
                    } else {
                        ev.is_hex = false;
                    }
                        } else if (val.type == CTokenType::PUNCTUATION && val.text == "(") {
                            // Parenthesized expression — collect until )
                            std::string expr = "(";
                            int depth = 1;
                            while (depth > 0) {
                                auto t = lexer.next();
                                if (t.type == CTokenType::END) break;
                                if (t.text == "(") depth++;
                                else if (t.text == ")") { depth--; if (depth == 0) { expr += ")"; break; } }
                                expr += t.text;
                            }
                            ev.value_str = expr;
                            // Try to evaluate the expression — for now, simple hex numbers
                            if (expr.size() > 2 && expr[0] == '(' && expr.back() == ')') {
                                std::string inner = expr.substr(1, expr.size() - 2);
                                // Check for (0xABCD), (1ULL << 4), etc.
                                size_t end = 0;
                                try {
                                    if (inner.size() > 2 && inner[0] == '0' && inner[1] == 'x') {
                                        auto val_hex = std::stoll(inner, &end, 16);
                                        if (end > 0) {
                                            ev.value_str = "0x" + std::to_string(val_hex);
                                            ev.is_hex = true;
                                        }
                                    }
                                } catch (...) {}
                            }
                        } else {
                            ev.value_str = val.text;
                        }
                        last_value = ev.value_str;
                    } else {
                        lexer.pos -= val.text.size();
                        lexer.line = val.line;
                    }
                } else {
                    // No value — auto-increment from last
                    if (!last_value.empty()) {
                        try {
                            next_value = std::stoll(last_value) + 1;
                            ev.value_str = std::to_string(next_value);
                        } catch (...) {
                            ev.value_str = std::to_string(next_value++);
                        }
                    } else {
                        ev.value_str = std::to_string(next_value++);
                    }
                    // Put back the non-= token
                    lexer.pos -= eq.text.size();
                    lexer.line = eq.line;
                }

                ei.values.push_back(ev);
            }

            // Skip comma
            auto comma = lexer.peek_token();
            if (comma.text == ",") lexer.next();
        }

        return ei;
    }

    // ── Declaration Dispatcher ─────────────────────────────────────────────

    void handle_declaration() {
        auto tok = lexer.peek_token();
        if (tok.type == CTokenType::IDENTIFIER) {
            std::string first = tok.text;

            // Check for typedef
            if (first == "typedef") {
                lexer.next(); // consume typedef
                handle_typedef();
                return;
            }

            // Check for macro-style function decl (GLFWAPI, BGFX_API, etc.)
            // These are #defined to dllexport or just nothing — skip the macro token
            if (first.find("API") != std::string::npos || first.find("api") != std::string::npos ||
                first.find("CALL") != std::string::npos) {
                lexer.next(); // consume the macro
            }

            // Check for extern
            auto next = lexer.peek_token();
            if (first == "extern" || first == "EXTERN") {
                lexer.next(); // consume extern
                handle_function_declaration();
                return;
            }

            // Try to parse as function declaration
            handle_function_declaration();
            return;
        }

        // If it's punctuation, skip it
        if (tok.type == CTokenType::PUNCTUATION) {
            lexer.next();
        }
    }

    void handle_typedef() {
        auto next = lexer.peek_token();
        if (next.type != CTokenType::IDENTIFIER) {
            // Skip typedef of non-identifier (rare)
            skip_typedef();
            return;
        }

        std::string kw = next.text;
        lexer.next(); // consume the keyword

        if (kw == "struct") {
            handle_typedef_struct();
        } else if (kw == "union") {
            handle_typedef_union();
        } else if (kw == "enum") {
            handle_typedef_enum();
        } else if (kw == "const") {
            // typedef const ... — skip
            skip_typedef();
        } else {
            // typedef existing_type new_name; (e.g. typedef uint32_t U32;)
            // Skip for now (we handle stdint types natively)
            skip_typedef();
        }
    }

    void handle_typedef_struct() {
        // Cases:
        // 1. typedef struct { ... } Name;
        // 2. typedef struct Name { ... } Name;
        // 3. typedef struct Name Name; (forward declaration)

        // Check for name after struct
        auto name_tok = lexer.peek_token();
        std::string struct_name;
        bool has_name_before_body = false;

        if (name_tok.type == CTokenType::IDENTIFIER) {
            struct_name = name_tok.text;
            has_name_before_body = true;
            lexer.next(); // consume name
        }

        // Check if next is { (struct body) or identifier (forward decl or typedef existing)
        auto after = lexer.peek_token();
        if (after.text == "{") {
            // Parse struct body
            auto si = parse_struct_body(struct_name);

            if (!si.is_forward_decl) {
                // After }, expect typedef name (alias)
                auto alias = lexer.next();
                if (alias.type == CTokenType::IDENTIFIER) {
                    if (si.name.empty() || has_name_before_body) {
                        // Use the typedef name instead of struct name
                        si.name = alias.text;
                    }
                    // Check for ; or ,
                    auto semi = lexer.next();
                    if (semi.text == ";") {
                        // done
                    } else if (semi.text == ",") {
                        // Multiple typedef names — skip
                        while (semi.text != ";" && semi.type != CTokenType::END) {
                            semi = lexer.next();
                        }
                    } else {
                        lexer.pos -= semi.text.size();
                    }
                }
                // Also consume any trailing ;
                auto final_check = lexer.peek_token();
                if (final_check.text == ";") lexer.next();

                if (!si.name.empty() && !si.fields.empty()) {
                    // Check not already forward-declared
                    if (forward_declared_structs_.count(si.name) == 0) {
                        structs_.push_back(si);
                    }
                }
            } else {
                // Forward declaration: struct Name; (no body)
                if (!struct_name.empty()) {
                    forward_declared_structs_.insert(struct_name);
                    StructInfo fwd;
                    fwd.name = struct_name;
                    structs_.push_back(fwd);
                    // Also check for typedef alias after ;
                    auto possible_alias = lexer.peek_token();
                    while (possible_alias.type == CTokenType::IDENTIFIER ||
                           possible_alias.text == ";") {
                        if (possible_alias.text == ";") {
                            lexer.next();
                            break;
                        }
                        lexer.next(); // consume alias name
                        possible_alias = lexer.peek_token();
                        if (possible_alias.text == ";") {
                            lexer.next();
                            break;
                        }
                        if (possible_alias.text == ",") {
                            lexer.next();
                            possible_alias = lexer.peek_token();
                        }
                    }
                }
            }
        } else if (after.type == CTokenType::IDENTIFIER) {
            // typedef struct Name Alias; (forward declaration)
            lexer.next(); // consume alias
            auto semi = lexer.next();
            if (semi.text != ";") {
                // skip extra
                while (semi.text != ";" && semi.type != CTokenType::END)
                    semi = lexer.next();
            }
            if (!struct_name.empty()) {
                forward_declared_structs_.insert(struct_name);
                StructInfo fwd;
                fwd.name = struct_name;
                structs_.push_back(fwd);
            }            } else if (after.text == ";") {
            // Just "typedef struct Name;" — forward decl
            lexer.next(); // consume ;
            if (!struct_name.empty()) {
                forward_declared_structs_.insert(struct_name);
                StructInfo fwd;
                fwd.name = struct_name;
                structs_.push_back(fwd);
            }
        } else {
            // Unknown pattern — skip
            skip_typedef();
        }
    }

    void handle_typedef_union() {
        // Similar to typedef struct
        auto name_tok = lexer.peek_token();
        std::string union_name;
        if (name_tok.type == CTokenType::IDENTIFIER) {
            union_name = name_tok.text;
            lexer.next();
        }

        auto after = lexer.peek_token();
        if (after.text == "{") {
            // Parse union body — for now, skip fields and just emit empty struct
            int depth = 1;
            while (depth > 0) {
                auto t = lexer.next();
                if (t.type == CTokenType::END) break;
                if (t.text == "{") depth++;
                if (t.text == "}") depth--;
            }
            // Get typedef name
            auto alias = lexer.next();
            if (alias.type == CTokenType::IDENTIFIER) {
                union_name = alias.text;
            }
            auto semi = lexer.next();
            if (semi.text != ";") {
                while (semi.text != ";" && semi.type != CTokenType::END)
                    semi = lexer.next();
            }

            // Emit as struct with @packed
            StructInfo si;
            si.name = union_name;
            structs_.push_back(si);
        }
    }

    void handle_typedef_enum() {
        auto name_tok = lexer.peek_token();
        std::string enum_name;

        if (name_tok.type == CTokenType::IDENTIFIER) {
            enum_name = name_tok.text;
            // Could be enum name, or we could be looking at { directly
            auto after = lexer.peek_token();
            if (after.text == "{") {
                lexer.next(); // consume name
            } else {
                // enum Name is the name, then { or typedef alias
                lexer.next(); // consume name
                enum_name = name_tok.text;
            }
        }

        auto ei = parse_enum_body(enum_name);

        // After }, expect typedef name
        auto alias = lexer.next();
        if (alias.type == CTokenType::IDENTIFIER) {
            if (ei.name.empty()) {
                ei.name = alias.text;
            }
            // Consume ; and anything else
            auto semi = lexer.next();
            while (semi.text != ";" && semi.type != CTokenType::END)
                semi = lexer.next();
        } else if (alias.text == ";") {
            // done
        } else {
            // put back
            lexer.pos -= alias.text.size();
        }

        if (!ei.name.empty() && !ei.values.empty()) {
            enums_.push_back(ei);
        }
    }

    void skip_typedef() {
        // Skip until ;
        while (true) {
            auto t = lexer.next();
            if (t.type == CTokenType::END || t.text == ";") break;
        }
    }

    // ── Function Declaration Parser ────────────────────────────────────────

    void handle_function_declaration() {
        // Save position in case this isn't a function declaration
        size_t saved_pos = lexer.pos;
        int saved_line = lexer.line;

        // Parse return type
        auto ret = parse_c_type();
        if (ret.type.base_name.empty()) {
            return;
        }

        // Next token should be function name (identifier)
        auto name_tok = lexer.next();
        if (name_tok.type != CTokenType::IDENTIFIER) {
            // Not a function declaration — restore position
            lexer.pos = saved_pos;
            lexer.line = saved_line;
            return;
        }

        std::string func_name = name_tok.text;

        // Next should be (
        auto paren = lexer.next();
        if (paren.text != "(") {
            // Might be a variable declaration, not a function — restore
            lexer.pos = saved_pos;
            lexer.line = saved_line;
            return;
        }

        // Parse parameters
        FunctionInfo fi;
        fi.return_type = ret.type;
        fi.name = func_name;

        // Check for void params
        // Must be strictly "void)" or "void," — NOT "void*" or "void name"
        auto first_param = lexer.peek_token();
        if (first_param.text == "void") {
            // Peek ahead to see if next token is ')', ',' or something else
            // Safely peek: save position, read next, restore
            size_t peek_pos = lexer.pos;
            int peek_line = lexer.line;
            auto after_void = lexer.next();
            lexer.pos = peek_pos;
            lexer.line = peek_line;

            if (after_void.text == ")") {
                // func(void) — no params
                lexer.next(); // consume void
                lexer.next(); // consume )
            } else if (after_void.text == ",") {
                // This shouldn't happen in valid C, but handle gracefully
                // void param followed by comma — consume void
                lexer.next(); // consume void
                // The , will be consumed below by the normal param loop
            } else {
                // void* or void NAME — not the void(void) pattern
                // Fall through to normal parameter parsing
                goto parse_params;
            }
        } else if (first_param.text == ")") {
            // Empty params: func()
            lexer.next(); // consume )
        } else if (first_param.type == CTokenType::ELLIPSIS) {
            // Variadic: func(...)
            lexer.next(); // consume ...
            fi.is_variadic = true;
            auto close = lexer.next(); // consume )
        } else {
        parse_params:
            // Parse parameter list
            while (true) {
                auto ptok = lexer.peek_token();
                if (ptok.text == ")") {
                    lexer.next(); // consume )
                    break;
                }
                if (ptok.type == CTokenType::ELLIPSIS) {
                    lexer.next(); // consume ...
                    fi.is_variadic = true;
                    auto close = lexer.next(); // consume )
                    if (close.text != ")") {
                        // Skip until )
                        while (close.text != ")" && close.type != CTokenType::END)
                            close = lexer.next();
                    }
                    break;
                }

                // Parse parameter type + optional name
                auto param = parse_c_type();
                ParamInfo pi;
                pi.type = param.type;
                pi.name = param.param_name;
                // Consume the peeked param name
                if (!param.param_name.empty()) {
                    lexer.next();
                }
                fi.params.push_back(pi);

                // Check for comma or )
                auto sep = lexer.next();
                if (sep.text == ")") break;
                if (sep.text != ",") {
                    // Unexpected — break
                    break;
                }
            }
        }

        // Consume trailing ; if present
        auto semi = lexer.peek_token();
        if (semi.text == ";") lexer.next();

        // Also handle cases where there's a macro after the function decl
        // e.g., GLFWAPI void glfwInit(void) GLFWAPI;
        semi = lexer.peek_token();
        if (semi.type == CTokenType::IDENTIFIER && semi.text.find("API") != std::string::npos) {
            lexer.next(); // consume the macro
            semi = lexer.peek_token();
            if (semi.text == ";") lexer.next();
        }

        functions_.push_back(fi);
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// Code Generator — emits Brick .brc
// ═════════════════════════════════════════════════════════════════════════════

struct Generator {
    std::ostringstream out;
    std::string header_path;
    std::string header_name;
    std::unordered_set<std::string> emitted_functions;
    std::unordered_set<std::string> emitted_structs;
    std::unordered_set<std::string> emitted_enums;

    Generator(const std::string& path) : header_path(path) {
        // Extract just the filename for the include directive
        auto pos = path.find_last_of("/\\");
        header_name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
    }

    // Escape a string for inclusion in .brc (though C headers shouldn't have special chars)
    std::string escape_string(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else result += c;
        }
        return result;
    }

    // Map C integer value to Brick literal (handle hex)
    std::string format_int_value(const std::string& val_str, bool is_hex) {
        if (is_hex) {
            // Ensure it has 0x prefix
            if (val_str.find("0x") == 0 || val_str.find("0X") == 0) {
                return val_str;
            }
            return "0x" + val_str;
        }
        return val_str;
    }

    // Generate the complete .brc file
    std::string generate(const std::vector<FunctionInfo>& functions,
                         const std::vector<StructInfo>& structs,
                         const std::vector<EnumInfo>& enums,
                         const std::vector<DefineInfo>& defines,
                         const Options& opts) {
        // Header comment
        out << "// Auto-generated Brick bindings for " << header_name << "\n";
        out << "// Generated by `brick bind " << header_path << "`\n\n";

        // Include the original C header (needed for C compiler)
        out << "include \"" << escape_string(header_name) << "\"\n\n";

        // ── Structs ──
        if (opts.generate_structs && !structs.empty()) {
            out << "// ═══ Structs ═══════════════════════════════════════════════════\n\n";
            for (const auto& si : structs) {
                if (emitted_structs.count(si.name)) continue;
                emitted_structs.insert(si.name);

                if (si.fields.empty()) {
                    // Forward declaration or empty struct — just emit @packed struct with no fields
                    out << "@packed struct " << si.name << " {}\n\n";
                } else {
                    out << "@packed struct " << si.name << " {\n";
                    for (const auto& f : si.fields) {
                        std::string brick_type = f.type.to_brick_type();
                        // Strip pointer for struct fields — C struct fields are by value
                        // (pointers in structs are okay, they become *T in Brick)
                        out << "    " << brick_type << " " << f.name;
                        if (f.bit_width > 0) {
                            out << " : " << f.bit_width;
                        }
                        out << "\n";
                    }
                    out << "}\n\n";
                }
            }
        }

        // ── Enums ──
        if (opts.generate_enums && !enums.empty()) {
            out << "// ═══ Enums ═════════════════════════════════════════════════════\n\n";
            for (const auto& ei : enums) {
                if (emitted_enums.count(ei.name)) continue;
                emitted_enums.insert(ei.name);

                out << "enum " << ei.name << " {\n";
                for (const auto& ev : ei.values) {
                    out << "    " << ev.name;
                    if (!ev.value_str.empty()) {
                        out << " = " << (ev.is_hex ? "0x" : "") << ev.value_str;
                    }
                    out << "\n";
                }
                out << "}\n\n";
            }
        }

        // ── Constants (#define) ──
        if (opts.generate_defines && !defines.empty()) {
            out << "// ═══ Constants ═════════════════════════════════════════════════\n\n";
            for (const auto& di : defines) {
                if (di.is_numeric) {
                    out << "const " << di.name << " = ";
                    if (di.is_hex) {
                        // Format hex value (uppercase, no leading zeros padding)
                        char buf[32];
                        snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)di.numeric_value);
                        out << buf;
                    } else {
                        out << di.numeric_value;
                    }
                    out << "\n";
                }
            }
            out << "\n";
        }

        // ── Functions ──
        if (opts.generate_functions && !functions.empty()) {
            out << "// ═══ Functions ═════════════════════════════════════════════════\n\n";
            for (const auto& fi : functions) {
                if (emitted_functions.count(fi.name)) continue;
                emitted_functions.insert(fi.name);

                out << "extern fn " << fi.name << "(";

                // Params
                for (size_t i = 0; i < fi.params.size(); i++) {
                    if (i > 0) out << ", ";
                    const auto& pi = fi.params[i];

                    // void param -> skip
                    if (pi.type.is_void() && !pi.type.is_void_ptr() && pi.name.empty()) {
                        out << "...";
                        continue;
                    }

                    // Skip void params with no name
                    if (pi.type.is_void() && !pi.type.is_void_ptr()) {
                        continue;
                    }

                    std::string brick_type = pi.type.to_brick_type();
                    out << brick_type;
                    if (!pi.name.empty()) {
                        out << " " << pi.name;
                    }
                }

                // Variadic
                if (fi.is_variadic) {
                    if (!fi.params.empty()) out << ", ";
                    out << "...";
                }

                out << ")";

                // Return type
                std::string ret_type = fi.return_type.to_brick_type();
                out << " -> " << ret_type;

                out << "\n";
            }
            out << "\n";
        }

        return out.str();
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// Public API
// ═════════════════════════════════════════════════════════════════════════════

Result generate(const std::string& header_path, const Options& opts) {
    Result result;

    // Read file
    std::ifstream file(header_path);
    if (!file.is_open()) {
        result.errors.push_back("could not open header: " + header_path);
        result.success = false;
        return result;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Parse
    CBindParser parser(source);
    parser.parse_all();

    // Generate
    Generator gen(header_path);
    std::string brc = gen.generate(
        parser.get_functions(),
        parser.get_structs(),
        parser.get_enums(),
        parser.get_defines(),
        opts
    );

    result.brc_code = brc;
    result.success = true;
    return result;
}

Result generate_from_source(const std::string& source, const Options& opts) {
    Result result;

    // Parse
    CBindParser parser(source);
    parser.parse_all();

    // Generate (use a placeholder path)
    Generator gen("source.brc");
    std::string brc = gen.generate(
        parser.get_functions(),
        parser.get_structs(),
        parser.get_enums(),
        parser.get_defines(),
        opts
    );

    result.brc_code = brc;
    result.success = true;
    return result;
}

} // namespace bind
} // namespace brick
