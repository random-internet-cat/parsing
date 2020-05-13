#include "randomcat/simple_parsing/token.hpp"

#include <string>
#include <string_view>

namespace randomcat::simple_parsing {
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

            case token_kind::lparen: {
                return "lparen"s;
                break;
            }

            case token_kind::rparen: {
                return "rparen"s;
                break;
            }

            case token_kind::plus: {
                return "plus"s;
                break;
            }

            case token_kind::minus: {
                return "minus"s;
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
            default: {
                return "unrecognized token"s;
                break;
            }
        }
    }
    
    token token::make_integer_literal(randomcat::simple_parsing::token::integer_literal_value_type _value) noexcept {
        return token(token_kind::integer_literal, _value);
    }
    
    bool token::is_integer_literal(randomcat::simple_parsing::token const& _tok) noexcept {
        return _tok.kind() == token_kind::integer_literal;
    }
    
    token::integer_literal_value_type token::integer_literal_value(randomcat::simple_parsing::token const& _tok) noexcept {
        return _tok.data<integer_literal_value_type>();
    }
    
    token token::make_variable(randomcat::simple_parsing::variable_kind _var) noexcept {
        return token(token_kind::variable, _var);
    }
    
    bool token::is_variable(randomcat::simple_parsing::token const& _tok) noexcept {
        return _tok.kind() == token_kind::variable;
    }
    
    variable_kind token::variable_value(randomcat::simple_parsing::token const& _tok) noexcept {
        return _tok.data<variable_kind>();
    }

    bool operator==(token const& _first, token const& _second) {
        if (_first.kind() != _second.kind()) return false;
        if (_first.m_data.has_value() != _first.m_data.has_value()) return false;
        if (not _first.m_data.has_value()) return true;

        switch (_first.kind()) {
            case token_kind::integer_literal: return token::integer_literal_value(_first) == token::integer_literal_value(_second);
            case token_kind::variable: return token::variable_value(_first) == token::variable_value(_second);
            default: throw std::logic_error("invalid token kind with data");
        }
    }
}