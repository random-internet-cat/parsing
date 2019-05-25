#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <randomcat/parser/detail/util.hpp>

namespace randomcat::parser {
    template<typename ResultType, typename ErrorType>
    class parse_result {
    public:
        using result_type = ResultType;
        using error_type = ErrorType;
        using size_type = std::size_t;

        static_assert(util_detail::is_simple_type_v<ResultType>);
        static_assert(util_detail::is_simple_type_v<ErrorType>);

        static_assert(not std::is_same_v<result_type, error_type>);

        /* implicit */ constexpr parse_result(result_type _value, size_type _amountParsed) noexcept(std::is_nothrow_move_constructible_v<result_type>)
        : m_value(std::in_place_index<0>, std::move(_value), std::move(_amountParsed)) {}

        /* implicit */ constexpr parse_result(error_type _value) noexcept(std::is_nothrow_move_constructible_v<error_type>)
        : m_value(std::in_place_index<1>, std::move(_value)) {}

        /* implicit */ constexpr parse_result() : parse_result(error_type()) {}

        [[nodiscard]] constexpr bool is_value() const noexcept { return m_value.index() == 0; }

        [[nodiscard]] constexpr bool is_error() const noexcept { return m_value.index() == 1; }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return is_value(); }

        [[nodiscard]] constexpr result_type const& value() const noexcept { return std::get<0>(m_value).first; }

        [[nodiscard]] constexpr error_type const& error() const noexcept { return std::get<1>(m_value); }

        [[nodiscard]] constexpr size_type amount_parsed() const noexcept { return std::get<0>(m_value).second; }

    private:
        std::variant<std::pair<ResultType, size_type>, ErrorType> m_value;
    };

    template<typename ErrorType>
    class parse_result<void, ErrorType> {
    public:
        using result_type = void;
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

        [[nodiscard]] constexpr error_type const& error() const noexcept { return std::get<1>(m_value); }

        [[nodiscard]] constexpr size_type amount_parsed() const noexcept { return std::get<0>(m_value); }

    private:
        std::variant<size_type, ErrorType> m_value;
    };

    template<typename ResultType>
    class parse_result<ResultType, void> {
    public:
        using result_type = ResultType;
        using error_type = void;
        using size_type = std::size_t;

        static_assert(util_detail::is_simple_type_v<ResultType>);

        /* implicit */ constexpr parse_result(result_type _value, size_type _amountParsed)
        : m_value({std::move(_value), std::move(_amountParsed)}) {}

        /* implicit */ constexpr parse_result() : m_value(std::nullopt) {}

        [[nodiscard]] constexpr bool is_value() const noexcept { return m_value.has_value(); }
        [[nodiscard]] constexpr bool is_error() const noexcept { return not is_value(); }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return is_value(); }

        [[nodiscard]] constexpr result_type const& value() const noexcept { return std::get<0>(m_value.value()); }
        [[nodiscard]] constexpr size_type amount_parsed() const noexcept { return std::get<1>(m_value.value()); }

    private:
        std::optional<std::pair<result_type, size_type>> m_value;
    };
}    // namespace randomcat::parser