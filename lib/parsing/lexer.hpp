#pragma once

#include <cctype>
#include <cstddef>
#include <format>
#include <string>
#include <vector>

namespace parsing {

struct Location {
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Span {
    Location start;
    Location end;
};

struct Token {
    enum class Kind {
        Eof,        // EOF
        IntType,    // Int
        BoolType,   // Bool
        CharType,   // Char
        StringType, // String
        Int,        // 123
        True,       // true
        False,      // false
        Char,       // 'a'
        String,     // "abc"
        Wild,       // _
        Id,         // abc
        LParen,     // (
        RParen,     // )
        LBrack,     // [
        RBrack,     // ]
        LBrace,     // {
        RBrace,     // }
        Comma,      // ,
        Dot,        // .
        DotDot,     // ..
        Col,        // :
        ColCol,     // ::
        Semi,       // ;
        Pipe,       // |
        At,         // @
        Eq,         // =
        AddEq,      // +=
        SubEq,      // -=
        MulEq,      // *=
        DivEq,      // /=
        ModEq,      // %=
        RArrow,     // ->
        LArrow,     // <-
        FatArrow,   // =>
        Add,        // +
        Sub,        // -
        Mul,        // *
        Div,        // /
        Mod,        // %
        Amp,        // &
        And,        // &&
        Or,         // ||
        Not,        // !
        EqEq,       // ==
        Neq,        // !=
        Lt,         // <
        Gt,         // >
        Lte,        // <=
        Gte,        // >=
        Try,        // ?
        Private,    // private
        Protected,  // protected
        As,         // as
        Package,    // package
        Module,     // module
        Import,     // import
        Open,       // open
        Func,       // func
        Init,       // init
        Type,       // type
        Class,      // class
        Enum,       // enum
        Interface,  // interface
        Extension,  // extension
        Where,      // where
        Let,        // let
        Mut,        // mut
        If,         // if
        Else,       // else
        Switch,     // switch
        Case,       // case
        Default,    // default
        For,        // for
        In,         // in
        While,      // while
        Loop,       // loop
        Return,     // return
        Continue,   // continue
        Break,      // break
    };

    Token(Kind kind, Span span): kind(kind), span(span) {}

    Kind get_kind() const {
        return kind;
    }

    Span get_span() const {
        return span;
    }

    bool operator==(Kind k) const {
        return kind == k;
    }

    bool operator!=(Kind k) const {
        return kind != k;
    }

private:
    Kind kind;
    Span span;
};

class Lexer {
public:
    explicit Lexer(std::string input): input(std::move(input)) {}
    Lexer(const Lexer&) = default;

    Token peek();
    Token next();
    bool is_at_end() const;

    const std::string& get_lexeme() const {
        return state.lexeme;
    }

    int get_int_value() const {
        return state.int_value;
    }

    char get_char_value() const {
        return state.char_value;
    }

    void push_checkpoint() {
        checkpoints.push_back(state);
    }

    void pop_checkpoint() {
        checkpoints.pop_back();
    }

    void restore_checkpoint() {
        if (checkpoints.empty()) {
            throw std::runtime_error("No checkpoint to restore");
        }
        state = checkpoints.back();
        checkpoints.pop_back();
    }

private:
    std::string input;

    struct State {
        std::size_t pos = 0;
        std::size_t line = 1;
        std::size_t column = 1;
        std::string lexeme;
        int int_value = 0;
        char char_value = 0;
        bool has_token = false;
        Location token_start;
        Token current_token = Token(Token::Kind::Eof, {});
    };

    State state;
    std::vector<State> checkpoints;

    char curr_char() const;
    char next_char() const;
    char advance();
    void skip_whitespace();
    void skip_line_comment();
    void skip_block_comment();
    Token::Kind lex_identifier_or_keyword();
    Token::Kind lex_number();
    Token::Kind lex_char();
    Token::Kind lex_string();
};

std::string format_token_kind(Token::Kind kind);

} // namespace parsing

template<>
struct std::formatter<parsing::Span>: std::formatter<std::string> {
    std::format_context::iterator format(parsing::Span span, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Token::Kind>: std::formatter<std::string> {
    std::format_context::iterator format(parsing::Token::Kind kind, std::format_context& ctx) const;
};

template<>
struct std::formatter<parsing::Token>: std::formatter<std::string> {
    std::format_context::iterator format(parsing::Token tok, std::format_context& ctx) const;
};
