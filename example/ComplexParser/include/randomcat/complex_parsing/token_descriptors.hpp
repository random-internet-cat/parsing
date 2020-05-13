#pragma once

#include <randomcat/parser/chars/tokenizer.hpp>

#include "randomcat/complex_parsing/lift.hpp"
#include "randomcat/complex_parsing/token.hpp"

namespace randomcat::complex_parsing {
    constexpr inline std::string_view identifier_start = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    constexpr inline std::string_view identifier_part = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";
    constexpr parser::default_priority_type identifier_priority = -1;

    class identifier_token_desc {
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
            using source_traits = parser::char_source_traits<CharSource>;

            auto const firstChar = source_traits::peek_char(_chars);
            if (std::find(begin(identifier_start), end(identifier_start), firstChar) == end(identifier_start))
                return parser::no_matching_token;


            auto inputWrapper = typename source_traits::access_wrapper(_chars);

            string_type value;

            while (true) {
                auto const c = inputWrapper.read_char();
                if (std::find(begin(identifier_part), end(identifier_part), c) == end(identifier_part)) break;
                value += c;
            }

            auto parsedChars = size(value);

            return {token::make_identifier(std::move(value)), parsedChars};
        }

        static constexpr priority_type priority() noexcept { return identifier_priority; }
    };

    struct invalid_char_t {};
    constexpr inline invalid_char_t invalid_char;

    struct parse_char_result {
        char character;
        bool isEscape;
    };

    template<typename CharSource>
    constexpr inline parser::parse_result<parse_char_result, invalid_char_t> parse_char(CharSource const& _chars) noexcept {
        typename parser::char_source_traits<CharSource>::access_wrapper accessWrapper(_chars);

        if (accessWrapper.at_end()) return invalid_char;

        auto const firstChar = accessWrapper.read_char();

        switch (firstChar) {
            case '\\': {
                if (accessWrapper.at_end()) return invalid_char;
                auto const escapeDesignator = accessWrapper.read_char();

                switch (escapeDesignator) {
                    case '\'': {
                        return {{'\'', true}, 2};
                    }
                    case '\"': {
                        return {{'\"', true}, 2};
                    }
                    case '\?': {
                        return {{'\?', true}, 2};
                    }
                    case '\\': {
                        return {{'\\', true}, 2};
                    }
                    case 'a': {
                        return {{'\a', true}, 2};
                    }
                    case 'b': {
                        return {{'\b', true}, 2};
                    }
                    case 'f': {
                        return {{'\f', true}, 2};
                    }
                    case 'n': {
                        return {{'\n', true}, 2};
                    }
                    case 'r': {
                        return {{'\r', true}, 2};
                    }
                    case 't': {
                        return {{'\t', true}, 2};
                    }
                    case 'v': {
                        return {{'\v', true}, 2};
                    }
                    default: {
                        return invalid_char;
                    }
                }
            }

            case '\a':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v': {
                return invalid_char;
            }

            default: {
                return {{firstChar, false}, 1};
            }
        }
    }

    constexpr parser::default_priority_type string_literal_priority = -1;

    class string_literal_token_desc {
    public:
        using token_type = token;
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;
        using error_type = parser::no_matching_token_t;
        using parse_result_type = parser::parse_result<token_type, error_type>;
        using priority_type = parser::default_priority_type;

        static inline constexpr auto quote_char = '\"';

        template<typename CharSource>
        parse_result_type parse_first_token(CharSource const& _chars) const noexcept {
            typename parser::char_source_traits<CharSource>::access_wrapper accessWrapper(_chars);

            if (not accessWrapper.expect(quote_char)) return parser::no_matching_token;

            if (accessWrapper.at_end()) return parser::no_matching_token;

            std::string value;

            while (true) {
                auto parseResult = accessWrapper.sub_parse(LIFT(parse_char));
                if (not parseResult) return parser::no_matching_token;

                auto parseValue = parseResult.value();

                accessWrapper.advance_head(parseResult.amount_parsed());

                if ((not parseValue.isEscape) && parseValue.character == quote_char) {
                    return {token::make_string_literal(std::move(value)), accessWrapper.chars_parsed()};
                }

                if ((not parseValue.isEscape) && parseValue.character == '\n') { return parser::no_matching_token; }

                value += parseValue.character;
            }
        }

        static constexpr priority_type priority() noexcept { return string_literal_priority; }
    };

    constexpr parser::default_priority_type raw_string_literal_priority = std::numeric_limits<decltype(raw_string_literal_priority)>::max();

    class raw_string_literal_token_desc {
    public:
        using token_type = token;
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;
        using error_type = parser::no_matching_token_t;
        using parse_result_type = parser::parse_result<token_type, error_type>;
        using priority_type = parser::default_priority_type;
        using size_type = string_type::size_type;

        static inline constexpr char_type introduction = 'R';
        static inline constexpr char_type quote_mark = '"';

        template<typename CharSource>
        parse_result_type parse_first_token(CharSource const& _chars) const noexcept {
            typename parser::char_source_traits<CharSource>::access_wrapper accessWrapper(_chars);

            if (not accessWrapper.expect(introduction)) return parser::no_matching_token;
            if (not accessWrapper.expect(quote_mark)) return parser::no_matching_token;

            string_type delimiter;

            {
                char_type delimChar;
                while ((not accessWrapper.at_end()) && (delimChar = accessWrapper.read_char()) != '(') {
                    if (delimChar == ' ' || delimChar == '\\') return parser::no_matching_token;

                    delimiter += delimChar;
                }
            }

            if (accessWrapper.at_end()) return parser::no_matching_token;

            string_type literalEnd = ")" + std::move(delimiter) + quote_mark;
            string_type str;

            while (not accessWrapper.at_end() && not accessWrapper.next_is(literalEnd)) { str += accessWrapper.read_char(); }

            if (accessWrapper.at_end()) return parser::no_matching_token;
            if (not accessWrapper.expect(literalEnd)) throw std::logic_error("Internal logic error while parsing raw string literal");

            return {token::make_string_literal(std::move(str)), accessWrapper.chars_parsed()};
        }

        static constexpr priority_type priority() noexcept { return raw_string_literal_priority; }
    };

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