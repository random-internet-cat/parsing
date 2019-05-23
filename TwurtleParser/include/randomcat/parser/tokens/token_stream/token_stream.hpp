#pragma once

#include "randomcat/parser/chars/char_source.hpp"
#include "randomcat/parser/detail/util.hpp"
#include "randomcat/parser/tokens/detail/token_traits.hpp"
//#include "randomcat/parser/chars/"

namespace randomcat::parser {
    template<typename TokenStream>
    class token_stream_traits {
    public:
        static_assert(util_detail::is_simple_type_v<TokenStream>);
        static_assert(std::is_move_constructible_v<TokenStream>);

        using token_type = token_traits_detail::token_type_t<TokenStream>;
        using location_type = token_traits_detail::location_type_t<TokenStream>;
        using size_type = token_traits_detail::size_type_t<TokenStream>;

        static token_type advance(TokenStream& _stream) noexcept(noexcept(_stream.advance())) { return _stream.advance(); }

        static token_type peek(TokenStream const& _stream) noexcept(noexcept(_stream.peek())) { return _stream.peek(); }

        static location_type head(TokenStream const& _stream) noexcept(noexcept(_stream.head())) { return _stream.head(); }

        static void set_head(TokenStream& _stream, location_type _head) noexcept(noexcept(_stream.set_head(std::move(_head)))) {
            _stream.set_head(std::move(_head));
        }

        static bool at_end(TokenStream const& _stream) noexcept(noexcept(_stream.at_end())) { return _stream.at_end(); }

        class access_wrapper {
        public:
            access_wrapper(access_wrapper const&) = delete;
            access_wrapper(access_wrapper&&) = delete;

            access_wrapper& operator=(access_wrapper const&) & = delete;
            access_wrapper& operator=(access_wrapper&&) & = delete;

            access_wrapper(TokenStream const& _source) noexcept : m_source(_source), m_startHead(token_stream_traits::head(_source)) {}

            ~access_wrapper() noexcept { token_stream_traits::set_head(m_source, std::move(m_startHead)); }

            token_type peek() const { return token_stream_traits::peek(m_source); }

            token_type advance() { return token_stream_traits::advance(m_source); }

        private:
            location_type m_startHead;
            TokenStream const& m_source;
        };
    };

    inline std::string token_stream_no_token_message = "Unable to fetch next token in a token stream!";

    template<typename ErrorType>
    class token_stream_no_token : public std::exception {
    public:
        static_assert(util_detail::is_simple_type_v<ErrorType>);

        token_stream_no_token(ErrorType _error) : m_error(std::move(_error)) {}

        virtual char const* what() const noexcept override { return token_stream_no_token_message.c_str(); }

        auto const& error() const noexcept { return m_error; }

    private:
        ErrorType m_error;
    };

    template<>
    class token_stream_no_token<void> : public std::exception {
    public:
        virtual char const* what() const noexcept override { return token_stream_no_token_message.c_str(); }
    };

    template<typename CharSource, typename Tokenizer>
    class char_source_token_stream {
    public:
        static_assert(util_detail::is_simple_type_v<CharSource>);
        static_assert(util_detail::is_simple_type_v<Tokenizer>);

        char_source_token_stream(CharSource _charSource, Tokenizer _tokenizer)
        : m_charSource(std::move(_charSource)), m_tokenizer(std::move(_tokenizer)) {}

        using token_type = typename tokenizer_traits<Tokenizer>::token_type;
        using location_type = typename char_source_traits<CharSource>::location_type;

        token_type advance() {
            auto parseResult = do_parse();
            throw_if_empty(parseResult);

            char_source_traits<CharSource>::advance_head(m_charSource, parseResult.amount_parsed());
            return std::move(parseResult).value();
        }

        token_type peek() const {
            auto parseResult = do_parse();
            throw_if_empty(parseResult);

            return std::move(parseResult).value();
        }

        bool at_end() const noexcept { return char_source_traits<CharSource>::at_end(m_charSource); }

        location_type head() const noexcept { return char_source_traits<CharSource>::head(m_charSource); }

        void set_head(location_type _head) noexcept { char_source_traits<CharSource>::set_head(m_charSource, std::move(_head)); }

    private:
        using tokenizer_error_type = typename tokenizer_traits<Tokenizer>::error_type;
        using tokenizer_parse_result_type = typename tokenizer_traits<Tokenizer>::parse_result_type;

        tokenizer_parse_result_type do_parse() const noexcept {
            return tokenizer_traits<Tokenizer>::parse_first_token(m_tokenizer, m_charSource);
        }

        static void throw_if_empty(tokenizer_parse_result_type const& _result) {
            if (_result.is_error()) throw token_stream_no_token<tokenizer_error_type>(_result.error());
        }

        CharSource m_charSource;
        Tokenizer m_tokenizer;
    };

    template<typename FromSource, typename Transform>
    class transform_token_stream {
    private:
        using derived_location_type = typename token_stream_traits<FromSource>::location_type;

    public:
        using token_type = typename token_stream_traits<FromSource>::token_type;
        using size_type = default_size_type;

        static_assert(util_detail::is_simple_type_v<FromSource>);
        static_assert(std::is_same_v<typename token_stream_traits<FromSource>::size_type, size_type>);

        transform_token_stream(FromSource _fromSource, Transform _transform)
        : m_transform(std::move(_transform)), m_fromSource(std::move(_fromSource)), m_location{token_stream_traits<FromSource>::head(_fromSource), 0} {
            fetch_tokens_if_needed();
        }

        struct location_type {
            derived_location_type fromSourceLocation;
            size_type subTokenIndex;
        };

        location_type head() const noexcept { return m_location; }

        void set_head(location_type _location) {
            m_location = std::move(_location);
            char_source_traits<FromSource>::set_head(_location.fromSourceLocation);
            m_pendingTokens.clear();

            // We assume that going back to a previous head will yield the same token sequence.
            // Thus, for any pending token index that we get, it must have been valid before.
            fetch_tokens_once();
        }

        token_type peek() const { return m_pendingTokens[m_location.subTokenIndex]; }

        token_type advance() {
            token_type token = peek();

            ++(m_location.subTokenIndex);
            fetch_tokens_if_needed();

            return token;
        }

        bool at_end() const noexcept {
            return token_stream_traits<FromSource>::at_end(m_fromSource) && m_location.subTokenIndex == size(m_pendingTokens);
        }

    private:
        void fetch_tokens_once() {
            if (not token_stream_traits<FromSource>::at_end(m_fromSource))
                m_transform([this] { return token_stream_traits<FromSource>::advance(m_fromSource); },
                            [this] { return token_stream_traits<FromSource>::peek(m_fromSource); },
                            [this] { return token_stream_traits<FromSource>::at_end(m_fromSource); },
                            [this](token_type token) { m_pendingTokens.push_back(token); });
        }

        void fetch_tokens_if_needed() {
            // First iteration will check if we have just exceeded the pending tokens list
            // Any subsequent iterations will occur only if no tokens were emitted by the transformer

            while (m_location.subTokenIndex == size(m_pendingTokens)) {
                m_location = {token_stream_traits<FromSource>::head(m_fromSource), 0};
                m_pendingTokens.clear();

                if (token_stream_traits<FromSource>::at_end(m_fromSource)) return;

                fetch_tokens_once();
            }
        }

        Transform m_transform;
        std::vector<token_type> m_pendingTokens;
        location_type m_location;
        FromSource m_fromSource;
    };
}    // namespace randomcat::parser