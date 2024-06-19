
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
        if (const auto& url = item.url(); !url.empty() && url != AvisoAttr::default_url) {
            output << " --url ";
            output << item.url();
        }
        if (const auto& schema = item.schema(); !schema.empty() && schema != AvisoAttr::default_schema) {
            output << " --schema ";
            output << item.schema();
        }
        if (const auto& polling = item.polling(); !polling.empty() && polling != AvisoAttr::default_polling) {
            output << " --polling ";
            output << item.polling();
        }
        output << " --revision ";
        output << std::to_string(item.revision());
        if (const auto& auth = item.auth(); !auth.empty() && auth != AvisoAttr::default_auth) {
            output << " --auth ";
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

#endif /* ecflow_node_formatter_AvisoFormatter.hpp */
