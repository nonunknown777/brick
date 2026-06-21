#ifndef META_C_LEXER_H
#define META_C_LEXER_H

#include <vector>
#include <string>
#include "../shared/types.h"

namespace meta_c {

std::vector<Token> tokenize(const std::string& source, const std::string& filename = "<input>");

} // namespace meta_c
  // namespace meta_c

#endif
