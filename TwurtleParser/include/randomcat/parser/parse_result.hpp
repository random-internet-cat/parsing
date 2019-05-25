#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <randomcat/parser/detail/util.hpp>

namespace randomcat::parser {
    template<typename ValueType, typename ErrorType>
    class parse_result {
    public:
        using value_type = ValueType;
        using error_type = ErrorType;
        using size_type = std::size_t;

        static_assert(util_detail::is_simple_type_v<ValueType>);
        static_assert(util_detail::is_simple_type_v<ErrorType>);

        static_assert(not std::is_same_v<value_type, error_type>);

        /* implicit */ constexpr parse_result(value_type _value, size_type _amountParsed) noexcept(std::is_nothrow_move_constructible_v<value_type>)
        : m_value(std::in_place_index<0>, std::move(_value), std::move(_amountParsed)) {}

        /* implicit */ constexpr parse_result(error_type _value) noexcept(std::is_nothrow_move_constructible_v<error_type>)
        : m_value(std::in_place_index<1>, std::move(_value)) {}

        /* implicit */ constexpr parse_result() : parse_result(error_type()) {}

        [[nodiscard]] constexpr bool is_value() const noexcept { return m_value.index() == 0; }

        [[nodiscard]] constexpr bool is_error() const noexcept { return m_value.index() == 1; }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return is_value(); }

        [[nodiscard]] constexpr value_type const& value() const& noexcept { return std::get<0>(m_value).first; }
        [[nodiscard]] constexpr value_type&& value() && noexcept { return std::get<0>(std::move(m_value)).first; }

        [[nodiscard]] constexpr error_type const& error() const& noexcept { return std::get<1>(m_value); }
        [[nodiscard]] constexpr error_type&& error() && noexcept { return std::get<1>(std::move(m_value)); }

        [[nodiscard]] constexpr size_type amount_parsed() const noexcept { return std::get<0>(m_value).second; }

    private:
        std::variant<std::pair<ValueType, size_type>, ErrorType> m_value;
    };

    template<typename ErrorType>
    class parse_result<void, ErrorType> {
    public:
        using value_type = void;
        using error_type = ErrorType;
        using size_type = std::size_t;

        static_assert(util_detail::is_simple_type_v<ErrorType>);

        /* implicit */ constexpr parse_result(size_type _amountParsed) noexcept
        : m_value(std::in_place_index<0>, std::move(_amountParsed)) {}

        /* implicit */ constexpr parse_result(error_type _value) noexcept(std::is_nothrow_move_constructible_v<error_type>)
        : m_value(std::in_place_index<1>, std::move(_value)) {}

        [[nodiscard]] constexpr bool is_value() const noexcept { return m_value.index() == 0; }

        [[nodiscard]] constexpr bool is_error() const noexcept { return m_value.index() == 1; }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return is_value(); }

        [[nodiscard]] constexpr error_type const& error() const& noexcept { return std::get<1>(m_value); }
        [[nodiscard]] constexpr error_type&& error() && noexcept { return std::get<1>(std::move(m_value)); }

        [[nodiscard]] constexpr size_type amount_parsed() const noexcept { return std::get<0>(m_value); }

    private:
        std::variant<size_type, ErrorType> m_value;
    };

    template<typename ValueType>
    class parse_result<ValueType, void> {
    public:
        using value_type = ValueType;
        using error_type = void;
        using size_type = std::size_t;

        static_assert(util_detail::is_simple_type_v<ValueType>);

        /* implicit */ constexpr parse_result(value_type _value, size_type _amountParsed)
        : m_value({std::move(_value), std::move(_amountParsed)}) {}

        /* implicit */ constexpr parse_result() : m_value(std::nullopt) {}

        [[nodiscard]] constexpr bool is_value() const noexcept { return m_value.has_value(); }
        [[nodiscard]] constexpr bool is_error() const noexcept { return not is_value(); }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return is_value(); }

        [[nodiscard]] constexpr value_type const& value() const& noexcept { return std::get<0>(m_value.value()); }
        [[nodiscard]] constexpr value_type&& value() && noexcept { return std::get<0>(std::move(m_value).value()); }

        [[nodiscard]] constexpr size_type amount_parsed() const noexcept { return std::get<1>(m_value.value()); }

    private:
        std::optional<std::pair<value_type, size_type>> m_value;
    };
}    // namespace randomcat::parser