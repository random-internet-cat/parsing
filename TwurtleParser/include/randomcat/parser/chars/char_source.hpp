#pragma once

#include "randomcat/parser/chars/char_parse_result.hpp"
#include "randomcat/parser/chars/detail/char_traits.hpp"
#include "randomcat/parser/detail/util.hpp"

namespace randomcat::parser {
    template<typename CharSource>
    struct char_source_traits {
        static_assert(std::is_move_constructible_v<CharSource>);

        using char_type = char_traits_detail::char_type_t<CharSource>;
        using char_traits_type = char_traits_detail::char_traits_type_t<CharSource>;
        using string_type = char_traits_detail::string_type_t<CharSource>;
        using string_view_type = char_traits_detail::string_view_type_t<CharSource>;
        using size_type = char_traits_detail::size_type_t<CharSource>;
        using location_type = char_traits_detail::location_type_t<CharSource>;

        // Only provided if source provides it
        template<typename CharSource_ = CharSource, typename = decltype(std::declval<CharSource_ const&>().chars_remaining())>
        static constexpr size_type chars_remaining(util_detail::no_deduce<CharSource_> const& _source) noexcept(noexcept(_source.chars_remaining())) {
            return _source.chars_remaining();
        }

        static constexpr bool at_end(CharSource const& _source) noexcept(noexcept(_source.at_end())) { return _source.at_end(); }

        static constexpr void advance_head(CharSource& _source, size_type _n) noexcept(noexcept(_source.advance_head(_n))) {
            _source.advance_head(_n);
        }

        static constexpr location_type head(CharSource const& _source) noexcept(noexcept(_source.head())) { return _source.head(); }

        static constexpr void set_head(CharSource& _source, location_type _head) noexcept(noexcept(_source.set_head(_head))) {
            return _source.set_head(_head);
        }

        static inline constexpr auto __has_peek = char_traits_detail::has_peek_v<CharSource const&, size_type>;
        static inline constexpr auto __has_peek_char = char_traits_detail::has_peek_char_v<CharSource const&>;

        template<typename CharSource_ = CharSource>
        static constexpr string_type peek(util_detail::no_deduce<CharSource_> const& _source, size_type _n) {
            if constexpr (__has_peek) {
                return _source.peek(_n);
            } else {
                return access_wrapper(_source).read(_n);
            }
        }

        template<typename CharSource_ = CharSource>
        static constexpr char_type peek_char(util_detail::no_deduce<CharSource_> const& _source) {
            if constexpr (__has_peek_char) {
                return _source.peek_char();
            } else {
                return char_source_traits::peek(_source, 1)[0];
            }
        }

        static inline constexpr auto __has_read = char_traits_detail::has_read_v<CharSource&, size_type>;
        static inline constexpr auto __has_read_char = char_traits_detail::has_read_char_v<CharSource&>;

        static_assert(__has_read || __has_peek || __has_read_char || __has_peek_char);

        template<typename CharSource_ = CharSource>
        static constexpr string_type read(util_detail::no_deduce<CharSource_>& _source, size_type _n) {
            if constexpr (__has_read) {
                return _source.read(_n);
            } else if constexpr (__has_peek) {
                auto str = char_source_traits::peek(_source, _n);
                char_source_traits::advance_head(_source, size(str));
                return str;
            } else {
                string_type str;
                str.reserve(_n);
                for (size_type i = 0; i < _n; ++i) {
                    if (char_source_traits::at_end(_source)) return str;
                    str += char_source_traits::read_char(_source);
                }
            }
        }

        template<typename CharSource_ = CharSource>
        static constexpr char_type read_char(util_detail::no_deduce<CharSource_>& _source) {
            if constexpr (__has_read_char) {
                return _source.read_char();
            } else if constexpr (__has_peek_char) {
                char_type c = char_source_traits::peek_char(_source);
                char_source_traits::advance_head(_source, 1);
                return c;
            } else {
                return char_source_traits::read(_source, 1)[0];
            }
        }

        class access_wrapper {
        public:
            access_wrapper(access_wrapper const&) = default;
            access_wrapper& operator=(access_wrapper const&) = delete;

            access_wrapper(access_wrapper&&) = delete;
            access_wrapper& operator=(access_wrapper&&) = delete;

            access_wrapper(CharSource const& _source) : m_source(_source), m_startHead(char_source_traits::head(m_source.get())) {}

            ~access_wrapper() noexcept { char_source_traits::set_head(as_mutable(), m_startHead); }

            auto peek(size_type _n) const noexcept(noexcept(char_source_traits::peek(as_immutable(), _n))) {
                return char_source_traits::peek(as_immutable(), _n);
            }

            auto read(size_type _n) noexcept(noexcept(char_source_traits::read(as_mutable(), _n))) {
                auto strRead = char_source_traits::read(as_mutable(), _n);
                m_charsParsed += size(strRead);
                return strRead;
            }

            auto at_end() const noexcept(noexcept(char_source_traits::at_end(as_immutable()))) {
                return char_source_traits::at_end(as_immutable());
            }

            auto peek_char() const noexcept(noexcept(char_source_traits::peek_char(as_immutable()))) {
                return char_source_traits::peek_char(as_immutable());
            }

            auto read_char() noexcept(noexcept(char_source_traits::read_char(as_mutable()))) {
                m_charsParsed += 1;
                return char_source_traits::read_char(as_mutable());
            }

            void advance_head(size_type _n) noexcept(noexcept(char_source_traits::advance_head(as_mutable(), _n))) {
                m_charsParsed += _n;
                char_source_traits::advance_head(as_mutable(), _n);
            }

            location_type head() const noexcept(noexcept(char_source_traits::head(as_immutable()))) {
                return char_source_traits::head(as_immutable());
            }

            bool next_is(string_view_type _str) const noexcept(noexcept(at_end()) && noexcept(peek(size(_str)))) {
                if (at_end()) return false;

                auto const readStr = peek(size(_str));
                return readStr == _str;
            }

            bool next_is(char_type _c) const noexcept(noexcept(at_end()) && noexcept(peek_char())) {
                if (at_end()) return false;

                auto const readChar = peek_char();
                return readChar == _c;
            }

            char_parse_result<void, failed_expectation_t> expect(string_view_type _str) noexcept(noexcept(next_is(_str))
                                                                                                 && noexcept(advance_head(std::declval<size_type>()))) {
                if (next_is(_str)) {
                    size_type strlen = size(_str);
                    advance_head(strlen);
                    return strlen;
                }

                return failed_expectation;
            }

            char_parse_result<void, failed_expectation_t> expect(char_type _c) noexcept(noexcept(next_is(_c))
                                                                                        && noexcept(advance_head(std::declval<size_type>()))) {
                if (next_is(_c)) {
                    size_type strlen = 1;
                    advance_head(strlen);
                    return strlen;
                }

                return failed_expectation;
            }

            size_type chars_parsed() const noexcept { return m_charsParsed; }

            template<typename F>
            decltype(auto) sub_parse(F&& _f) const noexcept(noexcept(std::forward<F>(_f)(as_immutable()))) {
                return std::forward<F>(_f)(as_immutable());
            }

        private:
            CharSource& as_mutable() const noexcept { return const_cast<CharSource&>(as_immutable()); }
            CharSource const& as_immutable() const noexcept { return m_source.get(); }

            std::reference_wrapper<CharSource const> m_source;
            size_type m_charsParsed;
            location_type m_startHead;
        };
    };

    namespace char_source_detail {
        template<typename Derived, typename CharT, typename Traits>
        class base_istream_char_source {
        public:
            using stream_type = std::basic_istream<CharT, Traits>;
            using pos_type = typename stream_type::pos_type;
            using int_type = typename stream_type::int_type;

            using char_type = CharT;
            using char_traits_type = Traits;
            using string_type = std::basic_string<char_type, char_traits_type>;
            using string_view_type = std::basic_string_view<char_type, char_traits_type>;
            using size_type = typename string_type::size_type;
            using location_type = pos_type;

            base_istream_char_source() = default;

            void advance_head(size_type _n) noexcept { stream().ignore(_n); }

            bool at_end() const noexcept { return peek_int_type() == char_traits_type::eof(); }

            location_type head() const noexcept { return stream().tellg(); }

            void set_head(location_type _head) noexcept { stream().seekg(_head); }

            char_type peek_char() const noexcept { return gsl::narrow<char_type>(peek_int_type()); }

            char_type read_char() noexcept { return gsl::narrow<char_type>(stream().get()); }

            string_type read(size_type _n) noexcept {
                auto charArr = std::make_unique<char_type[]>(_n);
                stream().read(charArr.get(), _n);
                stream().clear(stream().rdstate() & ~std::ios_base::failbit);
                return string_type(charArr.get(), (charArr.get()) + stream().gcount());
            }

        private:
            stream_type& stream() const noexcept { return static_cast<Derived const*>(this)->stream(); }

            int_type peek_int_type() const noexcept { return stream().peek(); }
        };
    }    // namespace char_source_detail

    template<typename CharT, typename Traits>
    class istream_ref_char_source : public char_source_detail::base_istream_char_source<istream_ref_char_source<CharT, Traits>, CharT, Traits> {
    public:
        using stream_type = std::basic_istream<CharT, Traits>;

        istream_ref_char_source(istream_ref_char_source const&) = delete;
        istream_ref_char_source(istream_ref_char_source&&) noexcept = default;

        istream_ref_char_source& operator=(istream_ref_char_source const&) & = delete;
        istream_ref_char_source& operator=(istream_ref_char_source&&) & noexcept = default;

        istream_ref_char_source(stream_type& _stream) : m_stream(_stream) {}

        stream_type& stream() const noexcept { return m_stream; }

    private:
        std::reference_wrapper<stream_type> m_stream;
    };

    template<typename Stream>
    class istream_inplace_char_source :
    public char_source_detail::base_istream_char_source<istream_inplace_char_source<Stream>, typename Stream::char_type, typename Stream::traits_type> {
    public:
        using stream_type = Stream;

        istream_inplace_char_source(stream_type _stream) : m_stream(std::move(_stream)) {}

        stream_type& stream() const noexcept { return m_stream; }

    private:
        mutable stream_type m_stream;
    };

    template<typename CharT, typename Traits>
    class string_char_source {
    public:
        using char_type = CharT;
        using char_traits_type = Traits;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;
        using size_type = typename string_type::size_type;
        using location_type = size_type;

        static_assert(std::is_same_v<typename string_type::size_type, typename string_view_type::size_type>);

        string_char_source(string_type _string) noexcept : m_string(std::move(_string)) {}

        bool at_end() const noexcept { return m_head == size(m_string); }
        location_type head() const noexcept { return m_head; }
        void set_head(location_type _head) noexcept { m_head = std::move(_head); }

        string_type peek(size_type _n) const noexcept { return m_string.substr(m_head, _n); }
        char_type peek_char() const noexcept { return m_string[head()]; }

        size_type chars_remaining() const noexcept { return size(m_string) - m_head; }

        void advance_head(size_type _n) noexcept { m_head += _n; }

    private:
        size_type m_head = 0;
        string_type m_string;
    };
}    // namespace randomcat::parser