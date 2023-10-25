#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include "token.h"
#include "parser_callback.h"

using yajp::detail::ParserCallback;

namespace yajp {
namespace detail {

struct ParserVertex;

struct IncompleteParserVertex {
    ParserVertex* go_for_each;

    IncompleteParserVertex(ParserVertex*);

    ~IncompleteParserVertex() noexcept = default;
};

template <typename T>
struct NestingParserVertex {
    std::unordered_map<T, ParserVertex*> to_go;
    ParserVertex* go_for_each;
    T last;

    NestingParserVertex(IncompleteParserVertex);
    NestingParserVertex(ParserVertex*, std::unordered_map<T, ParserVertex*>, T);

    ~NestingParserVertex() noexcept = default;
};

using ArrayParserVertex = NestingParserVertex<uint32_t>;
using ObjectParserVertex = NestingParserVertex<std::string>;

struct CallbackParserVertex {
    // std::weak_ptr<ParserUnit> last;
    ParserCallback callback;

    CallbackParserVertex(ParserCallback);

    ~CallbackParserVertex() noexcept = default;
};

using ParserVertexConcept = std::variant<IncompleteParserVertex, ArrayParserVertex,
                                         ObjectParserVertex, CallbackParserVertex>;

struct ParserVertex {
    StructureToken type;
    ParserVertex* prev;
    ParserVertexConcept self;

    ParserVertex(StructureToken, ParserVertex*, ParserCallback);
    
    void make_complete(StructureToken);
};

ParserVertexConcept create_parser_vertex(StructureToken, ParserVertex*, ParserCallback);

template <typename T>
inline NestingParserVertex<T>::NestingParserVertex(IncompleteParserVertex base)
    : to_go({}), go_for_each(base.go_for_each), last(T()) {
}

template <typename T>
NestingParserVertex<T>::NestingParserVertex(ParserVertex* go_for_each,
                                            std::unordered_map<T, ParserVertex*> to_go, T last)
    : to_go(to_go), go_for_each(go_for_each), last(last) {
}
}  // namespace detail
}  // namespace yajp