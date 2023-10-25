#include <cassert>

#include "parser_vertex.h"
#include "token.h"

namespace yajp {
namespace detail {
ParserVertex::ParserVertex(StructureToken type, ParserVertex* prev, ParserCallback callback)
    : type(type), prev(prev), self(create_parser_vertex(type, nullptr, std::move(callback))) {
}

void ParserVertex::make_complete(StructureToken type) {
    assert(this->type == StructureToken::incomplete && "Only incomplete vertex can be completed");
    assert((type == StructureToken::array || type == StructureToken::object) && "Vertex can be completed only to array or object");
    self = create_parser_vertex(type, std::get<IncompleteParserVertex>(self).go_for_each,
                                ParserCallback{});
}

CallbackParserVertex::CallbackParserVertex(ParserCallback callback)
    : callback(std::move(callback)) {
}

IncompleteParserVertex::IncompleteParserVertex(ParserVertex* go_for_each)
    : go_for_each(std::move(go_for_each)) {
}

ParserVertexConcept create_parser_vertex(StructureToken type, ParserVertex* go_for_each,
                                         ParserCallback callback) {
    switch (type) {
        case StructureToken::array:
            return ArrayParserVertex(go_for_each, {}, 0);
        case StructureToken::object:
            return ObjectParserVertex(go_for_each, {}, "");
        case StructureToken::incomplete:
            return IncompleteParserVertex(go_for_each);
        case StructureToken::callback:
            return CallbackParserVertex(std::move(callback));
    }
    assert(0);
}

}  // namespace detail
}  // namespace yajp
