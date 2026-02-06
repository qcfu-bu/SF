#pragma once

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "elaborate/syntax.hpp"
#include "parsing/syntax.hpp"

namespace elaborate {

struct Symbol {
    enum class Kind {
        Class,
        Enum,
        Typealias,
        Interface,
        Extension,
        Func,
        Init,
        Ctor,
        Var,
    };

    explicit Symbol(Kind kind): access(Access::Public), kind(kind) {}
    Symbol(Kind kind, Access access): access(access), kind(kind) {}

    Access get_access() const {
        return access;
    }

    Kind get_kind() const {
        return kind;
    }

    std::string get_path() const {
        return path;
    }

    bool operator<(const Symbol& other) const {
        if (kind != other.kind) {
            return kind < other.kind;
        }
        return path < other.path;
    }

private:
    Access access;
    Kind kind;
    std::string path;

    friend class Table;
};

struct TableNode {
    enum class Kind {
        Module,
        Class,
        Enum,
        Interface,
        Extension,
    };

    TableNode(Kind kind, std::string ident): kind(kind), ident(std::move(ident)) {}

    Kind get_kind() const {
        return kind;
    }
    std::string get_ident() const {
        return ident;
    }
    TableNode* find_node(const std::string& ident);
    Symbol find_type_symbol(const std::string& ident);
    Symbol find_expr_symbol(const std::string& ident);

private:
    Kind kind;
    std::string ident;
    std::string path;
    int counter = 0;
    std::map<std::string, std::set<Symbol>> types;
    std::map<std::string, std::set<Symbol>> exprs;
    std::map<std::string, std::set<std::shared_ptr<TableNode>>> nested;
    TableNode* parent = nullptr;

    friend class Table;
    friend std::string format_table_node(const TableNode* node, int indent);
};

struct Table {
    explicit Table(std::string ident):
        root(std::make_shared<TableNode>(TableNode::Kind::Module, std::move(ident))),
        active(root.get()) {
        root->path = root->ident;
    }

    TableNode* get_root() const {
        return root.get();
    }

    TableNode* get_active() const {
        return active;
    }

    int get_active_count() {
        return active->counter++;
    }

    void add_node(const std::string& ident, TableNode::Kind kind);
    void enter_node(const std::string& ident);
    void exit_node();

    void add_type_symbol(const std::string& ident, Symbol symbol);
    void add_expr_symbol(const std::string& ident, Symbol symbol);

    Symbol find_type_symbol(const std::string& ident, const std::vector<std::string>& path);
    Symbol find_expr_symbol(const std::string& ident, const std::vector<std::string>& path);

    void import(const parsing::Import& import);

    void pat_rewrite(std::unique_ptr<parsing::Pat>& pat);
    void pat_add_vars(const parsing::Pat& pat, Access access);

private:
    std::shared_ptr<TableNode> root;
    TableNode* active;
    void import_helper(
        TableNode& current,
        const parsing::Import& import,
        std::vector<std::string> path,
        std::map<std::vector<std::string>, std::set<Symbol>>& types,
        std::map<std::vector<std::string>, std::set<Symbol>>& exprs,
        std::map<std::vector<std::string>, std::set<std::shared_ptr<TableNode>>>& nested
    );
};

class TableBuilder {
public:
    explicit TableBuilder(parsing::Package& pkg): decls(&pkg.body), table(Table(pkg.ident)) {}

    Table build();

private:
    std::vector<std::unique_ptr<parsing::Decl>>* decls;
    Table table;

    void visit_module(parsing::ModuleDecl& decl, std::function<void()> fn);
    void visit_class(parsing::ClassDecl& decl, std::function<void()> fn);
    void visit_enum(parsing::EnumDecl& decl, std::function<void()> fn);
    void visit_interface(parsing::InterfaceDecl& decl, std::function<void()> fn);
    void visit_extension(parsing::ExtensionDecl& decl, std::function<void()> fn);

    void merge_symbols();
    void build_constants();
    void build_variables();
};

} // namespace elaborate

template<>
struct std::formatter<elaborate::Table>: std::formatter<std::string> {
    std::format_context::iterator
    format(const elaborate::Table& table, std::format_context& ctx) const;
};