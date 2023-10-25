#include "json_parser.h"

#include <type_traits>

#include "parser_vertex.h"
#include "lexer.h"

using yajp::JsonParser;
using yajp::ParserUnit;
using yajp::Token;
using yajp::TokenPattern;
using yajp::detail::ParserCallback;
using yajp::detail::ParserVertex;
using yajp::detail::StructureToken;

yajp::ParserUnit::ParserUnit(Lexer& lexer_state, ParserUnit* prev_parser_unit)
    : lexer_state(lexer_state), prev_parser_unit_(prev_parser_unit) {
    root_ = new ParserVertex(StructureToken::object, nullptr, ParserCallback{});
}

void ParserUnit::add_callback_listener(ParserCallback callback,
                                       std::initializer_list<ParserVariablePath> path) {
    ParserVertex* prev = nullptr;
    ParserVertex** cur = &root_;
    for (const auto& path_element : path) {
        assert(cur != nullptr && "Parsing tree constructing arrived in non-initialized vertex");
        auto type = StructureToken(path_element);
        if (*cur == nullptr) {
            *cur = new ParserVertex(type, prev, ParserCallback{});
        } else if ((*cur)->type == StructureToken::incomplete &&
                   type != StructureToken::incomplete) {
            (*cur)->make_complete(type);
        }
        prev = *cur;
        cur = std::visit(
            [&path_element](auto& arg) -> ParserVertex** {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, detail::ArrayParserVertex>) {
                    uint32_t index = std::get<uint32_t>(path_element);
                    return &(arg.to_go[index]);
                } else if constexpr (std::is_same_v<T, detail::ObjectParserVertex>) {
                    std::string name;
                    if (std::holds_alternative<const char*>(path_element)) {
                        name = std::string{get<const char*>(path_element)};
                    } else {
                        name = get<std::string>(path_element);
                    }
                    return &(arg.to_go[name]);
                } else if constexpr (std::is_same_v<T, detail::IncompleteParserVertex>) {
                    return &(arg.go_for_each);
                } else {
                    assert(0 && "Can't visit callback vertex during parsing tree construction");
                }
            },
            (*cur)->self);
    }
    assert(*cur == nullptr);  // TODO: replace with exception
    *cur = new ParserVertex(StructureToken::callback, nullptr, std::move(callback));
}

std::pair<bool, ParserVertex*> yajp::ParserUnit::process_value(
    ParserVertex* cur, uint16_t ignore_depth, bool to_ignore,
    std::vector<StructureToken>& structure_stack) {
    bool start_nesting = false;
    ParserVertex* result = cur;
    if (to_ignore) {
        Token token = lexer_state.expect_token(TokenPattern::value);
        if (yajp::token_pattern_check(token, TokenPattern::begin)) {
            structure_stack.push_back(StructureToken(token));
            ignore_depth++;
            start_nesting = true;
        } else {
            if (ignore_depth == 0) {
                to_ignore = false;
            }
        }
    } else {
        switch (cur->type) {
            case StructureToken::callback: {
                auto pu = new ParserUnit(lexer_state, this);
                std::get<detail::CallbackParserVertex>(cur->self).callback(pu);
                delete pu;
                result = cur->prev;  // after invoking callback we need to step back from its vertex
                break;
            }
            case StructureToken::array:
                start_nesting = true;
                lexer_state.expect_token(Token::begin_array);
                structure_stack.push_back(StructureToken::array);
                break;
            case StructureToken::object:
                start_nesting = true;
                lexer_state.expect_token(Token::begin_object);
                structure_stack.push_back(StructureToken::object);
                break;
            default:
                assert(false);  // TODO: replace with exception
        }
    }
    return {start_nesting, result};
}

ParserVertex* yajp::ParserUnit::process_key(ParserVertex* cur, bool& to_ignore,
                                            std::vector<StructureToken>& structure_stack) {
    static auto jump = []<typename T>(detail::NestingParserVertex<T>& ver,
                                      const T& name) -> detail::ParserVertex*& {
        if (ver.to_go.find(name) == ver.to_go.end()) {
            return ver.go_for_each;
        } else {
            return ver.to_go[name];
        }
    };
    switch (structure_stack.back()) {
        case StructureToken::array:
            if (!to_ignore) {
                auto& ver = std::get<detail::ArrayParserVertex>(cur->self);
                auto& index = ver.last;
                auto& result = jump(ver, index);
                if (result == nullptr) {
                    to_ignore = true;
                    result = cur;
                }
                index++;
                return result;
            } else {
                return cur;
            }
        case StructureToken::object:
            lexer_state.expect_token(Token::string, !to_ignore);
            lexer_state.expect_token(Token::name_delimiter);
            if (!to_ignore) {
                auto& ver = std::get<detail::ObjectParserVertex>(cur->self);
                auto name = lexer_state.get_string();
                auto& result = jump(ver, name);
                if (result == nullptr) {
                    to_ignore = true;
                    result = cur;
                } else {
                    ver.last = name;
                }
                return result;
            } else {
                return cur;
            }
        default:
            assert(0 && "Prohibited structure token");
    }
}

void ParserUnit::scan() {
    bool to_ignore = false;
    uint16_t ignore_depth = 0;
    std::vector<StructureToken> structure_stack;  // only for validating
    ParserVertex* cur = root_;
    while (cur != nullptr) {
        auto result = process_value(cur, ignore_depth, to_ignore, structure_stack);
        cur = result.second;

        if (!result.first) {
            while (cur != nullptr) {
                Token token =
                    lexer_state.expect_token(TokenPattern::end | Token::element_delimiter);
                if (token == Token::element_delimiter) {
                    break;
                } else {
                    assert(structure_stack.back() ==
                           StructureToken(token));  // TODO: replace with exception
                    structure_stack.pop_back();
                    if (to_ignore) {
                        ignore_depth--;
                        if (ignore_depth == 0) {
                            to_ignore = false;
                        }
                    } else {
                        cur = cur->prev;
                    }
                }
            }
            if (cur == nullptr) {
                break;
            }
        }
        cur = process_key(cur, to_ignore, structure_stack);
    }
}

JsonParser::JsonParser(IStreamWrapper&& is)
    : base_parser_unit(new ParserUnit(lexer_, nullptr)), lexer_(std::move(is)) {
}
