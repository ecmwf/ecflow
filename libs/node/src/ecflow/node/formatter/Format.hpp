/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_formatter_Format_HPP
#define ecflow_node_formatter_Format_HPP

#include "ecflow/node/formatter/AvisoFormatter.hpp"

namespace ecf {

template <typename T>
void format_as_defs(const T& value, std::string& output) {
    implementation::Formatter<T>::format(value, output);
}

} // namespace ecf

#endif /* ecflow_node_formatter_Format_HPP */
