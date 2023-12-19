/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_formatter_AvisoFormatter_HPP
#define ecflow_node_formatter_AvisoFormatter_HPP

#include "ecflow/attribute/AvisoAttr.hpp"
#include "ecflow/core/Indentor.hpp"
#include "ecflow/node/formatter/Formatter.hpp"

namespace ecf {
namespace implementation {

template <>
struct Formatter<AvisoAttr>
{
    static void format(const AvisoAttr& value, std::string& output) {
        ecf::Indentor in;
        ecf::Indentor::indent(output);
        append_defs_line(value, output);
        output += "\n";
    }

private:
    static void append_defs_line(const AvisoAttr& value, std::string& output) {
        output += "aviso ";
        output += value.name();
        output += " ";
        output += value.listener();
    }
};

} // namespace implementation
} // namespace ecf

#endif /* ecflow_node_formatter_AvisoFormatter_HPP */
