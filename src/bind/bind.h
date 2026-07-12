#pragma once

#include <string>
#include <vector>
#include <ostream>

namespace brick {
namespace bind {

struct BindError {
    std::string message;
    int line;
    int col;
    std::string file;
};

inline std::ostream& operator<<(std::ostream& os, const BindError& err) {
    if (err.line > 0) {
        os << err.file << ":" << err.line << ":" << err.col << ": " << err.message;
    } else {
        os << err.message;
    }
    return os;
}

struct BindResult {
    bool success;
    std::vector<BindError> errors;
    std::string brc_code;
};

struct Options {
    // Future: namespace/package prefix, type overrides, etc.
};

BindResult generate(const std::string& header_path, const Options& opts);

} // namespace bind
} // namespace brick
