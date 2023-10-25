#pragma once

#include <cstdint>
#include <type_traits>
#include <variant>
#include <string>

namespace yajp {
enum Token : uint32_t {
    boolean = 1,
    null = 2,
    unsigned_number = 4,
    signed_number = 8,
    float_number = 16,
    string = 32,
    begin_array = 64,
    end_array = 128,
    begin_object = 256,
    end_object = 512,
    name_delimiter = 1024,
    element_delimiter = 2048,
    eof = 4096,
    error = 8192
};

namespace detail {
enum class StructureParsingFlag { for_each };
using ParserVariablePath = std::variant<uint32_t, std::string, const char *, StructureParsingFlag>;

class StructureToken {
public:
    enum tokens {
        array,
        object,
        callback,
        incomplete
    };

    StructureToken(tokens);
    StructureToken(const ParserVariablePath&);
    StructureToken(Token);
    operator tokens() const;    

private:
    tokens value_;
};
}  // namespace detail

enum class TokenPattern : uint32_t {
    begin = Token::begin_array | Token::begin_object,
    end = Token::end_array | Token::end_object,

    boolean = Token::boolean,

    null = Token::null,

    unsigned_integer = Token::signed_number,
    signed_integer = Token::unsigned_number | unsigned_integer,
    number = Token::float_number | signed_integer,

    string = Token::string,

    value = begin | boolean | null | number | string,
};

template <typename T, typename P>
    requires std::is_same_v<T, Token> || std::is_same_v<T, TokenPattern> ||
             std::is_same_v<T, uint32_t>
constexpr uint32_t operator|(T lhs, P rhs) {
    return static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs);
}

inline constexpr bool token_pattern_check(Token token, TokenPattern pat) {
    return static_cast<uint32_t>(token) & static_cast<uint32_t>(pat);
}
}  // namespace yajp