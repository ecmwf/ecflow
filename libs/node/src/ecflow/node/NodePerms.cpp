/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/NodePerms.hpp"

#include "ecflow/node/Node.hpp"
namespace ecf {

ecf::Perms perms(const Node& node) {
    Node* current = const_cast<Node*>(&node);
    auto perms = current->perms();
    while( !perms.good() ) {
        current = current->parent();
        if (current == nullptr) {
            return ecf::Perms();
        }
        perms = current->perms();
    }
    return perms;
}

} // namespace ecf
