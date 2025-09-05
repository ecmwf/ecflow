/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_NodeState_HPP
#define ecflow_node_NodeState_HPP

#include "ecflow/core/NState.hpp"

///
/// \brief Given a set of nodes, theComputedNodeState returns the most significant state
/// Depend on the Node::computedState, hence include after Node.hpp
/// This will recurse down ****
///

namespace ecf {

template <class T>
NState::State theComputedNodeState(const std::vector<T>& nodeVec, bool immediate) {
    // std::cout << "theComputedNodeState vec size = " << nodeVec.size() << " immediate  = " <<  immediate << "\n";
    int completeCount  = 0;
    int queuedCount    = 0;
    int submittedCount = 0;
    int activeCount    = 0;
    int abortedCount   = 0;

    // We don't know the order, hence we must collate first
    size_t theVecSize = nodeVec.size();
    for (size_t n = 0; n < theVecSize; n++) {
        NState::State theState;
        if (immediate) {
            theState = nodeVec[n]->state();
        }
        else {
            theState = nodeVec[n]->computedState(Node::HIERARCHICAL);
        }

        // std::cout << "the computed state for " << nodeVec[n]->debugNodePath() << " is " << NState::toString(theState)
        // << "\n";

        switch (theState) {
            case NState::ABORTED:
                abortedCount++;
                break;
            case NState::ACTIVE:
                activeCount++;
                break;
            case NState::SUBMITTED:
                submittedCount++;
                break;
            case NState::QUEUED:
                queuedCount++;
                break;
            case NState::COMPLETE:
                completeCount++;
                break;
            case NState::UNKNOWN:
                // Does nothing...
                break;
            default:
                assert(false);
                break;
        }
    }
    if (abortedCount > 0) {
        return NState::ABORTED;
    }
    if (activeCount > 0) {
        return NState::ACTIVE;
    }
    if (submittedCount > 0) {
        return NState::SUBMITTED;
    }
    if (queuedCount > 0) {
        return NState::QUEUED;
    }
    if (completeCount > 0) {
        return NState::COMPLETE;
    }
    return NState::UNKNOWN;
}

} // namespace ecf

#endif /* ecflow_node_NodeState_HPP */
