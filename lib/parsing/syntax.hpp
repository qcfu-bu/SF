#pragma once

#include <format>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "lexer.hpp"

namespace parsing {

struct Name;
struct Import;
struct Type;
struct Lit;
struct Pat;
struct Cond;
struct Clause;
struct Expr;
struct Stmt;
struct Decl;
struct Package;

struct Name {
    using Seg = std::variant<std::string, int>;
    std::string ident;
    std::vector<Seg> path;

    explicit Name(std::string ident): ident(std::move(ident)) {}

    Name(std::string ident, std::vector<Seg> path):
        ident(std::move(ident)),
        path(std::move(path)) {}

    std::pair<std::vector<std::string>, std::vector<Seg>> slice() const;
};

enum class Access {
    Public,
    Private,
    Protected,
};

struct Import {
    enum class Kind {
        Node,
        Alias,
        Wild,
    };

    Import(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Import() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Type {
    enum class Kind {
        Meta,
        Int,
        Bool,
        Char,
        String,
        Unit,
        Name,
        Tuple,
        Arrow,
    };

    Type(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Type() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Lit {
    enum class Kind {
        Unit,
        Int,
        Bool,
        Char,
        String,
    };

    Lit(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Lit() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Pat {
    enum class Kind {
        Lit,
        Tuple,
        Ctor,
        Name,
        Wild,
        Or,
        At,
    };

    Pat(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Pat() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Cond {
    enum class Kind {
        Expr,
        Case,
    };

    Cond(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Cond() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Clause {
    enum class Kind {
        Case,
        Default,
    };

    Clause(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Clause() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Expr {
    enum class Kind {
        Lit,
        Unary,
        Binary,
        Tuple,
        Hint,
        Name,
        Hole,
        Lam,
        App,
        Block,
        Ite,
        Switch,
        For,
        While,
        Loop,
        Break,
        Continue,
        Return,
    };

    Expr(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Expr() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Stmt {
    enum class Kind {
        Open,
        Let,
        Func,
        Bind,
        Expr,
    };

    std::vector<std::unique_ptr<Expr>> attrs;

    Stmt(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Stmt() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

struct Decl {
    enum class Kind {
        Module,
        Open,
        Class,
        Enum,
        Typealias,
        Interface,
        Extension,
        Let,
        Func,
        Init,
        Ctor,
    };

    std::vector<std::unique_ptr<Expr>> attrs;
    Access access = Access::Public;

    Decl(Kind kind, Span span): kind(kind), span(span) {}
    virtual ~Decl() = default;

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

private:
    Kind kind;
    Span span;
};

// Imports
struct NodeImport: public Import {
    std::string name;
    std::vector<std::unique_ptr<Import>> nested;

    NodeImport(std::string name, std::unique_ptr<Import> import, Span span):
        Import(Kind::Node, span),
        name(std::move(name)) {
        nested.push_back(std::move(import));
    }

    NodeImport(std::string name, std::vector<std::unique_ptr<Import>> nested, Span span):
        Import(Kind::Node, span),
        name(std::move(name)),
        nested(std::move(nested)) {}
};

struct AliasImport: public Import {
    std::string name;
    std::optional<std::string> alias;

    AliasImport(std::string name, std::optional<std::string> alias, Span span):
        Import(Kind::Alias, span),
        name(std::move(name)),
        alias(std::move(alias)) {}
};

struct WildImport: public Import {
    explicit WildImport(Span span): Import(Kind::Wild, span) {}
};

// Types
struct MetaType: public Type {
    explicit MetaType(Span span): Type(Kind::Meta, span) {}
};

struct IntType: public Type {
    explicit IntType(Span span): Type(Kind::Int, span) {}
};

struct BoolType: public Type {
    explicit BoolType(Span span): Type(Kind::Bool, span) {}
};

struct CharType: public Type {
    explicit CharType(Span span): Type(Kind::Char, span) {}
};

struct StringType: public Type {
    explicit StringType(Span span): Type(Kind::String, span) {}
};

struct UnitType: public Type {
    explicit UnitType(Span span): Type(Kind::Unit, span) {}
};

struct NameType: public Type {
    Name name;
    std::optional<std::vector<std::unique_ptr<Type>>> type_args;

    NameType(Name name, std::optional<std::vector<std::unique_ptr<Type>>> type_args, Span span):
        Type(Kind::Name, span),
        name(std::move(name)),
        type_args(std::move(type_args)) {}
};

struct TupleType: public Type {
    std::vector<std::unique_ptr<Type>> elems;

    TupleType(std::vector<std::unique_ptr<Type>> elems, Span span):
        Type(Kind::Tuple, span),
        elems(std::move(elems)) {}
};

struct ArrowType: public Type {
    std::vector<std::unique_ptr<Type>> inputs;
    std::unique_ptr<Type> output;

    ArrowType(std::unique_ptr<Type> input, std::unique_ptr<Type> output, Span span):
        Type(Kind::Arrow, span),
        output(std::move(output)) {
        inputs.push_back(std::move(input));
    }

    ArrowType(std::vector<std::unique_ptr<Type>> inputs, std::unique_ptr<Type> output, Span span):
        Type(Kind::Arrow, span),
        inputs(std::move(inputs)),
        output(std::move(output)) {}
};

// Literals
struct UnitLit: public Lit {
    explicit UnitLit(Span span): Lit(Kind::Unit, span) {}
};

struct IntLit: public Lit {
    int value;

    IntLit(int value, Span span): Lit(Kind::Int, span), value(value) {}
};

struct BoolLit: public Lit {
    bool value;

    BoolLit(bool value, Span span): Lit(Kind::Bool, span), value(value) {}
};

struct CharLit: public Lit {
    char value;

    CharLit(char value, Span span): Lit(Kind::Char, span), value(value) {}
};

struct StringLit: public Lit {
    std::string value;

    StringLit(std::string value, Span span): Lit(Kind::String, span), value(std::move(value)) {}
};

// patterns
struct LitPat: public Pat {
    std::unique_ptr<Lit> literal;

    LitPat(std::unique_ptr<Lit> literal, Span span):
        Pat(Kind::Lit, span),
        literal(std::move(literal)) {}
};

struct TuplePat: public Pat {
    std::vector<std::unique_ptr<Pat>> elems;

    TuplePat(std::vector<std::unique_ptr<Pat>> elems, Span span):
        Pat(Kind::Tuple, span),
        elems(std::move(elems)) {}
};

struct CtorPat: public Pat {
    Name name;
    std::optional<std::vector<std::unique_ptr<Type>>> type_args;
    std::optional<std::vector<std::unique_ptr<Pat>>> args;

    CtorPat(
        Name name,
        std::optional<std::vector<std::unique_ptr<Type>>> type_args,
        std::optional<std::vector<std::unique_ptr<Pat>>> args,
        Span span
    ):
        Pat(Kind::Ctor, span),
        name(std::move(name)),
        type_args(std::move(type_args)),
        args(std::move(args)) {}
};

struct WildPat: public Pat {
    explicit WildPat(Span span): Pat(Kind::Wild, span) {}
};

struct NamePat: public Pat {
    Name name;
    std::optional<std::vector<std::unique_ptr<Type>>> type_args;
    std::unique_ptr<Type> hint;
    bool is_mut;

    NamePat(
        Name name,
        std::optional<std::vector<std::unique_ptr<Type>>> type_args,
        std::unique_ptr<Type> hint,
        bool is_mut,
        Span span
    ):
        Pat(Kind::Name, span),
        name(std::move(name)),
        type_args(std::move(type_args)),
        hint(std::move(hint)),
        is_mut(is_mut) {}
};

struct OrPat: public Pat {
    std::vector<std::unique_ptr<Pat>> options;

    OrPat(std::vector<std::unique_ptr<Pat>> options, Span span):
        Pat(Kind::Or, span),
        options(std::move(options)) {}
};

struct AtPat: public Pat {
    Name name;
    std::unique_ptr<Type> hint;
    bool is_mut;
    std::unique_ptr<Pat> pat;

    AtPat(Name name, std::unique_ptr<Type> hint, bool is_mut, std::unique_ptr<Pat> pat, Span span):
        Pat(Kind::At, span),
        name(std::move(name)),
        hint(std::move(hint)),
        is_mut(is_mut),
        pat(std::move(pat)) {}
};

// Statements
struct OpenStmt: public Stmt {
    std::unique_ptr<Import> import;

    OpenStmt(std::unique_ptr<Import> import, Span span):
        Stmt(Kind::Open, span),
        import(std::move(import)) {}
};

struct LetStmt: public Stmt {
    std::unique_ptr<Pat> pat;
    std::unique_ptr<Expr> expr;
    std::optional<std::unique_ptr<Expr>> else_branch;

    LetStmt(
        std::unique_ptr<Pat> pat,
        std::unique_ptr<Expr> expr,
        std::optional<std::unique_ptr<Expr>> else_branch,
        Span span
    ):
        Stmt(Kind::Let, span),
        pat(std::move(pat)),
        expr(std::move(expr)),
        else_branch(std::move(else_branch)) {}
};

struct FuncStmt: public Stmt {
    std::string ident;
    std::vector<std::unique_ptr<Pat>> params;
    std::unique_ptr<Type> ret_type;
    std::unique_ptr<Expr> body;

    FuncStmt(
        std::string ident,
        std::vector<std::unique_ptr<Pat>> params,
        std::unique_ptr<Type> ret_type,
        std::unique_ptr<Expr> body,
        Span span
    ):
        Stmt(Kind::Func, span),
        ident(std::move(ident)),
        params(std::move(params)),
        ret_type(std::move(ret_type)),
        body(std::move(body)) {}
};

struct BindStmt: public Stmt {
    std::unique_ptr<Pat> pat;
    std::unique_ptr<Expr> expr;

    BindStmt(std::unique_ptr<Pat> pat, std::unique_ptr<Expr> expr, Span span):
        Stmt(Kind::Bind, span),
        pat(std::move(pat)),
        expr(std::move(expr)) {}
};

struct ExprStmt: public Stmt {
    std::unique_ptr<Expr> expr;
    bool is_val;

    ExprStmt(std::unique_ptr<Expr> expr, bool is_val, Span span):
        Stmt(Kind::Expr, span),
        expr(std::move(expr)),
        is_val(is_val) {}
};

// Exprs
struct LitExpr: public Expr {
    std::unique_ptr<Lit> literal;

    LitExpr(std::unique_ptr<Lit> literal, Span span):
        Expr(Kind::Lit, span),
        literal(std::move(literal)) {}
};

struct UnaryExpr: public Expr {
    enum class Op {
        Pos,   // +
        Neg,   // -
        Not,   // !
        Addr,  // &
        Deref, // *
        Try,   // ?
        New,   // new
        Index, // []
        Dot,   // .
    };

    std::unique_ptr<Expr> expr;

    UnaryExpr(Op op, std::unique_ptr<Expr> expr, Span span):
        Expr(Kind::Unary, span),
        op(op),
        expr(std::move(expr)) {}

    Op get_op() const {
        return op;
    }

private:
    Op op;
};

struct PosExpr: public UnaryExpr {
    PosExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::Pos, std::move(expr), span) {}
};

struct NegExpr: public UnaryExpr {
    NegExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::Neg, std::move(expr), span) {}
};

struct NotExpr: public UnaryExpr {
    NotExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::Not, std::move(expr), span) {}
};

struct AddrExpr: public UnaryExpr {
    AddrExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::Addr, std::move(expr), span) {}
};

struct DerefExpr: public UnaryExpr {
    DerefExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::Deref, std::move(expr), span) {}
};

struct TryExpr: public UnaryExpr {
    TryExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::Try, std::move(expr), span) {}
};

struct NewExpr: public UnaryExpr {
    NewExpr(std::unique_ptr<Expr> expr, Span span): UnaryExpr(Op::New, std::move(expr), span) {}
};

struct IndexExpr: public UnaryExpr {
    std::vector<std::unique_ptr<Expr>> indices;

    IndexExpr(std::unique_ptr<Expr> base, std::vector<std::unique_ptr<Expr>> indices, Span span):
        UnaryExpr(Op::Index, std::move(base), span),
        indices(std::move(indices)) {}
};

struct DotExpr: public UnaryExpr {
    std::vector<Name::Seg> path;
    std::optional<std::vector<std::unique_ptr<Type>>> type_args;

    DotExpr(
        std::unique_ptr<Expr> base,
        std::vector<Name::Seg> path,
        std::optional<std::vector<std::unique_ptr<Type>>> type_args,
        Span span
    ):
        UnaryExpr(Op::Dot, std::move(base), span),
        path(std::move(path)),
        type_args(std::move(type_args)) {}
};

struct BinaryExpr: public Expr {
    enum class Op {
        Add,    // +
        Sub,    // -
        Mul,    // *
        Div,    // /
        Mod,    // %
        And,    // &&
        Or,     // ||
        Eq,     // ==
        Neq,    // !=
        Lt,     // <
        Gt,     // >
        Lte,    // <=
        Gte,    // >=
        Assign, // =
    };

    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(Op op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        Expr(Kind::Binary, span),
        op(op),
        left(std::move(left)),
        right(std::move(right)) {}

    Op get_op() const {
        return op;
    }

private:
    Op op;
};

struct AddExpr: public BinaryExpr {
    AddExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Add, std::move(left), std::move(right), span) {}
};

struct SubExpr: public BinaryExpr {
    SubExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Sub, std::move(left), std::move(right), span) {}
};

struct MulExpr: public BinaryExpr {
    MulExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Mul, std::move(left), std::move(right), span) {}
};

struct DivExpr: public BinaryExpr {
    DivExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Div, std::move(left), std::move(right), span) {}
};

struct ModExpr: public BinaryExpr {
    ModExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Mod, std::move(left), std::move(right), span) {}
};

struct AndExpr: public BinaryExpr {
    AndExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::And, std::move(left), std::move(right), span) {}
};

struct OrExpr: public BinaryExpr {
    OrExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Or, std::move(left), std::move(right), span) {}
};

struct EqExpr: public BinaryExpr {
    EqExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Eq, std::move(left), std::move(right), span) {}
};

struct NeqExpr: public BinaryExpr {
    NeqExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Neq, std::move(left), std::move(right), span) {}
};

struct LtExpr: public BinaryExpr {
    LtExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Lt, std::move(left), std::move(right), span) {}
};

struct GtExpr: public BinaryExpr {
    GtExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Gt, std::move(left), std::move(right), span) {}
};

struct LteExpr: public BinaryExpr {
    LteExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Lte, std::move(left), std::move(right), span) {}
};

struct GteExpr: public BinaryExpr {
    GteExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Gte, std::move(left), std::move(right), span) {}
};

struct AssignExpr: public BinaryExpr {
    BinaryExpr::Op mode;

    AssignExpr(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Assign, std::move(left), std::move(right), span),
        mode(Op::Assign) {}

    AssignExpr(Op mode, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, Span span):
        BinaryExpr(Op::Assign, std::move(left), std::move(right), span),
        mode(mode) {}
};

struct TupleExpr: public Expr {
    std::vector<std::unique_ptr<Expr>> elems;
    explicit TupleExpr(std::vector<std::unique_ptr<Expr>> elems, Span span):
        Expr(Expr::Kind::Tuple, span),
        elems(std::move(elems)) {}
};

struct HintExpr: public Expr {
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Type> type;
    HintExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Type> type, Span span):
        Expr(Expr::Kind::Hint, span),
        expr(std::move(expr)),
        type(std::move(type)) {}
};

struct NameExpr: public Expr {
    Name name;
    std::optional<std::vector<std::unique_ptr<Type>>> type_args;

    NameExpr(Name name, std::optional<std::vector<std::unique_ptr<Type>>> type_args, Span span):
        Expr(Kind::Name, span),
        name(std::move(name)),
        type_args(std::move(type_args)) {}
};

struct HoleExpr: public Expr {
    explicit HoleExpr(Span span): Expr(Kind::Hole, span) {}
};

struct LamExpr: public Expr {
    std::vector<std::unique_ptr<Pat>> params;
    std::unique_ptr<Expr> body;

    LamExpr(std::vector<std::unique_ptr<Pat>> params, std::unique_ptr<Expr> body, Span span):
        Expr(Kind::Lam, span),
        params(std::move(params)),
        body(std::move(body)) {}
};

struct AppExpr: public Expr {
    std::unique_ptr<Expr> func;
    std::vector<std::unique_ptr<Expr>> args;

    AppExpr(std::unique_ptr<Expr> func, std::vector<std::unique_ptr<Expr>> args, Span span):
        Expr(Kind::App, span),
        func(std::move(func)),
        args(std::move(args)) {}
};

struct BlockExpr: public Expr {
    std::vector<std::unique_ptr<Stmt>> stmts;
    std::optional<std::unique_ptr<Expr>> body;

    explicit BlockExpr(std::vector<std::unique_ptr<Stmt>> stmts, Span span):
        Expr(Kind::Block, span) {
        std::optional<std::unique_ptr<Expr>> body = std::nullopt;
        if (!stmts.empty() && stmts.back()->get_kind() == Stmt::Kind::Expr) {
            auto* expr = static_cast<ExprStmt*>(stmts.back().get());
            if (expr->is_val) {
                body = std::move(expr->expr);
                stmts.pop_back();
            }
        }
        this->stmts = std::move(stmts);
        this->body = std::move(body);
    }
};

struct BreakExpr: public Expr {
    explicit BreakExpr(Span span): Expr(Kind::Break, span) {}
};

struct ContinueExpr: public Expr {
    explicit ContinueExpr(Span span): Expr(Kind::Continue, span) {}
};

struct ReturnExpr: public Expr {
    std::optional<std::unique_ptr<Expr>> expr;

    explicit ReturnExpr(std::optional<std::unique_ptr<Expr>> expr, Span span):
        Expr(Kind::Return, span),
        expr(std::move(expr)) {}
};

struct IteThen {
    std::unique_ptr<Cond> cond;
    std::unique_ptr<Expr> then_branch;
};

struct IteExpr: public Expr {
    std::vector<IteThen> then_branches;
    std::optional<std::unique_ptr<Expr>> else_branch;

    IteExpr(
        std::vector<IteThen> then_branches,
        std::optional<std::unique_ptr<Expr>> else_branch,
        Span span
    ):
        Expr(Kind::Ite, span),
        then_branches(std::move(then_branches)),
        else_branch(std::move(else_branch)) {}
};

struct ExprCond: public Cond {
    std::unique_ptr<Expr> expr;

    explicit ExprCond(std::unique_ptr<Expr> expr, Span span):
        Cond(Kind::Expr, span),
        expr(std::move(expr)) {}
};

struct PatCond: public Cond {
    std::unique_ptr<Pat> pat;
    std::unique_ptr<Expr> expr;

    PatCond(std::unique_ptr<Pat> pat, std::unique_ptr<Expr> expr, Span span):
        Cond(Kind::Case, span),
        pat(std::move(pat)),
        expr(std::move(expr)) {}
};

struct CaseClause: public Clause {
    std::unique_ptr<Pat> pat;
    std::optional<std::unique_ptr<Expr>> guard;
    std::unique_ptr<Expr> expr;

    CaseClause(
        std::unique_ptr<Pat> pat,
        std::optional<std::unique_ptr<Expr>> guard,
        std::unique_ptr<Expr> expr,
        Span span
    ):
        Clause(Clause::Kind::Case, span),
        pat(std::move(pat)),
        guard(std::move(guard)),
        expr(std::move(expr)) {}
};

struct DefaultClause: public Clause {
    std::unique_ptr<Expr> expr;

    explicit DefaultClause(std::unique_ptr<Expr> expr, Span span):
        Clause(Clause::Kind::Default, span),
        expr(std::move(expr)) {}
};

struct SwitchExpr: public Expr {
    std::unique_ptr<Expr> expr;
    std::vector<std::unique_ptr<Clause>> clauses;

    SwitchExpr(std::unique_ptr<Expr> expr, std::vector<std::unique_ptr<Clause>> clauses, Span span):
        Expr(Kind::Switch, span),
        expr(std::move(expr)),
        clauses(std::move(clauses)) {}
};

struct ForExpr: public Expr {
    std::unique_ptr<Pat> pat;
    std::unique_ptr<Expr> iter;
    std::unique_ptr<Expr> body;

    ForExpr(
        std::unique_ptr<Pat> pat,
        std::unique_ptr<Expr> iter,
        std::unique_ptr<Expr> body,
        Span span
    ):
        Expr(Kind::For, span),
        pat(std::move(pat)),
        iter(std::move(iter)),
        body(std::move(body)) {}
};

struct WhileExpr: public Expr {
    std::unique_ptr<Cond> cond;
    std::unique_ptr<Expr> body;

    WhileExpr(std::unique_ptr<Cond> cond, std::unique_ptr<Expr> body, Span span):
        Expr(Kind::While, span),
        cond(std::move(cond)),
        body(std::move(body)) {}
};

struct LoopExpr: public Expr {
    std::unique_ptr<Expr> body;

    explicit LoopExpr(std::unique_ptr<Expr> body, Span span):
        Expr(Kind::Loop, span),
        body(std::move(body)) {}
};

// declarations
struct TypeBound {
    std::unique_ptr<Type> type;
    std::vector<std::unique_ptr<Type>> bounds;
};

struct ModuleDecl: public Decl {
    std::string ident;
    std::vector<std::unique_ptr<Decl>> body;

    ModuleDecl(std::string ident, std::vector<std::unique_ptr<Decl>> body, Span span):
        Decl(Kind::Module, span),
        ident(std::move(ident)),
        body(std::move(body)) {}
};

struct OpenDecl: public Decl {
    std::unique_ptr<Import> import;

    explicit OpenDecl(std::unique_ptr<Import> import, Span span):
        Decl(Kind::Open, span),
        import(std::move(import)) {}
};

struct ClassDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::vector<std::unique_ptr<Decl>> body;

    ClassDecl(
        std::string ident,
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::vector<std::unique_ptr<Decl>> body,
        Span span
    ):
        Decl(Kind::Class, span),
        ident(std::move(ident)),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        body(std::move(body)) {}
};

struct EnumDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::vector<std::unique_ptr<Decl>> body;

    EnumDecl(
        std::string ident,
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::vector<std::unique_ptr<Decl>> body,
        Span span
    ):
        Decl(Kind::Enum, span),
        ident(std::move(ident)),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        body(std::move(body)) {}
};

struct TypealiasDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::vector<std::unique_ptr<Type>> hint;
    std::optional<std::unique_ptr<Type>> aliased;

    TypealiasDecl(
        std::string ident,
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::vector<std::unique_ptr<Type>> hint,
        std::optional<std::unique_ptr<Type>> aliased,
        Span span
    ):
        Decl(Kind::Typealias, span),
        ident(std::move(ident)),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        hint(std::move(hint)),
        aliased(std::move(aliased)) {}
};

struct InterfaceDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::vector<std::unique_ptr<Decl>> body;

    InterfaceDecl(
        std::string ident,
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::vector<std::unique_ptr<Decl>> body,
        Span span
    ):
        Decl(Kind::Interface, span),
        ident(std::move(ident)),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        body(std::move(body)) {}
};

struct ExtensionDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::unique_ptr<Type> base_type;
    std::unique_ptr<Type> interface;
    std::vector<std::unique_ptr<Decl>> body;

    ExtensionDecl(
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::unique_ptr<Type> base_type,
        std::unique_ptr<Type> interface,
        std::vector<std::unique_ptr<Decl>> body,
        Span span
    ):
        Decl(Kind::Extension, span),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        base_type(std::move(base_type)),
        interface(std::move(interface)),
        body(std::move(body)) {}
};

struct LetDecl: public Decl {
    std::unique_ptr<Pat> pat;
    std::optional<std::unique_ptr<Expr>> expr;

    LetDecl(std::unique_ptr<Pat> pat, std::optional<std::unique_ptr<Expr>> expr, Span span):
        Decl(Kind::Let, span),
        pat(std::move(pat)),
        expr(std::move(expr)) {}
};

struct FuncDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::vector<std::unique_ptr<Pat>> params;
    std::unique_ptr<Type> ret_type;
    std::optional<std::unique_ptr<Expr>> body;

    FuncDecl(
        std::string ident,
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::vector<std::unique_ptr<Pat>> params,
        std::unique_ptr<Type> ret_type,
        std::optional<std::unique_ptr<Expr>> body,
        Span span
    ):
        Decl(Kind::Func, span),
        ident(std::move(ident)),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        params(std::move(params)),
        ret_type(std::move(ret_type)),
        body(std::move(body)) {}
};

struct InitDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::string>> type_params;
    std::vector<TypeBound> type_bounds;
    std::vector<std::unique_ptr<Pat>> params;
    std::unique_ptr<Type> ret_type;
    std::optional<std::unique_ptr<Expr>> body;

    InitDecl(
        std::string ident,
        std::optional<std::vector<std::string>> type_params,
        std::vector<TypeBound> type_bounds,
        std::vector<std::unique_ptr<Pat>> params,
        std::unique_ptr<Type> ret_type,
        std::optional<std::unique_ptr<Expr>> body,
        Span span
    ):
        Decl(Kind::Init, span),
        ident(std::move(ident)),
        type_params(std::move(type_params)),
        type_bounds(std::move(type_bounds)),
        params(std::move(params)),
        ret_type(std::move(ret_type)),
        body(std::move(body)) {}
};

struct CtorDecl: public Decl {
    std::string ident;
    std::optional<std::vector<std::unique_ptr<Type>>> params;

    CtorDecl(
        std::string ident,
        std::optional<std::vector<std::unique_ptr<Type>>> params,
        Span span
    ):
        Decl(Kind::Ctor, span),
        ident(std::move(ident)),
        params(std::move(params)) {}
};

struct Package {
    std::string ident;
    std::vector<std::unique_ptr<Import>> header;
    std::vector<std::unique_ptr<Decl>> body;

    Package(
        std::string ident,
        std::vector<std::unique_ptr<Import>> header,
        std::vector<std::unique_ptr<Decl>> body,
        Span span
    ):
        ident(std::move(ident)),
        header(std::move(header)),
        body(std::move(body)),
        span(span) {}

private:
    Span span;
};

} // namespace parsing

// std::formatter specializations
template<>
struct std::formatter<parsing::Name>: std::formatter<std::string> {
    auto format(const parsing::Name& name, std::format_context& ctx) const {
        std::string result = name.ident;
        for (const auto& p: name.path) {
            if (const auto* ptr = std::get_if<int>(&p)) {
                result += "." + std::to_string(*ptr);
            } else {
                result += "." + std::get<std::string>(p);
            }
        }
        return std::formatter<std::string>::format(result, ctx);
    }
};

template<>
struct std::formatter<parsing::Type>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Type& type, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Lit>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Lit& lit, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Pat>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Pat& pat, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Expr>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Expr& expr, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Stmt>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Stmt& stmt, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Import>: std::formatter<std::string> {
    std::format_context::iterator
    format(const parsing::Import& import, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Cond>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Cond& cond, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Clause>: std::formatter<std::string> {
    std::format_context::iterator
    format(const parsing::Clause& clause, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Decl>: std::formatter<std::string> {
    std::format_context::iterator format(const parsing::Decl& decl, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Package>: std::formatter<std::string> {
    std::format_context::iterator
    format(const parsing::Package& pkg, std::format_context& ctx) const;
};
