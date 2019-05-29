#pragma once

#include <optional>
#include <tuple>
#include <variant>

#include <randomcat/type_container/type_list.hpp>

#include "randomcat/parser/detail/util.hpp"
#include "randomcat/parser/tokens/token_stream/token_stream.hpp"

namespace randomcat::parser {
    template<typename Grammar, typename TokenStream>
    struct grammar_traits {
        using __traits = typename Grammar::template traits_for<TokenStream>;

        using value_type = typename __traits::value_type;
        using error_type = typename __traits::error_type;
        using result_type = parse_result<value_type, error_type>;

        static result_type advance_if_matches(Grammar const& _grammar, TokenStream& _tokenStream) {
            auto result = test(_grammar, _tokenStream);
            if (result) token_stream_traits<TokenStream>::advance(_tokenStream, result.amount_parsed());
            return result;
        }

        static result_type test(Grammar const& _grammar, TokenStream const& _tokenStream) { return _grammar.test(_tokenStream); }
    };

    template<typename Grammar, typename TokenStream>
    using grammar_value_type_t = typename grammar_traits<Grammar, TokenStream>::value_type;

    template<typename Grammar, typename TokenStream>
    using grammar_error_type_t = typename grammar_traits<Grammar, TokenStream>::error_type;

    template<typename Grammar, typename TokenStream>
    using grammar_result_type_t = typename grammar_traits<Grammar, TokenStream>::result_type;

    template<typename Grammar, typename TokenStream>
    inline auto grammar_test(Grammar const& _grammar, TokenStream const& _tokenStream)
        -> decltype(grammar_traits<Grammar, TokenStream>::test(_grammar, _tokenStream)) {
        return grammar_traits<Grammar, TokenStream>::test(_grammar, _tokenStream);
    }

    template<typename Grammar, typename TokenStream, typename = std::enable_if_t<not std::is_const_v<TokenStream>>>
    inline auto grammar_advance_if_matches(Grammar const& _grammar, TokenStream& _tokenStream)
        -> decltype(grammar_traits<Grammar, TokenStream>::advance_if_matches(_grammar, _tokenStream)) {
        return grammar_traits<Grammar, TokenStream>::advance_if_matches(_grammar, _tokenStream);
    }

    struct grammar_non_match_t {};
    inline constexpr grammar_non_match_t grammar_non_match;

    template<typename Matches>
    class single_token_grammar {
    public:
        template<typename TokenStream>
        struct traits_for {
            using value_type = typename token_stream_traits<TokenStream>::token_type;
            using error_type = grammar_non_match_t;
            using result_type = parse_result<value_type, error_type>;
        };

        explicit single_token_grammar(Matches _matches) : m_matches(std::move(_matches)) {}

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _stream) const {
            auto token = token_stream_traits<TokenStream>::peek(_stream);
            if (m_matches(token)) return {token, 1};

            return grammar_non_match;
        }

    private:
        Matches m_matches;
    };

    namespace grammar_variant_detail {
        template<typename GrammarList, typename ValueList>
        class grammar_variant;

        template<typename... Grammars, typename... Values>
        class grammar_variant<type_container::type_list<Grammars...>, type_container::type_list<Values...>> {
        private:
            using GrammarList = type_container::type_list<Grammars...>;
            using ValueList = type_container::type_list<Values...>;

            static_assert((util_detail::is_simple_type_v<Values> && ...));

        public:
            template<std::size_t I>
            constexpr explicit grammar_variant(std::in_place_index_t<I>, type_container::type_list_at_t<ValueList, I> _val)
            : m_value(std::in_place_index<I>, std::move(_val)) {}

            template<std::size_t I>
            constexpr auto get() const -> decltype(auto) {
                return std::get<I>(this->m_value);
            }

            template<typename Grammar>
            constexpr auto for_grammar() const -> decltype(get<type_container::type_list_first_index_of_v<GrammarList, Grammar>>()) {
                static_assert(type_container::type_list_count_v<GrammarList, Grammar> == 1);
                return get<type_container::type_list_first_index_of_v<GrammarList, Grammar>>();
            }

            constexpr auto const& underlying() const noexcept { return m_value; }

            constexpr auto index() const noexcept { return m_value.index(); }

        private:
            std::variant<Values...> m_value;
        };

        template<std::size_t I, typename... VariantArgs>
        inline constexpr auto get(grammar_variant<VariantArgs...> const& _value) -> decltype(_value.template get<I>()) {
            return _value.template get<I>();
        }

        template<typename Grammar, typename... VariantArgs>
        inline constexpr auto for_grammar(grammar_variant<VariantArgs...> const& _value) {
            return _value.template for_grammar<Grammar>();
        }

        template<typename Visitor, typename... VariantArgs>
        inline constexpr auto visit(Visitor&& _visitor, grammar_variant<VariantArgs...> const& _value)
            -> decltype(std::visit(std::forward<Visitor>(_visitor), _value.underlying())) {
            return std::visit(std::forward<Visitor>(_visitor), _value.underlying());
        }
    }    // namespace grammar_variant_detail

    using grammar_variant_detail::for_grammar;
    using grammar_variant_detail::get;
    using grammar_variant_detail::visit;

    namespace grammar_tuple_detail {
        template<typename GrammarList, typename ValueList>
        class grammar_tuple;

        template<typename... Grammars, typename... Values>
        class grammar_tuple<type_container::type_list<Grammars...>, type_container::type_list<Values...>> {
        private:
            static_assert((util_detail::is_simple_type_v<Values> && ...));

            using GrammarList = type_container::type_list<Grammars...>;
            using ValueList = type_container::type_list<Values...>;

        public:
            constexpr explicit grammar_tuple(Values... _values) noexcept : m_tuple(std::move(_values)...) {}

            template<std::size_t I>
            constexpr decltype(auto) get() const {
                return std::get<I>(m_tuple);
            }

            template<typename Grammar>
            constexpr decltype(auto) for_grammar() const {
                static_assert(type_container::type_list_count_v<GrammarList, Grammar> == 1);
                return get<type_container::type_list_first_index_of_v<GrammarList, Grammar>>();
            }

            constexpr auto const& underlying() const noexcept { return m_tuple; }

        private:
            std::tuple<Values...> m_tuple;
        };

        template<std::size_t I, typename... TupleArgs>
        inline constexpr auto get(grammar_tuple<TupleArgs...> const& _tuple) -> decltype(_tuple.template get<I>()) {
            return _tuple.template get<I>();
        }

        template<typename Grammar, typename... TupleArgs>
        inline constexpr auto for_grammar(grammar_tuple<TupleArgs...> const& _tuple) -> decltype(_tuple.template for_grammar<Grammar>()) {
            return _tuple.template for_grammar<Grammar>();
        }
    }    // namespace grammar_tuple_detail

    using grammar_tuple_detail::for_grammar;
    using grammar_tuple_detail::get;

    template<typename... SubGrammars>
    class sequence_grammar {
    public:
        template<typename TokenStream>
        struct traits_for {
            using value_type =
                grammar_tuple_detail::grammar_tuple<type_container::type_list<SubGrammars...>, type_container::type_list<grammar_value_type_t<SubGrammars, TokenStream>...>>;
            using error_type =
                grammar_variant_detail::grammar_variant<type_container::type_list<SubGrammars...>, type_container::type_list<grammar_error_type_t<SubGrammars, TokenStream>...>>;

            using result_type = parse_result<value_type, error_type>;
        };

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const {
            return test_helper(std::make_index_sequence<sizeof...(SubGrammars)>(), _tokenStream);
        }

        explicit sequence_grammar(SubGrammars... _subGrammar) : m_subGrammars{std::move(_subGrammar)...} {}

    private:
        template<std::size_t... Is, typename TokenStream>
        typename traits_for<TokenStream>::result_type test_helper(std::index_sequence<Is...>, TokenStream const& _tokenStream) const {
            auto optTuple = std::tuple<std::optional<grammar_value_type_t<SubGrammars, TokenStream>>...>();
            std::optional<typename traits_for<TokenStream>::error_type> error = std::nullopt;

            typename token_stream_traits<TokenStream>::access_wrapper accessWrapper(_tokenStream);
            typename traits_for<TokenStream>::result_type::size_type amountParsed = 0;

            ((
                 [&](auto const& subGrammar) {
                     if (error) return;

                     auto result = grammar_test(subGrammar, accessWrapper.get());

                     if (result.is_value()) {
                         auto thisParse = result.amount_parsed();
                         amountParsed += thisParse;
                         accessWrapper.advance(thisParse);

                         std::get<Is>(optTuple) = std::move(result).value();
                     } else {
                         error = typename traits_for<TokenStream>::error_type(std::in_place_index<Is>, std::move(result).error());
                     }
                 }(std::get<Is>(m_subGrammars)),
                 void()),
             ...);

            if (error) return std::move(*error);

            return {typename traits_for<TokenStream>::value_type{(std::get<Is>(std::move(optTuple)).value())...}, amountParsed};
        }

        std::tuple<SubGrammars...> m_subGrammars;
    };

    struct grammar_no_error {};

    template<typename SubGrammar>
    class optional_grammar {
    public:
        explicit optional_grammar(SubGrammar _subGrammar) : m_subGrammar(std::move(_subGrammar)) {}

        template<typename TokenStream>
        struct traits_for {
            using value_type = std::optional<grammar_value_type_t<SubGrammar, TokenStream>>;
            using error_type = grammar_no_error;
            using result_type = parse_result<value_type, error_type>;
        };

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const {
            auto result = grammar_test(m_subGrammar, _tokenStream);
            if (result.is_value()) {
                auto amountParsed = result.amount_parsed();
                return {std::move(result).value(), std::move(amountParsed)};
            }

            return {std::nullopt, 0};
        }

    private:
        SubGrammar m_subGrammar;
    };

    template<typename... SubGrammars>
    class selection_grammar {
    public:
        explicit selection_grammar(SubGrammars... _subGrammars) : m_subGrammars{std::move(_subGrammars)...} {}

        template<typename TokenStream>
        struct traits_for {
            using value_type =
                grammar_variant_detail::grammar_variant<type_container::type_list<SubGrammars...>, type_container::type_list<grammar_value_type_t<SubGrammars, TokenStream>...>>;
            using error_type =
                grammar_tuple_detail::grammar_tuple<type_container::type_list<SubGrammars...>, type_container::type_list<grammar_error_type_t<SubGrammars, TokenStream>...>>;

            using result_type = parse_result<value_type, error_type>;
        };

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const {
            return test_helper(std::make_index_sequence<sizeof...(SubGrammars)>(), _tokenStream);
        }

    private:
        template<typename TokenStream, std::size_t... Is>
        typename traits_for<TokenStream>::result_type test_helper(std::index_sequence<Is...>, TokenStream const& _tokenStream) const {
            std::optional<typename traits_for<TokenStream>::result_type> result = std::nullopt;
            std::tuple<std::optional<grammar_error_type_t<SubGrammars, TokenStream>>...> error;

            ((
                 [&](auto const& subGrammar) {
                     if (result) return;

                     auto parseResult = grammar_test(subGrammar, _tokenStream);
                     if (parseResult) {
                         auto amountParsed = parseResult.amount_parsed();
                         result = typename traits_for<TokenStream>::result_type(typename traits_for<TokenStream>::value_type{std::in_place_index<Is>,
                                                                                                                             std::move(parseResult)
                                                                                                                                 .value()},
                                                                                std::move(amountParsed));
                     } else {
                         std::get<Is>(error) = std::move(parseResult).error();
                     }
                 }(std::get<Is>(m_subGrammars)),
                 void()),
             ...);

            if (result) return *std::move(result);
            return typename traits_for<TokenStream>::error_type{std::get<Is>(std::move(error)).value()...};
        }

        std::tuple<SubGrammars...> m_subGrammars;
    };

    template<typename Base, typename... Tags>
    class tag_grammar_t {
    public:
        static_assert(util_detail::is_simple_type_v<Base>);

        constexpr explicit tag_grammar_t(Base _subGrammar) : m_subGrammar(std::move(_subGrammar)) {}

        template<typename TokenStream>
        struct traits_for {
            using value_type = grammar_value_type_t<Base, TokenStream>;
            using error_type = grammar_error_type_t<Base, TokenStream>;
            using result_type = grammar_result_type_t<Base, TokenStream>;
        };

        template<typename TokenStream>
        constexpr typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const
            noexcept(noexcept(grammar_test(m_subGrammar, _tokenStream))) {
            return grammar_test(m_subGrammar, _tokenStream);
        }

    private:
        Base m_subGrammar;
    };

    template<typename... Tags, typename Base>
    constexpr inline tag_grammar_t<Base, Tags...> tag_grammar(Base _baseGrammar) {
        return tag_grammar_t<Base, Tags...>(std::move(_baseGrammar));
    }
}    // namespace randomcat::parser
