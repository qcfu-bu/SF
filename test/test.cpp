#include "catch2/catch_test_macros.hpp"
#include "elaborate/table.hpp"
#include "parsing/lexer.hpp"

TEST_CASE("test token formatter") {
    parsing::Span span { { 1, 2 }, { 3, 4 } };
    parsing::Token token(parsing::Token::Kind::Id, span);
    std::string formatted = std::format("{}", token);
    REQUIRE(formatted == "<id>@1:2-3:4");
}

TEST_CASE("test table node addition and lookup") {
    elaborate::Table table("root");
    table.add_node("module1", elaborate::TableNode::Kind::Module);
    auto* node = table.get_active()->find_node("module1");
    REQUIRE(node->get_kind() == elaborate::TableNode::Kind::Module);
}

TEST_CASE("test table type symbol lookup") {
    elaborate::Table table("root");
    table.add_type_symbol("MyClass", elaborate::Symbol(elaborate::Symbol::Kind::Class));
    auto symbol = table.find_type_symbol("MyClass", {});
    REQUIRE(symbol.get_kind() == elaborate::Symbol::Kind::Class);
}

TEST_CASE("test table type symbol lookup with path") {
    elaborate::Table table("root");
    table.add_node("module1", elaborate::TableNode::Kind::Module);
    table.enter_node("module1");
    table.add_type_symbol("MyEnum", elaborate::Symbol(elaborate::Symbol::Kind::Enum));
    table.exit_node();
    auto symbol = table.find_type_symbol("module1", { "MyEnum" });
    REQUIRE(symbol.get_kind() == elaborate::Symbol::Kind::Enum);
}