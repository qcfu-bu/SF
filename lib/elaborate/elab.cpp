#include <memory>
#include <optional>
#include <ranges>

#include "elab.hpp"

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

} // namespace elaborate