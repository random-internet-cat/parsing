#pragma once

#include <optional>
#include <tuple>
#include <variant>

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

    template<typename... SubGrammars>
    class sequence_grammar {
    public:
        template<typename TokenStream>
        struct traits_for {
            using value_type = std::tuple<typename grammar_traits<SubGrammars, TokenStream>::value_type...>;
            using error_type = std::variant<typename grammar_traits<SubGrammars, TokenStream>::error_type...>;

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
            auto optTuple = std::tuple<std::optional<typename grammar_traits<SubGrammars, TokenStream>::value_type>...>();
            std::optional<typename traits_for<TokenStream>::error_type> error = std::nullopt;

            typename token_stream_traits<TokenStream>::access_wrapper accessWrapper(_tokenStream);
            typename traits_for<TokenStream>::result_type::size_type amountParsed = 0;

            ((
                 [&](auto const& subGrammar) {
                     if (error) return;

                     using current_grammar_traits = grammar_traits<std::tuple_element_t<Is, decltype(m_subGrammars)>, TokenStream>;

                     auto result = current_grammar_traits::test(subGrammar, accessWrapper.get());

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
            using value_type = std::optional<typename grammar_traits<SubGrammar, TokenStream>::value_type>;
            using error_type = grammar_no_error;
            using result_type = parse_result<value_type, error_type>;
        };

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const {
            auto result = grammar_traits<SubGrammar, TokenStream>::test(m_subGrammar, _tokenStream);
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
            using value_type = std::variant<typename grammar_traits<SubGrammars, TokenStream>::value_type...>;
            using error_type = std::tuple<typename grammar_traits<SubGrammars, TokenStream>::error_type...>;

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
            std::tuple<std::optional<typename grammar_traits<SubGrammars, TokenStream>::error_type>...> error;

            ((
                 [&](auto const& subGrammar) {
                     if (result) return;

                     using current_grammar_traits = grammar_traits<std::tuple_element_t<Is, decltype(m_subGrammars)>, TokenStream>;

                     auto parseResult = current_grammar_traits::test(subGrammar, _tokenStream);
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


}    // namespace randomcat::parser
