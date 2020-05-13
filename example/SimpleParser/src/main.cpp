#include <algorithm>
#include <any>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <randomcat/parser/chars/tokenizer.hpp>
#include <randomcat/parser/detail/util.hpp>
#include <randomcat/parser/grammar/grammar_terms.hpp>
#include <randomcat/parser/tokens/token_stream/token_stream.hpp>

#include "randomcat/simple_parsing/lift.hpp"
#include "randomcat/simple_parsing/token.hpp"
#include "randomcat/simple_parsing/token_descriptors.hpp"

namespace p = randomcat::parser;

using namespace randomcat::simple_parsing;

namespace randomcat::simple_parsing {
    auto keyword_parser(token_kind _kw, std::string _value) { return p::simple_token_descriptor(token(_kw), std::move(_value)); }

    template<typename TokenStream>
    auto strip_token_kind_token_stream(TokenStream _from, token_kind _kindToStrip) {
        return p::transform_token_stream(std::move(_from), [=](auto&& read, auto&& peek, auto&& at_end, auto&& emit) {
            auto token = read();
            if (token.kind() != _kindToStrip) emit(std::move(token));
        });
    }

    template<typename TokenStream>
    auto strip_whitespace_token_stream(TokenStream _from) {
        return simple_parsing::strip_token_kind_token_stream(std::move(_from), token_kind::whitespace);
    }

    auto token_kind_grammar(token_kind _kind) {
        return p::single_token_grammar([=](token const& tok) { return tok.kind() == _kind; });
    }

    struct integer_literal_token_descriptor {
        using token_type = token;
        using char_type = char;
        using char_traits_type = std::char_traits<char_type>;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;
        using error_type = parser::no_matching_token_t;
        using parse_result_type = parser::parse_result<token_type, error_type>;
        using priority_type = parser::default_priority_type;

        static constexpr bool is_digit(char_type c) { return '0' <= c && c <= '9'; }

        using number_type = token::integer_literal_value_type;

        static constexpr number_type digit_value(char_type c) { return c - '0'; }

        template<typename CharSource>
        parse_result_type parse_first_token(CharSource const& _charSource) const {
            typename p::char_source_traits<CharSource>::access_wrapper accessWrapper(_charSource);

            number_type value = 0;

            while (true) {
                auto c = accessWrapper.peek_char();
                if (not is_digit(c)) break;

                value *= 10;
                value += digit_value(c);

                accessWrapper.advance_head(1);
            }

            if (accessWrapper.chars_parsed() == 0) return p::no_matching_token;

            return {token::make_integer_literal(std::move(value)), accessWrapper.chars_parsed()};
        }

        static constexpr priority_type priority() noexcept { return 0; }
    };

    using pending_variable_list = std::vector<variable_kind>;

    namespace {
        pending_variable_list const empty_pending_variable_list;
    }

    template<typename... Ts, typename = std::enable_if_t<(std::is_same_v<Ts, pending_variable_list> && ...)>>
    inline pending_variable_list merge_pending_variables(Ts const&... _lists) {
        pending_variable_list totalList = {};

        (std::invoke([&](auto const& list) { totalList.insert(end(totalList), begin(list), end(list)); }, _lists), ...);

        return totalList;
    }

    class expression {
    public:
        using number_type = long double;

        expression(expression const&) = delete;
        expression(expression&&) = delete;
        expression& operator=(expression const&) & = delete;
        expression& operator=(expression&&) & = delete;

        virtual std::unique_ptr<expression> copy() const = 0;

        virtual number_type eval() const = 0;

        virtual pending_variable_list pending_variables() const noexcept = 0;

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept = 0;

        virtual ~expression() noexcept = default;

    protected:
        explicit expression() noexcept = default;
    };

    class add_expression final : public expression {
    public:
        explicit add_expression(expression const& _left, expression const& _right) : m_left(_left.copy()), m_right(_right.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<add_expression>(*m_left, *m_right); }

        number_type eval() const final { return m_left->eval() + m_right->eval(); }

        virtual pending_variable_list pending_variables() const noexcept override {
            return merge_pending_variables(m_left->pending_variables(), m_right->pending_variables());
        }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_left->substitute_variable(_var, _value);
            m_right->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_left;
        std::unique_ptr<expression> m_right;
    };

    class subtract_expression final : public expression {
    public:
        explicit subtract_expression(expression const& _left, expression const& _right) : m_left(_left.copy()), m_right(_right.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<subtract_expression>(*m_left, *m_right); }

        number_type eval() const final { return m_left->eval() - m_right->eval(); }

        virtual pending_variable_list pending_variables() const noexcept override {
            return merge_pending_variables(m_left->pending_variables(), m_right->pending_variables());
        }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_left->substitute_variable(_var, _value);
            m_right->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_left;
        std::unique_ptr<expression> m_right;
    };

    class divide_expression final : public expression {
    public:
        explicit divide_expression(expression const& _left, expression const& _right) : m_left(_left.copy()), m_right(_right.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<divide_expression>(*m_left, *m_right); }

        number_type eval() const final { return m_left->eval() / m_right->eval(); }

        virtual pending_variable_list pending_variables() const noexcept override {
            return merge_pending_variables(m_left->pending_variables(), m_right->pending_variables());
        }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_left->substitute_variable(_var, _value);
            m_right->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_left;
        std::unique_ptr<expression> m_right;
    };

    class multiply_expression final : public expression {
    public:
        explicit multiply_expression(expression const& _left, expression const& _right) : m_left(_left.copy()), m_right(_right.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<multiply_expression>(*m_left, *m_right); }

        number_type eval() const final { return m_left->eval() * m_right->eval(); }

        virtual pending_variable_list pending_variables() const noexcept override {
            return merge_pending_variables(m_left->pending_variables(), m_right->pending_variables());
        }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_left->substitute_variable(_var, _value);
            m_right->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_left;
        std::unique_ptr<expression> m_right;
    };

    class unary_minus_expression final : public expression {
    public:
        explicit unary_minus_expression(expression const& _value) : m_value(_value.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<unary_minus_expression>(*m_value); }

        number_type eval() const final { return -(m_value->eval()); }

        virtual pending_variable_list pending_variables() const noexcept override { return m_value->pending_variables(); }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_value->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_value;
    };

    class integer_literal_expression final : public expression {
    public:
        explicit integer_literal_expression(number_type _value) : m_value(std::move(_value)) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<integer_literal_expression>(m_value); }

        number_type eval() const final { return m_value; }

        virtual pending_variable_list pending_variables() const noexcept override { return empty_pending_variable_list; }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {}

    private:
        number_type m_value;
    };

    class pi_literal_expression final : public expression {
    public:
        explicit pi_literal_expression() {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<pi_literal_expression>(); }

        number_type eval() const final { return number_type{1068966896} / number_type{340262731}; }

        virtual pending_variable_list pending_variables() const noexcept override { return empty_pending_variable_list; }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {}
    };

    class sin_expression final : public expression {
    public:
        explicit sin_expression(expression const& _arg) : m_argument(_arg.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<sin_expression>(*m_argument); }

        number_type eval() const final { return std::sin(m_argument->eval()); }

        virtual pending_variable_list pending_variables() const noexcept override { return m_argument->pending_variables(); }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_argument->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_argument;
    };

    class cos_expression final : public expression {
    public:
        explicit cos_expression(expression const& _arg) : m_argument(_arg.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<cos_expression>(*m_argument); }

        number_type eval() const final { return std::cos(m_argument->eval()); }

        virtual pending_variable_list pending_variables() const noexcept override { return m_argument->pending_variables(); }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_argument->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_argument;
    };

    class tan_expression final : public expression {
    public:
        explicit tan_expression(expression const& _arg) : m_argument(_arg.copy()) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<tan_expression>(*m_argument); }

        number_type eval() const final { return std::tan(m_argument->eval()); }

        virtual pending_variable_list pending_variables() const noexcept override { return m_argument->pending_variables(); }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            m_argument->substitute_variable(_var, _value);
        }

    private:
        std::unique_ptr<expression> m_argument;
    };

    class bad_variable_exception : public std::exception {
    public:
        explicit bad_variable_exception(variable_kind _var) : m_value(std::string("Bad access to variable: ") + variable_name(_var)) {}

        virtual char const* what() const noexcept override { return m_value.c_str(); }

    private:
        std::string m_value;
    };

    class variable_expression final : public expression {
    public:
        explicit variable_expression(variable_kind _var) : m_var{std::move(_var)}, m_value(std::nullopt) {}

        std::unique_ptr<expression> copy() const final { return std::make_unique<variable_expression>(m_var); }

        number_type eval() const final {
            if (not m_value.has_value()) throw bad_variable_exception(m_var);

            return *m_value;
        }

        virtual pending_variable_list pending_variables() const noexcept override { return {m_var}; }

        virtual void substitute_variable(variable_kind _var, number_type _value) noexcept override {
            if (_var == m_var) m_value = std::move(_value);
        }

    private:
        variable_kind m_var;
        std::optional<number_type> m_value;
    };

    class wrap_expression {
    public:
        wrap_expression(wrap_expression const& _other) : m_value(_other.raw().copy()) {}
        
        wrap_expression& operator=(wrap_expression _other) {
            swap(*this, _other);
            return *this;
        }
        
        wrap_expression(expression const& _value) : m_value(_value.copy()) {}

        using number_type = expression::number_type;

        [[nodiscard]] expression const& raw() const noexcept { return *m_value; }

        [[nodiscard]] number_type eval() const { return raw().eval(); }

        /* implicit */ operator expression const&() const noexcept { return raw(); }

        [[nodiscard]] decltype(auto) pending_variables() const noexcept { return raw().pending_variables(); }

        void substitute_variable(variable_kind _var, number_type _value) noexcept { mutable_raw().substitute_variable(_var, _value); }

    private:
        expression& mutable_raw() noexcept { return *m_value; }
        
        friend void swap(wrap_expression& _first, wrap_expression& _second) {
            using std::swap;
            swap(_first.m_value, _second.m_value);
        }
        
        std::unique_ptr<expression> m_value;
    };

    struct invalid_expression_t {};
    inline constexpr invalid_expression_t invalid_expression;

    class expression_grammar : p::grammar_base {
    public:
        explicit expression_grammar() noexcept = default;

        template<typename TokenStream>
        struct traits_for {
            using value_type = wrap_expression;
            using error_type = invalid_expression_t;
            using result_type = p::parse_result<value_type, error_type>;
        };

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const;
    };

    class primary_expression_grammar : p::grammar_base {
    public:
        explicit primary_expression_grammar() noexcept = default;

        template<typename TokenStream>
        struct traits_for {
            using value_type = wrap_expression;
            using error_type = invalid_expression_t;
            using result_type = p::parse_result<value_type, error_type>;
        };

        template<typename TokenStream>
        typename traits_for<TokenStream>::result_type test(TokenStream const& _tokenStream) const;
    };

    inline auto parenthesised_expression_grammar() {
        return p::map_value_grammar(p::sequence_grammar(token_kind_grammar(token_kind::lparen), expression_grammar(), token_kind_grammar(token_kind::rparen)),
                                    [](auto&& _value) { return p::get<1>(std::forward<decltype(_value)>(_value)); });
    }

    inline auto unary_minus_expression_grammar() {
        return p::map_value_grammar(p::sequence_grammar(token_kind_grammar(token_kind::minus), primary_expression_grammar()), [](auto&& _value) {
            return wrap_expression(unary_minus_expression(p::get<1>(std::forward<decltype(_value)>(_value))));
        });
    }

    inline auto integer_literal_expression_grammar() {
        return p::map_value_grammar(p::single_token_grammar([](token const& tok) { return token::is_integer_literal(tok); }), [](token const& tok) {
            return wrap_expression(integer_literal_expression(token::integer_literal_value(tok)));
        });
    }

    inline auto pi_literal_expression_grammar() {
        return p::map_value_grammar(token_kind_grammar(token_kind::kw_pi), [](auto&&) { return wrap_expression(pi_literal_expression()); });
    }

    inline auto function_expression_grammar() {
        return p::map_value_grammar(p::selection_grammar(p::map_value_grammar(p::sequence_grammar(token_kind_grammar(token_kind::kw_sin),
                                                                                                  parenthesised_expression_grammar()),
                                                                              [](auto const& seq) {
                                                                                  return wrap_expression(sin_expression(p::get<1>(seq)));
                                                                              }),
                                                         p::map_value_grammar(p::sequence_grammar(token_kind_grammar(token_kind::kw_cos),
                                                                                                  parenthesised_expression_grammar()),
                                                                              [](auto const& seq) {
                                                                                  return wrap_expression(cos_expression(p::get<1>(seq)));
                                                                              }),
                                                         p::map_value_grammar(p::sequence_grammar(token_kind_grammar(token_kind::kw_tan),
                                                                                                  parenthesised_expression_grammar()),
                                                                              [](auto const& seq) {
                                                                                  return wrap_expression(tan_expression(p::get<1>(seq)));
                                                                              })),
                                    [](auto const& variant) {
                                        return p::visit([](expression const& exp) { return wrap_expression(exp); }, variant);
                                    });
    }

    inline auto variable_expression_grammar() {
        return p::map_value_grammar(p::single_token_grammar([](token const& _tok) { return token::is_variable(_tok); }),
                                    [](auto const& _tok) { return wrap_expression(variable_expression(token::variable_value(_tok))); });
    }

    template<typename Tree>
    inline auto times_expression_tree_to_expression(Tree const& _tree) -> wrap_expression {
        if (not _tree.has_left()) return _tree.right();

        auto separator = p::visit([](token const& _token) { return _token; }, _tree.left_separator());
        switch (separator.kind()) {
            case token_kind::slash: return divide_expression(times_expression_tree_to_expression(_tree.left_tree()), _tree.right());
            case token_kind::star: return multiply_expression(times_expression_tree_to_expression(_tree.left_tree()), _tree.right());
        }

        __builtin_unreachable();
    }

    template<typename Tree>
    inline auto plus_expression_tree_to_expression(Tree const& _tree) -> wrap_expression {
        if (not _tree.has_left()) return _tree.right();

        auto separator = p::visit([](token const& _token) { return _token; }, _tree.left_separator());
        switch (separator.kind()) {
            case token_kind::plus: return add_expression(plus_expression_tree_to_expression(_tree.left_tree()), _tree.right());
            case token_kind::minus: return subtract_expression(plus_expression_tree_to_expression(_tree.left_tree()), _tree.right());
        }

        __builtin_unreachable();
    }


    inline auto times_expression_grammar() {
        return p::map_value_grammar(p::left_recursive_grammar(primary_expression_grammar(),
                                                              p::selection_grammar(token_kind_grammar(token_kind::star),
                                                                                   token_kind_grammar(token_kind::slash))),
                                    LIFT(times_expression_tree_to_expression));
    }

    inline auto plus_expression_grammar() {
        return p::map_value_grammar(p::left_recursive_grammar(times_expression_grammar(),
                                                              p::selection_grammar(token_kind_grammar(token_kind::plus),
                                                                                   token_kind_grammar(token_kind::minus))),
                                    LIFT(plus_expression_tree_to_expression));
    }

    template<typename TokenStream>
    typename primary_expression_grammar::traits_for<TokenStream>::result_type primary_expression_grammar::test(TokenStream const& _tokenStream) const {
        auto result = p::grammar_test(p::map_value_grammar(p::selection_grammar(parenthesised_expression_grammar(),
                                                                                unary_minus_expression_grammar(),
                                                                                integer_literal_expression_grammar(),
                                                                                pi_literal_expression_grammar(),
                                                                                function_expression_grammar(),
                                                                                variable_expression_grammar()),
                                                           [](auto const& variant) {
                                                               return p::visit([](wrap_expression exp) { return exp; }, variant);
                                                           }),
                                      _tokenStream);
        if (result.is_error()) return invalid_expression;

        auto amountParsed = result.amount_parsed();
        return {std::move(result).value(), amountParsed};
    }

    template<typename TokenStream>
    typename expression_grammar::traits_for<TokenStream>::result_type expression_grammar::test(TokenStream const& _tokenStream) const {
        auto result = p::grammar_test(plus_expression_grammar(), _tokenStream);

        if (result.is_error()) return invalid_expression;

        auto amountParsed = result.amount_parsed();
        return {wrap_expression(std::move(result).value()), std::move(amountParsed)};
    }
}    // namespace randomcat::simple_parsing

auto main() -> int {
    auto tokenizer = p::make_simple_tokenizer<token>(p::simple_token_descriptor(token(token_kind::lparen), "("),
                                                     p::simple_token_descriptor(token(token_kind::rparen), ")"),
                                                     p::simple_token_descriptor(token(token_kind::plus), "+"),
                                                     p::simple_token_descriptor(token(token_kind::minus), "-"),
                                                     p::simple_token_descriptor(token(token_kind::slash), "/"),
                                                     p::simple_token_descriptor(token(token_kind::star), "*"),
                                                     p::make_multi_form_token_descriptor(token(token_kind::whitespace), 1, " ", "\t", "\n", "\r"),
                                                     p::simple_token_descriptor(token(token_kind::kw_sin), "sin"),
                                                     p::simple_token_descriptor(token(token_kind::kw_cos), "cos"),
                                                     p::simple_token_descriptor(token(token_kind::kw_tan), "tan"),
                                                     p::simple_token_descriptor(token(token_kind::kw_pi), "pi"),
                                                     p::simple_token_descriptor(token::make_variable(variable_kind::theta), "theta"),
                                                     p::simple_token_descriptor(token::make_variable(variable_kind::theta_prime), "omega"),
                                                     p::simple_token_descriptor(token::make_variable(variable_kind::error), "e"),
                                                     integer_literal_token_descriptor(),
                                                     invalid_token_desc());

    auto fileInput = p::istream_inplace_char_source(std::ifstream("input.txt"));
    
    if (not fileInput.stream()) {
        std::cout << "Error opening file!\n";
        return -1;
    }
    
    auto processedTokens = strip_whitespace_token_stream(p::char_source_token_stream(std::move(fileInput), tokenizer));

    auto result = p::grammar_advance_if_matches(expression_grammar(), processedTokens);
    if (processedTokens.at_end() && result) {
        auto expr = result.value();

        expr.substitute_variable(variable_kind::theta, 999);
        
        std::cout << "Result: " << expr.eval();
    } else {
        std::cout << "Bad expression";
    }
}
