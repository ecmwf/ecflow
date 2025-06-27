/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

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
    }
};

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_AvisoFormatter.hpp */
