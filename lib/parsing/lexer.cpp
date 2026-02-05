#include <unordered_map>

#include "lexer.hpp"

namespace parsing {

static const std::unordered_map<std::string, Token::Kind> KEYWORDS = {
    { "Int", Token::Kind::IntType },
    { "Bool", Token::Kind::BoolType },
    { "Char", Token::Kind::CharType },
    { "String", Token::Kind::StringType },
    { "true", Token::Kind::True },
    { "false", Token::Kind::False },
    { "private", Token::Kind::Private },
    { "protected", Token::Kind::Protected },
    { "as", Token::Kind::As },
    { "package", Token::Kind::Package },
    { "module", Token::Kind::Module },
    { "import", Token::Kind::Import },
    { "open", Token::Kind::Open },
    { "init", Token::Kind::Init },
    { "func", Token::Kind::Func },
    { "type", Token::Kind::Type },
    { "class", Token::Kind::Class },
    { "enum", Token::Kind::Enum },
    { "interface", Token::Kind::Interface },
    { "extension", Token::Kind::Extension },
    { "where", Token::Kind::Where },
    { "let", Token::Kind::Let },
    { "mut", Token::Kind::Mut },
    { "if", Token::Kind::If },
    { "else", Token::Kind::Else },
    { "switch", Token::Kind::Switch },
    { "case", Token::Kind::Case },
    { "default", Token::Kind::Default },
    { "for", Token::Kind::For },
    { "in", Token::Kind::In },
    { "while", Token::Kind::While },
    { "loop", Token::Kind::Loop },
    { "return", Token::Kind::Return },
    { "continue", Token::Kind::Continue },
    { "break", Token::Kind::Break },
};

char Lexer::curr_char() const {
    if (is_at_end()) {
        return '\0';
    }
    return input[state.pos];
}

char Lexer::next_char() const {
    if (state.pos + 1 >= input.size()) {
        return '\0';
    }
    return input[state.pos + 1];
}

char Lexer::advance() {
    char c = input[state.pos++];
    if (c == '\n') {
        state.line++;
        state.column = 1;
    } else {
        state.column++;
    }
    return c;
}

bool Lexer::is_at_end() const {
    return state.pos >= input.size();
}

void Lexer::skip_whitespace() {
    while (!is_at_end()) {
        char c = curr_char();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else if (c == '/' && next_char() == '/') {
            skip_line_comment();
        } else if (c == '/' && next_char() == '*') {
            skip_block_comment();
        } else {
            break;
        }
    }
}

void Lexer::skip_line_comment() {
    advance(); // skip first /
    advance(); // skip second /
    while (!is_at_end() && curr_char() != '\n') {
        advance();
    }
}

void Lexer::skip_block_comment() {
    advance(); // skip /
    advance(); // skip *
    while (!is_at_end()) {
        if (curr_char() == '*' && next_char() == '/') {
            advance(); // skip *
            advance(); // skip /
            return;
        }
        advance();
    }
    throw std::runtime_error("Unterminated block comment");
}

Token::Kind Lexer::lex_identifier_or_keyword() {
    std::size_t start = state.pos;
    while (!is_at_end() && (std::isalnum(curr_char()) || curr_char() == '_')) {
        advance();
    }
    state.lexeme = input.substr(start, state.pos - start);
    // Check for wildcard
    if (state.lexeme == "_") {
        return Token::Kind::Wild;
    }

    // Check for keyword
    auto it = KEYWORDS.find(state.lexeme);
    if (it != KEYWORDS.end()) {
        return it->second;
    }

    return Token::Kind::Id;
}

Token::Kind Lexer::lex_number() {
    std::size_t start = state.pos;
    while (!is_at_end() && std::isdigit(curr_char())) {
        advance();
    }
    state.lexeme = input.substr(start, state.pos - start);
    state.int_value = std::stoi(state.lexeme);
    return Token::Kind::Int;
}

Token::Kind Lexer::lex_char() {
    advance(); // skip opening '
    if (is_at_end()) {
        throw std::runtime_error("Unterminated character literal");
    }

    char c = advance();
    if (c == '\\') {
        // Handle escape sequences
        if (is_at_end()) {
            throw std::runtime_error("Unterminated character literal");
        }
        char escaped = advance();
        switch (escaped) {
            case 'n':
                state.char_value = '\n';
                break;
            case 't':
                state.char_value = '\t';
                break;
            case 'r':
                state.char_value = '\r';
                break;
            case '\\':
                state.char_value = '\\';
                break;
            case '\'':
                state.char_value = '\'';
                break;
            case '0':
                state.char_value = '\0';
                break;
            default:
                throw std::runtime_error("Unknown escape sequence");
        }
    } else {
        state.char_value = c;
    }

    if (is_at_end() || curr_char() != '\'') {
        throw std::runtime_error("Unterminated character literal");
    }
    advance(); // skip closing '
    return Token::Kind::Char;
}

Token::Kind Lexer::lex_string() {
    advance(); // skip opening "
    std::string result;
    while (!is_at_end() && curr_char() != '"') {
        char c = advance();
        if (c == '\\') {
            if (is_at_end()) {
                throw std::runtime_error("Unterminated string literal");
            }
            char escaped = advance();
            switch (escaped) {
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '"':
                    result += '"';
                    break;
                case '0':
                    result += '\0';
                    break;
                default:
                    throw std::runtime_error("Unknown escape sequence");
            }
        } else {
            result += c;
        }
    }
    if (is_at_end()) {
        throw std::runtime_error("Unterminated string literal");
    }
    advance(); // skip closing "
    state.lexeme = result;
    return Token::Kind::String;
}

Token Lexer::peek() {
    if (state.has_token) {
        return state.current_token;
    }
    state.current_token = next();
    state.has_token = true;
    return state.current_token;
}

Token Lexer::next() {
    if (state.has_token) {
        state.has_token = false;
        return state.current_token;
    }

    skip_whitespace();

    state.token_start = { state.line, state.column };

    auto make_token = [this](Token::Kind kind) {
        Location end = { state.line, state.column };
        return Token(kind, { state.token_start, end });
    };

    if (is_at_end()) {
        return make_token(Token::Kind::Eof);
    }

    char c = curr_char();

    // Identifiers and keywords
    if (std::isalpha(c) || c == '_') {
        return make_token(lex_identifier_or_keyword());
    }

    // Numbers
    if (std::isdigit(c)) {
        return make_token(lex_number());
    }

    // Character literals
    if (c == '\'') {
        return make_token(lex_char());
    }

    // String literals
    if (c == '"') {
        return make_token(lex_string());
    }

    // Single and multi-character tokens
    advance();
    switch (c) {
        case '(':
            return make_token(Token::Kind::LParen);
        case ')':
            return make_token(Token::Kind::RParen);
        case '[':
            return make_token(Token::Kind::LBrack);
        case ']':
            return make_token(Token::Kind::RBrack);
        case '{':
            return make_token(Token::Kind::LBrace);
        case '}':
            return make_token(Token::Kind::RBrace);
        case ',':
            return make_token(Token::Kind::Comma);
        case ';':
            return make_token(Token::Kind::Semi);
        case '@':
            return make_token(Token::Kind::At);
        case '?':
            return make_token(Token::Kind::Try);

        case '.':
            if (curr_char() == '.') {
                advance();
                return make_token(Token::Kind::DotDot);
            }
            return make_token(Token::Kind::Dot);

        case ':':
            if (curr_char() == ':') {
                advance();
                return make_token(Token::Kind::ColCol);
            }
            return make_token(Token::Kind::Col);

        case '|':
            if (curr_char() == '|') {
                advance();
                return make_token(Token::Kind::Or);
            }
            return make_token(Token::Kind::Pipe);

        case '&':
            if (curr_char() == '&') {
                advance();
                return make_token(Token::Kind::And);
            }
            return make_token(Token::Kind::Amp);

        case '=':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::EqEq);
            }
            if (curr_char() == '>') {
                advance();
                return make_token(Token::Kind::FatArrow);
            }
            return make_token(Token::Kind::Eq);

        case '!':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::Neq);
            }
            return make_token(Token::Kind::Not);

        case '<':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::Lte);
            }
            if (curr_char() == '-') {
                advance();
                return make_token(Token::Kind::LArrow);
            }
            return make_token(Token::Kind::Lt);

        case '>':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::Gte);
            }
            return make_token(Token::Kind::Gt);

        case '+':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::AddEq);
            }
            return make_token(Token::Kind::Add);

        case '-':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::SubEq);
            }
            if (curr_char() == '>') {
                advance();
                return make_token(Token::Kind::RArrow);
            }
            return make_token(Token::Kind::Sub);

        case '*':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::MulEq);
            }
            return make_token(Token::Kind::Mul);

        case '/':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::DivEq);
            }
            return make_token(Token::Kind::Div);

        case '%':
            if (curr_char() == '=') {
                advance();
                return make_token(Token::Kind::ModEq);
            }
            return make_token(Token::Kind::Mod);

        default:
            throw std::runtime_error(std::string("Unexpected character: ") + c);
    }
}

std::string format_token_kind(Token::Kind kind) {
    switch (kind) {
        case Token::Kind::Eof:
            return "EOF";
        case Token::Kind::IntType:
            return "Int";
        case Token::Kind::BoolType:
            return "Bool";
        case Token::Kind::CharType:
            return "Char";
        case Token::Kind::StringType:
            return "String";
        case Token::Kind::Int:
            return "<int>";
        case Token::Kind::True:
            return "true";
        case Token::Kind::False:
            return "false";
        case Token::Kind::Char:
            return "<char>";
        case Token::Kind::String:
            return "<string>";
        case Token::Kind::Wild:
            return "_";
        case Token::Kind::Id:
            return "<id>";
        case Token::Kind::LParen:
            return "(";
        case Token::Kind::RParen:
            return ")";
        case Token::Kind::LBrack:
            return "[";
        case Token::Kind::RBrack:
            return "]";
        case Token::Kind::LBrace:
            return "{";
        case Token::Kind::RBrace:
            return "}";
        case Token::Kind::Comma:
            return ",";
        case Token::Kind::Dot:
            return ".";
        case Token::Kind::DotDot:
            return "..";
        case Token::Kind::Col:
            return ":";
        case Token::Kind::ColCol:
            return "::";
        case Token::Kind::Semi:
            return ";";
        case Token::Kind::Pipe:
            return "|";
        case Token::Kind::At:
            return "@";
        case Token::Kind::Eq:
            return "=";
        case Token::Kind::AddEq:
            return "+=";
        case Token::Kind::SubEq:
            return "-=";
        case Token::Kind::MulEq:
            return "*=";
        case Token::Kind::DivEq:
            return "/=";
        case Token::Kind::ModEq:
            return "%=";
        case Token::Kind::RArrow:
            return "->";
        case Token::Kind::LArrow:
            return "<-";
        case Token::Kind::FatArrow:
            return "=>";
        case Token::Kind::Add:
            return "+";
        case Token::Kind::Sub:
            return "-";
        case Token::Kind::Mul:
            return "*";
        case Token::Kind::Div:
            return "/";
        case Token::Kind::Mod:
            return "%";
        case Token::Kind::Amp:
            return "&";
        case Token::Kind::And:
            return "&&";
        case Token::Kind::Or:
            return "||";
        case Token::Kind::Not:
            return "!";
        case Token::Kind::EqEq:
            return "==";
        case Token::Kind::Neq:
            return "!=";
        case Token::Kind::Lt:
            return "<";
        case Token::Kind::Gt:
            return ">";
        case Token::Kind::Lte:
            return "<=";
        case Token::Kind::Gte:
            return ">=";
        case Token::Kind::Try:
            return "?";
        case Token::Kind::Private:
            return "private";
        case Token::Kind::Protected:
            return "protected";
        case Token::Kind::As:
            return "as";
        case Token::Kind::Package:
            return "package";
        case Token::Kind::Module:
            return "module";
        case Token::Kind::Import:
            return "import";
        case Token::Kind::Open:
            return "open";
        case Token::Kind::Init:
            return "init";
        case Token::Kind::Func:
            return "func";
        case Token::Kind::Type:
            return "type";
        case Token::Kind::Class:
            return "class";
        case Token::Kind::Enum:
            return "enum";
        case Token::Kind::Interface:
            return "interface";
        case Token::Kind::Extension:
            return "extension";
        case Token::Kind::Where:
            return "where";
        case Token::Kind::Let:
            return "let";
        case Token::Kind::Mut:
            return "mut";
        case Token::Kind::If:
            return "if";
        case Token::Kind::Else:
            return "else";
        case Token::Kind::Switch:
            return "switch";
        case Token::Kind::Case:
            return "case";
        case Token::Kind::Default:
            return "default";
        case Token::Kind::For:
            return "for";
        case Token::Kind::In:
            return "in";
        case Token::Kind::While:
            return "while";
        case Token::Kind::Loop:
            return "loop";
        case Token::Kind::Return:
            return "return";
        case Token::Kind::Continue:
            return "continue";
        case Token::Kind::Break:
            return "break";
    }
    return "UNKNOWN";
}

} // namespace parsing

std::format_context::iterator
std::formatter<parsing::Span>::format(parsing::Span span, std::format_context& ctx) const {
    auto str = std::format(
        "{}:{}-{}:{}",
        span.start.line,
        span.start.column,
        span.end.line,
        span.end.column
    );
    return std::formatter<std::string>::format(str, ctx);
}

std::format_context::iterator std::formatter<parsing::Token::Kind>::format(
    parsing::Token::Kind kind,
    std::format_context& ctx
) const {
    return std::formatter<std::string>::format(parsing::format_token_kind(kind), ctx);
}

std::format_context::iterator
std::formatter<parsing::Token>::format(parsing::Token tok, std::format_context& ctx) const {
    auto str = std::format("{}@{}", tok.get_kind(), tok.get_span());
    return std::formatter<std::string>::format(str, ctx);
}
