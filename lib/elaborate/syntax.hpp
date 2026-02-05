#pragma once

#include "parsing/syntax.hpp"

namespace elaborate {

using Span = parsing::Span;
using Access = parsing::Access;

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

} // namespace elaborate
