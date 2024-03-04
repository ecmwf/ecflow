
#ifndef ecflow_node_formatter_AvisoFormatter_hpp
#define ecflow_node_formatter_AvisoFormatter_hpp

#include "ecflow/node/formatter/Formatter.hpp"

namespace ecf {
namespace implementation {

template <typename Stream>
struct Formatter<AvisoAttr, Stream>
{
    static void format(const std::vector<AvisoAttr>& items, Stream& output) { format_vector_as_defs(items, output); }

    static void format(const AvisoAttr& item, Stream& output) {
        ecf::Indentor in;
        ecf::Indentor::indent(output);
        output << "aviso ";
        output << item.name();
        output << " ";
        output << item.listener();
        output << '\n';
    }
};

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_AvisoFormatter.hpp */