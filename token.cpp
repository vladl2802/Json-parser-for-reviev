#include <cassert>
#include <variant>

#include "token.h"

using yajp::detail::StructureToken;
using yajp::detail::ParserVariablePath;

StructureToken::StructureToken(const ParserVariablePath & element) {
    value_ = std::visit(
        [](auto&& arg) -> StructureToken {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, uint32_t>) {
                return StructureToken::array;
            } else if constexpr (std::is_convertible_v<T, std::string>) {
                return StructureToken::object;
            } else if constexpr (std::is_same_v<T, detail::StructureParsingFlag>) {
                return StructureToken::incomplete;
            } else {
                assert(0);
            }
        },
        element);
}

yajp::detail::StructureToken::StructureToken(Token token) {
    switch (token) {
        case Token::begin_array:
            [[fallthrough]];
        case Token::end_array:
            value_ = StructureToken::array;
            break;
        case Token::begin_object:
            [[fallthrough]];
        case Token::end_object:
            value_ = StructureToken::object;
            break;
        default:
            assert(false);
    }
}

StructureToken::StructureToken(tokens token) : value_(token) {
}

yajp::detail::StructureToken::operator tokens() const {
    return value_;
}
