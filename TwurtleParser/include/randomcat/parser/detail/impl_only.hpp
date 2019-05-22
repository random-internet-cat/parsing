#pragma once

namespace randomcat::parser {
    struct impl_call_only {};
    inline auto constexpr impl_call = impl_call_only{};
}    // namespace randomcat::parser
