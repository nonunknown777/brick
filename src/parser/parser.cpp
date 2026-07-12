#include "parser.h"
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <charconv>

namespace brick {

static std::string process_string_escapes(std::string_view raw) {
    std::string result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) {
        if (raw[i] == '\\' && i + 1 < raw.size()) {
            switch (raw[++i]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: throw std::runtime_error("invalid escape sequence in string");
            }
        } else {
            result += raw[i];
        }
    }
    return result;
}

static char process_char_escape(std::string_view raw) {
    if (raw.size() == 1) return raw[0];
    if (raw.size() == 2 && raw[0] == '\\') {
        switch (raw[1]) {
            case 'n': return '\n';
            case 't': return '\t';
            case '\\': return '\\';
            case '\'': return '\'';
            default: throw std::runtime_error("invalid escape sequence in char");
        }
    }
    throw std::runtime_error("invalid char literal");
}

static int64_t parse_int_literal(std::string_view sv, bool& is_hex) {
    int64_t val = 0;
    if (sv.size() > 2 && sv[0] == '0' && (sv[1] == 'x' || sv[1] == 'X')) {
        uint64_t uval = 0;
        std::from_chars(sv.data() + 2, sv.data() + sv.size(), uval, 16);
        val = static_cast<int64_t>(uval);
        is_hex = true;
    } else if (sv.size() > 2 && sv[0] == '0' && (sv[1] == 'b' || sv[1] == 'B')) {
        std::from_chars(sv.data() + 2, sv.data() + sv.size(), val, 2);
    } else if (sv.size() > 2 && sv[0] == '0' && (sv[1] == 'o' || sv[1] == 'O')) {
        std::from_chars(sv.data() + 2, sv.data() + sv.size(), val, 8);
    } else if (sv.size() > 1 && sv[0] == '0' && sv[1] >= '0' && sv[1] <= '7') {
        std::from_chars(sv.data(), sv.data() + sv.size(), val, 8);
    } else {
        std::from_chars(sv.data(), sv.data() + sv.size(), val);
    }
    return val;
}

static bool is_auto_semicolon(TokenType t) {
    switch (t) {
        case TokenType::IDENTIFIER:
        case TokenType::INT_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::CHAR_LITERAL:
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::NULL_:
        case TokenType::RPAREN:
        case TokenType::RBRACKET:
        case TokenType::RETURN:
        case TokenType::PLUS_PLUS:
        case TokenType::MINUS_MINUS:
            return true;
        default:
            return false;
    }
}

static std::vector<Token> insert_auto_semicolons(const std::vector<Token>& input) {
    std::vector<Token> output;
    if (input.empty()) return output;

    output.push_back(input[0]);
    for (size_t i = 1; i < input.size(); i++) {
        const Token& prev = input[i - 1];
        const Token& curr = input[i];

        bool has_newline = prev.location.line < curr.location.line;

        if (has_newline && is_auto_semicolon(prev.type)) {
            output.emplace_back(TokenType::SEMICOLON, ";", prev.location);
        }
        output.push_back(curr);
    }
    return output;
}

class Parser {
public:
    Parser(const std::vector<Token>& tokens)
        : tokens(insert_auto_semicolons(tokens)), pos(0) {}

    ParseResult parse_all() {
        ParseResult result;
        try {
            result.ast = program();
        } catch (const std::exception& e) {
            result.errors.push_back(e.what());
        }
        return result;
    }

private:
    std::vector<Token> tokens;
    size_t pos;

    const Token& peek() const {
        return tokens[pos];
    }

    const Token& advance() {
        return tokens[pos++];
    }

    bool match(TokenType type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }

    Token expect(TokenType type, const std::string& msg) {
        if (peek().type != type) {
            throw std::runtime_error(msg + " at " +
                std::to_string(peek().location.line) + ":" +
                std::to_string(peek().location.col));
        }
        return advance();
    }

    bool is_type_keyword(TokenType t) const {
        switch (t) {
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::CHAR:
            case TokenType::STRING:
            case TokenType::VOID:
            case TokenType::U8:
            case TokenType::U16:
            case TokenType::U32:
            case TokenType::U64:
            case TokenType::I8:
            case TokenType::I16:
            case TokenType::I32:
            case TokenType::I64:
            case TokenType::F32:
            case TokenType::F64:
            case TokenType::USIZE:
            case TokenType::ISIZE:
            case TokenType::BYTE:
            case TokenType::FN:
            case TokenType::BITFIELD_TYPE:
                return true;
            default:
                return false;
        }
    }

    bool is_type_start() const {
        return is_type_keyword(peek().type) || peek().type == TokenType::IDENTIFIER
            || peek().type == TokenType::STAR;
    }

    std::string parse_type_name() {
        std::string prefix;
        while (peek().type == TokenType::STAR) {
            prefix += "*";
            advance();
        }
        if (is_type_keyword(peek().type)) {
            // Function pointer type: fn(params)->ret
            if (peek().type == TokenType::FN) {
                advance();
                std::string fn_type = "fn(";
                expect(TokenType::LPAREN, "expected '(' after 'fn' in function pointer type");
                bool first_param = true;
                while (peek().type != TokenType::RPAREN) {
                    if (!first_param) {
                        expect(TokenType::COMMA, "expected ',' between function pointer params");
                        fn_type += ",";
                    }
                    first_param = false;
                    fn_type += parse_type_name();
                }
                fn_type += ")";
                expect(TokenType::RPAREN, "expected ')' to close function pointer params");
                expect(TokenType::ARROW, "expected '->' after function pointer params");
                fn_type += "->" + parse_type_name();
                return fn_type;
            }
            return prefix + std::string(advance().lexeme);
        }
        return prefix + std::string(expect(TokenType::IDENTIFIER, "expected type name").lexeme);
    }

    // ─── New parsing functions ───

    std::unique_ptr<ASTNode> break_stmt() {
        SourceLocation loc = peek().location;
        advance(); // consume BREAK
        return std::make_unique<BreakStmt>(loc);
    }

    std::unique_ptr<ASTNode> continue_stmt() {
        SourceLocation loc = peek().location;
        advance(); // consume CONTINUE
        return std::make_unique<ContinueStmt>(loc);
    }

    std::unique_ptr<ASTNode> defer_stmt() {
        SourceLocation loc = peek().location;
        advance(); // consume DEFER
        auto body = statement();
        return std::make_unique<DeferStmt>(std::move(body), loc);
    }

    std::unique_ptr<ASTNode> const_decl() {
        SourceLocation loc = peek().location;
        advance(); // consume CONST
        std::string type_name;
        std::string name;
        if (is_type_keyword(peek().type) || peek().type == TokenType::IDENTIFIER) {
            // Could be "const type name = expr" or "const name = expr" (type inferred)
            // Look ahead: if next token after type is IDENTIFIER and then ASSIGN, it's typed
            if ((is_type_keyword(peek().type) || peek().type == TokenType::IDENTIFIER) &&
                pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::IDENTIFIER) {
                // Check if there's an ASSIGN after the identifier
                if (pos + 2 < tokens.size() && tokens[pos + 2].type == TokenType::ASSIGN) {
                    type_name = parse_type_name();  // "const type name = val"
                }
            }
        }
        if (type_name.empty()) {
            // No explicit type: "const name = expr"
            if (is_type_keyword(peek().type)) {
                // Actually this IS a type keyword, so it's "const type name"
                type_name = parse_type_name();
            }
        }
        name = expect(TokenType::IDENTIFIER, "expected constant name").lexeme;
        auto cd = std::make_unique<ConstDecl>(name, loc);
        cd->type_name = type_name;
        if (match(TokenType::ASSIGN)) {
            cd->value = expression();
        }
        return cd;
    }

    std::unique_ptr<ASTNode> match_stmt() {
        SourceLocation loc = peek().location;
        advance(); // consume MATCH
        auto ms = std::make_unique<MatchStmt>(loc);
        ms->value = expression();
        expect(TokenType::LBRACE, "expected '{' after match expression");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            MatchStmt::Arm arm;
            // Parse patterns (comma-separated list, last can be _ wildcard)
            while (true) {
                if (peek().type == TokenType::IDENTIFIER && peek().lexeme == "_") {
                    advance();
                    arm.patterns.push_back(std::make_unique<IdentExpr>("_", peek().location));
                } else {
                    arm.patterns.push_back(expression());
                }
                if (peek().type == TokenType::COMMA) {
                    advance();
                    continue;
                }
                break;
            }
            // Optional guard: if condition
            if (peek().type == TokenType::IF) {
                advance();
                arm.guard = expression();
            }
            // Body is a block or statement
            if (peek().type == TokenType::LBRACE) {
                arm.body = block_stmt();
            } else {
                arm.body = statement();
            }
            ms->arms.push_back(std::move(arm));
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' after match body");
        return ms;
    }

    // ─── Top-level ───

    std::unique_ptr<ProgramNode> program() {
        auto prog = std::make_unique<ProgramNode>(peek().location);
        while (peek().type != TokenType::EOF_) {
            prog->declarations.push_back(declaration());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        return prog;
    }

    std::unique_ptr<ASTNode> declaration() {
        switch (peek().type) {
            case TokenType::PACKAGE: return package_decl();
            case TokenType::USING:   return using_decl();
            case TokenType::PRIVATE: return private_decl();
            case TokenType::STRUCT:  return struct_decl();
            case TokenType::UNION:   return union_decl();
            case TokenType::INTERFACE: return interface_decl();
            case TokenType::IMPL:    return impl_decl();
            case TokenType::BLOCK:   return block_decl_or_scope();
            case TokenType::FN:      return func_decl();
            case TokenType::EXPORT:  return export_decl();
            case TokenType::EXTERN:  return extern_decl();
            case TokenType::INCLUDE: return include_decl();
            case TokenType::LINK:    return link_decl();
            case TokenType::MACRO:   return macro_decl();
            case TokenType::BUILD:   return build_block();
            case TokenType::TYPE:   return type_alias_decl();
            case TokenType::ENUM:   return enum_decl();
            case TokenType::CONST:  return const_decl();
            case TokenType::IDENTIFIER:
                return expr_stmt();
            default:
                throw std::runtime_error("unexpected token '" + std::string(peek().lexeme) + "'");
        }
    }

    // ─── Macro ───

    std::unique_ptr<ASTNode> macro_decl() {
        SourceLocation loc = peek().location;
        advance();

        std::string name{expect(TokenType::IDENTIFIER, "expected macro name").lexeme};
        auto md = std::make_unique<MacroDecl>(name, loc);

        expect(TokenType::LPAREN, "expected '(' after macro name");

        if (peek().type != TokenType::RPAREN) {
            do {
                if (peek().type == TokenType::ELLIPSIS) {
                    advance();
                    if (peek().type == TokenType::IDENTIFIER) {
                        md->params.emplace_back(advance().lexeme);
                        md->has_varargs = true;
                    }
                    break;
                }
                std::string pname{expect(TokenType::IDENTIFIER, "expected macro parameter name").lexeme};
                md->params.push_back(pname);
                if (peek().type == TokenType::ELLIPSIS) {
                    advance();
                    md->has_varargs = true;
                    break;
                }
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "expected ')' after macro parameters");

        expect(TokenType::LBRACE, "expected '{' for macro body");
        // Parse body as statements, but handle $name and param names specially
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            md->body.push_back(parse_macro_body_stmt());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' to close macro body");

        return md;
    }

    std::unique_ptr<ASTNode> parse_macro_body_stmt() {
        // Statements inside a macro body
        switch (peek().type) {
            case TokenType::IF:     return if_stmt();
            case TokenType::WHILE:  return while_stmt();
            case TokenType::FOR:    return for_stmt();
            case TokenType::RETURN: return return_stmt();
            case TokenType::BLOCK:  return block_decl_or_scope();
            case TokenType::LBRACE: return block_stmt();
            case TokenType::FN:     return func_decl();
            case TokenType::STRUCT: return struct_decl();
            case TokenType::EMIT:   return emit_stmt();
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::CHAR:
            case TokenType::STRING:
            case TokenType::VOID:
            case TokenType::U8: case TokenType::U16: case TokenType::U32: case TokenType::U64:
            case TokenType::I8: case TokenType::I16: case TokenType::I32: case TokenType::I64:
            case TokenType::F32: case TokenType::F64:
            case TokenType::USIZE: case TokenType::ISIZE:
            case TokenType::BYTE:
            case TokenType::STAR:
                return var_decl_macro();
            default: {
                if (peek().type == TokenType::IDENTIFIER && pos + 1 < tokens.size()) {
                    if (tokens[pos + 1].type == TokenType::LBRACKET) {
                        // Disambiguate T[N] name (declaration) from arr[idx] = val (expression)
                        bool is_decl = (pos + 3 < tokens.size() &&
                                        tokens[pos + 2].type == TokenType::INT_LITERAL &&
                                        tokens[pos + 3].type == TokenType::RBRACKET &&
                                        pos + 4 < tokens.size() &&
                                        tokens[pos + 4].type == TokenType::IDENTIFIER);
                        if (is_decl) return var_decl();
                        return expr_stmt_macro();
                    }
                    if (tokens[pos + 1].type == TokenType::IDENTIFIER) {
                        return var_decl();
                    }
                }
                return expr_stmt_macro();
            }
        }
    }

    std::unique_ptr<ASTNode> expr_stmt_macro() {
        auto expr = expression_macro();
        if (peek().type == TokenType::SEMICOLON) advance();
        return std::make_unique<ExprStmt>(std::move(expr), expr->location);
    }

    std::unique_ptr<ASTNode> expression_macro() {
        return assignment_macro();
    }

    std::unique_ptr<ASTNode> assignment_macro() {
        auto left = logical_or_macro();
        if (peek().type == TokenType::ASSIGN ||
            peek().type == TokenType::PLUS_ASSIGN ||
            peek().type == TokenType::MINUS_ASSIGN ||
            peek().type == TokenType::STAR_ASSIGN ||
            peek().type == TokenType::SLASH_ASSIGN) {
            TokenType op = advance().type;
            auto value = assignment_macro();
            auto assign = std::make_unique<Assignment>(op, left->location);
            assign->target = std::move(left);
            assign->value = std::move(value);
            return assign;
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_or_macro() {
        auto left = logical_and_macro();
        while (peek().type == TokenType::OR) {
            TokenType op = advance().type;
            auto right = logical_and_macro();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_and_macro() {
        auto left = equality_macro();
        while (peek().type == TokenType::AND) {
            TokenType op = advance().type;
            auto right = equality_macro();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> equality_macro() {
        auto left = comparison_macro();
        while (peek().type == TokenType::EQ || peek().type == TokenType::NEQ) {
            TokenType op = advance().type;
            auto right = comparison_macro();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> comparison_macro() {
        auto left = term_macro();
        while (peek().type == TokenType::LT || peek().type == TokenType::GT ||
               peek().type == TokenType::LEQ || peek().type == TokenType::GEQ) {
            TokenType op = advance().type;
            auto right = term_macro();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> term_macro() {
        auto left = factor_macro();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            TokenType op = advance().type;
            auto right = factor_macro();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> factor_macro() {
        auto left = unary_macro();
        while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH ||
               peek().type == TokenType::BIT_AND) {
            TokenType op = advance().type;
            auto right = unary_macro();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> unary_macro() {
        if (peek().type == TokenType::NOT || peek().type == TokenType::MINUS ||
            peek().type == TokenType::STAR || peek().type == TokenType::BIT_AND) {
            TokenType op = advance().type;
            auto operand = unary_macro();
            return std::make_unique<UnaryOp>(op, std::move(operand), operand->location);
        }
        // Prefix ++/-- desugar to x += 1 / x -= 1
        if (peek().type == TokenType::PLUS_PLUS || peek().type == TokenType::MINUS_MINUS) {
            TokenType prefix_op = advance().type;
            SourceLocation loc = peek().location;
            auto target = unary_macro();
            TokenType assign_op = (prefix_op == TokenType::PLUS_PLUS)
                ? TokenType::PLUS_ASSIGN : TokenType::MINUS_ASSIGN;
            auto assign = std::make_unique<Assignment>(assign_op, loc);
            assign->target = std::move(target);
            assign->value = std::make_unique<IntLiteral>(1, loc);
            return assign;
        }
        return call_macro();
    }

    std::unique_ptr<ASTNode> call_macro() {
        auto expr = primary_macro();

        while (true) {
            if (peek().type == TokenType::LPAREN) {
                advance();
                auto call = std::make_unique<CallExpr>(expr->location);
                call->callee = std::move(expr);
                if (peek().type != TokenType::RPAREN) {
                    do {
                        call->arguments.push_back(expression_macro());
                    } while (match(TokenType::COMMA));
                }
                expect(TokenType::RPAREN, "expected ')' after arguments");
                expr = std::move(call);
            } else if (peek().type == TokenType::DOT) {
                advance();
                std::string member;
                if (peek().type == TokenType::RESET) {
                    member = std::string{advance().lexeme};
                } else {
                    member = std::string{expect(TokenType::IDENTIFIER, "expected member name").lexeme};
                }
                expr = std::make_unique<MemberExpr>(std::move(expr), member, expr->location);
            } else if (peek().type == TokenType::LBRACKET) {
                advance();
                auto index = std::make_unique<IndexExpr>(expr->location);
                index->array = std::move(expr);
                index->index = expression_macro();
                expect(TokenType::RBRACKET, "expected ']' after index");
                expr = std::move(index);
            } else if (peek().type == TokenType::PLUS_PLUS) {
                advance();
                SourceLocation loc = expr->location;
                auto assign = std::make_unique<Assignment>(TokenType::PLUS_ASSIGN, loc);
                assign->target = std::move(expr);
                assign->value = std::make_unique<IntLiteral>(1, loc);
                expr = std::move(assign);
            } else if (peek().type == TokenType::MINUS_MINUS) {
                advance();
                SourceLocation loc = expr->location;
                auto assign = std::make_unique<Assignment>(TokenType::MINUS_ASSIGN, loc);
                assign->target = std::move(expr);
                assign->value = std::make_unique<IntLiteral>(1, loc);
                expr = std::move(assign);
            } else if (peek().type == TokenType::AT) {
                advance();
                std::string block_name{expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme};
                expr = std::make_unique<AllocInline>(std::move(expr), block_name, expr->location);
            } else {
                break;
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> primary_macro() {
        SourceLocation loc = peek().location;

        // Handle $ interpolation
        if (peek().type == TokenType::DOLLAR) {
            advance();
            if (peek().type == TokenType::LPAREN) {
                advance();
                auto expr = expression_macro();
                expect(TokenType::RPAREN, "expected ')' after interpolated expression");
                return std::make_unique<Interpolate>(std::move(expr), loc);
            }
            // $identifier - interpolate parameter or variable
            std::string name{expect(TokenType::IDENTIFIER, "expected identifier after '$'").lexeme};
            return std::make_unique<Interpolate>(name, loc);
        }

        switch (peek().type) {
            case TokenType::INT_LITERAL: {
                Token t = advance();
                bool is_hex = false;
                int64_t val = parse_int_literal(t.lexeme, is_hex);
                auto lit = std::make_unique<IntLiteral>(val, loc, is_hex);
                lit->literal_type = t.literal_type;
                return lit;
            }
            case TokenType::FLOAT_LITERAL: {
                Token t = advance();
                double val = 0;
                std::from_chars(t.lexeme.data(), t.lexeme.data() + t.lexeme.size(), val);
                auto lit = std::make_unique<FloatLiteral>(val, loc);
                lit->literal_type = t.literal_type;
                return lit;
            }
            case TokenType::STRING_LITERAL:
                return std::make_unique<StringLiteral>(process_string_escapes(advance().lexeme), loc);
            case TokenType::CHAR_LITERAL:
                return std::make_unique<CharLiteral>(process_char_escape(advance().lexeme), loc);
            case TokenType::TRUE:
                advance();
                return std::make_unique<BoolLiteral>(true, loc);
            case TokenType::FALSE:
                advance();
                return std::make_unique<BoolLiteral>(false, loc);
            case TokenType::NULL_:
                advance();
                return std::make_unique<NullLiteral>(loc);
            case TokenType::ERROR: {
                advance();
                expect(TokenType::LPAREN, "expected '(' after error");
                auto msg = process_string_escapes(expect(TokenType::STRING_LITERAL, "expected error message").lexeme);
                expect(TokenType::RPAREN, "expected ')' after error message");
                auto call = std::make_unique<CallExpr>(loc);
                call->callee = std::make_unique<IdentExpr>("error", loc);
                call->arguments.push_back(std::make_unique<StringLiteral>(msg, loc));
                return call;
            }
            case TokenType::IDENTIFIER: {
                // Check if it's a parameter name — if so, emit ValuePlaceholder
                std::string id{advance().lexeme};
                if (in_macro_params.count(id)) {
                    return std::make_unique<ValuePlaceholder>(id, loc);
                }
                return std::make_unique<IdentExpr>(id, loc);
            }
            case TokenType::LPAREN: {
                advance();
                auto expr = expression_macro();
                expect(TokenType::RPAREN, "expected ')' after expression");
                return expr;
            }
            case TokenType::LBRACE: {
                advance();
                auto arr = std::make_unique<ArrayLiteral>(loc);
                if (peek().type != TokenType::RBRACE) {
                    arr->elements.push_back(expression_macro());
                    while (match(TokenType::COMMA)) {
                        if (peek().type == TokenType::RBRACE) break;
                        arr->elements.push_back(expression_macro());
                    }
                }
                expect(TokenType::RBRACE, "expected '}' after array literal");
                return arr;
            }
            // Type keywords as expressions (for casts and .sizeof)
            case TokenType::U8: case TokenType::U16: case TokenType::U32: case TokenType::U64:
            case TokenType::I8: case TokenType::I16: case TokenType::I32: case TokenType::I64:
            case TokenType::F32: case TokenType::F64:
            case TokenType::USIZE: case TokenType::ISIZE:
            case TokenType::BYTE:
            case TokenType::BOOL: case TokenType::CHAR:
            case TokenType::INT: case TokenType::FLOAT: case TokenType::STRING: case TokenType::VOID:
                return std::make_unique<IdentExpr>(std::string{advance().lexeme}, loc);
            default:
                throw std::runtime_error("unexpected token '" + std::string(peek().lexeme) + "' in expression");
        }
    }

    // Track macro parameters during body parsing
    std::unordered_set<std::string> in_macro_params;

    std::unique_ptr<ASTNode> var_decl_macro() {
        SourceLocation loc = peek().location;
        std::string type_name = parse_type_name();

        while (match(TokenType::LBRACKET)) {
            type_name += "[]";
            if (peek().type == TokenType::INT_LITERAL) {
                type_name = type_name.substr(0, type_name.size() - 1) + std::string(advance().lexeme) + "]";
            } else if (peek().type == TokenType::IDENTIFIER) {
                type_name = type_name.substr(0, type_name.size() - 1) + std::string(advance().lexeme) + "]";
            }
            expect(TokenType::RBRACKET, "expected ']' after array size");
        }

        // Support $interpolated variable names inside macro bodies
        std::unique_ptr<ASTNode> name_node;
        if (peek().type == TokenType::DOLLAR) {
            advance(); // consume $
            if (peek().type == TokenType::LPAREN) {
                advance();
                auto inner_expr = expression_macro();
                expect(TokenType::RPAREN, "expected ')' after interpolated expr");
                name_node = std::make_unique<Interpolate>(std::move(inner_expr), loc);
            } else if (peek().type == TokenType::IDENTIFIER) {
                std::string iname{advance().lexeme};
                name_node = std::make_unique<Interpolate>(iname, loc);
            } else {
                throw std::runtime_error("expected identifier or expr after '$'");
            }
        } else {
            std::string name{expect(TokenType::IDENTIFIER, "expected variable name").lexeme};
            name_node = std::make_unique<IdentExpr>(name, loc);
            static_cast<IdentExpr*>(name_node.get())->declared_type = type_name;
        }

        if (match(TokenType::ASSIGN)) {
            auto init = expression_macro();
            if (peek().type == TokenType::AT) {
                advance();
                std::string block_name{expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme};
                init = std::make_unique<AllocInline>(std::move(init), block_name, init->location);
            }
            auto assign = std::make_unique<Assignment>(TokenType::ASSIGN, loc);
            assign->target = std::move(name_node);
            assign->value = std::move(init);
            return std::make_unique<ExprStmt>(std::move(assign), loc);
        }

        if (peek().type == TokenType::AT) {
            advance();
            expect(TokenType::IDENTIFIER, "expected block name after '@'");
        }

        return std::make_unique<ExprStmt>(std::move(name_node), loc);
    }

    // ─── Build block ───

    std::unique_ptr<ASTNode> build_block() {
        SourceLocation loc = peek().location;
        advance();
        expect(TokenType::LBRACE, "expected '{' after build");
        auto bb = std::make_unique<BuildBlock>(loc);
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            bb->body.push_back(build_body_stmt());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' to close build block");
        return bb;
    }

    std::unique_ptr<ASTNode> build_body_stmt() {
        if (peek().type == TokenType::EMIT) {
            return emit_stmt();
        }
        if (peek().type == TokenType::IF) return if_stmt_build();
        if (peek().type == TokenType::FOR) return for_stmt_build();
        if (peek().type == TokenType::WHILE) return while_stmt();

        // Fallback: expression
        auto expr = expression_build();
        if (peek().type == TokenType::SEMICOLON) advance();
        return std::make_unique<ExprStmt>(std::move(expr), expr->location);
    }

    std::unique_ptr<ASTNode> if_stmt_build() {
        advance(); // consume 'if'
        auto is = std::make_unique<IfStmt>(peek().location);
        is->condition = expression_build();
        if (peek().type == TokenType::LBRACE) {
            is->then_branch = build_block_body();
        } else {
            is->then_branch = build_body_stmt();
        }
        if (match(TokenType::ELSE)) {
            if (peek().type == TokenType::IF) {
                is->else_branch = if_stmt_build();
            } else if (peek().type == TokenType::LBRACE) {
                is->else_branch = build_block_body();
            } else {
                is->else_branch = build_body_stmt();
            }
        }
        return is;
    }

    std::unique_ptr<ASTNode> for_stmt_build() {
        advance();
        auto fs = std::make_unique<ForStmt>(peek().location);

        // for identifier in expr { body }
        if (peek().type == TokenType::IDENTIFIER) {
            std::string var_name{advance().lexeme};
            if (match(TokenType::IDENTIFIER) && peek().lexeme == "in") {
                advance();
                auto iter = expression_build();
                std::vector<std::unique_ptr<ASTNode>> body;
                if (peek().type == TokenType::LBRACE) {
                    body = build_block_body_body();
                }
                // Generate loop AST
                // Create a for statement: var index = 0; index < iter.len(); index++
                // For collections, we use a simple "for each" approach
                // Simplified: just store the iterator
                auto init = std::make_unique<ExprStmt>(
                    std::make_unique<IdentExpr>(var_name, fs->location), fs->location);
                fs->init = std::move(init);
                fs->condition = std::move(iter);
                // Body
                auto body_block = std::make_unique<BlockStmt>(fs->location);
                for (auto& s : body) body_block->statements.push_back(std::move(s));
                fs->body = std::move(body_block);
                return fs;
            }
        }

        // Default for(;;) style
        if (peek().type == TokenType::LPAREN) {
            advance();
            if (is_type_start()) {
                fs->init = var_decl();
            } else if (peek().type != TokenType::SEMICOLON) {
                fs->init = expression_build();
            }
            expect(TokenType::SEMICOLON, "expected ';' after for init");
            if (peek().type != TokenType::SEMICOLON) {
                fs->condition = expression_build();
            }
            expect(TokenType::SEMICOLON, "expected ';' after for condition");
            if (peek().type != TokenType::RPAREN) {
                fs->increment = expression_build();
            }
            expect(TokenType::RPAREN, "expected ')' after for clauses");
        }

        if (peek().type == TokenType::LBRACE) {
            fs->body = build_block_body();
        }
        return fs;
    }

    std::unique_ptr<BlockStmt> build_block_body() {
        auto bs = std::make_unique<BlockStmt>(peek().location);
        expect(TokenType::LBRACE, "expected '{'");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            bs->statements.push_back(build_body_stmt());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}'");
        return bs;
    }

    std::vector<std::unique_ptr<ASTNode>> build_block_body_body() {
        std::vector<std::unique_ptr<ASTNode>> stmts;
        expect(TokenType::LBRACE, "expected '{'");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            stmts.push_back(build_body_stmt());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}'");
        return stmts;
    }

    std::unique_ptr<ASTNode> var_decl_build() {
        SourceLocation loc = peek().location;
        // No type in build blocks (inferred)
        std::string name{expect(TokenType::IDENTIFIER, "expected variable name").lexeme};

        auto target = std::make_unique<IdentExpr>(name, loc);
        if (match(TokenType::ASSIGN)) {
            auto init = expression_build();
            auto assign = std::make_unique<Assignment>(TokenType::ASSIGN, loc);
            assign->target = std::move(target);
            assign->value = std::move(init);
            return std::make_unique<ExprStmt>(std::move(assign), loc);
        }
        return std::make_unique<ExprStmt>(std::move(target), loc);
    }

    std::unique_ptr<ASTNode> expression_build() {
        return assignment_build();
    }

    std::unique_ptr<ASTNode> assignment_build() {
        auto left = logical_or_build();
        if (peek().type == TokenType::ASSIGN) {
            advance();
            auto value = assignment_build();
            auto assign = std::make_unique<Assignment>(TokenType::ASSIGN, left->location);
            assign->target = std::move(left);
            assign->value = std::move(value);
            return assign;
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_or_build() {
        auto left = logical_and_build();
        while (peek().type == TokenType::OR) {
            advance();
            auto right = logical_and_build();
            left = std::make_unique<BinaryOp>(std::move(left), TokenType::OR, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_and_build() {
        auto left = equality_build();
        while (peek().type == TokenType::AND) {
            advance();
            auto right = equality_build();
            left = std::make_unique<BinaryOp>(std::move(left), TokenType::AND, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> equality_build() {
        auto left = comparison_build();
        while (peek().type == TokenType::EQ || peek().type == TokenType::NEQ) {
            TokenType op = advance().type;
            auto right = comparison_build();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> comparison_build() {
        auto left = term_build();
        while (peek().type == TokenType::LT || peek().type == TokenType::GT ||
               peek().type == TokenType::LEQ || peek().type == TokenType::GEQ) {
            TokenType op = advance().type;
            auto right = term_build();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> term_build() {
        auto left = factor_build();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            TokenType op = advance().type;
            auto right = factor_build();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> factor_build() {
        auto left = unary_build();
        while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH ||
               peek().type == TokenType::BIT_AND) {
            TokenType op = advance().type;
            auto right = unary_build();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> unary_build() {
        if (peek().type == TokenType::NOT || peek().type == TokenType::MINUS) {
            TokenType op = advance().type;
            auto operand = unary_build();
            return std::make_unique<UnaryOp>(op, std::move(operand), operand->location);
        }
        return call_build();
    }

    std::unique_ptr<ASTNode> call_build() {
        auto expr = primary_build();
        while (true) {
            if (peek().type == TokenType::LBRACKET && peek().lexeme == "[") {
                advance();
                auto index = std::make_unique<IndexExpr>(expr->location);
                index->array = std::move(expr);
                index->index = expression_build();
                expect(TokenType::RBRACKET, "expected ']' after index");
                expr = std::move(index);
            } else if (peek().type == TokenType::DOT) {
                advance();
                std::string member{expect(TokenType::IDENTIFIER, "expected member name").lexeme};
                expr = std::make_unique<MemberExpr>(std::move(expr), member, expr->location);
            } else if (peek().type == TokenType::LPAREN) {
                advance();
                auto call = std::make_unique<CallExpr>(expr->location);
                call->callee = std::move(expr);
                if (peek().type != TokenType::RPAREN) {
                    do {
                        call->arguments.push_back(expression_build());
                    } while (match(TokenType::COMMA));
                }
                expect(TokenType::RPAREN, "expected ')' after arguments");
                expr = std::move(call);
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<ASTNode> primary_build() {
        SourceLocation loc = peek().location;

        // Handle [ for list literals
        if (peek().type == TokenType::LBRACKET) {
            advance();
            std::vector<std::unique_ptr<ASTNode>> elements;
            if (peek().type != TokenType::RBRACKET) {
                do {
                    elements.push_back(expression_build());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RBRACKET, "expected ']' after list");
            if (elements.empty()) {
                return std::make_unique<NullLiteral>(loc);
            }
            return std::make_unique<IdentExpr>("__list_literal", loc);
        }

        switch (peek().type) {
            case TokenType::INT_LITERAL: {
                Token t = advance();
                bool is_hex = false;
                int64_t val = parse_int_literal(t.lexeme, is_hex);
                auto lit = std::make_unique<IntLiteral>(val, loc, is_hex);
                return lit;
            }
            case TokenType::FLOAT_LITERAL: {
                Token t = advance();
                double val = 0;
                std::from_chars(t.lexeme.data(), t.lexeme.data() + t.lexeme.size(), val);
                return std::make_unique<FloatLiteral>(val, loc);
            }
            case TokenType::STRING_LITERAL:
                return std::make_unique<StringLiteral>(process_string_escapes(advance().lexeme), loc);
            case TokenType::TRUE: advance(); return std::make_unique<BoolLiteral>(true, loc);
            case TokenType::FALSE: advance(); return std::make_unique<BoolLiteral>(false, loc);
            case TokenType::NULL_: advance(); return std::make_unique<NullLiteral>(loc);
            case TokenType::IDENTIFIER:
                return std::make_unique<IdentExpr>(std::string{advance().lexeme}, loc);
            case TokenType::LPAREN: {
                advance();
                auto expr = expression_build();
                expect(TokenType::RPAREN, "expected ')'");
                return expr;
            }
            case TokenType::U8: case TokenType::U16: case TokenType::U32: case TokenType::U64:
            case TokenType::I8: case TokenType::I16: case TokenType::I32: case TokenType::I64:
            case TokenType::F32: case TokenType::F64:
            case TokenType::USIZE: case TokenType::ISIZE:
            case TokenType::BYTE:
            case TokenType::BOOL: case TokenType::CHAR:
                return std::make_unique<IdentExpr>(std::string{advance().lexeme}, loc);
            default:
                throw std::runtime_error("unexpected token '" + std::string(peek().lexeme) + "' in build expression");
        }
    }

    // ─── Emit statement ───

    std::unique_ptr<ASTNode> emit_stmt() {
        SourceLocation loc = peek().location;
        advance(); // consume 'emit'

        if (peek().type == TokenType::LBRACE) {
            // emit { code }
            advance();
            auto block = std::make_unique<BlockStmt>(loc);
            // Capture everything until matching }
            int brace_depth = 1;
            std::vector<std::unique_ptr<ASTNode>> body_stmts;
            while (peek().type != TokenType::EOF_) {
                if (peek().type == TokenType::LBRACE) brace_depth++;
                if (peek().type == TokenType::RBRACE) {
                    brace_depth--;
                    if (brace_depth == 0) break;
                }
                // Parse a statement
                if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
                body_stmts.push_back(parse_macro_body_stmt());
                if (peek().type == TokenType::SEMICOLON) advance();
            }
            expect(TokenType::RBRACE, "expected '}' to close emit body");
            {
                auto content_block = std::make_unique<BlockStmt>(loc);
                for (auto& s : body_stmts)
                    content_block->statements.push_back(std::move(s));
                return std::make_unique<EmitStmt>(std::move(content_block), loc);
            }
        }

        // emit name(args) — shorthand for macro call
        if (peek().type == TokenType::IDENTIFIER) {
            std::string name{advance().lexeme};
            auto call = std::make_unique<MacroCall>(name, loc);
            if (peek().type == TokenType::LPAREN) {
                advance();
                if (peek().type != TokenType::RPAREN) {
                    do {
                        call->args.push_back(expression_macro());
                    } while (match(TokenType::COMMA));
                }
                expect(TokenType::RPAREN, "expected ')' after macro arguments");
            }
            return std::make_unique<EmitStmt>(std::move(call), loc);
        }

        throw std::runtime_error("expected '{' or macro name after 'emit'");
    }

    // ─── Enum declaration (for use inside macros) ───

    std::unique_ptr<ASTNode> enum_decl() {
        SourceLocation loc = peek().location;
        advance(); // consume ENUM
        std::string name{expect(TokenType::IDENTIFIER, "expected enum name").lexeme};
        auto ed = std::make_unique<EnumDecl>(name, loc);
        expect(TokenType::LBRACE, "expected '{' after enum name");
        int64_t next_val = 0;
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            std::string vname{expect(TokenType::IDENTIFIER, "expected variant name").lexeme};
            EnumDecl::Variant var;
            var.name = vname;
            if (peek().type == TokenType::ASSIGN) {
                advance();
                int64_t val = 0;
                bool negate = false;
                if (peek().type == TokenType::MINUS) {
                    advance();
                    negate = true;
                }
                if (peek().type == TokenType::INT_LITERAL) {
                    bool is_hex = false;
                    val = parse_int_literal(advance().lexeme, is_hex);
                    if (negate) val = -val;
                }
                var.value = val;
                var.has_explicit_value = true;
                next_val = val + 1;
            } else {
                var.value = next_val;
                next_val++;
            }
            ed->variants.push_back(std::move(var));
            if (peek().type == TokenType::COMMA) advance();
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' after enum body");
        return ed;
    }

    // ─── Existing declarations (unchanged) ───

    std::unique_ptr<ASTNode> package_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::vector<std::string> parts;
        parts.emplace_back(expect(TokenType::IDENTIFIER, "expected package name").lexeme);
        while (match(TokenType::DOT)) {
            parts.emplace_back(expect(TokenType::IDENTIFIER, "expected sub-package name").lexeme);
        }
        return std::make_unique<PackageDecl>(std::move(parts), loc);
    }

    std::unique_ptr<ASTNode> using_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::vector<std::string> parts;
        parts.emplace_back(expect(TokenType::IDENTIFIER, "expected package name").lexeme);
        while (match(TokenType::DOT)) {
            parts.emplace_back(expect(TokenType::IDENTIFIER, "expected sub-package name").lexeme);
        }
        return std::make_unique<UsingDecl>(std::move(parts), loc);
    }

    std::unique_ptr<ASTNode> private_decl() {
        advance();
        auto decl = declaration();
        auto set_private = [](ASTNode* node) {
            if (auto* sd = dynamic_cast<StructDecl*>(node)) sd->is_private = true;
            else if (auto* fd = dynamic_cast<FuncDecl*>(node)) fd->is_private = true;
            else if (auto* fld = dynamic_cast<FieldDecl*>(node)) fld->is_private = true;
            else if (auto* cd = dynamic_cast<ConstDecl*>(node)) cd->is_private = true;
            else if (auto* ed = dynamic_cast<EnumDecl*>(node)) ed->is_private = true;
            else if (auto* ud = dynamic_cast<UnionDecl*>(node)) ud->is_private = true;
            else if (auto* id = dynamic_cast<InterfaceDecl*>(node)) id->is_private = true;
            else if (auto* ta = dynamic_cast<TypeAliasDecl*>(node)) ta->is_private = true;
            else if (auto* md = dynamic_cast<MacroDecl*>(node)) md->is_private = true;
        };
        set_private(decl.get());
        return decl;
    }

    std::unique_ptr<ASTNode> struct_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::string name{expect(TokenType::IDENTIFIER, "expected struct name").lexeme};
        auto sd = std::make_unique<StructDecl>(name, loc);

        while (peek().type == TokenType::AT) {
            advance();
            if (peek().type == TokenType::IDENTIFIER && peek().lexeme == "packed") {
                advance();
                sd->packed = true;
            } else if (peek().type == TokenType::IDENTIFIER && peek().lexeme == "align") {
                advance();
                expect(TokenType::LPAREN, "expected '(' after @align");
                int64_t align_val = 0;
                bool is_hex = false;
                align_val = parse_int_literal(
                    expect(TokenType::INT_LITERAL, "expected alignment value").lexeme, is_hex);
                sd->alignment = static_cast<int>(align_val);
                expect(TokenType::RPAREN, "expected ')' after @align value");
            } else {
                throw std::runtime_error("expected '@packed' or '@align(N)' after struct name");
            }
        }

        if (match(TokenType::EXTENDS)) {
            sd->extends = std::string{expect(TokenType::IDENTIFIER, "expected base struct name").lexeme};
            while (match(TokenType::COMMA)) {
                sd->interfaces.emplace_back(
                    expect(TokenType::IDENTIFIER, "expected interface name").lexeme);
            }
        }

        expect(TokenType::LBRACE, "expected '{' after struct name");

        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }

            if (peek().type == TokenType::FN) {
                sd->methods.push_back(func_decl());
            } else if (peek().type == TokenType::UNION) {
                sd->fields.push_back(union_decl());
            } else {
                auto field = field_decl();
                if (field) sd->fields.push_back(std::move(field));
            }
            if (peek().type == TokenType::SEMICOLON) advance();
        }

        expect(TokenType::RBRACE, "expected '}' after struct body");
        return sd;
    }

    std::unique_ptr<ASTNode> union_decl() {
        SourceLocation loc = peek().location;
        advance();

        // Anonymous union inside struct: union { ... }
        // Uniao anonima dentro de struct: union { ... }
        if (peek().type == TokenType::LBRACE) {
            auto ud = std::make_unique<UnionDecl>(loc);
            advance();
            while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
                if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
                if (peek().type == TokenType::STRUCT) {
                    advance();
                    auto sd = std::make_unique<StructDecl>("", loc);
                    sd->is_anonymous = true;
                    expect(TokenType::LBRACE, "expected '{' after anonymous struct");
                    while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
                        if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
                        auto field = field_decl();
                        if (field) sd->fields.push_back(std::move(field));
                        if (peek().type == TokenType::SEMICOLON) advance();
                    }
                    expect(TokenType::RBRACE, "expected '}' to close anonymous struct");
                    ud->fields.push_back(std::move(sd));
                } else {
                    auto field = field_decl();
                    if (field) ud->fields.push_back(std::move(field));
                }
                if (peek().type == TokenType::SEMICOLON) advance();
            }
            expect(TokenType::RBRACE, "expected '}' to close anonymous union");
            return ud;
        }

        // Named union: union Name { ... }
        // Uniao nomeada: union Name { ... }
        std::string name{expect(TokenType::IDENTIFIER, "expected union name").lexeme};
        auto ud = std::make_unique<UnionDecl>(name, loc);
        expect(TokenType::LBRACE, "expected '{' after union name");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            if (peek().type == TokenType::STRUCT) {
                advance();
                auto sd = std::make_unique<StructDecl>("", loc);
                sd->is_anonymous = true;
                expect(TokenType::LBRACE, "expected '{' after anonymous struct");
                while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
                    if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
                    auto field = field_decl();
                    if (field) sd->fields.push_back(std::move(field));
                    if (peek().type == TokenType::SEMICOLON) advance();
                }
                expect(TokenType::RBRACE, "expected '}' to close anonymous struct");
                ud->fields.push_back(std::move(sd));
            } else {
                auto field = field_decl();
                if (field) ud->fields.push_back(std::move(field));
            }
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' after union body");
        return ud;
    }

    std::unique_ptr<ASTNode> field_decl() {
        SourceLocation loc = peek().location;
        std::string type_name = parse_type_name();

        while (match(TokenType::LBRACKET)) {
            type_name += "[]";
            if (peek().type == TokenType::INT_LITERAL) {
                type_name = type_name.substr(0, type_name.size() - 1) + std::string(advance().lexeme) + "]";
            } else if (peek().type == TokenType::IDENTIFIER) {
                type_name = type_name.substr(0, type_name.size() - 1) + std::string(advance().lexeme) + "]";
            }
            expect(TokenType::RBRACKET, "expected ']' after array size");
        }

        std::string name{expect(TokenType::IDENTIFIER, "expected field name").lexeme};

        // Bitfield syntax: name : width
        // Sintaxe bitfield: nome : largura
        int bit_width = 0;
        if (match(TokenType::COLON)) {
            std::string_view wv = expect(TokenType::INT_LITERAL, "expected bit width after ':'").lexeme;
            std::from_chars(wv.data(), wv.data() + wv.size(), bit_width);
        }

        auto fd = std::make_unique<FieldDecl>(type_name, name, loc);
        fd->bit_width = bit_width;
        return fd;
    }

    std::unique_ptr<ASTNode> interface_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::string name{expect(TokenType::IDENTIFIER, "expected interface name").lexeme};
        auto id = std::make_unique<InterfaceDecl>(name, loc);

        expect(TokenType::LBRACE, "expected '{' after interface name");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }

            expect(TokenType::FN, "expected 'fn' in interface method");
            auto method = interface_method_decl();
            id->methods.push_back(std::move(method));

            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' after interface body");
        return id;
    }

    std::unique_ptr<ASTNode> type_alias_decl() {
        SourceLocation loc = peek().location;
        advance();
        std::string alias{expect(TokenType::IDENTIFIER, "expected type alias name").lexeme};
        expect(TokenType::ASSIGN, "expected '=' in type alias");
        std::string underlying = parse_type_name();
        // Handle array suffix on type alias
        while (match(TokenType::LBRACKET)) {
            underlying += "[]";
            if (peek().type == TokenType::INT_LITERAL) {
                underlying = underlying.substr(0, underlying.size() - 1) + std::string(advance().lexeme) + "]";
            } else if (peek().type == TokenType::IDENTIFIER) {
                underlying = underlying.substr(0, underlying.size() - 1) + std::string(advance().lexeme) + "]";
            }
            expect(TokenType::RBRACKET, "expected ']' after array size");
        }
        return std::make_unique<TypeAliasDecl>(alias, underlying, loc);
    }

    std::unique_ptr<ASTNode> interface_method_decl() {
        SourceLocation loc = peek().location;
        std::string name{expect(TokenType::IDENTIFIER, "expected method name").lexeme};
        auto fd = std::make_unique<FuncDecl>(name, loc);

        param_list(fd.get());

        if (match(TokenType::ARROW)) {
            fd->return_type = parse_type_name();
        }

        return fd;
    }

    std::unique_ptr<ASTNode> func_decl() {
        SourceLocation loc = peek().location;
        advance();

        // Support $interpolated function names inside macro bodies
        std::unique_ptr<ASTNode> name_expr;
        std::string name;
        if (peek().type == TokenType::DOLLAR) {
            advance();
            if (peek().type == TokenType::LPAREN) {
                advance();
                auto inner_expr = expression_macro();
                expect(TokenType::RPAREN, "expected ')' after interpolated expr");
                name_expr = std::make_unique<Interpolate>(std::move(inner_expr), loc);
            } else if (peek().type == TokenType::IDENTIFIER) {
                std::string iname{advance().lexeme};
                name_expr = std::make_unique<Interpolate>(iname, loc);
            } else {
                throw std::runtime_error("expected identifier or expr after '$' in function name");
            }
        } else {
            name = std::string{expect(TokenType::IDENTIFIER, "expected function name").lexeme};
        }
        auto fd = std::make_unique<FuncDecl>(name, loc);
        if (name_expr) fd->name_expr = std::move(name_expr);

        param_list(fd.get());

        if (match(TokenType::ARROW)) {
            fd->return_type = parse_type_name();
        }

        fd->body = block_stmt();
        return fd;
    }

    std::unique_ptr<ASTNode> impl_decl() {
        SourceLocation loc = peek().location;
        advance(); // consume 'impl'

        std::string struct_name{expect(TokenType::IDENTIFIER, "expected struct name after 'impl'").lexeme};

        if (peek().type == TokenType::COLON) {
            advance(); // consume ':'
        } else {
            // Single colon syntax: impl Struct : Interface
            // If no colon, it's impl Struct { ... } without interface
        }
        std::string interface_name;
        if (peek().type == TokenType::IDENTIFIER) {
            interface_name = std::string{advance().lexeme};
        }

        auto id = std::make_unique<ImplDecl>(struct_name, interface_name, loc);

        expect(TokenType::LBRACE, "expected '{' after impl header");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            // func_decl consumes 'fn' itself
            id->methods.push_back(func_decl());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' after impl body");

        return id;
    }

    std::unique_ptr<ASTNode> export_decl() {
        advance(); // consume 'export'
        auto decl = declaration();
        if (auto* fd = dynamic_cast<FuncDecl*>(decl.get())) {
            fd->is_export = true;
        } else if (auto* sd = dynamic_cast<StructDecl*>(decl.get())) {
            sd->is_private = false; // explicit public
        } else if (auto* cd = dynamic_cast<ConstDecl*>(decl.get())) {
            cd->is_private = false;
        } else if (auto* ed = dynamic_cast<EnumDecl*>(decl.get())) {
            ed->is_private = false;
        } else if (auto* ud = dynamic_cast<UnionDecl*>(decl.get())) {
            ud->is_private = false;
        } else if (auto* id = dynamic_cast<InterfaceDecl*>(decl.get())) {
            id->is_private = false;
        } else if (auto* ta = dynamic_cast<TypeAliasDecl*>(decl.get())) {
            ta->is_private = false;
        } else if (auto* md = dynamic_cast<MacroDecl*>(decl.get())) {
            md->is_private = false;
        } else {
            throw std::runtime_error("'export' can only be applied to functions, structs, consts, enums, unions, interfaces, type aliases, or macros");
        }
        return decl;
    }

    std::unique_ptr<ASTNode> extern_decl() {
        SourceLocation loc = peek().location;
        advance();

        expect(TokenType::FN, "expected 'fn' after 'extern'");
        std::string name{expect(TokenType::IDENTIFIER, "expected function name").lexeme};
        auto fd = std::make_unique<FuncDecl>(name, loc);
        fd->is_extern = true;

        param_list(fd.get(), true);  // allow variadic ...

        if (match(TokenType::ARROW)) {
            fd->return_type = parse_type_name();
        }

        return fd;
    }

    std::unique_ptr<ASTNode> include_decl() {
        SourceLocation loc = peek().location;
        advance();

        std::string header{expect(TokenType::STRING_LITERAL, "expected header path").lexeme};
        auto inc = std::make_unique<IncludeDecl>(header, loc);

        // Optional @system attribute → #include <...> instead of "..."
        // Atributo @system opcional → #include <...> em vez de "..."
        if (peek().type == TokenType::AT) {
            size_t save = pos;
            advance();
            if (peek().type == TokenType::IDENTIFIER && peek().lexeme == "system") {
                advance();
                inc->is_system = true;
            } else {
                pos = save; // Not @system, restore
            }
        }

        if (peek().type == TokenType::AND) {
            advance();
            expect(TokenType::LINK, "expected 'link' after 'and'");
            inc->link_lib = std::string{expect(TokenType::IDENTIFIER, "expected library name").lexeme};
        }

        return inc;
    }

    std::unique_ptr<ASTNode> link_decl() {
        SourceLocation loc = peek().location;
        advance();

        std::string lib{expect(TokenType::IDENTIFIER, "expected library name").lexeme};
        return std::make_unique<LinkDecl>(lib, loc);
    }

    void param_list(FuncDecl* fd, bool allow_variadic = false) {
        expect(TokenType::LPAREN, "expected '(' after function name");
        if (peek().type != TokenType::RPAREN) {
            do {
                // Variadic ... (only allowed as last param for extern fn)
                if (allow_variadic && peek().type == TokenType::ELLIPSIS) {
                    advance(); // consume ...
                    fd->is_variadic = true;
                    break;
                }
                SourceLocation loc = peek().location;
                std::string type_name = parse_type_name();
                // Allow unnamed params for extern fn (e.g. "int", "void" without name)
                std::string name;
                if (peek().type == TokenType::IDENTIFIER &&
                    peek().lexeme != "(" && peek().lexeme != ")" &&
                    peek().lexeme != "," && peek().lexeme != ";") {
                    name = std::string{advance().lexeme};
                }
                auto pd = std::make_unique<ParamDecl>(type_name, name, loc);
                if (!allow_variadic && peek().type == TokenType::ASSIGN) {
                    advance();
                    pd->default_value = expression();
                }
                fd->params.push_back(std::move(pd));
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "expected ')' after parameters");
    }

    // ─── Block decl/scope (unchanged) ───

    std::unique_ptr<ASTNode> block_decl_or_scope() {
        SourceLocation loc = peek().location;
        advance();
        std::string name{expect(TokenType::IDENTIFIER, "expected block name").lexeme};

        if (match(TokenType::ASSIGN)) {
            int64_t size = 0;
            if (peek().type == TokenType::INT_LITERAL) {
                std::string_view sv = advance().lexeme;
                std::from_chars(sv.data(), sv.data() + sv.size(), size);
            }
            std::string unit = "B";
            if (peek().type == TokenType::IDENTIFIER) {
                unit = std::string{advance().lexeme};
            }
            return std::make_unique<BlockDecl>(name, size, unit, loc);
        }

        // block name: changes default block for following scope
        if (match(TokenType::COLON)) {
            auto bs = std::make_unique<BlockScope>(name, loc);
            bs->is_default_change = true;
            return bs;
        }

        if (match(TokenType::LBRACE)) {
            auto bs = std::make_unique<BlockScope>(name, loc);
            while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
                if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
                bs->body.push_back(statement());
                if (peek().type == TokenType::SEMICOLON) advance();
            }
            expect(TokenType::RBRACE, "expected '}' after block scope");
            return bs;
        }

        return std::make_unique<BlockScope>(name, loc);
    }

    // ─── Statements (unchanged) ───

    std::unique_ptr<BlockStmt> block_stmt() {
        auto bs = std::make_unique<BlockStmt>(peek().location);
        expect(TokenType::LBRACE, "expected '{' for block");
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::EOF_) {
            if (peek().type == TokenType::SEMICOLON) { advance(); continue; }
            bs->statements.push_back(statement());
            if (peek().type == TokenType::SEMICOLON) advance();
        }
        expect(TokenType::RBRACE, "expected '}' to close block");
        return bs;
    }

    std::unique_ptr<ASTNode> statement() {
        switch (peek().type) {
            case TokenType::IF:     return if_stmt();
            case TokenType::WHILE:  return while_stmt();
            case TokenType::BREAK:    return break_stmt();
            case TokenType::CONTINUE: return continue_stmt();
            case TokenType::DEFER:    return defer_stmt();
            case TokenType::MATCH:    return match_stmt();
            case TokenType::ENUM:     return enum_decl();
            case TokenType::CONST:    return const_decl();
            case TokenType::FOR:    return for_stmt();
            case TokenType::RETURN: return return_stmt();
            case TokenType::BLOCK:  return block_decl_or_scope();
            case TokenType::LBRACE: return block_stmt();
            case TokenType::MACRO:   return macro_decl();
            case TokenType::BUILD:  return build_block();
            case TokenType::EMIT:   return emit_stmt();
            case TokenType::TYPE:   return type_alias_decl();
            case TokenType::INT:
            case TokenType::FLOAT:
            case TokenType::BOOL:
            case TokenType::CHAR:
            case TokenType::STRING:
            case TokenType::VOID:
            case TokenType::U8:
            case TokenType::U16:
            case TokenType::U32:
            case TokenType::U64:
            case TokenType::I8:
            case TokenType::I16:
            case TokenType::I32:
            case TokenType::I64:
            case TokenType::F32:
            case TokenType::F64:
            case TokenType::USIZE:
            case TokenType::ISIZE:
            case TokenType::BYTE:
            case TokenType::FN:
            case TokenType::BITFIELD_TYPE:
                return var_decl();
            case TokenType::STAR: {
                // Distinguish *int x (var decl) from *p = 42 (deref expr)
                // Olha adiante: * + tipo → declaracao, * + expr → dereferencia
                if (pos + 1 < tokens.size() && is_type_keyword(tokens[pos + 1].type))
                    return var_decl();
                if (pos + 2 < tokens.size() &&
                    tokens[pos + 1].type == TokenType::IDENTIFIER &&
                    tokens[pos + 2].type == TokenType::IDENTIFIER)
                    return var_decl();
                if (pos + 2 < tokens.size() &&
                    tokens[pos + 1].type == TokenType::IDENTIFIER &&
                    tokens[pos + 2].type == TokenType::LBRACKET)
                    return var_decl();
                return expr_stmt();
            }
            default:
                if (peek().type == TokenType::IDENTIFIER && pos + 1 < tokens.size()) {
                    // Disambiguate T[N] name (declaration) from arr[idx] = val (expression)
                    if (tokens[pos + 1].type == TokenType::LBRACKET) {
                        // Skip all [N] or [] pairs to find variable name
                        // Pula todos os pares [N] ou [] para achar o nome da variavel
                        size_t lp = pos + 1;
                        while (lp < tokens.size() && tokens[lp].type == TokenType::LBRACKET) {
                            if (lp + 1 < tokens.size() && tokens[lp + 1].type == TokenType::RBRACKET) {
                                lp += 2; // T[]
                            } else if (lp + 2 < tokens.size() && tokens[lp + 2].type == TokenType::RBRACKET &&
                                       (tokens[lp + 1].type == TokenType::INT_LITERAL ||
                                        tokens[lp + 1].type == TokenType::IDENTIFIER)) {
                                lp += 3; // T[N]
                            } else {
                                break;
                            }
                        }
                        if (lp < tokens.size() && tokens[lp].type == TokenType::IDENTIFIER)
                            return var_decl();
                        return expr_stmt();
                    }
                    if (tokens[pos + 1].type == TokenType::IDENTIFIER) {
                        return var_decl();
                    }
                }
                return expr_stmt();
        }
    }

    std::unique_ptr<ASTNode> var_decl() {
        SourceLocation loc = peek().location;
        std::string type_name = parse_type_name();

        while (match(TokenType::LBRACKET)) {
            type_name += "[]";
            if (peek().type == TokenType::INT_LITERAL) {
                type_name = type_name.substr(0, type_name.size() - 1) + std::string(advance().lexeme) + "]";
            } else if (peek().type == TokenType::IDENTIFIER) {
                type_name = type_name.substr(0, type_name.size() - 1) + std::string(advance().lexeme) + "]";
            }
            expect(TokenType::RBRACKET, "expected ']' after array size");
        }

        std::string name{expect(TokenType::IDENTIFIER, "expected variable name").lexeme};

        std::string block_name;
        if (match(TokenType::ASSIGN)) {
            auto init = expression();
            if (peek().type == TokenType::AT) {
                advance();
                block_name = std::string{expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme};
                init = std::make_unique<AllocInline>(std::move(init), block_name, init->location);
            }
            auto assign = std::make_unique<Assignment>(TokenType::ASSIGN, loc);
            auto target = std::make_unique<IdentExpr>(name, loc);
            target->declared_type = type_name;
            target->block_name = block_name;
            assign->target = std::move(target);
            assign->value = std::move(init);
            return std::make_unique<ExprStmt>(std::move(assign), loc);
        }

        if (peek().type == TokenType::AT) {
            advance();
            block_name = std::string{expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme};
        }

        auto target = std::make_unique<IdentExpr>(name, loc);
        target->declared_type = type_name;
        target->block_name = block_name;
        return std::make_unique<ExprStmt>(std::move(target), loc);
    }

    std::unique_ptr<ASTNode> if_stmt() {
        advance();
        auto is = std::make_unique<IfStmt>(peek().location);
        if (peek().type == TokenType::LPAREN) {
            advance();
            is->condition = expression();
            expect(TokenType::RPAREN, "expected ')' after condition");
        } else {
            is->condition = expression();
        }
        is->then_branch = block_stmt();
        if (match(TokenType::ELSE)) {
            is->else_branch = statement();
        }
        return is;
    }

    std::unique_ptr<ASTNode> while_stmt() {
        advance();
        auto ws = std::make_unique<WhileStmt>(peek().location);
        if (peek().type == TokenType::LPAREN) {
            advance();
            ws->condition = expression();
            expect(TokenType::RPAREN, "expected ')' after condition");
        } else {
            ws->condition = expression();
        }
        ws->body = block_stmt();
        return ws;
    }

    std::unique_ptr<ASTNode> for_stmt() {
        SourceLocation loc = peek().location;
        advance(); // consume FOR

        // for x in iterable { body }  (for-each)
        if (peek().type == TokenType::IDENTIFIER && pos + 1 < tokens.size() &&
            tokens[pos + 1].type == TokenType::IDENTIFIER && tokens[pos + 1].lexeme == "in") {
            std::string var_name{advance().lexeme};  // x
            advance(); // consume IN
            // Parse the iterable expression
            auto iterable = expression();
            auto body = block_stmt();

            // Desugar to for(;;) with a counter variable
            // for (int __i = 0; __i < iterable_len; __i++) { type x = iterable[__i]; body }
            // For now, we just emit a basic range pattern: for(i; i < N; i++)
            // The iterable must resolve to an integer (range length)
            auto fs = std::make_unique<ForStmt>(loc);
            // init: int __i = 0
            auto init_target = std::make_unique<IdentExpr>("__i", loc);
            init_target->declared_type = "int";
            auto init_assign = std::make_unique<Assignment>(TokenType::ASSIGN, loc);
            init_assign->target = std::move(init_target);
            init_assign->value = std::make_unique<IntLiteral>(0, loc);
            fs->init = std::make_unique<ExprStmt>(std::move(init_assign), loc);
            // condition: __i < iterable
            fs->condition = std::make_unique<BinaryOp>(
                std::make_unique<IdentExpr>("__i", loc),
                TokenType::LT,
                std::move(iterable),
                loc);
            // increment: __i++
            auto inc_target = std::make_unique<IdentExpr>("__i", loc);
            auto inc_assign = std::make_unique<Assignment>(TokenType::PLUS_ASSIGN, loc);
            inc_assign->target = std::move(inc_target);
            inc_assign->value = std::make_unique<IntLiteral>(1, loc);
            fs->increment = std::move(inc_assign);
            // Wrap body to declare the iteration variable
            auto wrapper = std::make_unique<BlockStmt>(loc);
            auto var_target = std::make_unique<IdentExpr>(var_name, loc);
            var_target->declared_type = "int";  // default type for now
            auto var_assign = std::make_unique<Assignment>(TokenType::ASSIGN, loc);
            var_assign->target = std::move(var_target);
            var_assign->value = std::make_unique<IdentExpr>("__i", loc);
            wrapper->statements.push_back(std::make_unique<ExprStmt>(std::move(var_assign), loc));
            wrapper->statements.push_back(std::move(body));
            fs->body = std::move(wrapper);
            return fs;
        }

        // for (init; cond; inc) { body }  (C-style)
        expect(TokenType::LPAREN, "expected '(' after for");
        auto fs = std::make_unique<ForStmt>(loc);
        if (is_type_start()) {
            fs->init = var_decl();
        } else if (peek().type != TokenType::SEMICOLON) {
            fs->init = expr_stmt();
        }
        expect(TokenType::SEMICOLON, "expected ';' after for init");

        if (peek().type != TokenType::SEMICOLON) {
            fs->condition = expression();
        }
        expect(TokenType::SEMICOLON, "expected ';' after for condition");

        if (peek().type != TokenType::RPAREN) {
            fs->increment = expression();
        }
        expect(TokenType::RPAREN, "expected ')' after for clauses");

        fs->body = block_stmt();
        return fs;
    }

    std::unique_ptr<ASTNode> return_stmt() {
        SourceLocation loc = peek().location;
        advance();
        auto rs = std::make_unique<ReturnStmt>(loc);
        if (peek().type != TokenType::SEMICOLON && peek().type != TokenType::RBRACE &&
            peek().type != TokenType::EOF_) {
            rs->value = expression();
        }
        if (peek().type == TokenType::SEMICOLON) advance();
        return rs;
    }

    std::unique_ptr<ASTNode> expr_stmt() {
        auto expr = expression();
        if (peek().type == TokenType::SEMICOLON) advance();
        return std::make_unique<ExprStmt>(std::move(expr), expr->location);
    }

    // ─── Expressions (unchanged) ───

    std::unique_ptr<ASTNode> expression() {
        return assignment();
    }

    std::unique_ptr<ASTNode> assignment() {
        auto left = logical_or();
        if (peek().type == TokenType::ASSIGN ||
            peek().type == TokenType::PLUS_ASSIGN ||
            peek().type == TokenType::MINUS_ASSIGN ||
            peek().type == TokenType::STAR_ASSIGN ||
            peek().type == TokenType::SLASH_ASSIGN) {
            TokenType op = advance().type;
            auto value = assignment();
            auto assign = std::make_unique<Assignment>(op, left->location);
            assign->target = std::move(left);
            assign->value = std::move(value);
            return assign;
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_or() {
        auto left = logical_and();
        while (peek().type == TokenType::OR) {
            TokenType op = advance().type;
            auto right = logical_and();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> logical_and() {
        auto left = equality();
        while (peek().type == TokenType::AND) {
            TokenType op = advance().type;
            auto right = equality();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> equality() {
        auto left = comparison();
        while (peek().type == TokenType::EQ || peek().type == TokenType::NEQ) {
            TokenType op = advance().type;
            auto right = comparison();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> comparison() {
        auto left = term();
        while (peek().type == TokenType::LT || peek().type == TokenType::GT ||
               peek().type == TokenType::LEQ || peek().type == TokenType::GEQ) {
            TokenType op = advance().type;
            auto right = term();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> term() {
        auto left = factor();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            TokenType op = advance().type;
            auto right = factor();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> factor() {
        auto left = unary();
        while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH ||
               peek().type == TokenType::BIT_AND) {
            TokenType op = advance().type;
            auto right = unary();
            left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right), left->location);
        }
        return left;
    }

    std::unique_ptr<ASTNode> unary() {
        if (peek().type == TokenType::NOT || peek().type == TokenType::MINUS ||
            peek().type == TokenType::STAR || peek().type == TokenType::BIT_AND) {
            TokenType op = advance().type;
            auto operand = unary();
            return std::make_unique<UnaryOp>(op, std::move(operand), operand->location);
        }
        // Prefix ++/-- desugar to x += 1 / x -= 1
        if (peek().type == TokenType::PLUS_PLUS || peek().type == TokenType::MINUS_MINUS) {
            TokenType prefix_op = advance().type;
            SourceLocation loc = peek().location;
            auto target = unary();
            TokenType assign_op = (prefix_op == TokenType::PLUS_PLUS)
                ? TokenType::PLUS_ASSIGN : TokenType::MINUS_ASSIGN;
            auto assign = std::make_unique<Assignment>(assign_op, loc);
            assign->target = std::move(target);
            assign->value = std::make_unique<IntLiteral>(1, loc);
            return assign;
        }
        return call();
    }

    std::unique_ptr<ASTNode> call() {
        auto expr = primary();

        while (true) {
            if (peek().type == TokenType::LPAREN) {
                advance();
                auto call = std::make_unique<CallExpr>(expr->location);
                call->callee = std::move(expr);
                if (peek().type != TokenType::RPAREN) {
                    do {
                        call->arguments.push_back(expression());
                    } while (match(TokenType::COMMA));
                }
                expect(TokenType::RPAREN, "expected ')' after arguments");
                expr = std::move(call);
            } else if (peek().type == TokenType::DOT) {
                advance();
                std::string member;
                if (peek().type == TokenType::RESET) {
                    member = std::string{advance().lexeme};
                } else {
                    member = std::string{expect(TokenType::IDENTIFIER, "expected member name").lexeme};
                }
                expr = std::make_unique<MemberExpr>(std::move(expr), member, expr->location);
            } else if (peek().type == TokenType::LBRACKET) {
                advance();
                auto index = std::make_unique<IndexExpr>(expr->location);
                index->array = std::move(expr);
                index->index = expression();
                expect(TokenType::RBRACKET, "expected ']' after index");
                expr = std::move(index);
            } else if (peek().type == TokenType::PLUS_PLUS) {
                advance();
                SourceLocation loc = expr->location;
                auto assign = std::make_unique<Assignment>(TokenType::PLUS_ASSIGN, loc);
                assign->target = std::move(expr);
                assign->value = std::make_unique<IntLiteral>(1, loc);
                expr = std::move(assign);
            } else if (peek().type == TokenType::MINUS_MINUS) {
                advance();
                SourceLocation loc = expr->location;
                auto assign = std::make_unique<Assignment>(TokenType::MINUS_ASSIGN, loc);
                assign->target = std::move(expr);
                assign->value = std::make_unique<IntLiteral>(1, loc);
                expr = std::move(assign);
            } else if (peek().type == TokenType::AT) {
                advance();
                std::string block_name{expect(TokenType::IDENTIFIER, "expected block name after '@'").lexeme};
                expr = std::make_unique<AllocInline>(std::move(expr), block_name, expr->location);
            } else if (peek().type == TokenType::AS) {
                advance();
                std::string target_type = parse_type_name();
                expr = std::make_unique<CastExpr>(std::move(expr), target_type, expr->location);
            } else {
                break;
            }
        }

        return expr;
    }

    std::unique_ptr<ASTNode> primary() {
        SourceLocation loc = peek().location;

        switch (peek().type) {
            case TokenType::INT_LITERAL: {
                Token t = advance();
                bool is_hex = false;
                int64_t val = parse_int_literal(t.lexeme, is_hex);
                auto lit = std::make_unique<IntLiteral>(val, loc, is_hex);
                lit->literal_type = t.literal_type;
                return lit;
            }
            case TokenType::FLOAT_LITERAL: {
                Token t = advance();
                double val = 0;
                std::from_chars(t.lexeme.data(), t.lexeme.data() + t.lexeme.size(), val);
                auto lit = std::make_unique<FloatLiteral>(val, loc);
                lit->literal_type = t.literal_type;
                return lit;
            }
            case TokenType::STRING_LITERAL:
                return std::make_unique<StringLiteral>(process_string_escapes(advance().lexeme), loc);
            case TokenType::CHAR_LITERAL:
                return std::make_unique<CharLiteral>(process_char_escape(advance().lexeme), loc);
            case TokenType::TRUE:
                advance();
                return std::make_unique<BoolLiteral>(true, loc);
            case TokenType::FALSE:
                advance();
                return std::make_unique<BoolLiteral>(false, loc);
            case TokenType::NULL_:
                advance();
                return std::make_unique<NullLiteral>(loc);
            case TokenType::ERROR: {
                advance();
                expect(TokenType::LPAREN, "expected '(' after error");
                auto msg = process_string_escapes(expect(TokenType::STRING_LITERAL, "expected error message").lexeme);
                expect(TokenType::RPAREN, "expected ')' after error message");
                auto call = std::make_unique<CallExpr>(loc);
                call->callee = std::make_unique<IdentExpr>("error", loc);
                call->arguments.push_back(std::make_unique<StringLiteral>(msg, loc));
                return call;
            }
            case TokenType::IDENTIFIER:
                return std::make_unique<IdentExpr>(std::string{advance().lexeme}, loc);
            case TokenType::DOLLAR: {
                advance();
                std::string macro_name{expect(TokenType::IDENTIFIER, "expected macro name after '$'").lexeme};
                auto mc = std::make_unique<MacroCall>(macro_name, loc);
                if (peek().type == TokenType::LPAREN) {
                    advance();
                    if (peek().type != TokenType::RPAREN) {
                        do {
                            mc->args.push_back(expression());
                        } while (match(TokenType::COMMA));
                    }
                    expect(TokenType::RPAREN, "expected ')' after macro arguments");
                }
                return mc;
            }
            case TokenType::LPAREN: {
                advance();
                auto expr = expression();
                expect(TokenType::RPAREN, "expected ')' after expression");
                return expr;
            }
            case TokenType::LBRACE: {
                advance();
                auto arr = std::make_unique<ArrayLiteral>(loc);
                if (peek().type != TokenType::RBRACE) {
                    arr->elements.push_back(expression());
                    while (match(TokenType::COMMA)) {
                        if (peek().type == TokenType::RBRACE) break;
                        arr->elements.push_back(expression());
                    }
                }
                expect(TokenType::RBRACE, "expected '}' after array literal");
                return arr;
            }
            // Type keywords as expressions (for casts: f32(expr), i32(x), etc. and .sizeof)
            // Palavras-chave de tipo como expressoes (para casts: f32(expr), i32(x), etc. e .sizeof)
            case TokenType::U8: case TokenType::U16: case TokenType::U32: case TokenType::U64:
            case TokenType::I8: case TokenType::I16: case TokenType::I32: case TokenType::I64:
            case TokenType::F32: case TokenType::F64:
            case TokenType::USIZE: case TokenType::ISIZE:
            case TokenType::BYTE:
            case TokenType::BOOL: case TokenType::CHAR:
            case TokenType::INT: case TokenType::FLOAT: case TokenType::STRING: case TokenType::VOID:
                return std::make_unique<IdentExpr>(std::string{advance().lexeme}, loc);
            default:
                throw std::runtime_error("unexpected token '" + std::string(peek().lexeme) + "' in expression");
        }
    }
};

ParseResult parse(const std::vector<Token>& tokens) {
    Parser parser(tokens);
    return parser.parse_all();
}

} // namespace brick
