#include "randomcat/complex_parsing/token.hpp"

#include <string>
#include <string_view>

namespace randomcat::complex_parsing {
    std::string token::name(token _tok) noexcept {
        using namespace std::string_literals;

        switch (_tok.kind()) {
            case token_kind::invalid_token: {
                return "invalid_token"s;
            }

            case token_kind::whitespace: {
                return "whitespace"s;
                break;
            }

            case token_kind::newline: {
                return "newline"s;
                break;
            }

            case token_kind::identifier: {
                return "identifier: " + identifier_value(_tok);
                break;
            }

            case token_kind::lparen: {
                return "lparen"s;
                break;
            }

            case token_kind::rparen: {
                return "rparen"s;
                break;
            }

            case token_kind::lbrace: {
                return "lbrace"s;
                break;
            }

            case token_kind::rbrace: {
                return "rbrace"s;
                break;
            }

            case token_kind::semicolon: {
                return "semicolon"s;
                break;
            }

            case token_kind::colon: {
                return "colon"s;
                break;
            }

            case token_kind::colon_colon: {
                return "colon_colon"s;
                break;
            }

            case token_kind::plus: {
                return "plus"s;
                break;
            }

            case token_kind::plus_plus: {
                return "plus_plus"s;
                break;
            }

            case token_kind::minus: {
                return "minus"s;
                break;
            }

            case token_kind::minus_minus: {
                return "minus_minus"s;
                break;
            }

            case token_kind::star: {
                return "star"s;
                break;
            }

            case token_kind::slash: {
                return "slash"s;
                break;
            }

            case token_kind::slash_slash: {
                return "slash_slash"s;
                break;
            }

            case token_kind::ampersand: {
                return "ampersand"s;
                break;
            }

            case token_kind::ampersand_ampersand: {
                return "ampersand_ampersand"s;
                break;
            }

            case token_kind::pipe: {
                return "pipe"s;
                break;
            }

            case token_kind::pipe_pipe: {
                return "pipe_pipe"s;
                break;
            }

            case token_kind::carat: {
                return "carat"s;
                break;
            }

            case token_kind::tilde: {
                return "tilde"s;
                break;
            }

            case token_kind::percentage: {
                return "percentage"s;
                break;
            }

            case token_kind::question_mark: {
                return "question_mark"s;
                break;
            }

            case token_kind::slash_star: {
                return "slash_star"s;
                break;
            }

            case token_kind::star_slash: {
                return "star_slash"s;
                break;
            }

            case token_kind::string_literal: {
                return "string literal: " + string_literal_value(_tok);
                break;
            }

            case token_kind::backslash: {
                return "backslash"s;
                break;
            }

            case token_kind::period: {
                return "period"s;
                break;
            }

            default: {
                return "unrecognized token"s;
                break;
            }
        }
    }

    bool operator==(token const& _first, token const& _second) {
        if (_first.kind() != _second.kind()) return false;
        if (_first.m_data.has_value() != _first.m_data.has_value()) return false;
        if (not _first.m_data.has_value()) return true;

        switch (_first.kind()) {
            case token_kind::identifier: return token::identifier_value(_first) == token::identifier_value(_second);
            case token_kind::string_literal: return token::string_literal_value(_first) == token::string_literal_value(_second);

            default: throw std::logic_error("invalid token kind with data");
        }
    }

    bool token::is_identifier(token const& _tok) noexcept { return _tok.kind() == token_kind::identifier; }

    token token::make_identifier(std::string _value) noexcept { return token(token_kind::identifier, std::move(_value)); }

    std::string token::identifier_value(token const& _tok) noexcept { return _tok.data<std::string>(); }

    bool token::is_string_literal(token const& _tok) noexcept { return _tok.kind() == token_kind::string_literal; }

    token token::make_string_literal(std::string _value) noexcept {
        return token(token_kind::string_literal, std::move(_value));
    }

    std::string token::string_literal_value(token const& _tok) noexcept { return _tok.data<std::string>(); }
}