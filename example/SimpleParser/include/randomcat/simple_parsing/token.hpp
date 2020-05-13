#pragma once

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>

namespace randomcat::simple_parsing {
    enum class token_kind {
        invalid_token,

        whitespace,

        integer_literal,
        variable,

        lparen,
        rparen,
        plus,
        minus,
        star,
        slash,
        
        kw_pi,
        
        kw_sin,
        kw_cos,
        kw_tan,
    };

    enum class variable_kind {
        theta,
        theta_prime,
        error,
    };

    inline std::string variable_name(variable_kind _var) noexcept {
        switch (_var) {
            case variable_kind::theta: return "theta";
            case variable_kind::theta_prime: return "theta_prime";
            case variable_kind::error: return "error";
        }
        
        return "<unknown>";
    }
    
    class token {
    public:
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;

        using integer_literal_value_type = std::uintmax_t;

        explicit token(token_kind _kind) noexcept : m_kind(std::move(_kind)) {}

        auto kind() const noexcept { return m_kind; }

        friend bool operator==(token const& _first, token const& _second);

        static std::string name(token _tok) noexcept;
        
        static token make_integer_literal(integer_literal_value_type _value) noexcept;
        static bool is_integer_literal(token const& _tok) noexcept;
        static integer_literal_value_type integer_literal_value(token const& _tok) noexcept;
        
        static token make_variable(variable_kind _var) noexcept;
        static bool is_variable(token const& _tok) noexcept;
        static variable_kind variable_value(token const& _tok) noexcept;

    private:
        template<typename T>
        explicit token(token_kind _kind, T _data) : m_kind(_kind), m_data(std::move(_data)) {}

        template<typename T>
        T data() const noexcept {
            return std::any_cast<T>(m_data);
        }

        token_kind m_kind;
        std::any m_data;
    };
}