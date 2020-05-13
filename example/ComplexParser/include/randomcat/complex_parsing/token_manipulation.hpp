#pragma once

#include <algorithm>

#include <randomcat/parser/chars/tokenizer.hpp>

#include "randomcat/complex_parsing/token.hpp"

namespace randomcat::complex_parsing {
    template<typename ForwardIt>
    ForwardIt strip_comments(ForwardIt _begin, ForwardIt _end) noexcept {
        bool inLineComment = false;
        bool inMultiLineComment = false;

        return std::remove_if(_begin, _end, [&](auto const& token) {
            if (inMultiLineComment) {
                if (token.kind() == token_kind::multiline_comment_end) inMultiLineComment = false;

                return true;
            }

            if (inLineComment) {
                if (token.kind() == token_kind::line_comment_end) inLineComment = false;

                return true;
            }

            if (token.kind() == token_kind::line_comment_begin) {
                inLineComment = true;
                return true;
            }

            if (token.kind() == token_kind::multiline_comment_begin) {
                inMultiLineComment = true;
                return true;
            }

            return false;
        });
    }

    template<typename ForwardIt, typename Match, typename Combine>
    ForwardIt combine_consecutive_elements(ForwardIt _begin, ForwardIt _end, Match&& _match, Combine&& _combine) {
        if (std::distance(_begin, _end) == 0) return _end;

        auto sentinel = static_cast<token_kind>(std::numeric_limits<std::underlying_type_t<token_kind>>::max());

        for (; std::next(_begin, 1) != _end; std::advance(_begin, 1)) {
            auto firstIt = _begin;
            auto secondIt = std::next(firstIt, 1);

            if (std::forward<Match>(_match)(*firstIt, *secondIt)) {
                (*firstIt) = std::forward<Combine>(_combine)(*firstIt, *secondIt);
                (*secondIt) = token(sentinel);
            }
        }

        return std::remove_if(_begin, _end, [&](auto const& tok) { return tok.kind() == sentinel; });
    }

    template<typename ForwardIt>
    ForwardIt strip_comments_and_whitespace(ForwardIt _begin, ForwardIt _end) noexcept {
        _end = strip_comments(_begin, _end);
        _end = std::remove_if(_begin, _end, [](auto const& token) { return token.kind() == token_kind::whitespace; });
        _end = std::remove_if(_begin, _end, [](auto const& token) { return token.kind() == token_kind::newline; });

        return _end;
    }

    template<typename ForwardIt>
    ForwardIt combine_string_literals(ForwardIt _begin, ForwardIt _end) noexcept {
        return combine_consecutive_elements(_begin,
                                            _end,
                                            [](auto const& first, auto const& second) {
                                                return token::is_string_literal(first) && token::is_string_literal(second);
                                            },
                                            [](auto const& first, auto const& second) {
                                                return token::make_string_literal(token::string_literal_value(first) + token::string_literal_value(second));
                                            });
    }
}