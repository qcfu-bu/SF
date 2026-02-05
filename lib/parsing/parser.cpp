#include <memory>
#include <ranges>
#include <vector>

#include "parser.hpp"
#include "parsing/lexer.hpp"
#include "parsing/syntax.hpp"

namespace parsing {

template<typename T>
std::vector<T> Parser::parse_sep(
    std::function<T()> fn,
    Token::Kind sep,
    bool allow_trailing,
    std::size_t min_size
) {
    std::vector<T> items;
    try {
        items.push_back(fn());
    } catch (...) {
        if (min_size > 0) {
            throw;
        }
        return items;
    }
    while (peek() == sep) {
        next();
        try {
            items.push_back(fn());
        } catch (...) {
            if (allow_trailing) {
                break;
            } else {
                throw;
            }
        }
    }
    if (items.size() < min_size) {
        throw std::runtime_error(
            std::format("Expected at least {} items, got {}", min_size, items.size())
        );
    }
    return items;
}

std::string Parser::parse_ident() {
    auto token = peek();
    if (token != Token::Kind::Id) {
        throw std::runtime_error(std::format("Expected identifier, got {}", token));
    }
    auto lexeme = lexer.get_lexeme();
    next();
    return lexeme;
}

std::vector<Name::Seg> Parser::parse_path() {
    std::vector<Name::Seg> path;
    while (peek() == Token::Kind::Dot) {
        next();
        auto token = peek();
        if (token == Token::Kind::Id) {
            path.emplace_back(parse_ident());
        } else if (token == Token::Kind::Int) {
            path.emplace_back(lexer.get_int_value());
            next();
        } else {
            throw std::runtime_error(
                std::format("Expected identifier or integer in path, got {}", token)
            );
        }
    }
    return path;
}

Name Parser::parse_name() {
    auto ident = parse_ident();
    auto path = parse_path();
    return Name(std::move(ident), std::move(path));
}

std::unique_ptr<Import> Parser::parse_import() {
    auto start = start_loc();
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::Id: {
            auto name = parse_ident();
            if (peek() == Token::Kind::Dot) {
                // nested import
                next(); // consume '.'
                if (peek() == Token::Kind::LBrace) {
                    // multiple nested imports
                    next(); // consume '{'
                    auto nested = parse_sep<std::unique_ptr<Import>>(
                        [this]() { return parse_import(); },
                        Token::Kind::Comma,
                        true,
                        1
                    );
                    expect(Token::Kind::RBrace); // consume '}'
                    return std::make_unique<NodeImport>(
                        std::move(name),
                        std::move(nested),
                        make_span(start)
                    );
                }
                // single nested import
                auto import = parse_import();
                return std::make_unique<NodeImport>(
                    std::move(name),
                    std::move(import),
                    make_span(start)
                );
            }
            if (peek() == Token::Kind::As) {
                // alias import
                next(); // consume 'as'
                auto token = peek();
                if (token == Token::Kind::Wild) {
                    next(); // consume '_'
                    return std::make_unique<AliasImport>(
                        std::move(name),
                        std::nullopt,
                        make_span(start)
                    );
                } else if (token == Token::Kind::Id) {
                    auto alias = parse_ident();
                    return std::make_unique<AliasImport>(
                        std::move(name),
                        std::move(alias),
                        make_span(start)
                    );
                } else {
                    throw std::runtime_error(
                        std::format(
                            "Expected identifier or '_' after 'as' in import, got {}",
                            token
                        )
                    );
                }
            }
            return std::make_unique<NodeImport>(
                std::move(name),
                std::vector<std::unique_ptr<Import>>(),
                make_span(start)
            );
        }
        case Token::Kind::Mul:
            next(); // consume '*'
            return std::make_unique<WildImport>(make_span(start));
        default:
            throw std::runtime_error(std::format("Unexpected token in import, got {}", token));
    }
}

std::optional<std::vector<std::unique_ptr<Type>>> Parser::parse_type_args() {
    std::optional<std::vector<std::unique_ptr<Type>>> type_args;

    if (peek() == Token::Kind::Lt) {
        lexer.push_checkpoint();
        next(); // consume '<'
        type_args = parse_sep<std::unique_ptr<Type>>(
            [this]() { return parse_type(); },
            Token::Kind::Comma,
            false
        );
        if (peek() == Token::Kind::Gt) {
            lexer.pop_checkpoint();
            next(); // consume '>'
        } else {
            lexer.restore_checkpoint();
            type_args.reset();
        }
    }

    return type_args;
}

std::unique_ptr<Type> Parser::parse_tuple_type() {
    auto start = start_loc();
    expect(Token::Kind::LParen); // consume '('
    auto types =
        parse_sep<std::unique_ptr<Type>>([this]() { return parse_type(); }, Token::Kind::Comma);
    expect(Token::Kind::RParen); // consume ')'
    if (types.empty()) {
        return std::make_unique<UnitType>(make_span(start));
    } else if (types.size() == 1) {
        return std::move(types[0]);
    } else {
        return std::make_unique<TupleType>(std::move(types), make_span(start));
    }
}

std::unique_ptr<Type> Parser::parse_type_basic() {
    auto start = start_loc();
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::IntType:
            next();
            return std::make_unique<IntType>(make_span(start));
        case Token::Kind::BoolType:
            next();
            return std::make_unique<BoolType>(make_span(start));
        case Token::Kind::CharType:
            next();
            return std::make_unique<CharType>(make_span(start));
        case Token::Kind::StringType:
            next();
            return std::make_unique<StringType>(make_span(start));
        case Token::Kind::Id: {
            auto name = parse_name();
            auto type_args = parse_type_args();
            return std::make_unique<NameType>(
                std::move(name),
                std::move(type_args),
                make_span(start)
            );
        }
        case Token::Kind::LParen:
            return parse_tuple_type();
        default:
            throw std::runtime_error(std::format("Unexpected token in type, got {}", token));
    }
}

std::unique_ptr<Type> Parser::parse_type() {
    auto start = start_loc();
    std::vector<std::unique_ptr<Type>> inputs;
    std::unique_ptr<Type> rhs = parse_type_basic();
    while (peek() == Token::Kind::RArrow) {
        next(); // consume '->'
        inputs.push_back(std::move(rhs));
        rhs = parse_type_basic();
    }
    for (auto& t: std::views::reverse(inputs)) {
        if (t->get_kind() == Type::Kind::Tuple) {
            rhs = std::make_unique<ArrowType>(
                std::move(static_cast<TupleType*>(t.get())->elems),
                std::move(rhs),
                make_span(start)
            );
        } else {
            rhs = std::make_unique<ArrowType>(std::move(t), std::move(rhs), make_span(start));
        }
    }
    return rhs;
}

std::unique_ptr<Type> Parser::parse_hint() {
    if (peek() == Token::Kind::Col) {
        next(); // consume ':'
        return parse_type();
    }
    return std::make_unique<MetaType>(Span {});
}

std::unique_ptr<Pat> Parser::parse_pat_basic(bool use_hint) {
    auto start = start_loc();
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::Int: {
            int value = lexer.get_int_value();
            next();
            auto span = make_span(start);
            return std::make_unique<LitPat>(std::make_unique<IntLit>(value, span), span);
        }
        case Token::Kind::True: {
            next();
            auto span = make_span(start);
            return std::make_unique<LitPat>(std::make_unique<BoolLit>(true, span), span);
        }
        case Token::Kind::False: {
            next();
            auto span = make_span(start);
            return std::make_unique<LitPat>(std::make_unique<BoolLit>(false, span), span);
        }
        case Token::Kind::Char: {
            char value = lexer.get_char_value();
            next();
            auto span = make_span(start);
            return std::make_unique<LitPat>(std::make_unique<CharLit>(value, span), span);
        }
        case Token::Kind::String: {
            std::string value = lexer.get_lexeme();
            next();
            auto span = make_span(start);
            return std::make_unique<LitPat>(std::make_unique<StringLit>(value, span), span);
        }
        case Token::Kind::Wild: {
            next();
            return std::make_unique<WildPat>(make_span(start));
        }
        case Token::Kind::Mut:
        case Token::Kind::Id: {
            bool is_mut = false;
            if (peek() == Token::Kind::Mut) {
                is_mut = true;
                next(); // consume 'mut'
            }
            auto name = parse_name();
            auto type_args = parse_type_args();

            if (peek() == Token::Kind::LParen) {
                // constructor pattern
                next(); // consume '('
                auto args = parse_sep<std::unique_ptr<Pat>>(
                    [this]() { return parse_pat(true); },
                    Token::Kind::Comma
                );
                expect(Token::Kind::RParen); // consume ')'
                return std::make_unique<CtorPat>(
                    std::move(name),
                    std::move(type_args),
                    std::move(args),
                    make_span(start)
                );
            }

            std::unique_ptr<Type> hint = std::make_unique<MetaType>(Span {});
            if (use_hint) {
                hint = parse_hint();
            }

            if (peek() == Token::Kind::At) {
                next(); // consume '@'
                if (type_args.has_value()) {
                    throw std::runtime_error("Type arguments not allowed in '@' pattern");
                }
                auto pat = parse_pat_basic();
                return std::make_unique<AtPat>(
                    std::move(name),
                    std::move(hint),
                    is_mut,
                    std::move(pat),
                    make_span(start)
                );
            }

            return std::make_unique<NamePat>(
                std::move(name),
                std::move(type_args),
                std::move(hint),
                is_mut,
                make_span(start)
            );
        }
        case Token::Kind::LParen:
            return parse_tuple_pat();
        default:
            throw std::runtime_error(std::format("Unexpected token in pattern, got {}", token));
    }
}

std::unique_ptr<Pat> Parser::parse_tuple_pat() {
    auto start = start_loc();
    expect(Token::Kind::LParen); // consume '('
    auto elems =
        parse_sep<std::unique_ptr<Pat>>([this]() { return parse_pat(true); }, Token::Kind::Comma);
    expect(Token::Kind::RParen); // consume ')'
    if (elems.empty()) {
        auto span = make_span(start);
        return std::make_unique<LitPat>(std::make_unique<UnitLit>(span), span);
    } else if (elems.size() == 1) {
        return std::move(elems[0]);
    } else {
        return std::make_unique<TuplePat>(std::move(elems), make_span(start));
    }
}

std::unique_ptr<Pat> Parser::parse_pat(bool use_hint) {
    auto start = start_loc();
    auto pats = parse_sep<std::unique_ptr<Pat>>(
        [this, use_hint]() { return parse_pat_basic(use_hint); },
        Token::Kind::Pipe,
        false,
        1
    );
    if (pats.size() == 1) {
        return std::move(pats[0]);
    } else {
        return std::make_unique<OrPat>(std::move(pats), make_span(start));
    }
}

std::unique_ptr<Expr> Parser::parse_tuple_expr() {
    auto start = start_loc();
    expect(Token::Kind::LParen); // consume '('
    auto elems = parse_sep<std::unique_ptr<Expr>>(
        [this]() -> std::unique_ptr<Expr> {
            auto elem_start = start_loc();
            auto expr = parse_expr();
            if (peek() == Token::Kind::Col) {
                auto hint = parse_hint();
                return std::make_unique<HintExpr>(
                    std::move(expr),
                    std::move(hint),
                    make_span(elem_start)
                );
            }
            return expr;
        },
        Token::Kind::Comma
    );
    expect(Token::Kind::RParen); // consume ')'
    if (elems.empty()) {
        auto span = make_span(start);
        return std::make_unique<LitExpr>(std::make_unique<UnitLit>(span), span);
    } else if (elems.size() == 1) {
        return std::move(elems[0]);
    } else {
        return std::make_unique<TupleExpr>(std::move(elems), make_span(start));
    }
}

std::unique_ptr<Expr> Parser::parse_lam_expr() {
    auto start = start_loc();
    auto pat = parse_pat_basic();
    std::vector<std::unique_ptr<Pat>> params;
    if (pat->get_kind() == Pat::Kind::Tuple) {
        params = std::move(static_cast<TuplePat*>(pat.get())->elems);
    } else {
        params.push_back(std::move(pat));
    }
    expect(Token::Kind::FatArrow); // consume '=>'
    auto body = parse_expr();
    return std::make_unique<LamExpr>(std::move(params), std::move(body), make_span(start));
}

std::unique_ptr<Cond> Parser::parse_cond() {
    auto start = start_loc();
    if (peek() == Token::Kind::Let) {
        next(); // consume 'let'
        auto pat = parse_pat();
        expect(Token::Kind::Eq); // consume '='
        auto expr = parse_expr();
        return std::make_unique<PatCond>(std::move(pat), std::move(expr), make_span(start));
    } else {
        auto expr = parse_expr();
        return std::make_unique<ExprCond>(std::move(expr), make_span(start));
    }
}

std::unique_ptr<Expr> Parser::parse_ite_expr() {
    auto start = start_loc();
    expect(Token::Kind::If); // consume 'if'
    std::vector<IteThen> then_branches;
    auto cond = parse_cond();
    auto then_branch = parse_block_expr();
    then_branches.push_back({ std::move(cond), std::move(then_branch) });

    // optional else branch
    std::optional<std::unique_ptr<Expr>> else_branch = std::nullopt;
    while (peek() == Token::Kind::Else) {
        next(); // consume 'else'
        if (peek() == Token::Kind::If) {
            next(); // consume 'if'
            auto cond = parse_cond();
            auto then_branch = parse_block_expr();
            then_branches.push_back({ std::move(cond), std::move(then_branch) });
            continue;
        }
        else_branch = parse_block_expr();
    }

    return std::make_unique<IteExpr>(
        std::move(then_branches),
        std::move(else_branch),
        make_span(start)
    );
}

std::unique_ptr<Clause> Parser::parse_clause() {
    auto start = start_loc();
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::Case: {
            next(); // consume 'case'
            auto pat = parse_pat(false);
            std::optional<std::unique_ptr<Expr>> guard = std::nullopt;
            if (peek() == Token::Kind::If) {
                next(); // consume 'if'
                guard = parse_expr();
            }
            expect(Token::Kind::Col); // consume ':'
            auto block_start = start_loc();
            std::vector<std::unique_ptr<Stmt>> stmts;
            while (peek() != Token::Kind::Case && peek() != Token::Kind::Default
                   && peek() != Token::Kind::RBrace)
            {
                stmts.push_back(parse_stmt());
            }
            auto expr = std::make_unique<BlockExpr>(std::move(stmts), make_span(block_start));
            return std::make_unique<CaseClause>(
                std::move(pat),
                std::move(guard),
                std::move(expr),
                make_span(start)
            );
        }
        case Token::Kind::Default: {
            next();                   // consume 'default'
            expect(Token::Kind::Col); // consume ':'
            auto block_start = start_loc();
            std::vector<std::unique_ptr<Stmt>> stmts;
            while (peek() != Token::Kind::Case && peek() != Token::Kind::Default
                   && peek() != Token::Kind::RBrace)
            {
                stmts.push_back(parse_stmt());
            }
            auto expr = std::make_unique<BlockExpr>(std::move(stmts), make_span(block_start));
            return std::make_unique<DefaultClause>(std::move(expr), make_span(start));
        }
        default:
            throw std::runtime_error(std::format("Unexpected token in clause, got {}", token));
    }
}

std::unique_ptr<Expr> Parser::parse_switch_expr() {
    auto start = start_loc();
    expect(Token::Kind::Switch); // consume 'switch'
    auto expr = parse_expr();

    expect(Token::Kind::LBrace); // consume '{'
    std::vector<std::unique_ptr<Clause>> clauses;
    while (peek() != Token::Kind::RBrace) {
        clauses.push_back(parse_clause());
    }
    expect(Token::Kind::RBrace); // consume '}'

    return std::make_unique<SwitchExpr>(std::move(expr), std::move(clauses), make_span(start));
}

std::unique_ptr<Expr> Parser::parse_for_expr() {
    auto start = start_loc();
    expect(Token::Kind::For); // consume 'for'
    auto pat = parse_pat_basic();
    expect(Token::Kind::In); // consume 'in'
    auto expr = parse_expr();
    auto body = parse_block_expr();
    return std::make_unique<ForExpr>(
        std::move(pat),
        std::move(expr),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<Expr> Parser::parse_while_expr() {
    auto start = start_loc();
    expect(Token::Kind::While); // consume 'while'
    auto cond = parse_cond();
    auto body = parse_block_expr();
    return std::make_unique<WhileExpr>(std::move(cond), std::move(body), make_span(start));
}

std::unique_ptr<Expr> Parser::parse_loop_expr() {
    auto start = start_loc();
    expect(Token::Kind::Loop); // consume 'loop'
    auto body = parse_block_expr();
    return std::make_unique<LoopExpr>(std::move(body), make_span(start));
}

std::unique_ptr<Expr> Parser::parse_block_expr() {
    auto start = start_loc();
    expect(Token::Kind::LBrace); // consume '{'
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (peek() != Token::Kind::RBrace) {
        stmts.push_back(parse_stmt());
    }
    expect(Token::Kind::RBrace); // consume '}'
    return std::make_unique<BlockExpr>(std::move(stmts), make_span(start));
}

std::unique_ptr<Expr> Parser::parse_expr0() {
    auto start = start_loc();
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::Int: {
            int value = lexer.get_int_value();
            next();
            auto span = make_span(start);
            return std::make_unique<LitExpr>(std::make_unique<IntLit>(value, span), span);
        }
        case Token::Kind::True: {
            next();
            auto span = make_span(start);
            return std::make_unique<LitExpr>(std::make_unique<BoolLit>(true, span), span);
        }
        case Token::Kind::False: {
            next();
            auto span = make_span(start);
            return std::make_unique<LitExpr>(std::make_unique<BoolLit>(false, span), span);
        }
        case Token::Kind::Char: {
            char value = lexer.get_char_value();
            next();
            auto span = make_span(start);
            return std::make_unique<LitExpr>(std::make_unique<CharLit>(value, span), span);
        }
        case Token::Kind::String: {
            std::string value = lexer.get_lexeme();
            next();
            auto span = make_span(start);
            return std::make_unique<LitExpr>(std::make_unique<StringLit>(value, span), span);
        }
        case Token::Kind::Id: {
            auto name = parse_name();
            auto type_args = parse_type_args();
            return std::make_unique<NameExpr>(
                std::move(name),
                std::move(type_args),
                make_span(start)
            );
        }
        case Token::Kind::Wild:
            next();
            return std::make_unique<HoleExpr>(make_span(start));
        case Token::Kind::LParen:
            return parse_tuple_expr();
        default:
            throw std::runtime_error(std::format("Unexpected token in expression, got {}", token));
    }
}

std::unique_ptr<Expr> Parser::parse_expr1() {
    auto start = start_loc();
    auto expr = parse_expr0();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::Try) {
            next();
            expr = std::make_unique<TryExpr>(std::move(expr), make_span(start));
        } else if (token == Token::Kind::Dot) {
            auto path = parse_path();
            auto type_args = parse_type_args();
            expr = std::make_unique<DotExpr>(
                std::move(expr),
                std::move(path),
                std::move(type_args),
                make_span(start)
            );
        } else if (token == Token::Kind::LBrack) {
            next(); // consume '['
            auto indices = parse_sep<std::unique_ptr<Expr>>(
                [this]() { return parse_expr(); },
                Token::Kind::Comma
            );
            expect(Token::Kind::RBrack); // consume ']'
            expr =
                std::make_unique<IndexExpr>(std::move(expr), std::move(indices), make_span(start));
        } else if (token == Token::Kind::LParen) {
            next(); // consume '('
            auto args = parse_sep<std::unique_ptr<Expr>>(
                [this]() { return parse_expr(); },
                Token::Kind::Comma
            );
            expect(Token::Kind::RParen); // consume ')'
            expr = std::make_unique<AppExpr>(std::move(expr), std::move(args), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr2() {
    auto start = start_loc();
    auto token = peek();
    if (token == Token::Kind::Add) {
        next();
        auto expr = parse_expr2();
        return std::make_unique<PosExpr>(std::move(expr), make_span(start));
    } else if (token == Token::Kind::Sub) {
        next();
        auto expr = parse_expr2();
        return std::make_unique<NegExpr>(std::move(expr), make_span(start));
    } else if (token == Token::Kind::Amp) {
        next();
        auto expr = parse_expr2();
        return std::make_unique<AddrExpr>(std::move(expr), make_span(start));
    } else if (token == Token::Kind::Mul) {
        next();
        auto expr = parse_expr2();
        return std::make_unique<DerefExpr>(std::move(expr), make_span(start));
    } else {
        return parse_expr1();
    }
}

std::unique_ptr<Expr> Parser::parse_expr3() {
    auto start = start_loc();
    auto expr = parse_expr2();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::Mul) {
            next();
            auto right = parse_expr2();
            expr = std::make_unique<MulExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Div) {
            next();
            auto right = parse_expr2();
            expr = std::make_unique<DivExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Mod) {
            next();
            auto right = parse_expr2();
            expr = std::make_unique<ModExpr>(std::move(expr), std::move(right), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr4() {
    auto start = start_loc();
    auto expr = parse_expr3();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::Add) {
            next();
            auto right = parse_expr3();
            expr = std::make_unique<AddExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Sub) {
            next();
            auto right = parse_expr3();
            expr = std::make_unique<SubExpr>(std::move(expr), std::move(right), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr5() {
    auto start = start_loc();
    auto expr = parse_expr4();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::Lt) {
            next();
            auto right = parse_expr4();
            expr = std::make_unique<LtExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Gt) {
            next();
            auto right = parse_expr4();
            expr = std::make_unique<GtExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Lte) {
            next();
            auto right = parse_expr4();
            expr = std::make_unique<LteExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Gte) {
            next();
            auto right = parse_expr4();
            expr = std::make_unique<GteExpr>(std::move(expr), std::move(right), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr6() {
    auto start = start_loc();
    auto expr = parse_expr5();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::EqEq) {
            next();
            auto right = parse_expr5();
            expr = std::make_unique<EqExpr>(std::move(expr), std::move(right), make_span(start));
        } else if (token == Token::Kind::Neq) {
            next();
            auto right = parse_expr5();
            expr = std::make_unique<NeqExpr>(std::move(expr), std::move(right), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr7() {
    auto start = start_loc();
    auto expr = parse_expr6();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::And) {
            next();
            auto right = parse_expr6();
            expr = std::make_unique<AndExpr>(std::move(expr), std::move(right), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr8() {
    auto start = start_loc();
    auto expr = parse_expr7();
    while (true) {
        auto token = peek();
        if (token == Token::Kind::Or) {
            next();
            auto right = parse_expr7();
            expr = std::make_unique<OrExpr>(std::move(expr), std::move(right), make_span(start));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_expr9() {
    auto start = start_loc();
    auto rhs = parse_expr8();
    std::vector<std::pair<BinaryExpr::Op, std::unique_ptr<Expr>>> exprs;
    while (true) {
        auto token = peek();
        if (Token::Kind::Eq <= token.get_kind() && token.get_kind() <= Token::Kind::ModEq) {
            BinaryExpr::Op mode;
            if (token == Token::Kind::Eq) {
                mode = BinaryExpr::Op::Assign;
            } else if (token == Token::Kind::AddEq) {
                mode = BinaryExpr::Op::Add;
            } else if (token == Token::Kind::SubEq) {
                mode = BinaryExpr::Op::Sub;
            } else if (token == Token::Kind::MulEq) {
                mode = BinaryExpr::Op::Mul;
            } else if (token == Token::Kind::DivEq) {
                mode = BinaryExpr::Op::Div;
            } else if (token == Token::Kind::ModEq) {
                mode = BinaryExpr::Op::Mod;
            } else {
                throw std::runtime_error(
                    std::format("Unexpected assignment operator, got {}", token)
                );
            }
            next();
            exprs.emplace_back(mode, std::move(rhs));
            rhs = parse_expr8();
        } else {
            break;
        }
    }
    for (auto& expr: std::views::reverse(exprs)) {
        rhs = std::make_unique<AssignExpr>(
            expr.first,
            std::move(expr.second),
            std::move(rhs),
            make_span(start)
        );
    }
    return rhs;
}

std::unique_ptr<Expr> Parser::parse_expr() {
    auto start = start_loc();
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::If:
            return parse_ite_expr();
        case Token::Kind::Switch:
            return parse_switch_expr();
        case Token::Kind::For:
            return parse_for_expr();
        case Token::Kind::While:
            return parse_while_expr();
        case Token::Kind::Loop:
            return parse_loop_expr();
        case Token::Kind::LBrace:
            return parse_block_expr();
        case Token::Kind::Break:
            next(); // consume 'break'
            return std::make_unique<BreakExpr>(make_span(start));
        case Token::Kind::Continue:
            next(); // consume 'continue'
            return std::make_unique<ContinueExpr>(make_span(start));
        case Token::Kind::Return: {
            next(); // consume 'return'
            std::optional<std::unique_ptr<Expr>> expr;
            try {
                expr = parse_expr();
            } catch (...) {
                expr = std::nullopt;
            }
            return std::make_unique<ReturnExpr>(std::move(expr), make_span(start));
        }
        default:
            lexer.push_checkpoint();
            try {
                auto expr = parse_lam_expr();
                lexer.pop_checkpoint();
                return expr;
            } catch (...) {
                lexer.restore_checkpoint();
            }
            return parse_expr9();
    }
}

std::unique_ptr<Stmt> Parser::parse_open_stmt() {
    auto start = start_loc();
    expect(Token::Kind::Open); // consume 'open'
    auto import = parse_import();
    expect(Token::Kind::Semi); // consume ';'
    return std::make_unique<OpenStmt>(std::move(import), make_span(start));
}

std::unique_ptr<Stmt> Parser::parse_let_stmt() {
    auto start = start_loc();
    expect(Token::Kind::Let); // consume 'let'

    auto pat = parse_pat_basic();
    std::unique_ptr<Stmt> stmt;

    auto token = peek();
    if (token == Token::Kind::Eq) {
        next(); // consume '='
        auto expr = parse_expr();
        std::optional<std::unique_ptr<Expr>> else_expr;
        if (peek() == Token::Kind::Else) {
            next(); // consume 'else'
            else_expr = parse_block_expr();
        }
        stmt = std::make_unique<LetStmt>(
            std::move(pat),
            std::move(expr),
            std::move(else_expr),
            make_span(start)
        );
    } else if (token == Token::Kind::LArrow) {
        next(); // consume '<-'
        auto expr = parse_expr();
        stmt = std::make_unique<BindStmt>(std::move(pat), std::move(expr), make_span(start));
    } else {
        throw std::runtime_error(
            std::format("Expected '=' or '<-' in let statement, got {}", token)
        );
    }

    expect(Token::Kind::Semi); // consume ';'

    return stmt;
}

std::unique_ptr<Stmt> Parser::parse_func_stmt() {
    auto start = start_loc();
    expect(Token::Kind::Func); // consume 'func'
    auto ident = parse_ident();
    expect(Token::Kind::LParen); // consume '('
    auto params =
        parse_sep<std::unique_ptr<Pat>>([this]() { return parse_pat(); }, Token::Kind::Comma);
    expect(Token::Kind::RParen); // consume ')'

    std::unique_ptr<Type> return_type = std::make_unique<MetaType>(Span {});
    if (peek() == Token::Kind::RArrow) {
        next(); // consume '->'
        return_type = parse_type();
    }

    auto body = parse_block_expr();
    return std::make_unique<FuncStmt>(
        std::move(ident),
        std::move(params),
        std::move(return_type),
        std::move(body),
        make_span(start)
    );
}

std::vector<std::unique_ptr<Expr>> Parser::parse_attrs() {
    std::vector<std::unique_ptr<Expr>> attrs;
    while (peek() == Token::Kind::At) {
        next(); // consume '@'
        auto attr = parse_expr();
        attrs.push_back(std::move(attr));
    }
    return attrs;
}

std::unique_ptr<Stmt> Parser::parse_stmt() {
    auto start = start_loc();
    auto attrs = parse_attrs();
    auto token = peek();
    std::unique_ptr<Stmt> stmt;
    switch (token.get_kind()) {
        case Token::Kind::Open:
            stmt = parse_open_stmt();
            break;
        case Token::Kind::Let:
            stmt = parse_let_stmt();
            break;
        case Token::Kind::Func:
            stmt = parse_func_stmt();
            break;
        default:
            auto expr = parse_expr();
            auto is_val = true;
            if (peek() == Token::Kind::Semi) {
                next(); // consume ';'
                is_val = false;
            }
            stmt = std::make_unique<ExprStmt>(std::move(expr), is_val, make_span(start));
            break;
    }
    stmt->attrs = std::move(attrs);
    return stmt;
}

Access Parser::parse_access() {
    auto token = peek();
    switch (token.get_kind()) {
        case Token::Kind::Private:
            next();
            return Access::Private;
        case Token::Kind::Protected:
            next();
            return Access::Protected;
        default:
            return Access::Public;
    }
}

std::vector<std::unique_ptr<Type>> Parser::parse_type_bound() {
    return parse_sep<std::unique_ptr<Type>>(
        [this]() { return parse_type(); },
        Token::Kind::Add,
        false,
        1
    );
}

std::string Parser::parse_type_param(std::vector<TypeBound>& bounds) {
    auto start = start_loc();
    auto ident = parse_ident();
    if (peek() == Token::Kind::Col) {
        next(); // consume ':'
        auto bound = parse_type_bound();
        auto name = Name(ident);
        auto name_type = std::make_unique<NameType>(name, std::nullopt, make_span(start));
        bounds.emplace_back(std::move(name_type), std::move(bound));
    }
    return ident;
}

std::optional<std::vector<std::string>> Parser::parse_type_params(std::vector<TypeBound>& bounds) {
    std::optional<std::vector<std::string>> type_params;

    if (peek() == Token::Kind::Lt) {
        next(); // consume '<'
        type_params =
            parse_sep<std::string>([&]() { return parse_type_param(bounds); }, Token::Kind::Comma);
        expect(Token::Kind::Gt); // consume '>'
    }

    return type_params;
}

std::vector<TypeBound> Parser::parse_where_bounds() {
    return parse_sep<TypeBound>(
        [this]() {
            auto type = parse_type();
            expect(Token::Kind::Col); // consume ':'
            auto bound = parse_type_bound();
            return TypeBound(std::move(type), std::move(bound));
        },
        Token::Kind::Comma,
        true
    );
}

std::unique_ptr<ModuleDecl> Parser::parse_module_decl() {
    auto start = start_loc();
    expect(Token::Kind::Module); // consume 'module'
    auto name = parse_ident();
    expect(Token::Kind::LBrace); // consume '{'

    std::vector<std::unique_ptr<Decl>> decls;
    while (peek() != Token::Kind::RBrace) {
        decls.push_back(parse_decl());
    }
    expect(Token::Kind::RBrace); // consume '}'

    return std::make_unique<ModuleDecl>(std::move(name), std::move(decls), make_span(start));
}

std::unique_ptr<OpenDecl> Parser::parse_open_decl() {
    auto start = start_loc();
    expect(Token::Kind::Open); // consume 'open'
    auto import = parse_import();
    expect(Token::Kind::Semi); // consume ';'

    return std::make_unique<OpenDecl>(std::move(import), make_span(start));
}

std::unique_ptr<ClassDecl> Parser::parse_class_decl() {
    auto start = start_loc();
    expect(Token::Kind::Class); // consume 'class'
    auto name = parse_ident();

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::vector<std::unique_ptr<Decl>> body;
    if (peek() == Token::Kind::LBrace) {
        next(); // consume '{'
        while (peek() != Token::Kind::RBrace) {
            body.push_back(parse_decl());
        }
        expect(Token::Kind::RBrace); // consume '}'
    } else {
        expect(Token::Kind::Semi); // consume ';'
    }

    return std::make_unique<ClassDecl>(
        std::move(name),
        std::move(type_params),
        std::move(type_bounds),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<EnumDecl> Parser::parse_enum_decl() {
    auto start = start_loc();
    expect(Token::Kind::Enum); // consume 'enum'
    auto name = parse_ident();

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::vector<std::unique_ptr<Decl>> body;
    if (peek() == Token::Kind::LBrace) {
        next(); // consume '{'
        while (peek() != Token::Kind::RBrace) {
            body.push_back(parse_decl());
        }
        expect(Token::Kind::RBrace); // consume '}'
    } else {
        expect(Token::Kind::Semi); // consume ';'
    }

    return std::make_unique<EnumDecl>(
        std::move(name),
        std::move(type_params),
        std::move(type_bounds),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<TypealiasDecl> Parser::parse_typealias_decl() {
    auto start = start_loc();
    expect(Token::Kind::Type); // consume 'type'
    auto name = parse_ident();

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    std::vector<std::unique_ptr<Type>> hint;
    if (peek() == Token::Kind::Col) {
        next(); // consume ':'
        hint = parse_type_bound();
    }

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::optional<std::unique_ptr<Type>> aliased = std::nullopt;
    if (peek() == Token::Kind::Eq) {
        next(); // consume '='
        aliased = parse_type();
    }
    expect(Token::Kind::Semi); // consume ';'

    return std::make_unique<TypealiasDecl>(
        std::move(name),
        std::move(type_params),
        std::move(type_bounds),
        std::move(hint),
        std::move(aliased),
        make_span(start)
    );
}

std::unique_ptr<InterfaceDecl> Parser::parse_interface_decl() {
    auto start = start_loc();
    expect(Token::Kind::Interface); // consume 'interface'
    auto name = parse_ident();

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::vector<std::unique_ptr<Decl>> body;
    if (peek() == Token::Kind::LBrace) {
        next(); // consume '{'
        while (peek() != Token::Kind::RBrace) {
            body.push_back(parse_decl());
        }
        expect(Token::Kind::RBrace); // consume '}'
    } else {
        expect(Token::Kind::Semi); // consume ';'
    }

    return std::make_unique<InterfaceDecl>(
        std::move(name),
        std::move(type_params),
        std::move(type_bounds),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<ExtensionDecl> Parser::parse_extension_decl() {
    auto start = start_loc();
    expect(Token::Kind::Extension); // consume 'extension'

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    auto base = parse_type();
    expect(Token::Kind::Col); // consume ':'
    auto ext = parse_type();

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::vector<std::unique_ptr<Decl>> body;
    if (peek() == Token::Kind::LBrace) {
        next(); // consume '{'
        while (peek() != Token::Kind::RBrace) {
            body.push_back(parse_decl());
        }
        expect(Token::Kind::RBrace); // consume '}'
    } else {
        expect(Token::Kind::Semi); // consume ';'
    }

    return std::make_unique<ExtensionDecl>(
        std::move(type_params),
        std::move(type_bounds),
        std::move(base),
        std::move(ext),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<LetDecl> Parser::parse_let_decl() {
    auto start = start_loc();
    expect(Token::Kind::Let); // consume 'let'
    auto pat = parse_pat_basic();

    std::optional<std::unique_ptr<Expr>> expr;
    if (peek() == Token::Kind::Eq) {
        next(); // consume '='
        expr = parse_expr();
    }
    expect(Token::Kind::Semi); // consume ';'

    return std::make_unique<LetDecl>(std::move(pat), std::move(expr), make_span(start));
}

std::unique_ptr<FuncDecl> Parser::parse_func_decl() {
    auto start = start_loc();
    expect(Token::Kind::Func); // consume 'func'
    auto ident = parse_ident();

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    expect(Token::Kind::LParen); // consume '('
    auto params =
        parse_sep<std::unique_ptr<Pat>>([this]() { return parse_pat(); }, Token::Kind::Comma);
    expect(Token::Kind::RParen); // consume ')'

    std::unique_ptr<Type> return_type = std::make_unique<MetaType>(Span {});
    if (peek() == Token::Kind::RArrow) {
        next(); // consume '->'
        return_type = parse_type();
    }

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::optional<std::unique_ptr<Expr>> body = std::nullopt;
    auto token = peek();
    if (token == Token::Kind::LBrace) {
        auto block = parse_block_expr();
        body = std::move(block);
    } else if (token == Token::Kind::Semi) {
        next(); // consume ';'
    } else {
        throw std::runtime_error(
            std::format("Expected function body or ';' in function declaration, got {}", token)
        );
    }

    return std::make_unique<FuncDecl>(
        std::move(ident),
        std::move(type_params),
        std::move(type_bounds),
        std::move(params),
        std::move(return_type),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<InitDecl> Parser::parse_init_decl() {
    auto start = start_loc();
    expect(Token::Kind::Init); // consume 'init'
    std::string ident;
    if (peek() == Token::Kind::Id) {
        ident = parse_ident();
    }

    std::vector<TypeBound> type_bounds;
    auto type_params = parse_type_params(type_bounds);

    expect(Token::Kind::LParen); // consume '('
    auto params =
        parse_sep<std::unique_ptr<Pat>>([this]() { return parse_pat(); }, Token::Kind::Comma);
    expect(Token::Kind::RParen); // consume ')'

    std::unique_ptr<Type> return_type = std::make_unique<MetaType>(Span {});
    if (peek() == Token::Kind::RArrow) {
        next(); // consume '->'
        return_type = parse_type();
    }

    if (peek() == Token::Kind::Where) {
        next(); // consume 'where'
        auto where_bounds = parse_where_bounds();
        type_bounds.insert(
            type_bounds.end(),
            std::make_move_iterator(where_bounds.begin()),
            std::make_move_iterator(where_bounds.end())
        );
    }

    std::optional<std::unique_ptr<Expr>> body = std::nullopt;
    auto token = peek();
    if (token == Token::Kind::LBrace) {
        auto block = parse_block_expr();
        body = std::move(block);
    } else if (token == Token::Kind::Semi) {
        next(); // consume ';'
    } else {
        throw std::runtime_error(
            std::format("Expected function body or ';' in function declaration, got {}", token)
        );
    }

    return std::make_unique<InitDecl>(
        std::move(ident),
        std::move(type_params),
        std::move(type_bounds),
        std::move(params),
        std::move(return_type),
        std::move(body),
        make_span(start)
    );
}

std::unique_ptr<CtorDecl> Parser::parse_ctor_decl() {
    auto start = start_loc();
    expect(Token::Kind::Case); // consume 'case'
    auto name = parse_ident();
    std::optional<std::vector<std::unique_ptr<Type>>> params;
    if (peek() == Token::Kind::LParen) {
        next(); // consume '('
        params =
            parse_sep<std::unique_ptr<Type>>([this]() { return parse_type(); }, Token::Kind::Comma);
        expect(Token::Kind::RParen); // consume ')'
    }
    return std::make_unique<CtorDecl>(std::move(name), std::move(params), make_span(start));
}

std::unique_ptr<Decl> Parser::parse_decl() {
    auto attrs = parse_attrs();
    auto access = parse_access();
    auto token = peek();
    std::unique_ptr<Decl> decl;
    switch (token.get_kind()) {
        case Token::Kind::Module:
            decl = parse_module_decl();
            break;
        case Token::Kind::Open:
            decl = parse_open_decl();
            break;
        case Token::Kind::Class:
            decl = parse_class_decl();
            break;
        case Token::Kind::Enum:
            decl = parse_enum_decl();
            break;
        case Token::Kind::Type:
            decl = parse_typealias_decl();
            break;
        case Token::Kind::Interface:
            decl = parse_interface_decl();
            break;
        case Token::Kind::Extension:
            decl = parse_extension_decl();
            break;
        case Token::Kind::Let:
            decl = parse_let_decl();
            break;
        case Token::Kind::Func:
            decl = parse_func_decl();
            break;
        case Token::Kind::Init:
            decl = parse_init_decl();
            break;
        case Token::Kind::Case:
            decl = parse_ctor_decl();
            break;
        default:
            throw std::runtime_error(std::format("Unexpected token in declaration: {}", token));
    }
    decl->attrs = std::move(attrs);
    decl->access = access;
    return decl;
}

Package Parser::parse_package() {
    auto start = start_loc();
    std::vector<std::unique_ptr<Import>> header;
    std::vector<std::unique_ptr<Decl>> body;
    while (peek() == Token::Kind::Import) {
        next(); // consume 'import'
        header.push_back(parse_import());
        expect(Token::Kind::Semi); // consume ';'
    }
    while (peek() != Token::Kind::Eof) {
        body.push_back(parse_decl());
    }
    done();
    return Package(pkg_name, std::move(header), std::move(body), make_span(start));
}

} // namespace parsing
