#ifndef BRICK_BIND_H
#define BRICK_BIND_H

#include <string>
#include <vector>

namespace brick {
namespace bind {

// ── Options for binding generation ──────────────────────────────────────────
struct Options {
    bool generate_structs   = true;
    bool generate_enums     = true;
    bool generate_defines   = true;
    bool generate_functions = true;
};

// ── Result of binding generation ───────────────────────────────────────────
struct Result {
    std::string brc_code;
    std::vector<std::string> errors;
    bool success = false;
};

// ── API ────────────────────────────────────────────────────────────────────

/// Generate Brick bindings (.brc) from a C header file.
/// Gera bindings Brick (.brc) a partir de um header C.
Result generate(const std::string& header_path, const Options& opts = {});

/// Generate Brick bindings (.brc) from a C header source string directly.
/// Useful for tests and in-memory generation.
/// Gera bindings Brick (.brc) a partir de um texto de header C diretamente.
Result generate_from_source(const std::string& source, const Options& opts = {});

} // namespace bind
} // namespace brick

#endif // BRICK_BIND_H
