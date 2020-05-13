#pragma once

#include <randomcat/parser/chars/tokenizer.hpp>

#include "randomcat/simple_parsing/lift.hpp"
#include "randomcat/simple_parsing/token.hpp"

namespace randomcat::simple_parsing {
    inline constexpr int invalid_token_priority = std::numeric_limits<parser::default_priority_type>::min();
    
    class invalid_token_desc {
    public:
        using token_type = token;
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;
        using error_type = parser::no_matching_token_t;
        using parse_result_type = parser::parse_result<token_type, error_type>;
        using priority_type = parser::default_priority_type;

        template<typename CharSource>
        parse_result_type parse_first_token(CharSource const& _chars) const noexcept {
            return {token(token_kind::invalid_token), 1};
        }

        static constexpr priority_type priority() noexcept { return invalid_token_priority; }
    };
}