#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <gsl/gsl_util>

#include "randomcat/parser/chars/char_source.hpp"
#include "randomcat/parser/chars/detail/char_traits.hpp"
#include "randomcat/parser/detail/defaults.hpp"
#include "randomcat/parser/detail/util.hpp"
#include "randomcat/parser/parse_result.hpp"

namespace randomcat::parser {
    template<typename Tokenizer>
    struct tokenizer_traits {
        using token_type = char_traits_detail::token_type_t<Tokenizer>;

        using char_type = char_traits_detail::char_type_t<Tokenizer>;
        using char_traits_type = char_traits_detail::char_traits_type_t<Tokenizer>;

        using string_type = char_traits_detail::string_type_t<Tokenizer>;
        using string_view_type = char_traits_detail::string_view_type_t<Tokenizer>;

        using error_type = char_traits_detail::error_type_t<Tokenizer>;

        using parse_result_type = parse_result<token_type, error_type>;

        template<typename CharSource>
        static constexpr parse_result_type parse_first_token(Tokenizer const& _tokenizer,
                                                             CharSource const& _input) noexcept(noexcept(_tokenizer.parse_first_token(_input))) {
            return _tokenizer.parse_first_token(_input);
        }
    };

    struct no_matching_token_t {};
    inline constexpr no_matching_token_t no_matching_token{};

    template<typename TokenDescriptor>
    struct token_descriptor_traits {
        using token_type = char_traits_detail::token_type_t<TokenDescriptor>;

        using char_type = char_traits_detail::char_type_t<TokenDescriptor>;
        using char_traits_type = char_traits_detail::char_traits_type_t<TokenDescriptor>;

        using string_type = char_traits_detail::string_type_t<TokenDescriptor>;
        using string_view_type = char_traits_detail::string_view_type_t<TokenDescriptor>;

        using priority_type = char_traits_detail::priority_type_t<TokenDescriptor>;

        static constexpr priority_type priority(TokenDescriptor const& _tokenDescriptor) noexcept(noexcept(_tokenDescriptor.priority())) {
            return _tokenDescriptor.priority();
        }

        using error_type = char_traits_detail::error_type_t<TokenDescriptor>;
        using parse_result_type = parse_result<token_type, error_type>;

        template<typename CharSource>
        static constexpr parse_result_type parse_first_token(TokenDescriptor const& _tokenDescriptor,
                                                             CharSource const& _chars) noexcept(noexcept(_tokenDescriptor.parse_first_token(_chars))) {
            return _tokenDescriptor.parse_first_token(_chars);
        }
    };

    template<typename Token>
    class simple_token_descriptor {
    public:
        using token_type = Token;
        using char_type = char_traits_detail::char_type_t<Token>;
        using char_traits_type = char_traits_detail::char_traits_type_t<Token>;

        using string_type = char_traits_detail::string_type_t<Token>;
        using string_view_type = char_traits_detail::string_view_type_t<Token>;

        using error_type = no_matching_token_t;

        using parse_result_type = parse_result<token_type, error_type>;

        using priority_type = default_priority_type;
        using size_type = char_traits_detail::size_type_t<string_type>;

        constexpr explicit simple_token_descriptor(token_type _token, string_type _string) noexcept
        // This is okay, since braced-init-list rules guarantee left-to-right evalution
        : simple_token_descriptor{_token, gsl::narrow<priority_type>(_string.size()), std::move(_string)} {}

        constexpr simple_token_descriptor(token_type _token, priority_type _priority, string_type _string) noexcept
        : m_string(std::move(_string)), m_token(std::move(_token)), m_priority(std::move(_priority)) {}

        template<typename CharSource>
        constexpr parse_result_type parse_first_token(CharSource const& _chars) const noexcept {
            if (char_source_traits<CharSource>::at_end(_chars)) return no_matching_token;
            if (m_string == char_source_traits<CharSource>::peek(_chars, size())) return {m_token, size()};

            return no_matching_token;
        }

        constexpr priority_type priority() const noexcept { return m_priority; }

    private:
        size_type size() const noexcept { return m_string.size(); }

        string_type m_string;
        token_type m_token;
        priority_type m_priority;
    };

    template<typename Token, typename... Strings>
    class multi_form_token_descriptor {
    public:
        using token_type = Token;
        using char_type = char_traits_detail::char_type_t<Token>;
        using char_traits_type = char_traits_detail::char_traits_type_t<Token>;

        using string_type = char_traits_detail::string_type_t<Token>;
        using string_view_type = char_traits_detail::string_view_type_t<Token>;

        using error_type = no_matching_token_t;

        using parse_result_type = parse_result<token_type, error_type>;

        using priority_type = default_priority_type;
        using size_type = char_traits_detail::size_type_t<string_type>;

        constexpr explicit multi_form_token_descriptor(token_type _token, priority_type _priority, Strings... _strings) noexcept
        : m_strings(std::move(_strings)...), m_token(std::move(_token)), m_priority(std::move(_priority)) {}

        template<typename CharSource>
        constexpr parse_result_type parse_first_token(CharSource const& _chars) const noexcept {
            if (char_source_traits<CharSource>::at_end(_chars)) return no_matching_token;
            return parse_first_token_helper(std::make_index_sequence<num_strings>(), _chars);
        }

        constexpr auto priority() const noexcept { return m_priority; }

    private:
        static constexpr auto num_strings = sizeof...(Strings);

        template<std::size_t... Is, typename CharSource>
        constexpr parse_result_type parse_first_token_helper(std::index_sequence<Is...>, CharSource const& _input) const noexcept {
            bool done = false;
            size_type charsRead;

            ((
                 [&](auto const& string) {
                     if (done) return;

                     if (char_source_traits<CharSource>::peek(_input, size(string)) == string) {
                         done = true;
                         charsRead = size(string);
                     }
                 }(std::get<Is>(m_strings)),
                 void()),
             ...);

            if (done) { return {m_token, charsRead}; }

            return no_matching_token;
        }

        std::tuple<Strings...> m_strings;
        token_type m_token;
        priority_type m_priority;
    };

    template<typename Token, typename... Strings>
    constexpr inline auto make_multi_form_token_descriptor(Token _token, std::int32_t _priority, Strings&&... _strings) noexcept {
        return multi_form_token_descriptor<Token, util_detail::first_t<std::string, Strings>...>(_token, _priority, std::forward<Strings>(_strings)...);
    }

    template<typename Token, typename... TokenParsers>
    class simple_tokenizer {
    public:
        using token_type = Token;

        using error_type = std::tuple<typename token_descriptor_traits<TokenParsers>::error_type...>;
        using parse_result_type = parse_result<token_type, error_type>;

        static_assert(util_detail::all_are_same_v<char_traits_detail::char_type_t<Token>, char_traits_detail::char_type_t<TokenParsers>...>);
        static_assert(util_detail::all_are_same_v<char_traits_detail::char_traits_type_t<Token>, char_traits_detail::char_traits_type_t<TokenParsers>...>);

        static_assert(util_detail::all_are_same_v<char_traits_detail::string_type_t<Token>, char_traits_detail::string_type_t<TokenParsers>...>);
        static_assert(util_detail::all_are_same_v<char_traits_detail::string_view_type_t<Token>, char_traits_detail::string_view_type_t<TokenParsers>...>);

        static_assert(util_detail::all_are_same_v<char_traits_detail::size_type_t<Token>, char_traits_detail::size_type_t<TokenParsers>...>);

        using char_type = char_traits_detail::char_type_t<Token>;
        using char_traits_type = char_traits_detail::char_traits_type_t<Token>;
        using string_type = char_traits_detail::string_type_t<Token>;
        using string_view_type = char_traits_detail::string_view_type_t<Token>;

        using size_type = char_traits_detail::size_type_t<Token>;

        static inline constexpr auto token_parser_count = sizeof...(TokenParsers);

        explicit constexpr simple_tokenizer(TokenParsers... _parsers) : m_descriptors(std::move(_parsers)...) {}

        template<typename CharSource>
        constexpr parse_result_type parse_first_token(CharSource const& _input) const noexcept {
            return parse_first_token_helper(std::make_index_sequence<token_parser_count>(), _input);
        }

    private:
        static_assert(util_detail::all_are_same_v<char_traits_detail::priority_type_t<token_descriptor_traits<TokenParsers>>...>);
        static_assert(sizeof...(TokenParsers) > 0);

        using priority_type = char_traits_detail::priority_type_t<util_detail::first_t<token_descriptor_traits<TokenParsers>...>>;

        struct max_token_t {
            token_type token;
            priority_type priority;
            size_type size;
        };

        template<std::size_t... Is, typename CharSource>
        constexpr parse_result_type parse_first_token_helper(std::index_sequence<Is...>, CharSource const& _chars) const noexcept {
            std::optional<max_token_t> maxToken;

            std::tuple<std::optional<typename token_descriptor_traits<TokenParsers>::error_type>...> errors;

            ((
                 [&](auto const& tokenDescriptor) {
                     using descriptor_traits = token_descriptor_traits<std::tuple_element_t<Is, decltype(m_descriptors)>>;
                     auto const priority = descriptor_traits::priority(tokenDescriptor);
                     if (maxToken.has_value() && (priority <= maxToken->priority)) return;

                     auto tokenResult = descriptor_traits::parse_first_token(tokenDescriptor, _chars);
                     if (tokenResult) {
                         auto const token = tokenResult.value();
                         maxToken = {token, priority, tokenResult.amount_parsed()};
                     } else {
                         std::get<Is>(errors) = tokenResult.error();
                     }
                 }(std::get<Is>(m_descriptors)),
                 void()),
             ...);

            if (not maxToken) return error_type{std::move(std::get<Is>(errors)).value()...};

            return {maxToken->token, maxToken->size};
        }

        std::tuple<TokenParsers...> m_descriptors;
    };

    template<typename Token, typename... TokenDescriptions>
    constexpr inline simple_tokenizer<Token, TokenDescriptions...> make_simple_tokenizer(TokenDescriptions... _parsers) {
        return simple_tokenizer<Token, TokenDescriptions...>(std::move(_parsers)...);
    }

    template<typename Tokenizer, typename CharSource>
    constexpr inline parse_result<std::vector<typename tokenizer_traits<Tokenizer>::token_type>, typename tokenizer_traits<Tokenizer>::error_type> tokenize(
        Tokenizer const& _tokenizer,
        CharSource const& _chars) {
        using token_type = char_traits_detail::token_type_t<tokenizer_traits<Tokenizer>>;
        using source_traits = char_source_traits<CharSource>;

        typename source_traits::access_wrapper accessWrapper(_chars);

        std::vector<token_type> tokens;

        while (not accessWrapper.at_end()) {
            auto tokenResult = accessWrapper.sub_parse([&](CharSource const& source) -> decltype(auto) {
                return tokenizer_traits<Tokenizer>::parse_first_token(_tokenizer, source);
            });

            if (tokenResult) {
                tokens.push_back(tokenResult.value());
                accessWrapper.advance_head(tokenResult.amount_parsed());
            } else {
                return tokenResult.error();
            }
        }

        return {std::move(tokens), accessWrapper.chars_parsed()};
    }
}    // namespace randomcat::parser
