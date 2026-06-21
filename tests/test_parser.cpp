#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include <cassert>
#include <iostream>

using namespace meta_c;

void test_parse_struct() {
    std::string source = R"(
package TEST

struct Player {
    int hp
    String name

    fn Player(int h, String n) {
        hp = h
        name = n
    }
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.ast != nullptr);
    assert(result.errors.empty());
    assert(result.ast->declarations.size() > 0);

    std::cout << "[PASS] test_parse_struct\n";
}

void test_parse_block() {
    std::string source = R"(
package TEST

block global = 256MB
block game = 64MB
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_parse_block\n";
}

void test_parse_if_while() {
    std::string source = R"(
package TEST

fn main() {
    int x = 5 @global

    if (x > 0) {
        x -= 1
    }

    while (x > 0) {
        x -= 1
    }
}
)";

    auto tokens = tokenize(source);
    auto result = parse(tokens);

    assert(result.errors.empty());

    std::cout << "[PASS] test_parse_if_while\n";
}

int main() {
    test_parse_struct();
    test_parse_block();
    test_parse_if_while();
    std::cout << "All parser tests passed!\n";
    return 0;
}
