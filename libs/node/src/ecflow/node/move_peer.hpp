/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
