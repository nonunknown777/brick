#ifndef META_C_CODEGEN_H
#define META_C_CODEGEN_H

#include <string>
#include <vector>
#include <memory>
#include "../parser/ast.h"
#include "../parser/package.h"

namespace meta_c {

struct CodegenResult {
    std::string c_code;
    std::vector<std::string> errors;
    bool success = false;
};

// Generate C code from AST + PackageTable
// Gera codigo C a partir de AST + PackageTable
CodegenResult generate_c(
    const std::vector<std::unique_ptr<ProgramNode>>& asts,
    const PackageTable& packages
);

} // namespace meta_c
  // namespace meta_c

#endif
