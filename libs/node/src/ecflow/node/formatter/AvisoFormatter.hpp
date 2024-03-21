
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
        output << "aviso";
        output << " --name ";
        output << item.name();
        output << " --listener ";
        output << item.listener();
        output << " --url ";
        output << item.url();
        output << " --schema ";
        output << item.schema();
        output << " --revision ";
        output << std::to_string(item.revision());
        output << '\n';
    }
};

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_AvisoFormatter.hpp */
