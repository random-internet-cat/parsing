#include <algorithm>
#include <any>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <randomcat/parser/chars/tokenizer.hpp>
#include <randomcat/parser/detail/util.hpp>
#include <randomcat/parser/tokens/token_stream/token_stream.hpp>

#include "randomcat/complex_parsing/lift.hpp"
#include "randomcat/complex_parsing/token.hpp"
#include "randomcat/complex_parsing/token_descriptors.hpp"
#include "randomcat/complex_parsing/token_manipulation.hpp"

namespace p = randomcat::parser;

using namespace randomcat::complex_parsing;

namespace randomcat::complex_parsing {
    auto keyword_parser(token_kind _kw, std::string _value) { return p::simple_token_descriptor(token(_kw), std::move(_value)); }

    template<typename TokenStream>
    auto strip_token_kind_token_stream(TokenStream _from, token_kind _kindToStrip) {
        return p::transform_token_stream(std::move(_from), [=](auto&& read, auto&& peek, auto&& at_end, auto&& emit) {
            auto token = read();
            if (token.kind() != _kindToStrip) emit(std::move(token));
        });
    }

    template<typename TokenStream>
    auto strip_from_kind_to_kind_token_stream(TokenStream _from, token_kind _firstStrip, token_kind _lastStrip) {
        return p::transform_token_stream(std::move(_from), [=](auto&& read, auto&& peek, auto&& at_end, auto&& emit) {
            auto token = read();
            if (token.kind() == _firstStrip) {
                while (not at_end() && read().kind() != _lastStrip) {}
            } else {
                emit(std::move(token));
            }
        });
    }

    template<typename TokenStream>
    auto strip_multiline_comments_token_stream(TokenStream _from) {
        return complex_parsing::strip_from_kind_to_kind_token_stream(std::move(_from), token_kind::multiline_comment_begin, token_kind::multiline_comment_end);
    }

    template<typename TokenStream>
    auto strip_line_comments_token_stream(TokenStream _from) {
        return complex_parsing::strip_from_kind_to_kind_token_stream(std::move(_from), token_kind::line_comment_begin, token_kind::line_comment_end);
    }

    template<typename TokenStream>
    auto strip_comments_token_stream(TokenStream _from) {
        return complex_parsing::strip_line_comments_token_stream(complex_parsing::strip_multiline_comments_token_stream(std::move(_from)));
    }

    template<typename TokenStream>
    auto strip_whitespace_token_stream(TokenStream _from) {
        return complex_parsing::strip_token_kind_token_stream(complex_parsing::strip_token_kind_token_stream(std::move(_from), token_kind::whitespace),
                                                              token_kind::newline);
    }
}    // namespace randomcat::complex_parsing

int main() {
#undef KW
    auto tokenizer = p::make_simple_tokenizer<token>(p::simple_token_descriptor(token(token_kind::colon_colon), "::"),
                                                     p::simple_token_descriptor(token(token_kind::lparen), "("),
                                                     p::simple_token_descriptor(token(token_kind::rparen), ")"),
                                                     p::simple_token_descriptor(token(token_kind::colon), ":"),
                                                     p::simple_token_descriptor(token(token_kind::carat), "^"),
                                                     p::simple_token_descriptor(token(token_kind::plus), "+"),
                                                     p::simple_token_descriptor(token(token_kind::plus_plus), "++"),
                                                     p::simple_token_descriptor(token(token_kind::minus), "-"),
                                                     p::simple_token_descriptor(token(token_kind::minus_minus), "--"),
                                                     p::simple_token_descriptor(token(token_kind::semicolon), ";"),
                                                     p::simple_token_descriptor(token(token_kind::slash), "/"),
                                                     p::simple_token_descriptor(token(token_kind::slash_slash), "//"),
                                                     p::simple_token_descriptor(token(token_kind::slash_star), "/*"),
                                                     p::simple_token_descriptor(token(token_kind::star_slash), "*/"),
                                                     p::simple_token_descriptor(token(token_kind::star), "*"),
                                                     p::simple_token_descriptor(token(token_kind::ampersand), "&"),
                                                     p::simple_token_descriptor(token(token_kind::ampersand_ampersand), "&&"),
                                                     p::simple_token_descriptor(token(token_kind::pipe), "|"),
                                                     p::simple_token_descriptor(token(token_kind::tilde), "~"),
                                                     p::simple_token_descriptor(token(token_kind::percentage), "%"),
                                                     p::simple_token_descriptor(token(token_kind::pipe_pipe), "||"),
                                                     p::simple_token_descriptor(token(token_kind::question_mark), "?"),
                                                     p::simple_token_descriptor(token(token_kind::backslash), "\\"),
                                                     p::simple_token_descriptor(token(token_kind::period), "."),
                                                     p::make_multi_form_token_descriptor(token(token_kind::whitespace), 1, " ", "\t"),
                                                     p::make_multi_form_token_descriptor(token(token_kind::newline), 1, "\n", "\r"),
                                                     identifier_token_desc(),
                                                     string_literal_token_desc(),
                                                     raw_string_literal_token_desc(),
                                                     invalid_token_desc(),

#define KW(x) (keyword_parser(token_kind::kw_##x, #x))
                                                     KW(asm),
                                                     KW(auto),
                                                     KW(bool),
                                                     KW(break),
                                                     KW(case),
                                                     KW(catch),
                                                     KW(char),
                                                     KW(char8_t),
                                                     KW(char16_t),
                                                     KW(class),
                                                     KW(const),
                                                     KW(const_cast),
                                                     KW(continue),
                                                     KW(default),
                                                     KW(delete),
                                                     KW(do),
                                                     KW(double),
                                                     KW(dynamic_cast),
                                                     KW(else),
                                                     KW(enum),
                                                     KW(explicit),
                                                     KW(extern),
                                                     KW(false),
                                                     KW(float));

    auto fileInput = p::istream_inplace_char_source(std::ifstream("input.txt"));
    //    auto strInput = p::string_char_source(std::string(R"parsing_test(
    //"Str First"
    //
    ////break do continue const_cast char16_t
    //
    //.between "Str second"
    //
    //    __0_? "This is? a\"\\\\\' string!"+ //comment ++++?
    //    +++(/*std::hello)+ ++;*/ R"( Raw string """""""" \ literal! )";
    //
    //
    //)parsing_test"));

    auto processedTokens = strip_whitespace_token_stream(strip_comments_token_stream(p::char_source_token_stream(std::move(fileInput), tokenizer)));

    bool readAny = true;
    auto head = processedTokens.head();

    while (readAny) {
        readAny = false;

        processedTokens.set_head(head);

        while (not processedTokens.at_end()) {
            readAny = true;

            auto tok = processedTokens.read();
            std::cout << token::name(tok) << '\n';
        }

        if (readAny) {
            processedTokens.set_head(head);
            processedTokens.read();
            head = processedTokens.head();
            std::cout << '\n';
        }
    }
}
