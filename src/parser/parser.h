#ifndef META_C_PARSER_H
#define META_C_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "ast.h"
#include "../shared/types.h"

namespace meta_c {

struct ParseResult {
    std::unique_ptr<ProgramNode> ast;
    std::vector<std::string> errors;
};

ParseResult parse(const std::vector<Token>& tokens);

} // namespace meta_c

#endif
