#pragma once

#include "elaborate/syntax.hpp"
#include "elaborate/table.hpp"
#include "parsing/syntax.hpp"
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace elaborate {

struct Scope {
    std::set<std::string> type_vars;
    std::map<std::string, std::shared_ptr<Type>> expr_vars;
};

class Context {
public:
    Context() = default;

    void push_scope();
    void pop_scope();

    void add_type_var(const std::string& ident);
    void add_expr_var(const std::string& ident, std::shared_ptr<Type> type);

    bool has_type_var(const std::string& ident) const;
    std::optional<std::shared_ptr<Type>> find_expr_var(const std::string& ident);

private:
    std::vector<Scope> scopes;
};

class Elaborator {
public:
    explicit Elaborator(Table table): table(table) {}

    Package elab(parsing::Package& pkg);

private:
    std::map<std::string, std::shared_ptr<Decl>> decl_map;
    Table table;
    Context ctx;

    std::shared_ptr<Type> elab_type(parsing::Type& type);
    std::shared_ptr<Pat> elab_pat(parsing::Pat& pat);
    std::shared_ptr<Cond> elab_cond(parsing::Expr& expr);
    std::shared_ptr<Expr> elab_expr(parsing::Expr& expr);
    std::shared_ptr<Stmt> elab_stmt(parsing::Stmt& stmt);
    std::shared_ptr<Decl> elab_decl(parsing::Decl& decl);
};

} // namespace elaborate
