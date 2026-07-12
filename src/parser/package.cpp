#include "package.h"
#include <stdexcept>

namespace brick {

PackageTable resolve_packages(
    std::unique_ptr<ProgramNode>& ast,
    const std::string& filename)
{
    PackageTable table;
    table.parsed_files.push_back(filename);

    for (auto& decl : ast->declarations) {
        if (decl->type == ASTNodeType::PACKAGE_DECL) {
            auto* pkg = static_cast<PackageDecl*>(decl.get());
            std::string full_name;
            for (const auto& part : pkg->name_parts) {
                if (!full_name.empty()) full_name += ".";
                full_name += part;
            }
            table.current_package = full_name;
        }
    }

    if (table.current_package.empty()) {
        table.current_package = "__main__";
    }

    auto ensure_package = [&](const std::string& name) -> PackageInfo* {
        auto it = table.packages.find(name);
        if (it == table.packages.end()) {
            auto info = std::make_unique<PackageInfo>();
            info->full_name = name;
            PackageInfo* ptr = info.get();
            table.packages[name] = ptr;
            table._owned.push_back(std::move(info));
            return ptr;
        }
        return it->second;
    };

    PackageInfo* current = ensure_package(table.current_package);
    current->visited = true;

    for (auto& decl : ast->declarations) {
        bool is_private = false;
        if (decl->type == ASTNodeType::STRUCT_DECL) {
            auto* sd = static_cast<StructDecl*>(decl.get());
            is_private = sd->is_private;
            if (!is_private) current->exported_structs.insert(sd->name);
        } else if (decl->type == ASTNodeType::FUNC_DECL) {
            auto* fd = static_cast<FuncDecl*>(decl.get());
            is_private = fd->is_private;
            if (!is_private) current->exported_functions.insert(fd->name);
        } else if (decl->type == ASTNodeType::CONST_DECL) {
            auto* cd = static_cast<ConstDecl*>(decl.get());
            is_private = cd->is_private;
            if (!is_private) current->exported_consts.insert(cd->name);
        } else if (decl->type == ASTNodeType::ENUM_DECL) {
            auto* ed = static_cast<EnumDecl*>(decl.get());
            is_private = ed->is_private;
            if (!is_private) current->exported_enums.insert(ed->name);
        } else if (decl->type == ASTNodeType::UNION_DECL) {
            auto* ud = static_cast<UnionDecl*>(decl.get());
            is_private = ud->is_private;
            if (!is_private && !ud->is_anonymous)
                current->exported_unions.insert(ud->name);
        } else if (decl->type == ASTNodeType::INTERFACE_DECL) {
            auto* id = static_cast<InterfaceDecl*>(decl.get());
            is_private = id->is_private;
            if (!is_private) current->exported_interfaces.insert(id->name);
        } else if (decl->type == ASTNodeType::TYPE_ALIAS) {
            auto* ta = static_cast<TypeAliasDecl*>(decl.get());
            is_private = ta->is_private;
            if (!is_private) current->exported_type_aliases.insert(ta->alias_name);
        } else if (decl->type == ASTNodeType::MACRO_DECL) {
            auto* md = static_cast<MacroDecl*>(decl.get());
            is_private = md->is_private;
            if (!is_private) current->exported_macros.insert(md->name);
        }
        current->declarations.push_back(decl.get());
    }

    return table;
}

void merge_package_tables(PackageTable& global, PackageTable&& local) {
    auto ensure_package = [&](const std::string& name) -> PackageInfo* {
        auto it = global.packages.find(name);
        if (it == global.packages.end()) {
            auto info = std::make_unique<PackageInfo>();
            info->full_name = name;
            PackageInfo* ptr = info.get();
            global.packages[name] = ptr;
            global._owned.push_back(std::move(info));
            return ptr;
        }
        return it->second;
    };

    for (auto& [name, info] : local.packages) {
        PackageInfo* global_info = ensure_package(name);
        // Merge all export sets
        for (const auto& s : info->exported_structs)     global_info->exported_structs.insert(s);
        for (const auto& f : info->exported_functions)   global_info->exported_functions.insert(f);
        for (const auto& c : info->exported_consts)      global_info->exported_consts.insert(c);
        for (const auto& e : info->exported_enums)       global_info->exported_enums.insert(e);
        for (const auto& u : info->exported_unions)      global_info->exported_unions.insert(u);
        for (const auto& i : info->exported_interfaces)  global_info->exported_interfaces.insert(i);
        for (const auto& t : info->exported_type_aliases) global_info->exported_type_aliases.insert(t);
        for (const auto& m : info->exported_macros)      global_info->exported_macros.insert(m);
        for (auto* decl : info->declarations) {
            global_info->declarations.push_back(decl);
        }
        global_info->visited = global_info->visited || info->visited;
    }

    for (const auto& f : local.parsed_files) {
        global.parsed_files.push_back(f);
    }
}

bool is_accessible(const PackageTable& table,
                   const std::string& from_package,
                   const std::string& symbol_name)
{
    // Check all packages for exported symbol
    for (auto& [name, info] : table.packages) {
        if (info->exported_structs.count(symbol_name) ||
            info->exported_functions.count(symbol_name) ||
            info->exported_consts.count(symbol_name) ||
            info->exported_enums.count(symbol_name) ||
            info->exported_unions.count(symbol_name) ||
            info->exported_interfaces.count(symbol_name) ||
            info->exported_type_aliases.count(symbol_name) ||
            info->exported_macros.count(symbol_name)) {
            return true;
        }
        // Internal symbols are accessible from the same package
        if (name == from_package) {
            for (auto* decl : info->declarations) {
                if (decl->type == ASTNodeType::STRUCT_DECL) {
                    auto* sd = static_cast<StructDecl*>(decl);
                    if (sd->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::FUNC_DECL) {
                    auto* fd = static_cast<FuncDecl*>(decl);
                    if (fd->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::CONST_DECL) {
                    auto* cd = static_cast<ConstDecl*>(decl);
                    if (cd->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::ENUM_DECL) {
                    auto* ed = static_cast<EnumDecl*>(decl);
                    if (ed->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::UNION_DECL) {
                    auto* ud = static_cast<UnionDecl*>(decl);
                    if (ud->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::INTERFACE_DECL) {
                    auto* id = static_cast<InterfaceDecl*>(decl);
                    if (id->name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::TYPE_ALIAS) {
                    auto* ta = static_cast<TypeAliasDecl*>(decl);
                    if (ta->alias_name == symbol_name) return true;
                }
                if (decl->type == ASTNodeType::MACRO_DECL) {
                    auto* md = static_cast<MacroDecl*>(decl);
                    if (md->name == symbol_name) return true;
                }
            }
        }
    }
    return false;
}

} // namespace brick
