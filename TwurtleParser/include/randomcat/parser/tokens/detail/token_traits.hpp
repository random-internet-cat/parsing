#pragma once

#include <type_traits>

#include "randomcat/parser/detail/defaults.hpp"

namespace randomcat::parser::token_traits_detail {
    template<typename T>
    using token_type_t = typename T::token_type;

    template<typename T>
    using location_type_t = typename T::location_type;

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
}    // namespace randomcat::parser::token_traits_detail