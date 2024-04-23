
#ifndef ecflow_node_formatter_MirrorFormatter_hpp
#define ecflow_node_formatter_MirrorFormatter_hpp

#include "ecflow/node/formatter/Formatter.hpp"

namespace ecf {
namespace implementation {

template <typename Stream>
struct Formatter<MirrorAttr, Stream>
{
    static void format(const std::vector<MirrorAttr>& items, Stream& output) { format_vector_as_defs(items, output); }

    static void format(const MirrorAttr& item, Stream& output) {
        ecf::Indentor in;
        ecf::Indentor::indent(output);
        output << "mirror";
        output << " --name ";
        output << item.name();
        output << " --remote_path ";
        output << item.remote_path();
        output << " --remote_host ";
        output << item.remote_host();
        output << " --remote_port ";
        output << item.remote_port();
        output << " --polling ";
        output << item.polling();
        output << '\n';
    }
};

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_MirrorFormatter.hpp */
