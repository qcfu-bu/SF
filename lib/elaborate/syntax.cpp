#include "syntax.hpp"

namespace elaborate {

// Forward declarations for recursive formatting
std::string format_type(const Type& type);
std::string format_lit(const Lit& lit);
std::string format_pat(const Pat& pat);
std::string format_expr(const Expr& expr, int indent = 0);
std::string format_stmt(const Stmt& stmt, int indent = 0);
std::string format_import(const Import& import);
std::string format_cond(const Cond& cond);
std::string format_clause(const Clause& clause, int indent = 0);
std::string format_decl(const Decl& decl, int indent = 0);

static std::string indent_str(int indent) {
    return std::string(indent * 4, ' ');
}

// Format type arguments
static std::string
format_type_args(const std::optional<std::vector<std::shared_ptr<Type>>>& type_args) {
    if (!type_args.has_value() || type_args->empty()) {
        return "";
    }
    std::string result = "<";
    for (size_t i = 0; i < type_args->size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += format_type(*(*type_args)[i]);
    }
    result += ">";
    return result;
}

// Format Type
std::string format_type(const Type& type) {
    switch (type.get_kind()) {
        case Type::Kind::Meta:
            return "_";
        case Type::Kind::Int:
            return "Int";
        case Type::Kind::Bool:
            return "Bool";
        case Type::Kind::Char:
            return "Char";
        case Type::Kind::String:
            return "String";
        case Type::Kind::Unit:
            return "()";
        case Type::Kind::Var: {
            const auto& t = static_cast<const VarType&>(type);
            return t.ident;
        }
        case Type::Kind::Enum: {
            const auto& t = static_cast<const EnumType&>(type);
            return t.ident + format_type_args(t.type_args);
        }
        case Type::Kind::Class: {
            const auto& t = static_cast<const ClassType&>(type);
            return t.ident + format_type_args(t.type_args);
        }
        case Type::Kind::Typealias: {
            const auto& t = static_cast<const TypealiasType&>(type);
            return t.ident + format_type_args(t.type_args);
        }
        case Type::Kind::Interface: {
            const auto& t = static_cast<const InterfaceType&>(type);
            return t.ident + format_type_args(t.type_args);
        }
        case Type::Kind::Tuple: {
            const auto& t = static_cast<const TupleType&>(type);
            std::string result = "(";
            for (size_t i = 0; i < t.elems.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_type(*t.elems[i]);
            }
            result += ")";
            return result;
        }
        case Type::Kind::Arrow: {
            const auto& t = static_cast<const ArrowType&>(type);
            std::string result;
            if (t.inputs.size() == 1) {
                result = format_type(*t.inputs[0]);
            } else {
                result = "(";
                for (size_t i = 0; i < t.inputs.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += format_type(*t.inputs[i]);
                }
                result += ")";
            }
            result += " -> " + format_type(*t.output);
            return result;
        }
    }
    return "<?type>";
}

// Format Literal
std::string format_lit(const Lit& lit) {
    switch (lit.get_kind()) {
        case Lit::Kind::Unit:
            return "()";
        case Lit::Kind::Int:
            return std::to_string(static_cast<const IntLit&>(lit).value);
        case Lit::Kind::Bool:
            return static_cast<const BoolLit&>(lit).value ? "true" : "false";
        case Lit::Kind::Char: {
            char c = static_cast<const CharLit&>(lit).value;
            if (c == '\'') {
                return "'\\''";
            }
            if (c == '\\') {
                return "'\\\\'";
            }
            if (c == '\n') {
                return "'\\n'";
            }
            if (c == '\t') {
                return "'\\t'";
            }
            if (c == '\r') {
                return "'\\r'";
            }
            return std::string("'") + c + "'";
        }
        case Lit::Kind::String: {
            const auto& s = static_cast<const StringLit&>(lit).value;
            std::string result = "\"";
            for (char c: s) {
                if (c == '"') {
                    result += "\\\"";
                } else if (c == '\\') {
                    result += "\\\\";
                } else if (c == '\n') {
                    result += "\\n";
                } else if (c == '\t') {
                    result += "\\t";
                } else if (c == '\r') {
                    result += "\\r";
                } else {
                    result += c;
                }
            }
            result += "\"";
            return result;
        }
    }
    return "<?lit>";
}

// Format Pattern
std::string format_pat(const Pat& pat) {
    switch (pat.get_kind()) {
        case Pat::Kind::Lit:
            return format_lit(*static_cast<const LitPat&>(pat).literal);
        case Pat::Kind::Var: {
            const auto& p = static_cast<const VarPat&>(pat);
            std::string result;
            if (p.is_mut) {
                result += "mut ";
            }
            result += "%" + p.ident;
            if (p.hint && p.hint->get_kind() != Type::Kind::Meta) {
                result += ": " + format_type(*p.hint);
            }
            return result;
        }
        case Pat::Kind::Tuple: {
            const auto& p = static_cast<const TuplePat&>(pat);
            std::string result = "(";
            for (size_t i = 0; i < p.elems.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_pat(*p.elems[i]);
            }
            result += ")";
            return result;
        }
        case Pat::Kind::Ctor: {
            const auto& p = static_cast<const CtorPat&>(pat);
            std::string result = p.ident + format_type_args(p.type_args);
            if (!p.args.has_value()) {
                return result;
            }
            result += "(";
            for (size_t i = 0; i < p.args->size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_pat(*(*p.args)[i]);
            }
            result += ")";
            return result;
        }
        case Pat::Kind::Wild:
            return "_";
        case Pat::Kind::Or: {
            const auto& p = static_cast<const OrPat&>(pat);
            std::string result;
            for (size_t i = 0; i < p.options.size(); ++i) {
                if (i > 0) {
                    result += " | ";
                }
                result += format_pat(*p.options[i]);
            }
            return result;
        }
        case Pat::Kind::At: {
            const auto& p = static_cast<const AtPat&>(pat);
            std::string result;
            if (p.is_mut) {
                result += "mut ";
            }
            result += p.ident;
            if (p.hint && p.hint->get_kind() != Type::Kind::Meta) {
                result += ": " + format_type(*p.hint);
            }
            result += " @ " + format_pat(*p.pat);
            return result;
        }
    }
    return "<?pat>";
}

// Format Import
std::string format_import(const Import& import) {
    switch (import.get_kind()) {
        case Import::Kind::Node: {
            const auto& i = static_cast<const NodeImport&>(import);
            std::string result = i.name;
            if (!i.nested.empty()) {
                result += ".";
                if (i.nested.size() == 1) {
                    result += format_import(*i.nested[0]);
                } else {
                    result += "{";
                    for (size_t j = 0; j < i.nested.size(); ++j) {
                        if (j > 0) {
                            result += ", ";
                        }
                        result += format_import(*i.nested[j]);
                    }
                    result += "}";
                }
            }
            return result;
        }
        case Import::Kind::Alias: {
            const auto& i = static_cast<const AliasImport&>(import);
            std::string result = i.name;
            if (i.alias.has_value()) {
                result += " as " + *i.alias;
            } else {
                result += " as _";
            }
            return result;
        }
        case Import::Kind::Wild:
            return "*";
    }
    return "<?import>";
}

// Format Condition
std::string format_cond(const Cond& cond) {
    switch (cond.get_kind()) {
        case Cond::Kind::Expr:
            return format_expr(*static_cast<const ExprCond&>(cond).expr);
        case Cond::Kind::Case: {
            const auto& c = static_cast<const PatCond&>(cond);
            return "let " + format_pat(*c.pat) + " = " + format_expr(*c.expr);
        }
    }
    return "<?cond>";
}

// Format Clause
std::string format_clause(const Clause& clause, int indent) {
    switch (clause.get_kind()) {
        case Clause::Kind::Case: {
            const auto& c = static_cast<const CaseClause&>(clause);
            std::string result = indent_str(indent) + "case " + format_pat(*c.pat);
            if (c.guard.has_value()) {
                result += " if " + format_expr(**c.guard);
            }
            result += ": " + format_expr(*c.expr, indent);
            return result;
        }
        case Clause::Kind::Default: {
            const auto& c = static_cast<const DefaultClause&>(clause);
            return indent_str(indent) + "default: " + format_expr(*c.expr, indent);
        }
    }
}

// Format Expression
std::string format_expr(const Expr& expr, int indent) {
    switch (expr.get_kind()) {
        case Expr::Kind::Lit:
            return format_lit(*static_cast<const LitExpr&>(expr).literal);

        case Expr::Kind::Unary: {
            const auto& u = static_cast<const UnaryExpr&>(expr);
            auto result = format_expr(*u.expr, indent);
            switch (u.get_op()) {
                case UnaryExpr::Op::Pos:
                    return "+" + result;
                case UnaryExpr::Op::Neg:
                    return "-" + result;
                case UnaryExpr::Op::Not:
                    return "!" + result;
                case UnaryExpr::Op::Addr:
                    return "&" + result;
                case UnaryExpr::Op::Deref:
                    return "*" + result;
                case UnaryExpr::Op::Try:
                    return result + "?";
                case UnaryExpr::Op::New:
                    return "new " + result;
                case UnaryExpr::Op::Index: {
                    const auto& e = static_cast<const IndexExpr&>(expr);
                    result += "[";
                    for (size_t i = 0; i < e.indices.size(); ++i) {
                        if (i > 0) {
                            result += ", ";
                        }
                        result += format_expr(*e.indices[i], indent);
                    }
                    result += "]";
                    return result;
                }
                case UnaryExpr::Op::Field: {
                    const auto& e = static_cast<const FieldExpr&>(expr);
                    result += "." + e.path;
                    result += format_type_args(e.type_args);
                    return result;
                }
                case UnaryExpr::Op::Proj: {
                    const auto& e = static_cast<const ProjExpr&>(expr);
                    return result + "." + std::to_string(e.index);
                }
            }
            break;
        }

        case Expr::Kind::Binary: {
            const auto& b = static_cast<const BinaryExpr&>(expr);
            std::string op;
            const Expr* left = b.left.get();
            const Expr* right = b.right.get();

            switch (b.get_op()) {
                case BinaryExpr::Op::Add:
                    op = " + ";
                    break;
                case BinaryExpr::Op::Sub:
                    op = " - ";
                    break;
                case BinaryExpr::Op::Mul:
                    op = " * ";
                    break;
                case BinaryExpr::Op::Div:
                    op = " / ";
                    break;
                case BinaryExpr::Op::Mod:
                    op = " % ";
                    break;
                case BinaryExpr::Op::And:
                    op = " && ";
                    break;
                case BinaryExpr::Op::Or:
                    op = " || ";
                    break;
                case BinaryExpr::Op::Eq:
                    op = " == ";
                    break;
                case BinaryExpr::Op::Neq:
                    op = " != ";
                    break;
                case BinaryExpr::Op::Lt:
                    op = " < ";
                    break;
                case BinaryExpr::Op::Gt:
                    op = " > ";
                    break;
                case BinaryExpr::Op::Lte:
                    op = " <= ";
                    break;
                case BinaryExpr::Op::Gte:
                    op = " >= ";
                    break;
                case BinaryExpr::Op::Assign: {
                    const auto& e = static_cast<const AssignExpr&>(expr);
                    switch (e.mode) {
                        case BinaryExpr::Op::Assign:
                            op = " = ";
                            break;
                        case BinaryExpr::Op::Add:
                            op = " += ";
                            break;
                        case BinaryExpr::Op::Sub:
                            op = " -= ";
                            break;
                        case BinaryExpr::Op::Mul:
                            op = " *= ";
                            break;
                        case BinaryExpr::Op::Div:
                            op = " /= ";
                            break;
                        case BinaryExpr::Op::Mod:
                            op = " %= ";
                            break;
                        default:
                            op = " = ";
                            break;
                    }
                    break;
                }
            }
            return format_expr(*left, indent) + op + format_expr(*right, indent);
        }

        case Expr::Kind::Tuple: {
            const auto& e = static_cast<const TupleExpr&>(expr);
            std::string result = "(";
            for (size_t i = 0; i < e.elems.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_expr(*e.elems[i], indent);
            }
            result += ")";
            return result;
        }

        case Expr::Kind::Hint: {
            const auto& e = static_cast<const HintExpr&>(expr);
            return "(" + format_expr(*e.expr, indent) + ": " + format_type(*e.type) + ")";
        }

        case Expr::Kind::Const: {
            const auto& e = static_cast<const ConstExpr&>(expr);
            return e.ident + format_type_args(e.type_args);
        }

        case Expr::Kind::Var: {
            const auto& e = static_cast<const VarExpr&>(expr);
            return e.ident;
        }

        case Expr::Kind::Lam: {
            const auto& e = static_cast<const LamExpr&>(expr);
            std::string result;
            if (e.params.size() == 1) {
                result = format_pat(*e.params[0]);
            } else {
                result = "(";
                for (size_t i = 0; i < e.params.size(); ++i) {
                    if (i > 0) {
                        result += ", ";
                    }
                    result += format_pat(*e.params[i]);
                }
                result += ")";
            }
            result += " => " + format_expr(*e.body, indent);
            return result;
        }

        case Expr::Kind::App: {
            const auto& e = static_cast<const AppExpr&>(expr);
            std::string result = format_expr(*e.func, indent) + "(";
            for (size_t i = 0; i < e.args.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_expr(*e.args[i], indent);
            }
            result += ")";
            return result;
        }

        case Expr::Kind::Block: {
            const auto& e = static_cast<const BlockExpr&>(expr);
            if (e.stmts.empty() && !e.body.has_value()) {
                return "{}";
            }
            std::string result = "{\n";
            for (const auto& stmt: e.stmts) {
                result += format_stmt(*stmt, indent + 1) + "\n";
            }
            if (e.body.has_value()) {
                result += indent_str(indent + 1) + format_expr(**e.body, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            return result;
        }

        case Expr::Kind::Ite: {
            const auto& e = static_cast<const IteExpr&>(expr);
            std::string result = "if " + format_cond(*e.then_branches[0].cond) + " ";
            result += format_expr(*e.then_branches[0].then_branch, indent);

            for (size_t i = 1; i < e.then_branches.size(); ++i) {
                result += " else if " + format_cond(*e.then_branches[i].cond) + " ";
                result += format_expr(*e.then_branches[i].then_branch, indent);
            }

            if (e.else_branch.has_value()) {
                result += " else " + format_expr(**e.else_branch, indent);
            }
            return result;
        }

        case Expr::Kind::Switch: {
            const auto& e = static_cast<const SwitchExpr&>(expr);
            std::string result = "switch " + format_expr(*e.expr, indent) + " {\n";
            for (const auto& clause: e.clauses) {
                result += format_clause(*clause, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            return result;
        }

        case Expr::Kind::For: {
            const auto& e = static_cast<const ForExpr&>(expr);
            return "for " + format_pat(*e.pat) + " in " + format_expr(*e.iter, indent) + " "
                + format_expr(*e.body, indent);
        }

        case Expr::Kind::While: {
            const auto& e = static_cast<const WhileExpr&>(expr);
            return "while " + format_cond(*e.cond) + " " + format_expr(*e.body, indent);
        }

        case Expr::Kind::Loop: {
            const auto& e = static_cast<const LoopExpr&>(expr);
            return "loop " + format_expr(*e.body, indent);
        }

        case Expr::Kind::Break:
            return "break";
        case Expr::Kind::Continue:
            return "continue";
        case Expr::Kind::Return: {
            const auto& s = static_cast<const ReturnExpr&>(expr);
            std::string result = "return";
            if (s.expr.has_value()) {
                result += " " + format_expr(**s.expr, indent);
            }
            return result;
        }
    }
    return "<?expr>";
}

// Format Statement
std::string format_stmt(const Stmt& stmt, int indent) {
    std::string result = indent_str(indent);

    // Format attributes
    for (const auto& attr: stmt.attrs) {
        result += "@" + format_expr(*attr, indent) + "\n" + indent_str(indent);
    }

    switch (stmt.get_kind()) {
        case Stmt::Kind::Let: {
            const auto& s = static_cast<const LetStmt&>(stmt);
            result += "let " + format_pat(*s.pat) + " = " + format_expr(*s.expr, indent);
            if (s.else_branch) {
                result += " else " + format_expr(**s.else_branch, indent);
            }
            result += ";";
            break;
        }
        case Stmt::Kind::Func: {
            const auto& s = static_cast<const FuncStmt&>(stmt);
            result += "func " + s.ident + "(";
            for (size_t i = 0; i < s.params.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_pat(*s.params[i]);
            }
            result += ")";
            if (s.ret_type && s.ret_type->get_kind() != Type::Kind::Meta) {
                result += " -> " + format_type(*s.ret_type);
            }
            result += " " + format_expr(*s.body, indent);
            break;
        }
        case Stmt::Kind::Bind: {
            const auto& s = static_cast<const BindStmt&>(stmt);
            result += "let " + format_pat(*s.pat) + " <- " + format_expr(*s.expr, indent) + ";";
            break;
        }
        case Stmt::Kind::Expr: {
            const auto& s = static_cast<const ExprStmt&>(stmt);
            result += format_expr(*s.expr, indent);
            if (!s.is_val) {
                result += ";";
            }
            break;
        }
    }
    return result;
}

// Format access
static std::string format_access(Access vis) {
    switch (vis) {
        case Access::Public:
            return "public ";
        case Access::Private:
            return "private ";
        case Access::Protected:
            return "protected ";
    }
    return "";
}

// Format type parameters
static std::string format_type_params(const std::optional<std::vector<std::string>>& type_params) {
    if (!type_params.has_value() || type_params->empty()) {
        return "";
    }
    std::string result = "<";
    for (size_t i = 0; i < type_params->size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += (*type_params)[i];
    }
    result += ">";
    return result;
}

// Format type bounds
static std::string format_type_bounds(const std::vector<TypeBound>& bounds) {
    if (bounds.empty()) {
        return "";
    }
    std::string result = " where ";
    for (size_t i = 0; i < bounds.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += format_type(*bounds[i].type);
        if (!bounds[i].bounds.empty()) {
            result += ": ";
            for (size_t j = 0; j < bounds[i].bounds.size(); ++j) {
                if (j > 0) {
                    result += " + ";
                }
                result += format_type(*bounds[i].bounds[j]);
            }
        }
    }
    return result;
}

// Format enum constructor
static std::string format_ctor(const CtorDecl& ctor) {
    std::string result = "case " + ctor.ident;
    if (ctor.params.has_value() && !ctor.params->empty()) {
        result += "(";
        for (size_t i = 0; i < ctor.params->size(); ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += format_type(*(*ctor.params)[i]);
        }
        result += ")";
    }
    return result;
}

// Format Declaration
std::string format_decl(const Decl& decl, int indent) {
    std::string result = indent_str(indent);

    // Format attributes
    for (const auto& attr: decl.attrs) {
        result += "@" + format_expr(*attr, indent) + "\n" + indent_str(indent);
    }

    // Format access (skip for public as it's default)
    if (decl.access != Access::Public) {
        result += format_access(decl.access);
    }

    switch (decl.get_kind()) {
        case Decl::Kind::Module: {
            const auto& d = static_cast<const ModuleDecl&>(decl);
            result += "module " + d.ident + " {\n";
            for (const auto& inner: d.body) {
                result += format_decl(*inner, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            break;
        }
        case Decl::Kind::Class: {
            const auto& d = static_cast<const ClassDecl&>(decl);
            result += "class " + d.ident;
            result += format_type_params(d.type_params);
            result += format_type_bounds(d.type_bounds);
            result += " {\n";
            for (const auto& inner: d.body) {
                result += format_decl(*inner, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            break;
        }
        case Decl::Kind::Enum: {
            const auto& d = static_cast<const EnumDecl&>(decl);
            result += "enum " + d.ident;
            result += format_type_params(d.type_params);
            result += format_type_bounds(d.type_bounds);
            result += " {\n";
            for (const auto& inner: d.body) {
                result += format_decl(*inner, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            break;
        }
        case Decl::Kind::Typealias: {
            const auto& d = static_cast<const TypealiasDecl&>(decl);
            result += "type " + d.ident;
            result += format_type_params(d.type_params);
            result += format_type_bounds(d.type_bounds);
            if (!d.hint.empty()) {
                result += ": ";
                for (size_t i = 0; i < d.hint.size(); ++i) {
                    if (i > 0) {
                        result += " + ";
                    }
                    result += format_type(*d.hint[i]);
                }
            }
            if (d.aliased.has_value()) {
                result += " = " + format_type(**d.aliased);
            }
            result += ";";
            break;
        }
        case Decl::Kind::Interface: {
            const auto& d = static_cast<const InterfaceDecl&>(decl);
            result += "interface " + d.ident;
            result += format_type_params(d.type_params);
            result += format_type_bounds(d.type_bounds);
            result += " {\n";
            for (const auto& inner: d.body) {
                result += format_decl(*inner, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            break;
        }
        case Decl::Kind::Extension: {
            const auto& d = static_cast<const ExtensionDecl&>(decl);
            result += "extension";
            result += format_type_params(d.type_params);
            if (!d.ident.empty()) {
                result += " " + d.ident;
            }
            result += " " + format_type(*d.base_type);
            result += ": " + format_type(*d.interface);
            result += format_type_bounds(d.type_bounds);
            result += " {\n";
            for (const auto& inner: d.body) {
                result += format_decl(*inner, indent + 1) + "\n";
            }
            result += indent_str(indent) + "}";
            break;
        }
        case Decl::Kind::Let: {
            const auto& d = static_cast<const LetDecl&>(decl);
            result += "let " + format_pat(*d.pat);
            if (d.expr.has_value()) {
                result += " = " + format_expr(**d.expr, indent);
            }
            result += ";";
            break;
        }
        case Decl::Kind::Func: {
            const auto& d = static_cast<const FuncDecl&>(decl);
            result += "func " + d.ident;
            result += format_type_params(d.type_params);
            result += "(";
            for (size_t i = 0; i < d.params.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_pat(*d.params[i]);
            }
            result += ")";
            if (d.ret_type && d.ret_type->get_kind() != Type::Kind::Meta) {
                result += " -> " + format_type(*d.ret_type);
            }
            result += format_type_bounds(d.type_bounds);
            if (d.body.has_value()) {
                result += " " + format_expr(**d.body, indent);
            } else {
                result += ";";
            }
            break;
        }

        case Decl::Kind::Init: {
            const auto& d = static_cast<const InitDecl&>(decl);
            result += "init";
            if (!d.ident.empty()) {
                result += " " + d.ident;
            }
            result += format_type_params(d.type_params);
            result += "(";
            for (size_t i = 0; i < d.params.size(); ++i) {
                if (i > 0) {
                    result += ", ";
                }
                result += format_pat(*d.params[i]);
            }
            result += ")";
            if (d.ret_type && d.ret_type->get_kind() != Type::Kind::Meta) {
                result += " -> " + format_type(*d.ret_type);
            }
            result += format_type_bounds(d.type_bounds);
            if (d.body.has_value()) {
                result += " " + format_expr(**d.body, indent);
            } else {
                result += ";";
            }
            break;
        }
        case Decl::Kind::Ctor: {
            const auto& d = static_cast<const CtorDecl&>(decl);
            result += format_ctor(d);
            break;
        }
    }
    return result;
}

// Format Declaration
std::string format_package(const Package& pkg) {
    std::string result = "package \"" + pkg.ident + "\" {\n";
    for (const auto& import: pkg.header) {
        result += "import " + format_import(*import) + ";\n";
    }
    for (const auto& decl: pkg.body) {
        result += format_decl(*decl, 0) + "\n";
    }
    result += "}";
    return result;
}

} // namespace elaborate

// std::formatter specializations
std::format_context::iterator std::formatter<elaborate::Type>::format(
    const elaborate::Type& type,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_type(type), ctx);
}

std::format_context::iterator
std::formatter<elaborate::Lit>::format(const elaborate::Lit& lit, std::format_context& ctx) const {
    return std::formatter<std::string>::format(elaborate::format_lit(lit), ctx);
}

std::format_context::iterator
std::formatter<elaborate::Pat>::format(const elaborate::Pat& pat, std::format_context& ctx) const {
    return std::formatter<std::string>::format(elaborate::format_pat(pat), ctx);
}

std::format_context::iterator std::formatter<elaborate::Expr>::format(
    const elaborate::Expr& expr,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_expr(expr), ctx);
}

std::format_context::iterator std::formatter<elaborate::Stmt>::format(
    const elaborate::Stmt& stmt,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_stmt(stmt, 0), ctx);
}

std::format_context::iterator std::formatter<elaborate::Import>::format(
    const elaborate::Import& import,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_import(import), ctx);
}

std::format_context::iterator std::formatter<elaborate::Cond>::format(
    const elaborate::Cond& cond,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_cond(cond), ctx);
}

std::format_context::iterator std::formatter<elaborate::Clause>::format(
    const elaborate::Clause& clause,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_clause(clause, 0), ctx);
}

std::format_context::iterator std::formatter<elaborate::Decl>::format(
    const elaborate::Decl& decl,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_decl(decl, 0), ctx);
}

std::format_context::iterator std::formatter<elaborate::Package>::format(
    const elaborate::Package& pkg,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(elaborate::format_package(pkg), ctx);
}
