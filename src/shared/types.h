#ifndef BRICK_TYPES_H
#define BRICK_TYPES_H

#include <string>
#include <string_view>
#include <cstdint>

namespace brick {

enum class TokenType {
    // Keywords
    PACKAGE, USING, PRIVATE, PUBLIC,
    STRUCT, UNION, EXTENDS, INTERFACE, FN, RETURN,
    IF, ELSE, WHILE, FOR,
    BLOCK, RESET,
    TRUE, FALSE, NULL_, ERROR,
    INT, FLOAT, BOOL, CHAR, STRING, VOID,
    U8, U16, U32, U64,
    I8, I16, I32, I64,
    F32, F64,
    USIZE, ISIZE,
    BYTE,
    EXPORT, EXTERN, INCLUDE, LINK, IMPL,

    // Macro keywords
    MACRO, BUILD, EMIT, TYPE,

    // Literals
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, CHAR_LITERAL,

    // Identifier
    IDENTIFIER,

    // Operators
    PLUS, MINUS, STAR, SLASH, ASSIGN,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,
    EQ, NEQ, LT, GT, LEQ, GEQ,
    AND, OR, NOT,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LSHIFT, RSHIFT,
    DOT, ARROW, AT, PIPE,
    PLUS_PLUS, MINUS_MINUS,

    // Delimiters
    LBRACE, RBRACE, LPAREN, RPAREN,
    LBRACKET, RBRACKET, SEMICOLON, COMMA, COLON,

    // Macro symbols
    DOLLAR, ELLIPSIS,

    // Special
    BITFIELD_TYPE,

    BREAK, CONTINUE, CONST, DEFER, ENUM, MATCH, IS, AS,

    EOF_
};

struct SourceLocation {
    int line;
    int col;
    std::string file;
};

struct Token {
    TokenType type;
    std::string_view lexeme;
    SourceLocation location;
    std::string literal_type;

    Token() : type(TokenType::EOF_), location{0, 0, ""} {}
    Token(TokenType t, std::string_view l, SourceLocation loc)
        : type(t), lexeme(l), location(loc) {}
};

} // namespace brick

#endif
