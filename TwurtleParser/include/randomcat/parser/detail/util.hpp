#pragma once

#include <string>
#include <string_view>

#include "randomcat/parser/detail/impl_only.hpp"

namespace randomcat::parser::util_detail {
    template<typename... Ts>
    inline constexpr bool invalid_v = false;

    template<typename... Ts>
    struct all_are_same;

    template<>
    struct all_are_same<> : std::true_type {};

    template<typename T>
    struct all_are_same<T> : std::true_type {};

    template<typename First, typename Second, typename... Rest>
    struct all_are_same<First, Second, Rest...> : std::conjunction<std::is_same<First, Second>, all_are_same<First, Rest...>> {};

    template<typename... Ts>
    inline constexpr bool all_are_same_v = all_are_same<Ts...>::value;

    template<typename... Ts>
    struct all_are_same_or_void;
    
    template<>
    struct all_are_same_or_void<> : std::true_type {};
    
    template<typename T>
    struct all_are_same_or_void : std::true_type {};
    
    template<typename... Rest>
    struct all_are_same_or_void<void, Rest...> : all_are_same_or_void<Rest...> {};
    
    template<typename First, typename... Rest>
    struct all_are_same_or_void<First, void, Rest...> : all_are_same_or_void<First, Rest...> {};
    
    template<typename First, typename Second, typename... Rest>
    struct all_are_same_or_void<First, Second, Rest...> : std::conjunction<std::is_same<First, Second>, all_are_same_or_void<Second, Rest...>> {};

    template<typename... Ts>
    inline constexpr bool are_are_same_or_void_v = all_are_same_or_void<Ts...>::value;
    
    template<typename... Ts>
    struct first;

    template<typename First, typename... Ts>
    struct first<First, Ts...> {
        using type = First;
    };

    template<typename... Ts>
    using first_t = typename first<Ts...>::type;

    template<typename T>
    struct type_identity {
        using type = T;
    };

    template<typename T>
    using type_identity_t = typename type_identity<T>::type;

    template<typename T>
    using no_deduce = type_identity_t<T>;

    // Just a plain object type
    // Not an array type (decays to pointer), not a function type (decays to pointer), not cv-qualified, not a reference type
    template<typename T>
    inline auto constexpr is_simple_type_v = std::is_same_v<T, std::decay_t<T>>;
}    // namespace randomcat::parser::util_detail
