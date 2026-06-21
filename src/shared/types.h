#ifndef META_C_TYPES_H
#define META_C_TYPES_H

#include <string>
#include <cstdint>

namespace meta_c {

enum class TokenType {
    // Keywords
    // Palavras-chave
    PACKAGE, USING, PRIVATE, PUBLIC,
    STRUCT, EXTENDS, INTERFACE, FN, RETURN,
    IF, ELSE, WHILE, FOR,
    BLOCK, RESET,
    TRUE, FALSE, NULL_, ERROR,
    INT, FLOAT, BOOL, CHAR, STRING, VOID,

    // Literals
    // Literais
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, CHAR_LITERAL,

    // Identifier
    // Identificador
    IDENTIFIER,

    // Operators
    // Operadores
    PLUS, MINUS, STAR, SLASH, ASSIGN,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,
    EQ, NEQ, LT, GT, LEQ, GEQ,
    AND, OR, NOT,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LSHIFT, RSHIFT,
    DOT, ARROW, AT, PIPE,

    // Delimiters
    // Delimitadores
    LBRACE, RBRACE, LPAREN, RPAREN,
    LBRACKET, RBRACKET, SEMICOLON, COMMA,

    // Special
    // Especial
    EOF_
};

struct SourceLocation {
    int line;
    int col;
    std::string file;
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation location;

    Token() : type(TokenType::EOF_), location{0, 0, ""} {}
    Token(TokenType t, std::string l, SourceLocation loc)
        : type(t), lexeme(std::move(l)), location(loc) {}
};

} // namespace meta_c
  // namespace meta_c

#endif
