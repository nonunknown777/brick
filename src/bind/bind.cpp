#include "bind.h"

#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace brick {
namespace bind {

// ─── Tokenizer for C headers ─────────────────────────────

enum class CTokType {
    IDENTIFIER, LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON, COMMA, STAR, ELLIPSIS, LBRACKET, RBRACKET,
    NUMBER, STRING, CHAR, NEWLINE, END
};

struct CToken {
    CTokType type;
    std::string text;
    int line;
    int col;
};

static std::vector<CToken> tokenize_header(const std::string& content, std::vector<BindError>& errors) {
    std::vector<CToken> tokens;
    size_t i = 0;
    int line = 1;
    int col = 1;
    const size_t len = content.size();

    while (i < len) {
        char c = content[i];

        // Skip whitespace (but track newlines for line counting)
        if (c == ' ' || c == '\t') { i++; col++; continue; }
        if (c == '\n') { i++; line++; col = 1; continue; }
        if (c == '\r') { i++; continue; }

        // Line comments: //
        if (c == '/' && i + 1 < len && content[i + 1] == '/') {
            while (i < len && content[i] != '\n') i++;
            continue;
        }

        // Block comments: /* ... */
        if (c == '/' && i + 1 < len && content[i + 1] == '*') {
            i += 2;
            while (i + 1 < len && !(content[i] == '*' && content[i + 1] == '/')) {
                if (content[i] == '\n') { line++; col = 1; }
                i++;
            }
            if (i + 1 < len) i += 2;
            continue;
        }

        // Preprocessor directives: #include, #define, etc.
        if (c == '#') {
            while (i < len && content[i] != '\n') i++;
            continue;
        }

        // Single character tokens
        if (c == '(') { tokens.push_back({CTokType::LPAREN, "(", line, col}); i++; col++; continue; }
        if (c == ')') { tokens.push_back({CTokType::RPAREN, ")", line, col}); i++; col++; continue; }
        if (c == '{') { tokens.push_back({CTokType::LBRACE, "{", line, col}); i++; col++; continue; }
        if (c == '}') { tokens.push_back({CTokType::RBRACE, "}", line, col}); i++; col++; continue; }
        if (c == ';') { tokens.push_back({CTokType::SEMICOLON, ";", line, col}); i++; col++; continue; }
        if (c == ',') { tokens.push_back({CTokType::COMMA, ",", line, col}); i++; col++; continue; }
        if (c == '*') { tokens.push_back({CTokType::STAR, "*", line, col}); i++; col++; continue; }
        if (c == '[') { tokens.push_back({CTokType::LBRACKET, "[", line, col}); i++; col++; continue; }
        if (c == ']') { tokens.push_back({CTokType::RBRACKET, "]", line, col}); i++; col++; continue; }

        // Ellipsis: ...
        if (c == '.' && i + 2 < len && content[i + 1] == '.' && content[i + 2] == '.') {
            tokens.push_back({CTokType::ELLIPSIS, "...", line, col});
            i += 3; col += 3;
            continue;
        }

        // Numbers
        if (std::isdigit(c) || (c == '-' && i + 1 < len && std::isdigit(content[i + 1]))) {
            size_t start = i;
            if (c == '-') { i++; col++; }
            while (i < len && (std::isalnum(content[i]) || content[i] == '.')) i++;
            tokens.push_back({CTokType::NUMBER, content.substr(start, i - start), line, col});
            col += (i - start);
            continue;
        }

        // String literals
        if (c == '"') {
            size_t start = i;
            i++; col++;
            while (i < len && content[i] != '"') {
                if (content[i] == '\\') { i++; col++; }
                i++; col++;
            }
            if (i < len) { i++; col++; }
            tokens.push_back({CTokType::STRING, content.substr(start, i - start), line, col});
            continue;
        }

        // Char literals
        if (c == '\'') {
            size_t start = i;
            i += 2; col += 2;
            if (i < len && content[i] == '\'') i++;
            tokens.push_back({CTokType::CHAR, content.substr(start, i - start), line, col});
            continue;
        }

        // Identifiers and keywords
        if (std::isalpha(c) || c == '_') {
            size_t start = i;
            while (i < len && (std::isalnum(content[i]) || content[i] == '_')) i++;
            std::string word = content.substr(start, i - start);
            tokens.push_back({CTokType::IDENTIFIER, word, line, col});
            col += (i - start);
            continue;
        }

        // Skip other characters
        i++; col++;
    }

    tokens.push_back({CTokType::END, "", line, col});
    return tokens;
}

// ─── C Type to Brick Type ────────────────────────────────

static std::string c_type_to_brick(const std::string& c_type) {
    if (c_type == "int" || c_type == "int32_t") return "i32";
    if (c_type == "unsigned int" || c_type == "uint32_t") return "u32";
    if (c_type == "short" || c_type == "int16_t" || c_type == "short int") return "i16";
    if (c_type == "unsigned short" || c_type == "uint16_t") return "u16";
    if (c_type == "char" || c_type == "uint8_t") return "u8";
    if (c_type == "signed char" || c_type == "int8_t") return "i8";
    if (c_type == "long" || c_type == "int64_t" || c_type == "long long") return "i64";
    if (c_type == "unsigned long" || c_type == "uint64_t" || c_type == "unsigned long long") return "u64";
    if (c_type == "float") return "f32";
    if (c_type == "double") return "f64";
    if (c_type == "size_t") return "usize";
    if (c_type == "ptrdiff_t" || c_type == "ssize_t") return "isize";
    if (c_type == "void") return "void";
    if (c_type == "bool" || c_type == "_Bool") return "bool";
    if (c_type == "char*" || c_type == "const char*" || c_type == "const char *" || c_type == "char *") return "*u8";
    if (c_type == "void*" || c_type == "void *") return "*void";
    return c_type; // Pass through for user-defined types
}

// ─── Type Keyword Check ──────────────────────────────────

static bool is_type_keyword(const std::string& s) {
    return s == "int" || s == "float" || s == "double" || s == "char" ||
           s == "void" || s == "short" || s == "long" || s == "unsigned" ||
           s == "signed" || s == "const" || s == "struct" || s == "union" ||
           s == "enum" || s == "volatile" || s == "extern" || s == "static" ||
           s == "inline" || s == "restrict" || s == "size_t" || s == "ssize_t" ||
           s == "ptrdiff_t" || s == "intptr_t" || s == "uintptr_t" ||
           s == "int8_t" || s == "int16_t" || s == "int32_t" || s == "int64_t" ||
           s == "uint8_t" || s == "uint16_t" || s == "uint32_t" || s == "uint64_t" ||
           s == "bool" || s == "_Bool" || s == "wchar_t";
}

// ─── C Name Cleanup ──────────────────────────────────────

static std::string clean_name(const std::string& name) {
    // Remove trailing _t suffix (common in typedef names)
    if (name.size() > 2 && name.substr(name.size() - 2) == "_t") {
        return name.substr(0, name.size() - 2);
    }
    return name;
}

// ─── Binding Generation ─────────────────────────────────

BindResult generate(const std::string& header_path, const Options& opts) {
    BindResult result;
    result.success = false;

    // Read the header file
    std::ifstream file(header_path);
    if (!file.is_open()) {
        result.errors.push_back({"Could not open header file: " + header_path, 0, 0});
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Tokenize
    auto tokens = tokenize_header(content, result.errors);

    // Parse function declarations
    std::stringstream brc_out;
    std::vector<std::string> includes_done;
    std::vector<std::string> used_functions;

    size_t idx = 0;

    // Skip to first function-like declaration
    // Look for patterns: return_type name ( params ) ;
    // Also handle: EXTERN_C return_type name ( params ) ;
    // Also handle: DLL_EXPORT return_type name ( params ) ;

    while (idx < tokens.size()) {
        auto& tok = tokens[idx];

        // Skip struct/union declarations { ... }
        if (tok.type == CTokType::IDENTIFIER &&
            (tok.text == "struct" || tok.text == "union" || tok.text == "enum" || tok.text == "typedef")) {
            // Skip to matching ;
            idx++;
            while (idx < tokens.size() && tokens[idx].type != CTokType::SEMICOLON) {
                if (tokens[idx].type == CTokType::LBRACE) {
                    int depth = 1;
                    idx++;
                    while (idx < tokens.size() && depth > 0) {
                        if (tokens[idx].type == CTokType::LBRACE) depth++;
                        if (tokens[idx].type == CTokType::RBRACE) depth--;
                        if (depth > 0) idx++;
                    }
                }
                idx++;
            }
            if (idx < tokens.size()) idx++;
            continue;
        }

        // Skip extern \"C\" { blocks
        if (tok.type == CTokType::IDENTIFIER && tok.text == "extern") {
            idx++;
            if (idx < tokens.size() && tokens[idx].type == CTokType::STRING &&
                tokens[idx].text == "\"C\"") {
                idx++;
                if (idx < tokens.size() && tokens[idx].type == CTokType::LBRACE) {
                    int depth = 1;
                    idx++;
                    while (idx < tokens.size() && depth > 0) {
                        if (tokens[idx].type == CTokType::LBRACE) depth++;
                        if (tokens[idx].type == CTokType::RBRACE) depth--;
                        if (depth > 0) idx++;
                    }
                    idx++;
                }
            }
            continue;
        }

        // Skip DLL export / attributes: __declspec(dllexport), __attribute__, etc.
        if (tok.type == CTokType::IDENTIFIER && (tok.text.find("__declspec") == 0 ||
                                                  tok.text.find("__attribute__") == 0 ||
                                                  tok.text.find("DLL") == 0 ||
                                                  tok.text.find("API") != std::string::npos)) {
            // Skip to next )
            while (idx < tokens.size() && tokens[idx].type != CTokType::RPAREN &&
                   tokens[idx].type != CTokType::SEMICOLON) idx++;
            if (idx < tokens.size()) idx++;
            continue;
        }

        // Look for function: return_type IDENTIFIER ( params... )
        // Collect return type tokens
        std::string ret_type;
        std::string func_name;
        std::vector<std::string> param_types;
        std::vector<std::string> param_names;

        int lookahead = idx;

        // Build the return type expression
        while (lookahead < tokens.size()) {
            auto& lt = tokens[lookahead];

            if (lt.type == CTokType::IDENTIFIER) {
                if (ret_type.empty()) {
                    ret_type = lt.text;
                } else {
                    ret_type += " " + lt.text;
                }
                lookahead++;
            } else if (lt.type == CTokType::STAR) {
                ret_type += "*";
                lookahead++;
            } else if (lt.type == CTokType::LPAREN) {
                // Check if this is a function pointer syntax, e.g., void (*name)(params)
                if (lookahead + 1 < tokens.size() && tokens[lookahead + 1].type == CTokType::STAR) {
                    // Function pointer declaration - skip it
                    while (lookahead < tokens.size() && tokens[lookahead].type != CTokType::SEMICOLON) {
                        lookahead++;
                    }
                    if (lookahead < tokens.size()) lookahead++;
                    break;
                }

                // This is the function parameter list start
                lookahead++;

                // Get function name (it should be between ret_type and LPAREN)
                // For C, the pattern is: return_type name ( params )
                // But if ret_type has multiple words, we need to extract the last word as function name
                {
                    auto parts = ret_type;
                    auto last_space = parts.rfind(' ');
                    if (last_space != std::string::npos) {
                        func_name = parts.substr(last_space + 1);
                        ret_type = parts.substr(0, last_space);
                    } else {
                        // Could be just a type name without return type
                        // Check if this looks like a known type
                        if (ret_type == "const" || ret_type == "static" || ret_type == "inline") {
                            ret_type = "";
                            func_name = parts;
                        }
                    }
                }

                if (func_name.empty()) {
                    func_name = ret_type;
                    ret_type = "";
                }

                // Clean up ret_type
                // Remove const, static, inline, extern prefixes
                {
                    std::string cleaned;
                    std::stringstream ss(ret_type);
                    std::string word;
                    while (ss >> word) {
                        if (word != "const" && word != "static" && word != "inline" && word != "extern") {
                            if (!cleaned.empty()) cleaned += " ";
                            cleaned += word;
                        }
                    }
                    ret_type = cleaned;
                }

                // Parse parameters
                while (lookahead < tokens.size() && tokens[lookahead].type != CTokType::RPAREN) {
                    if (tokens[lookahead].type == CTokType::COMMA ||
                        tokens[lookahead].type == CTokType::ELLIPSIS) {
                        if (tokens[lookahead].type == CTokType::ELLIPSIS) {
                            param_types.push_back("...");
                            param_names.push_back("args");
                        }
                        lookahead++;
                        continue;
                    }

                    // Collect parameter type + name
                    std::string ptype;
                    std::string pname;

                    while (lookahead < tokens.size()) {
                        auto& pt = tokens[lookahead];
                        if (pt.type == CTokType::RPAREN || pt.type == CTokType::COMMA) break;

                        if (pt.type == CTokType::STAR) {
                            if (!ptype.empty() && ptype.back() != '*') ptype += " ";
                            ptype += "*";
                            lookahead++;
                        } else if (pt.type == CTokType::IDENTIFIER) {
                            // Check if next token is LPAREN (function pointer param)
                            if (lookahead + 1 < tokens.size() && tokens[lookahead + 1].type == CTokType::LPAREN) {
                                // This is a function pointer param like void (*cb)(void*)
                                ptype += "fnptr";
                                pname = pt.text;
                                lookahead++;
                                // Skip to matching )
                                int pdepth = 1;
                                lookahead++;
                                while (lookahead < tokens.size() && pdepth > 0) {
                                    if (tokens[lookahead].type == CTokType::LPAREN) pdepth++;
                                    if (tokens[lookahead].type == CTokType::RPAREN) pdepth--;
                                    lookahead++;
                                }
                                break;
                            }

                            // Check if next token is LBRACKET (array param)
                            if (lookahead + 1 < tokens.size() && tokens[lookahead + 1].type == CTokType::LBRACKET) {
                                ptype += pt.text + "[]";
                                lookahead += 2;
                                // Skip array size if present
                                if (lookahead < tokens.size() && tokens[lookahead].type == CTokType::NUMBER) lookahead++;
                                if (lookahead < tokens.size() && tokens[lookahead].type == CTokType::RBRACKET) lookahead++;
                                // Check for parameter name
                                if (lookahead < tokens.size() && tokens[lookahead].type == CTokType::IDENTIFIER &&
                                    tokens[lookahead].text != "void" && tokens[lookahead].text != "const") {
                                    pname = tokens[lookahead].text;
                                    lookahead++;
                                }
                                break;
                            }

                            if (pt.text == "void" && (lookahead + 1 >= tokens.size() ||
                                                      tokens[lookahead + 1].type == CTokType::RPAREN)) {
                                // void with no parameter name - void function
                                break;
                            }

                            if (pname.empty() && !is_type_keyword(pt.text)) {
                                // This could be a parameter name (not a type word)
                                // Try to determine: if we already have a type, this is name
                                // If no type yet, it could be a typedef type name (uppercase)
                                if (!ptype.empty()) {
                                    pname = pt.text;
                                } else {
                                    // It's a type name (possibly user-defined)
                                    ptype = pt.text;
                                }
                            } else {
                                if (!ptype.empty()) ptype += " ";
                                ptype += pt.text;
                            }
                            lookahead++;
                        } else if (pt.type == CTokType::NUMBER) {
                            if (!ptype.empty()) ptype += " ";
                            ptype += pt.text;
                            lookahead++;
                        } else {
                            lookahead++;
                        }
                    }

                    if (!ptype.empty() && ptype != "void") {
                        param_types.push_back(ptype);
                        param_names.push_back(pname.empty() ? "p" + std::to_string(param_types.size()) : pname);
                    }
                }

                if (lookahead < tokens.size() && tokens[lookahead].type == CTokType::RPAREN) {
                    lookahead++;
                }

                break;
            } else if (lt.type == CTokType::SEMICOLON) {
                // Not a function declaration
                break;
            } else {
                lookahead++;
            }
        }

        // If we found a function name and the next token after RPAREN is ; or LBRACE, emit it
        if (!func_name.empty() && lookahead < tokens.size()) {
            // Check for trailing specifiers: const, __attribute__, etc.
            while (lookahead < tokens.size() &&
                   (tokens[lookahead].type == CTokType::IDENTIFIER ||
                    tokens[lookahead].type == CTokType::LPAREN ||
                    tokens[lookahead].type == CTokType::STAR ||
                    tokens[lookahead].type == CTokType::NUMBER)) {
                if (tokens[lookahead].text == "const") { lookahead++; continue; }
                if (tokens[lookahead].text.find("__attribute__") == 0) {
                    // Skip __attribute__ ((...))
                    while (lookahead < tokens.size() && tokens[lookahead].type != CTokType::RPAREN &&
                           tokens[lookahead].type != CTokType::SEMICOLON) lookahead++;
                    if (lookahead < tokens.size()) lookahead++;
                    continue;
                }
                break;
            }

            if (lookahead < tokens.size() &&
                (tokens[lookahead].type == CTokType::SEMICOLON || tokens[lookahead].type == CTokType::LBRACE)) {
                // Only generate if not already generated
                if (std::find(used_functions.begin(), used_functions.end(), func_name) == used_functions.end()) {
                    used_functions.push_back(func_name);

                    std::string brick_ret_type = c_type_to_brick(ret_type);
                    std::string brick_name = clean_name(func_name);

                    brc_out << "extern fn " << brick_name << "(";
                    for (size_t p = 0; p < param_types.size(); p++) {
                        if (p > 0) brc_out << ", ";
                        std::string brick_p = c_type_to_brick(param_types[p]);
                        if (brick_p == "*void") {
                            brc_out << "*void " << param_names[p];
                        } else if (brick_p.find("*") != std::string::npos) {
                            brc_out << brick_p << " " << param_names[p];
                        } else if (brick_p == "...") {
                            brc_out << "...";
                        } else {
                            brc_out << brick_p << " " << param_names[p];
                        }
                    }
                    brc_out << ") -> " << brick_ret_type << "\n";
                }

                idx = lookahead;
                if (idx < tokens.size() && tokens[idx].type == CTokType::LBRACE) {
                    // Skip function body
                    int depth = 1;
                    idx++;
                    while (idx < tokens.size() && depth > 0) {
                        if (tokens[idx].type == CTokType::LBRACE) depth++;
                        if (tokens[idx].type == CTokType::RBRACE) depth--;
                        if (depth > 0) idx++;
                    }
                    idx++;
                } else {
                    idx++; // Skip ;
                }
                continue;
            }
        }

        idx++;
    }

    result.brc_code = brc_out.str();
    result.success = true;

    // If nothing was generated, add a note to errors
    if (used_functions.empty()) {
        result.errors.push_back({"No function declarations found in header (or all are struct/typedef declarations)", 0, 0});
        result.success = true; // Still return success - empty binding is valid
    }

    return result;
}

} // namespace bind
} // namespace brick
