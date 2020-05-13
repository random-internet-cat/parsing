#pragma once

#include <any>
#include <stdexcept>
#include <string>
#include <string_view>

//#include "randomcat/complex_parsing/impl_call.hpp"

namespace randomcat::complex_parsing {
    enum class token_kind {
        invalid_token,

        whitespace,
        newline,

        comma,
        period,
        period_star,
        minus_greater,
        minus_greater_star,
        lparen,
        rparen,
        lbrace,
        rbrace,
        semicolon,
        colon,
        colon_colon,
        plus,
        plus_plus,
        minus,
        minus_minus,
        star,
        slash,
        slash_slash,
        ampersand,
        ampersand_ampersand,
        pipe,
        pipe_pipe,
        carat,
        tilde,
        percentage,
        exclamation,
        exclamation_equal,
        less,
        less_less,
        greater,
        greater_greater,
        equal,
        equal_equal,
        plus_equal,
        minus_equal,
        star_equal,
        slash_equal,
        percentage_equal,
        ampersand_equal,
        pipe_equal,
        carat_equal,
        less_less_equal,
        greater_greater_equal,
        less_equal,
        greater_equal,

        question_mark,
        backslash,
        slash_star,
        star_slash,

        identifier,
        string_literal,

        kw_asm,
        kw_auto,
        kw_bool,
        kw_break,
        kw_case,
        kw_catch,
        kw_char,
        kw_char8_t,
        kw_char16_t,
        kw_class,
        kw_const,
        kw_const_cast,
        kw_continue,
        kw_default,
        kw_delete,
        kw_do,
        kw_double,
        kw_dynamic_cast,
        kw_else,
        kw_enum,
        kw_explicit,
        kw_extern,
        kw_false,
        kw_float,
        kw_for,
        kw_friend,
        kw_goto,
        kw_if,
        kw_inline,
        kw_int,
        kw_long,
        kw_mutable,
        kw_new,
        kw_noexcept,
        kw_nullptr,
        kw_operator,
        kw_private,
        kw_protected,
        kw_public,
        kw_register,
        kw_reinterpret_cast,
        kw_return,
        kw_short,
        kw_signed,
        kw_sizeof,
        kw_static,
        kw_static_assert,
        kw_static_cast,
        kw_struct,
        kw_switch,
        kw_template,
        kw_this,
        kw_thread_local,
        kw_throw,
        kw_true,
        kw_try,
        kw_typedef,
        kw_typeid,
        kw_typename,
        kw_union,
        kw_unsigned,
        kw_using,
        kw_virtual,
        kw_void,
        kw_volatile,
        kw_wchar_t,
        kw_while,

        line_comment_begin = slash_slash,
        line_comment_end = newline,
        multiline_comment_begin = slash_star,
        multiline_comment_end = star_slash,

        arrow = minus_greater,
        arrow_star = minus_greater_star,

        statement_end = semicolon,

        block_begin = lbrace,
        block_end = rbrace,

        member_access = period,
        member_pointer_use = period_star,

        indirect_access = star,
        indirect_member_access = arrow,
        indirect_member_pointer_use = arrow_star,

        assignment = equal,

        comp_eq = equal_equal,
        comp_ne = exclamation_equal,
        comp_lt = less,
        comp_gt = greater,
        comp_le = less_equal,
        comp_ge = greater_equal,

        add = plus,
        add_assign = plus_equal,
        subtract = minus,
        subtract_asssign = minus_equal,
        multiply = star,
        multiply_assign = star_equal,
        divide = slash,
        divide_assign = slash_star,
        modulo = percentage,
        modulo_assign = percentage_equal,
    };

    class token {
    public:
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;

        explicit token(token_kind _kind) noexcept : m_kind(std::move(_kind)) {}
        
        auto kind() const noexcept { return m_kind; }

        friend bool operator==(token const& _first, token const& _second);

        static bool is_identifier(token const& _tok) noexcept;

        static token make_identifier(std::string _value) noexcept;

        static std::string identifier_value(token const& _tok) noexcept;

        static bool is_string_literal(token const& _tok) noexcept;

        static token make_string_literal(std::string _value) noexcept;

        static std::string string_literal_value(token const& _tok) noexcept;

        static std::string name(token _tok) noexcept;

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