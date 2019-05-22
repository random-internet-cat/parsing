#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <randomcat/parser/chars/tokenizer.hpp>
#include <randomcat/parser/detail/util.hpp>

namespace randomcat::parser::char_traits_detail {
    using default_size_type = std::size_t;
    using default_priority_type = std::int32_t;

    template<typename T, typename = void>
    struct char_source_char_types {
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
    };

    template<typename T>
    struct char_source_char_types<T, std::void_t<typename T::char_type>> {
        using char_type = typename T::char_type;
        using char_traits_type = typename T::char_traits_type;
    };

    template<typename Tokenizer>
    using char_type_t = typename char_source_char_types<Tokenizer>::char_type;

    template<typename Tokenizer>
    using char_traits_type_t = typename char_source_char_types<Tokenizer>::char_traits_type;

    template<typename T, typename Default, typename = void>
    struct size_type {
        using type = Default;
    };

    template<typename T, typename Default>
    struct size_type<T, Default, std::void_t<typename T::size_type>> {
        using type = typename T::size_type;
    };

    template<typename T, typename Default = default_size_type>
    using size_type_t = typename size_type<T, Default>::type;

    template<typename T, typename = void>
    struct string_types {
        using string_type = std::basic_string<char_type_t<T>, char_traits_type_t<T>>;
        using string_view_type = std::basic_string_view<char_type_t<T>, char_traits_type_t<T>>;
    };

    template<typename T>
    using string_type_t = typename string_types<T>::string_type;

    template<typename T>
    using string_view_type_t = typename string_types<T>::string_view_type;

    template<typename T, typename Default, typename = void>
    struct priority_type {
        using type = Default;
    };

    template<typename T, typename Default>
    struct priority_type<T, Default, std::void_t<typename T::priority_type>> {
        using type = typename T::priority_type;
    };

    template<typename T, typename Default = default_priority_type>
    using priority_type_t = typename priority_type<T, Default>::type;

    template<typename T>
    using token_type_t = typename T::token_type;

    template<typename T>
    using error_type_t = typename T::error_type;

    template<typename T>
    using location_type_t = typename T::location_type;

    template<typename CharSource, typename Enable, typename... Args>
    struct has_peek : std::false_type {};

    template<typename CharSource, typename... Args>
    struct has_peek<CharSource, std::void_t<decltype(std::declval<CharSource>().peek(std::declval<Args>()...))>, Args...> : std::true_type {};

    template<typename CharSource, typename... Args>
    inline constexpr auto has_peek_v = has_peek<CharSource, void, Args...>::value;

    template<typename CharSource, typename = void>
    struct has_peek_char : std::false_type {};

    template<typename CharSource>
    struct has_peek_char<CharSource, std::void_t<decltype(std::declval<CharSource>().peek_char())>> : std::true_type {};

    template<typename CharSource>
    inline auto constexpr has_peek_char_v = has_peek_char<CharSource>::value;

    template<typename CharSource, typename Enable, typename... Args>
    struct has_read : std::false_type {};

    template<typename CharSource, typename... Args>
    struct has_read<CharSource, std::void_t<decltype(std::declval<CharSource>().read(std::declval<Args>()...))>, Args...> : std::true_type {};

    template<typename CharSource, typename... Args>
    inline constexpr auto has_read_v = has_read<CharSource, void, Args...>::value;

    template<typename CharSource, typename = void>
    struct has_read_char : std::false_type {};

    template<typename CharSource>
    struct has_read_char<CharSource, std::void_t<decltype(std::declval<CharSource>().read_char())>> : std::true_type {};

    template<typename CharSource>
    inline auto constexpr has_read_char_v = has_read_char<CharSource>::value;
}    // namespace randomcat::parser::char_traits_detail