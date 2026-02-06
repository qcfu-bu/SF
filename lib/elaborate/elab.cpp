#include <memory>
#include <optional>
#include <ranges>

#include "elab.hpp"
#include "elaborate/table.hpp"
#include "parsing/syntax.hpp"

namespace elaborate {

void Context::push_scope() {
    scopes.emplace_back();
}

void Context::pop_scope() {
    if (scopes.empty()) {
        throw std::runtime_error("No scope to pop");
    }
    scopes.pop_back();
}

void Context::add_expr_var(const std::string& ident, std::shared_ptr<Type> type) {
    if (scopes.empty()) {
        throw std::runtime_error("No scope to add variable to");
    }
    scopes.back().expr_vars[ident] = std::move(type);
}

void Context::add_type_var(const std::string& ident) {
    if (scopes.empty()) {
        throw std::runtime_error("No scope to add type variable to");
    }
    scopes.back().type_vars.insert(ident);
}

bool Context::has_type_var(const std::string& ident) const {
    for (const auto& scope: scopes) {
        if (scope.type_vars.contains(ident)) {
            return true;
        }
    }
    return false;
}

std::optional<std::shared_ptr<Type>> Context::find_expr_var(const std::string& ident) {
    for (auto& scope: std::views::reverse(scopes)) {
        auto it = scope.expr_vars.find(ident);
        if (it != scope.expr_vars.end()) {
            return it->second;
        }
    }
    return std::nullopt;
}

void Context::pat_add_vars(const elaborate::Pat& pat) {
    switch (pat.get_kind()) {
        case Pat::Kind::Var: {
            const auto& var_pat = static_cast<const elaborate::VarPat&>(pat);
            add_expr_var(var_pat.ident, var_pat.hint);
            break;
        }
        case Pat::Kind::Tuple: {
            const auto& tuple_pat = static_cast<const elaborate::TuplePat&>(pat);
            for (const auto& elem: tuple_pat.elems) {
                pat_add_vars(*elem);
            }
            break;
        }
        case Pat::Kind::Ctor: {
            const auto& ctor_pat = static_cast<const elaborate::CtorPat&>(pat);
            if (ctor_pat.args.has_value()) {
                for (const auto& arg: *ctor_pat.args) {
                    pat_add_vars(*arg);
                }
            }
            break;
        }
        case Pat::Kind::Or: {
            const auto& or_pat = static_cast<const elaborate::OrPat&>(pat);
            for (const auto& option: or_pat.options) {
                pat_add_vars(*option);
            }
            break;
        }
        case Pat::Kind::At: {
            const auto& at_pat = static_cast<const elaborate::AtPat&>(pat);
            add_expr_var(at_pat.ident, at_pat.hint);
            pat_add_vars(*at_pat.pat);
            break;
        }
        default:
            break;
    }
}

std::shared_ptr<Type> Elaborator::elab_type(parsing::Type& type) {
    switch (type.get_kind()) {
        case parsing::Type::Kind::Meta:
            return std::make_shared<MetaType>(type.get_span());
        case parsing::Type::Kind::Int:
            return std::make_shared<IntType>(type.get_span());
        case parsing::Type::Kind::Bool:
            return std::make_shared<BoolType>(type.get_span());
        case parsing::Type::Kind::Char:
            return std::make_shared<CharType>(type.get_span());
        case parsing::Type::Kind::String:
            return std::make_shared<StringType>(type.get_span());
        case parsing::Type::Kind::Unit:
            return std::make_shared<UnitType>(type.get_span());
        case parsing::Type::Kind::Name: {
            auto& name_type = static_cast<parsing::NameType&>(type);
            auto [path, rest] = name_type.name.slice();
            if (!rest.empty()) {
                throw std::runtime_error(std::format("Invalid type: {}", name_type.name));
            }
            if (path.empty() && !name_type.type_args && ctx.has_type_var(name_type.name.ident)) {
                // type variable
                return std::make_shared<VarType>(name_type.name.ident, type.get_span());
            }
            // otherwise, resolve as type constant
            auto symbol = table.find_type_symbol(name_type.name.ident, path);
            std::optional<std::vector<std::shared_ptr<Type>>> type_args;
            if (name_type.type_args.has_value()) {
                type_args = std::vector<std::shared_ptr<Type>> {};
                for (auto& arg: *name_type.type_args) {
                    type_args->push_back(elab_type(*arg));
                }
            }
            if (symbol.get_kind() == Symbol::Kind::Enum) {
                return std::make_shared<EnumType>(name_type.name.ident, type_args, type.get_span());
            } else if (symbol.get_kind() == Symbol::Kind::Class) {
                return std::make_shared<ClassType>(
                    name_type.name.ident,
                    type_args,
                    type.get_span()
                );
            } else if (symbol.get_kind() == Symbol::Kind::Typealias) {
                return std::make_shared<TypealiasType>(
                    name_type.name.ident,
                    type_args,
                    type.get_span()
                );
            } else if (symbol.get_kind() == Symbol::Kind::Interface) {
                return std::make_shared<InterfaceType>(
                    name_type.name.ident,
                    type_args,
                    type.get_span()
                );
            } else {
                throw std::runtime_error(std::format("Invalid type: {}", name_type.name));
            }
        }
        case parsing::Type::Kind::Tuple: {
            auto& tuple_type = static_cast<parsing::TupleType&>(type);
            std::vector<std::shared_ptr<Type>> elem_types;
            for (auto& elem: tuple_type.elems) {
                elem_types.push_back(elab_type(*elem));
            }
            return std::make_shared<TupleType>(std::move(elem_types), type.get_span());
        }
        case parsing::Type::Kind::Arrow: {
            auto& arrow_type = static_cast<parsing::ArrowType&>(type);
            std::vector<std::shared_ptr<Type>> inputs;
            for (auto& input: arrow_type.inputs) {
                inputs.push_back(elab_type(*input));
            }
            auto output = elab_type(*arrow_type.output);
            return std::make_shared<ArrowType>(
                std::move(inputs),
                std::move(output),
                type.get_span()
            );
        }
    }
}

std::shared_ptr<Lit> Elaborator::elab_lit(parsing::Lit& lit) {
    switch (lit.get_kind()) {
        case parsing::Lit::Kind::Unit:
            return std::make_shared<UnitLit>(lit.get_span());
        case parsing::Lit::Kind::Int: {
            auto& int_lit = static_cast<parsing::IntLit&>(lit);
            return std::make_shared<IntLit>(int_lit.value, lit.get_span());
        }
        case parsing::Lit::Kind::Bool: {
            auto& bool_lit = static_cast<parsing::BoolLit&>(lit);
            return std::make_shared<BoolLit>(bool_lit.value, lit.get_span());
        }
        case parsing::Lit::Kind::Char: {
            auto& char_lit = static_cast<parsing::CharLit&>(lit);
            return std::make_shared<CharLit>(char_lit.value, lit.get_span());
        }
        case parsing::Lit::Kind::String: {
            auto& string_lit = static_cast<parsing::StringLit&>(lit);
            return std::make_shared<StringLit>(string_lit.value, lit.get_span());
        }
    }
}

std::shared_ptr<Pat> Elaborator::elab_pat(parsing::Pat& pat) {
    switch (pat.get_kind()) {
        case parsing::Pat::Kind::Lit: {
            auto& lit_pat = static_cast<parsing::LitPat&>(pat);
            auto lit = elab_lit(*lit_pat.literal);
            return std::make_shared<LitPat>(std::move(lit), pat.get_span());
        }
        case parsing::Pat::Kind::Tuple: {
            auto& tuple_pat = static_cast<parsing::TuplePat&>(pat);
            std::vector<std::shared_ptr<Pat>> elems;
            for (auto& elem: tuple_pat.elems) {
                elems.push_back(elab_pat(*elem));
            }
            return std::make_shared<TuplePat>(std::move(elems), pat.get_span());
        }
        case parsing::Pat::Kind::Ctor: {
            auto& ctor_pat = static_cast<parsing::CtorPat&>(pat);
            std::optional<std::vector<std::shared_ptr<Type>>> type_args;
            auto [path, rest] = ctor_pat.name.slice();
            if (!rest.empty()) {
                throw std::runtime_error(
                    std::format("Invalid constructor pattern: {}", ctor_pat.name)
                );
            }
            auto symbol = table.find_expr_symbol(ctor_pat.name.ident, path);
            if (symbol.get_kind() != Symbol::Kind::Ctor) {
                throw std::runtime_error(
                    std::format("Invalid constructor pattern: {}", ctor_pat.name)
                );
            }
            if (ctor_pat.type_args.has_value()) {
                type_args = std::vector<std::shared_ptr<Type>> {};
                for (auto& arg: *ctor_pat.type_args) {
                    type_args->push_back(elab_type(*arg));
                }
            }
            std::optional<std::vector<std::shared_ptr<Pat>>> args;
            if (ctor_pat.args.has_value()) {
                args = std::vector<std::shared_ptr<Pat>> {};
                for (auto& arg: *ctor_pat.args) {
                    args->push_back(elab_pat(*arg));
                }
            }
            return std::make_shared<CtorPat>(
                symbol.get_path(),
                std::move(type_args),
                std::move(args),
                pat.get_span()
            );
        }
        case parsing::Pat::Kind::Name: {
            auto& name_pat = static_cast<parsing::NamePat&>(pat);
            auto hint = elab_type(*name_pat.hint);
            return std::make_shared<VarPat>(
                name_pat.name.ident,
                std::move(hint),
                name_pat.is_mut,
                pat.get_span()
            );
        }
        case parsing::Pat::Kind::Wild: {
            auto& wild_pat = static_cast<parsing::WildPat&>(pat);
            return std::make_shared<WildPat>(pat.get_span());
        }
        case parsing::Pat::Kind::Or: {
            auto& or_pat = static_cast<parsing::OrPat&>(pat);
            std::vector<std::shared_ptr<Pat>> options;
            for (auto& option: or_pat.options) {
                options.push_back(elab_pat(*option));
            }
            return std::make_shared<OrPat>(std::move(options), pat.get_span());
        }
        case parsing::Pat::Kind::At: {
            auto& at_pat = static_cast<parsing::AtPat&>(pat);
            auto hint = elab_type(*at_pat.hint);
            auto sub_pat = elab_pat(*at_pat.pat);
            if (!at_pat.name.path.empty()) {
                throw std::runtime_error(
                    std::format("Invalid @pattern variable name: {}", at_pat.name)
                );
            }
            return std::make_shared<AtPat>(
                at_pat.name.ident,
                std::move(hint),
                at_pat.is_mut,
                std::move(sub_pat),
                pat.get_span()
            );
        }
    }
}

std::shared_ptr<Cond> Elaborator::elab_cond(parsing::Cond& expr) {
    switch (expr.get_kind()) {
        case parsing::Cond::Kind::Expr: {
            auto& expr_cond = static_cast<parsing::ExprCond&>(expr);
            auto elab_expr_ptr = elab_expr(*expr_cond.expr);
            return std::make_shared<ExprCond>(std::move(elab_expr_ptr), expr.get_span());
        }
        case parsing::Cond::Kind::Case: {
            auto& pat_cond = static_cast<parsing::PatCond&>(expr);
            auto elab_pat_ptr = elab_pat(*pat_cond.pat);
            auto elab_expr_ptr = elab_expr(*pat_cond.expr);
            return std::make_shared<PatCond>(
                std::move(elab_pat_ptr),
                std::move(elab_expr_ptr),
                expr.get_span()
            );
        }
    }
}

} // namespace elaborate
