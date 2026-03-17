/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_move_peer_HPP
#define ecflow_node_move_peer_HPP

#include "ecflow/node/Node.hpp"

template <typename ct>
void move_peer_node(std::vector<ct>& vec, Node* source, Node* dest, const std::string& error_str) {
    if (!source) {
        throw std::runtime_error(MESSAGE(error_str << "::move source is NULL"));
    }
    if (!dest) {
        throw std::runtime_error(MESSAGE(error_str << "::move destination is NULL"));
    }
    if (source == dest) {
        throw std::runtime_error(MESSAGE(error_str << "move choose a different location as sibling "
                                                   << dest->absNodePath() << " matches node to be moved"));
    }
    if (source->parent() != dest->parent()) {
        throw std::runtime_error(MESSAGE(error_str << "move source and destination node are not siblings"));
    }

    for (size_t t = 0; t < vec.size(); t++) {
        if (vec[t].get() == source) {
            for (size_t s = 0; s < vec.size(); s++) {
                if (vec[s].get() == dest) {

                    ct node = vec[t];
                    vec.erase(vec.begin() + t);

                    vec.insert(vec.begin() + s, node);
                    return;
                }
            }
            throw std::runtime_error(MESSAGE(error_str << "::move could not find sibling node " << dest->absNodePath()
                                                       << " when moving node " << source->absNodePath()));
        }
    }

    throw std::runtime_error(
        MESSAGE(error_str << "::move source node " << source->absNodePath() << " not found on parent"));
}

#endif /* ecflow_node_move_peer_HPP */
