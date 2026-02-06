#include <format>
#include <memory>
#include <optional>
#include <print>

#include "elaborate/syntax.hpp"
#include "elaborate/table.hpp"

namespace elaborate {

Symbol TableNode::find_type_symbol(const std::string& ident) {
    auto it = types.find(ident);
    if (it == types.end()) {
        throw std::runtime_error("Type symbol not found: " + ident);
    }
    auto symbols = it->second;
    if (symbols.size() != 1) {
        throw std::runtime_error("Ambiguous type symbol: " + ident);
    }
    return *symbols.begin();
}

Symbol TableNode::find_expr_symbol(const std::string& ident) {
    auto it = exprs.find(ident);
    if (it == exprs.end()) {
        throw std::runtime_error("Expr symbol not found: " + ident);
    }
    auto symbols = it->second;
    if (symbols.size() != 1) {
        throw std::runtime_error("Ambiguous expr symbol: " + ident);
    }
    return *symbols.begin();
}

TableNode* TableNode::find_node(const std::string& ident) {
    auto it = nested.find(ident);
    if (it == nested.end()) {
        throw std::runtime_error("Node not found: " + ident);
    }
    auto& nodes = it->second;
    if (nodes.size() != 1) {
        throw std::runtime_error("Ambiguous node: " + ident);
    }
    return nodes.begin()->get();
}

void Table::add_node(const std::string& ident, TableNode::Kind kind) {
    auto node = std::make_unique<TableNode>(kind, ident);
    node->parent = active;
    node->path = active->path + "." + ident;
    active->nested[ident].insert(std::move(node));
}

void Table::enter_node(const std::string& ident) {
    active = active->find_node(ident);
}

void Table::exit_node() {
    if (active->parent) {
        active = active->parent;
    } else {
        throw std::runtime_error("Cannot exit root node");
    }
}

void Table::add_type_symbol(const std::string& ident, Symbol symbol) {
    symbol.path = active->path + "." + ident;
    active->types[ident].insert(symbol);
}

void Table::add_expr_symbol(const std::string& ident, Symbol symbol) {
    symbol.path = active->path + "." + ident;
    active->exprs[ident].insert(symbol);
}

Symbol Table::find_type_symbol(const std::string& ident, const std::vector<std::string>& path) {
    TableNode* current = active;
    // If path is empty, search upwards for the symbol
    if (path.empty()) {
        while (current) {
            try {
                return current->find_type_symbol(ident);
            } catch (...) {
                current = current->parent;
            }
        }
        throw std::runtime_error("Type symbol not found: " + ident);
    }
    // If path is not empty, find base node
    while (current && current->nested.find(ident) == current->nested.end()) {
        current = current->parent;
    }
    if (!current) {
        throw std::runtime_error("Base node not found: " + ident);
    }
    current = current->find_node(ident);
    auto end = path.end() - 1;
    for (auto it = path.begin(); it != end; ++it) {
        current = current->find_node(*it);
    }
    return current->find_type_symbol(path.back());
}

Symbol Table::find_expr_symbol(const std::string& ident, const std::vector<std::string>& path) {
    TableNode* current = active;
    // If path is empty, search upwards for the symbol
    if (path.empty()) {
        while (current) {
            try {
                return current->find_expr_symbol(ident);
            } catch (...) {
                current = current->parent;
            }
        }
        throw std::runtime_error("Expr symbol not found: " + ident);
    }
    // If path is not empty, find base node
    while (current && current->nested.find(ident) == current->nested.end()) {
        current = current->parent;
    }
    if (!current) {
        throw std::runtime_error("Base node not found: " + ident);
    }
    current = current->find_node(ident);
    auto end = path.end() - 1;
    for (auto it = path.begin(); it != end; ++it) {
        current = current->find_node(*it);
    }
    return current->find_expr_symbol(path.back());
}

void Table::import_helper(
    TableNode& current,
    const parsing::Import& import,
    std::vector<std::string> path,
    std::map<std::vector<std::string>, std::set<Symbol>>& types,
    std::map<std::vector<std::string>, std::set<Symbol>>& exprs,
    std::map<std::vector<std::string>, std::set<std::shared_ptr<TableNode>>>& nested
) {
    switch (import.get_kind()) {
        case parsing::Import::Kind::Node: {
            const auto& node_import = static_cast<const parsing::NodeImport&>(import);
            path.push_back(node_import.name);
            if (node_import.nested.empty()) {
                types[path].insert(
                    current.types[node_import.name].begin(),
                    current.types[node_import.name].end()
                );
                exprs[path].insert(
                    current.exprs[node_import.name].begin(),
                    current.exprs[node_import.name].end()
                );
                nested[path].insert(
                    current.nested[node_import.name].begin(),
                    current.nested[node_import.name].end()
                );
            } else {
                auto* next_node = current.find_node(node_import.name);
                for (const auto& nested_import: node_import.nested) {
                    import_helper(*next_node, *nested_import, path, types, exprs, nested);
                }
            }
            path.pop_back();
            break;
        }
        case parsing::Import::Kind::Alias: {
            const auto& alias_import = static_cast<const parsing::AliasImport&>(import);
            if (alias_import.alias.has_value()) {
                path.push_back(*alias_import.alias);
                types[path].insert(
                    current.types[alias_import.name].begin(),
                    current.types[alias_import.name].end()
                );
                exprs[path].insert(
                    current.exprs[alias_import.name].begin(),
                    current.exprs[alias_import.name].end()
                );
                nested[path].insert(
                    current.nested[alias_import.name].begin(),
                    current.nested[alias_import.name].end()
                );
                path.pop_back();
            }
            path.push_back(alias_import.name);
            types.erase(path);
            exprs.erase(path);
            nested.erase(path);
            path.pop_back();
            break;
        }
        case parsing::Import::Kind::Wild: {
            for (const auto& [name, symbols]: current.types) {
                path.push_back(name);
                types[path].insert(current.types[name].begin(), current.types[name].end());
                path.pop_back();
            }
            for (const auto& [name, symbols]: current.exprs) {
                path.push_back(name);
                exprs[path].insert(current.exprs[name].begin(), current.exprs[name].end());
                path.pop_back();
            }
            for (const auto& [name, nodes]: current.nested) {
                path.push_back(name);
                nested[path].insert(current.nested[name].begin(), current.nested[name].end());
                path.pop_back();
            }
            break;
        }
    }
}

void Table::import(const parsing::Import& import) {
    auto* current = active;
    if (import.get_kind() == parsing::Import::Kind::Node) {
        const auto& node_import = static_cast<const parsing::NodeImport&>(import);
        while (current && current->nested.find(node_import.name) == current->nested.end()) {
            current = current->parent;
        }
        if (!current) {
            throw std::runtime_error("Import base node not found: " + node_import.name);
        }
        current = current->find_node(node_import.name);
        std::vector<std::string> path { node_import.name };
        std::map<std::vector<std::string>, std::set<Symbol>> types;
        std::map<std::vector<std::string>, std::set<Symbol>> exprs;
        std::map<std::vector<std::string>, std::set<std::shared_ptr<TableNode>>> nested;
        for (const auto& nested_import: node_import.nested) {
            import_helper(*current, *nested_import, path, types, exprs, nested);
        }
        for (const auto& [path, symbols]: types) {
            active->types[path.back()].insert(symbols.begin(), symbols.end());
        }
        for (const auto& [path, symbols]: exprs) {
            active->exprs[path.back()].insert(symbols.begin(), symbols.end());
        }
        for (const auto& [path, nodes]: nested) {
            active->nested[path.back()].insert(nodes.begin(), nodes.end());
        }
    }
}

Table TableBuilder::build() {
    build_constants();

    std::println("/* Constant table built successfully.");
    std::println("{}", table);
    std::println("*/");

    merge_symbols();

    std::println("/* Constant table merged successfully.");
    std::println("{}", table);
    std::println("*/");

    build_variables();

    std::println("/* Variable table built successfully.");
    std::println("{}", table);
    std::println("*/");

    merge_symbols();

    std::println("/* Variable table merged successfully.");
    std::println("{}", table);
    std::println("*/");

    return std::move(table);
}

void TableBuilder::visit_module(parsing::ModuleDecl& decl, std::function<void()> fn) {
    auto* decls_backup = this->decls;
    this->decls = &decl.body;
    table.enter_node(decl.ident);
    fn();
    table.exit_node();
    this->decls = decls_backup;
}

void TableBuilder::visit_class(parsing::ClassDecl& decl, std::function<void()> fn) {
    auto* decls_backup = this->decls;
    this->decls = &decl.body;
    table.enter_node(decl.ident);
    fn();
    table.exit_node();
    this->decls = decls_backup;
}

void TableBuilder::visit_enum(parsing::EnumDecl& decl, std::function<void()> fn) {
    auto* decls_backup = this->decls;
    this->decls = &decl.body;
    table.enter_node(decl.ident);
    fn();
    table.exit_node();
    this->decls = decls_backup;
}

void TableBuilder::visit_interface(parsing::InterfaceDecl& decl, std::function<void()> fn) {
    auto* decls_backup = this->decls;
    this->decls = &decl.body;
    table.enter_node(decl.ident);
    fn();
    table.exit_node();
    this->decls = decls_backup;
}

void TableBuilder::visit_extension(parsing::ExtensionDecl& decl, std::function<void()> fn) {
    auto* decls_backup = this->decls;
    this->decls = &decl.body;
    table.enter_node(decl.ident);
    fn();
    table.exit_node();
    this->decls = decls_backup;
}

void TableBuilder::build_constants() {
    for (const auto& decl: *decls) {
        switch (decl->get_kind()) {
            case parsing::Decl::Kind::Module: {
                auto& module_decl = static_cast<parsing::ModuleDecl&>(*decl);
                table.add_node(module_decl.ident, TableNode::Kind::Module);
                visit_module(module_decl, [this]() { build_constants(); });
                break;
            }
            case parsing::Decl::Kind::Class: {
                auto& class_decl = static_cast<parsing::ClassDecl&>(*decl);
                table.add_type_symbol(
                    class_decl.ident,
                    Symbol(Symbol::Kind::Class, class_decl.access)
                );
                table.add_node(class_decl.ident, TableNode::Kind::Class);
                visit_class(class_decl, [this]() { build_constants(); });
                break;
            }
            case parsing::Decl::Kind::Enum: {
                auto& enum_decl = static_cast<parsing::EnumDecl&>(*decl);
                table.add_type_symbol(
                    enum_decl.ident,
                    Symbol(Symbol::Kind::Enum, enum_decl.access)
                );
                table.add_node(enum_decl.ident, TableNode::Kind::Enum);
                visit_enum(enum_decl, [this]() { build_constants(); });
                break;
            }
            case parsing::Decl::Kind::Typealias: {
                auto& typealias_decl = static_cast<parsing::TypealiasDecl&>(*decl);
                table.add_type_symbol(
                    typealias_decl.ident,
                    Symbol(Symbol::Kind::Typealias, typealias_decl.access)
                );
                break;
            }
            case parsing::Decl::Kind::Interface: {
                auto& interface_decl = static_cast<parsing::InterfaceDecl&>(*decl);
                table.add_type_symbol(
                    interface_decl.ident,
                    Symbol(Symbol::Kind::Interface, interface_decl.access)
                );
                table.add_node(interface_decl.ident, TableNode::Kind::Interface);
                visit_interface(interface_decl, [this]() { build_constants(); });
                break;
            }
            case parsing::Decl::Kind::Extension: {
                auto& extension_decl = static_cast<parsing::ExtensionDecl&>(*decl);
                extension_decl.ident = std::format(
                    "ext\%{}",
                    table.get_active_count(),
                    *extension_decl.base_type,
                    *extension_decl.interface
                );
                table.add_expr_symbol(
                    extension_decl.ident,
                    Symbol(Symbol::Kind::Extension, extension_decl.access)
                );
                table.add_node(extension_decl.ident, TableNode::Kind::Extension);
                visit_extension(extension_decl, [this]() { build_constants(); });
                break;
            }
            case parsing::Decl::Kind::Func: {
                auto& func_decl = static_cast<parsing::FuncDecl&>(*decl);
                table.add_expr_symbol(
                    func_decl.ident,
                    Symbol(Symbol::Kind::Func, func_decl.access)
                );
                break;
            }
            case parsing::Decl::Kind::Init: {
                auto& init_decl = static_cast<parsing::InitDecl&>(*decl);
                if (init_decl.ident.empty()) {
                    init_decl.ident = std::format("init\%{}", table.get_active_count());
                }
                table.add_expr_symbol(
                    init_decl.ident,
                    Symbol(Symbol::Kind::Init, init_decl.access)
                );
                break;
            }
            case parsing::Decl::Kind::Ctor: {
                auto& enum_ctor_decl = static_cast<parsing::CtorDecl&>(*decl);
                table.add_expr_symbol(
                    enum_ctor_decl.ident,
                    Symbol(Symbol::Kind::Ctor, enum_ctor_decl.access)
                );
                break;
            }
            default:
                break;
        }
    }
}

void TableBuilder::merge_symbols() {
    for (const auto& decl: *decls) {
        switch (decl->get_kind()) {
            case parsing::Decl::Kind::Module: {
                auto& module_decl = static_cast<parsing::ModuleDecl&>(*decl);
                visit_module(module_decl, [this]() { merge_symbols(); });
                break;
            }
            case parsing::Decl::Kind::Open: {
                auto& open_decl = static_cast<parsing::OpenDecl&>(*decl);
                table.import(*open_decl.import);
                break;
            }
            case parsing::Decl::Kind::Class: {
                auto& class_decl = static_cast<parsing::ClassDecl&>(*decl);
                visit_class(class_decl, [this]() { merge_symbols(); });
                break;
            }
            case parsing::Decl::Kind::Enum: {
                auto& enum_decl = static_cast<parsing::EnumDecl&>(*decl);
                visit_enum(enum_decl, [this]() { merge_symbols(); });
                break;
            }
            case parsing::Decl::Kind::Interface: {
                auto& interface_decl = static_cast<parsing::InterfaceDecl&>(*decl);
                visit_interface(interface_decl, [this]() { merge_symbols(); });
                break;
            }
            case parsing::Decl::Kind::Extension: {
                auto& extension_decl = static_cast<parsing::ExtensionDecl&>(*decl);
                visit_extension(extension_decl, [this]() { merge_symbols(); });
                break;
            }
            default:
                break;
        }
    }
}

void Table::pat_rewrite(std::unique_ptr<parsing::Pat>& pat) {
    switch (pat->get_kind()) {
        case parsing::Pat::Kind::Tuple: {
            auto& tuple_pat = static_cast<parsing::TuplePat&>(*pat);
            for (auto& elem: tuple_pat.elems) {
                pat_rewrite(elem);
            }
            break;
        }
        case parsing::Pat::Kind::Ctor: {
            auto& ctor_pat = static_cast<parsing::CtorPat&>(*pat);
            if (ctor_pat.args.has_value()) {
                for (auto& arg: *ctor_pat.args) {
                    pat_rewrite(arg);
                }
            }
            break;
        }
        case parsing::Pat::Kind::Name: {
            auto& name_pat = static_cast<parsing::NamePat&>(*pat);
            auto [path, rest] = name_pat.name.slice();
            if (!rest.empty()) {
                throw std::runtime_error(std::format("Invalid pattern name: {}", name_pat.name));
            }
            std::optional<Symbol> symbol;
            try {
                symbol = find_expr_symbol(name_pat.name.ident, path);
            } catch (...) {
                // Not found, do nothing
            }
            if (symbol.has_value() && symbol->get_kind() == Symbol::Kind::Ctor) {
                // Rewrite to CtorPat
                if (name_pat.is_mut) {
                    throw std::runtime_error("Cannot use 'mut' with constructor pattern");
                }
                if (name_pat.hint->get_kind() != parsing::Type::Kind::Meta) {
                    throw std::runtime_error("Cannot use type hint with constructor pattern");
                }
                pat = std::make_unique<parsing::CtorPat>(
                    name_pat.name,
                    std::move(name_pat.type_args),
                    std::nullopt,
                    pat->get_span()
                );
            } else if (path.empty() && !name_pat.type_args.has_value()) {
                // Keep as NamePat
            } else {
                std::println("{} {}", name_pat.name, path);
                throw std::runtime_error(std::format("Invalid pattern name: {}", name_pat.name));
            }
            break;
        }
        case parsing::Pat::Kind::Or: {
            auto& or_pat = static_cast<parsing::OrPat&>(*pat);
            for (auto& option: or_pat.options) {
                pat_rewrite(option);
            }
            break;
        }
        case parsing::Pat::Kind::At: {
            auto& at_pat = static_cast<parsing::AtPat&>(*pat);
            pat_rewrite(at_pat.pat);
            break;
        }
        default:
            break;
    }
}

void Table::pat_add_vars(const parsing::Pat& pat, Access access) {
    switch (pat.get_kind()) {
        case parsing::Pat::Kind::Tuple: {
            const auto& tuple_pat = static_cast<const parsing::TuplePat&>(pat);
            for (const auto& elem: tuple_pat.elems) {
                pat_add_vars(*elem, access);
            }
            break;
        }
        case parsing::Pat::Kind::Ctor: {
            const auto& ctor_pat = static_cast<const parsing::CtorPat&>(pat);
            if (ctor_pat.args.has_value()) {
                for (const auto& arg: *ctor_pat.args) {
                    pat_add_vars(*arg, access);
                }
            }
            break;
        }
        case parsing::Pat::Kind::Name: {
            const auto& name_pat = static_cast<const parsing::NamePat&>(pat);
            add_expr_symbol(name_pat.name.ident, Symbol(Symbol::Kind::Var, access));
            break;
        }
        case parsing::Pat::Kind::Or: {
            const auto& or_pat = static_cast<const parsing::OrPat&>(pat);
            for (const auto& option: or_pat.options) {
                pat_add_vars(*option, access);
            }
            break;
        }
        case parsing::Pat::Kind::At: {
            const auto& at_pat = static_cast<const parsing::AtPat&>(pat);
            if (!at_pat.name.path.empty()) {
                throw std::runtime_error(std::format("Invalid pattern name: {}", at_pat.name));
            }
            add_expr_symbol(at_pat.name.ident, Symbol(Symbol::Kind::Var, access));
            pat_add_vars(*at_pat.pat, access);
            break;
        }
        default:
            break;
    }
}

void TableBuilder::build_variables() {
    for (const auto& decl: *decls) {
        switch (decl->get_kind()) {
            case parsing::Decl::Kind::Module: {
                auto& module_decl = static_cast<parsing::ModuleDecl&>(*decl);
                visit_module(module_decl, [this]() { build_variables(); });
                break;
            }
            case parsing::Decl::Kind::Class: {
                auto& class_decl = static_cast<parsing::ClassDecl&>(*decl);
                visit_class(class_decl, [this]() { build_variables(); });
                break;
            }
            case parsing::Decl::Kind::Enum: {
                auto& enum_decl = static_cast<parsing::EnumDecl&>(*decl);
                visit_enum(enum_decl, [this]() { build_variables(); });
                break;
            }
            case parsing::Decl::Kind::Interface: {
                auto& interface_decl = static_cast<parsing::InterfaceDecl&>(*decl);
                visit_interface(interface_decl, [this]() { build_variables(); });
                break;
            }
            case parsing::Decl::Kind::Extension: {
                auto& extension_decl = static_cast<parsing::ExtensionDecl&>(*decl);
                visit_extension(extension_decl, [this]() { build_variables(); });
                break;
            }
            case parsing::Decl::Kind::Let: {
                auto& let_decl = static_cast<parsing::LetDecl&>(*decl);
                table.pat_rewrite(let_decl.pat);
                table.pat_add_vars(*let_decl.pat, let_decl.access);
                break;
            }
            default:
                break;
        }
    }
} // namespace elaborate

static std::string indent_str(int indent) {
    return std::string(indent * 4, ' ');
}

static std::string format_node_kind(TableNode::Kind kind) {
    switch (kind) {
        case TableNode::Kind::Module:
            return "Module";
        case TableNode::Kind::Class:
            return "Class";
        case TableNode::Kind::Enum:
            return "Enum";
        case TableNode::Kind::Interface:
            return "Interface";
        case TableNode::Kind::Extension:
            return "Extension";
    }
    return "<?node>";
}

static std::string format_symbol_kind(Symbol::Kind kind) {
    switch (kind) {
        case Symbol::Kind::Class:
            return "Class";
        case Symbol::Kind::Enum:
            return "Enum";
        case Symbol::Kind::Typealias:
            return "Typealias";
        case Symbol::Kind::Interface:
            return "Interface";
        case Symbol::Kind::Extension:
            return "Extension";
        case Symbol::Kind::Func:
            return "Func";
        case Symbol::Kind::Init:
            return "Init";
        case Symbol::Kind::Ctor:
            return "Ctor";
        case Symbol::Kind::Var:
            return "Var";
    }
    return "<?symbol>";
}

static std::string format_symbol(const Symbol& sym) {
    std::string access;
    switch (sym.get_access()) {
        case Access::Public:
            access = "Public ";
            break;
        case Access::Private:
            access = "Private ";
            break;
        case Access::Protected:
            access = "Protected ";
            break;
    }
    return access + format_symbol_kind(sym.get_kind()) + " " + sym.get_path();
}

std::string format_table_node(const TableNode* node, int indent = 0) {
    std::string result =
        indent_str(indent) + format_node_kind(node->get_kind()) + " " + node->ident + "\n";

    if (!node->types.empty()) {
        result += indent_str(indent + 1) + "types:\n";
        for (const auto& [name, symbols]: node->types) {
            for (const auto& sym: symbols) {
                result += indent_str(indent + 2) + name + ": " + format_symbol(sym) + "\n";
            }
        }
    }

    if (!node->exprs.empty()) {
        result += indent_str(indent + 1) + "exprs:\n";
        for (const auto& [name, symbols]: node->exprs) {
            for (const auto& sym: symbols) {
                result += indent_str(indent + 2) + name + ": " + format_symbol(sym) + "\n";
            }
        }
    }

    for (const auto& [name, nodes]: node->nested) {
        for (const auto& child: nodes) {
            result += format_table_node(child.get(), indent + 1);
        }
    }

    return result;
}

} // namespace elaborate

std::format_context::iterator std::formatter<elaborate::Table>::format(
    const elaborate::Table& table,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_table_node(table.get_root()), ctx);
}