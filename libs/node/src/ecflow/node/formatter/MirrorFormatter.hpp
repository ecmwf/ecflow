
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
        if (const auto& host = item.remote_host(); !host.empty() && host != MirrorAttr::default_remote_host) {
            output << " --remote_host ";
            output << item.remote_host();
        }
        if (const auto& port = item.remote_port(); !port.empty() && port != MirrorAttr::default_remote_port) {
            output << " --remote_port ";
            output << item.remote_port();
        }
        if (const auto& polling = item.polling(); !polling.empty() && polling != MirrorAttr::default_polling) {
            output << " --polling ";
            output << item.polling();
        }
        if (item.ssl()) {
            output << " --ssl";
        }
        if (const auto& auth = item.auth(); !auth.empty() && auth != MirrorAttr::default_remote_auth) {
            output << " --remote_auth ";
            output << item.auth();
        }
        if (const auto& reason = item.reason(); !reason.empty()) {
            output << " --reason ";
            output << item.reason();
        }
        output << '\n';
    }
};

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_MirrorFormatter.hpp */
