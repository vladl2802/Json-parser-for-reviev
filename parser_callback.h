#include <functional>

namespace yajp {
class ParserUnit;
namespace detail {
using ParserCallback = std::function<void(const ParserUnit*)>;
}
}  // namespace yajp