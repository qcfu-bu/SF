#pragma once

#include <functional>
#include <memory>

#include "lexer.hpp"
#include "syntax.hpp"

namespace parsing {

class Parser {
public:
    explicit Parser(std::string pkgName, std::string input):
        pkgName(std::move(pkgName)),
        lexer(std::move(input)) {}

    std::unique_ptr<Type> parse_type();
    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Decl> parse_decl();
    Package parse_package();

private:
    std::string pkgName;
    Lexer lexer;
    Span last_span = {};

    Token peek() {
        return lexer.peek();
    }

    Token next() {
        auto tok = lexer.next();
        last_span = tok.get_span();
        return tok;
    }

    Location start_loc() {
        return peek().get_span().start;
    }

    Span make_span(Location start) {
        return Span{start, last_span.end};
    }

    void expect(Token::Kind expected) {
        auto token = peek();
        if (token.get_kind() != expected) {
            throw std::runtime_error(
                std::format("Expected token {}, got {}", format_token_kind(expected), token)
            );
        }
        next();
    }

    void done() {
        if (peek().get_kind() != Token::Kind::Eof) {
            throw std::runtime_error("Expected end of input");
        }
    }

    template<typename T>
    std::vector<T> parse_sep(
        std::function<T()> fn,
        Token::Kind sep,
        bool allow_trailing = false,
        std::size_t min_size = 0
    );

    std::string parse_ident();
    std::vector<Name::Seg> parse_path();
    Name parse_name();
    std::unique_ptr<Type> parse_type_basic();
    std::unique_ptr<Type> parse_tuple_type();
    std::optional<std::vector<std::unique_ptr<Type>>> parse_type_args();
    std::unique_ptr<Type> parse_hint();

    std::unique_ptr<Pat> parse_pat(bool use_hint = true);
    std::unique_ptr<Pat> parse_pat_basic(bool use_hint = true);
    std::unique_ptr<Pat> parse_tuple_pat();

    std::unique_ptr<Cond> parse_cond();
    std::unique_ptr<Clause> parse_clause();
    std::vector<std::unique_ptr<Expr>> parse_attrs();
    std::unique_ptr<Import> parse_import();

    std::unique_ptr<Expr> parse_expr0();
    std::unique_ptr<Expr> parse_expr1();
    std::unique_ptr<Expr> parse_expr2();
    std::unique_ptr<Expr> parse_expr3();
    std::unique_ptr<Expr> parse_expr4();
    std::unique_ptr<Expr> parse_expr5();
    std::unique_ptr<Expr> parse_expr6();
    std::unique_ptr<Expr> parse_expr7();
    std::unique_ptr<Expr> parse_expr8();
    std::unique_ptr<Expr> parse_expr9();

    std::unique_ptr<Expr> parse_tuple_expr();
    std::unique_ptr<Expr> parse_lam_expr();
    std::unique_ptr<Expr> parse_ite_expr();
    std::unique_ptr<Expr> parse_switch_expr();
    std::unique_ptr<Expr> parse_for_expr();
    std::unique_ptr<Expr> parse_while_expr();
    std::unique_ptr<Expr> parse_loop_expr();
    std::unique_ptr<Expr> parse_block_expr();

    std::unique_ptr<Stmt> parse_open_stmt();
    std::unique_ptr<Stmt> parse_let_stmt();
    std::unique_ptr<Stmt> parse_func_stmt();

    Access parse_access();
    std::string parse_type_param(std::vector<TypeBound>&);
    std::optional<std::vector<std::string>> parse_type_params(std::vector<TypeBound>&);
    std::vector<std::unique_ptr<Type>> parse_type_bound();
    std::vector<TypeBound> parse_type_bounds();
    std::vector<TypeBound> parse_where_bounds();

    std::unique_ptr<ModuleDecl> parse_module_decl();
    std::unique_ptr<OpenDecl> parse_open_decl();
    std::unique_ptr<ClassDecl> parse_class_decl();
    std::unique_ptr<EnumDecl> parse_enum_decl();
    std::unique_ptr<TypealiasDecl> parse_typealias_decl();
    std::unique_ptr<InterfaceDecl> parse_interface_decl();
    std::unique_ptr<ExtensionDecl> parse_extension_decl();
    std::unique_ptr<LetDecl> parse_let_decl();
    std::unique_ptr<FuncDecl> parse_func_decl();
    std::unique_ptr<InitDecl> parse_init_decl();
    std::unique_ptr<CtorDecl> parse_ctor_decl();
};

} // namespace parsing