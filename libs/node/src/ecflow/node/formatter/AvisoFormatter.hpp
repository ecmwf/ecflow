// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
        if (item.collapse()) {
            output << " --collapse";
        }
    }
};

} // namespace implementation
} // namespace ecf
