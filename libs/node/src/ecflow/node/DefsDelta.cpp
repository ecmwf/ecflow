/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/DefsDelta.hpp"

#include <stdexcept>

#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Memento.hpp"

using namespace std;

// #define DEBUG_MEMENTO 1

//===============================================================
// DefsDelta

/// Defs delta can be re-used. reset all data members
void DefsDelta::init(unsigned int client_state_change_no, bool sync_suite_clock) {
    sync_suite_clock_       = sync_suite_clock;
    client_state_change_no_ = client_state_change_no;

    server_state_change_no_  = 0;
    server_modify_change_no_ = 0;
    compound_mementos_.clear();
}

bool DefsDelta::incremental_sync(defs_ptr client_def,
                                 std::vector<std::string>& changed_nodes,
                                 int client_handle) const {
    // ****************************************************
    // On the client side
    // ****************************************************
    if (!client_def.get()) {
        return false;
    }

    // Clear the changed nodes since we want it to match number of changed memento's
    // This should have been clear anyway in ServerReply::clear_for_invoke
    changed_nodes.clear();

    // Update the client defs with latest server *handle* based state change/modify number
    // to keep pace with the state changes. Passed back later on, to get further changes
    client_def->set_state_change_no(server_state_change_no_);
    client_def->set_modify_change_no(server_modify_change_no_);

    try {
#ifdef DEBUG_MEMENTO
        std::cout << "DefsDelta::incremental_sync compound_mementos_.size() = " << compound_mementos_.size() << "\n";
#endif

        // For each compound memento, we should have a changed node,
        for (compound_memento_ptr m : compound_mementos_) {
            changed_nodes.push_back(m->abs_node_path()); // Record changed nodes for the Python interface
            m->incremental_sync(client_def);
        }
    }
    catch (std::exception& e) {
        std::stringstream ss;
        ss << "Could not apply incremental server changes to client defs( with client handle: " << client_handle
           << "), because: " << e.what();
        throw std::runtime_error(ss.str());
    }

    // For each compound memento, we should have a changed node.(for use with python interface)
    assert(compound_mementos_.size() == changed_nodes.size());

    // return true if there were any changes made
    return !compound_mementos_.empty();
}

void DefsDelta::add(compound_memento_ptr memento) {
    compound_mementos_.push_back(memento);
}

template <class Archive>
void DefsDelta::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(server_state_change_no_), CEREAL_NVP(server_modify_change_no_), CEREAL_NVP(compound_mementos_));
}
CEREAL_TEMPLATE_SPECIALIZE_V(DefsDelta);
