#pragma once

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#include "lexer.h"
#include "stream_wrapper.h"
#include "token.h"

namespace yajp {
class ParserUnit;
}

#include "parser_callback.h"
#include "parser_vertex.h"

using yajp::IStreamWrapper;
using yajp::Lexer;
using yajp::detail::ParserCallback;
using yajp::detail::ParserVertex;
using yajp::detail::StructureToken;

namespace yajp {

class ParserUnit {
    using ParserVariablePath = detail::ParserVariablePath;

public:
    Lexer &lexer_state;

    explicit ParserUnit(Lexer &, ParserUnit *);
    ParserUnit() = delete;

    void scan();

    template <typename T>
        requires std::is_same_v<T, bool>
    void add_variable_listener(T &, std::initializer_list<ParserVariablePath>);
    template <typename T>
        requires std::is_convertible_v<T, std::string>
    void add_variable_listener(T &, std::initializer_list<ParserVariablePath>);
    template <typename T>
        requires std::is_integral_v<T> && std::is_unsigned_v<T> && (!std::is_same_v<T, bool>)
    void add_variable_listener(T &, std::initializer_list<ParserVariablePath>);
    template <typename T>
        requires std::is_integral_v<T> && std::is_signed_v<T> && (!std::is_same_v<T, bool>)
    void add_variable_listener(T &, std::initializer_list<ParserVariablePath>);
    template <typename T>
        requires std::is_floating_point_v<T>
    void add_variable_listener(T &, std::initializer_list<ParserVariablePath>);

    void add_callback_listener(ParserCallback, std::initializer_list<ParserVariablePath>);

private:
    std::pair<bool, ParserVertex *> process_value(ParserVertex *, uint16_t, bool,
                                                  std::vector<StructureToken> &);
    ParserVertex *process_key(ParserVertex *, bool &, std::vector<StructureToken> &);

    ParserUnit *const prev_parser_unit_;
    ParserVertex *root_;
};

class JsonParser {
public:
    ParserUnit *const base_parser_unit;

    explicit JsonParser(IStreamWrapper &&);

private:
    friend ParserUnit;

    Lexer lexer_;
};

// the rest of functions are located in .cpp

template <typename T, TokenPattern pattern_token, Token type_token>
ParserCallback create_variable_parser_callback(T variable) {
    auto result = [&variable](const ParserUnit *parent) {
        parent->lexer_state.expect_token(pattern_token, true);
        variable = parent->lexer_state.get<type_token>();
    };
    return result;
}

template <typename T>
    requires std::is_same_v<T, bool>
void ParserUnit::add_variable_listener(T &variable,
                                       std::initializer_list<ParserVariablePath> path) {
    add_callback_listener(
        create_variable_parser_callback<T, TokenPattern::boolean, Token::boolean>(variable), path);
}

template <typename T>
    requires std::is_convertible_v<T, std::string>
void ParserUnit::add_variable_listener(T &variable,
                                       std::initializer_list<ParserVariablePath> path) {
    add_callback_listener(
        create_variable_parser_callback<T, TokenPattern::string, Token::string>(variable), path);
}

template <typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T> && (!std::is_same_v<T, bool>)
void ParserUnit::add_variable_listener(T &variable,
                                       std::initializer_list<ParserVariablePath> path) {
    add_callback_listener(
        create_variable_parser_callback<T, TokenPattern::unsigned_integer, Token::unsigned_number>(
            variable),
        path);
}

template <typename T>
    requires std::is_integral_v<T> && std::is_signed_v<T> && (!std::is_same_v<T, bool>)
void ParserUnit::add_variable_listener(T &variable,
                                       std::initializer_list<ParserVariablePath> path) {
    add_callback_listener(
        create_variable_parser_callback<T, TokenPattern::signed_integer, Token::signed_number>(
            variable),
        path);
}

template <typename T>
    requires std::is_floating_point_v<T>
void ParserUnit::add_variable_listener(T &variable,
                                       std::initializer_list<ParserVariablePath> path) {
    add_callback_listener(
        create_variable_parser_callback<T, TokenPattern::number, Token::float_number>(variable),
        path);
}
};  // namespace yajp
