/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_cts_DefsCache_HPP
#define ecflow_base_cts_DefsCache_HPP

#include "ecflow/node/NodeFwd.hpp"

//================================================================================
// Cache the de-serialisation cost, in the *SERVER* when returning the FULL definition
// When there are no state changes, we can just return the cache string for other clients
// Thereby saving time in the server and client for de-serialisation.
// Here we are trading memory for speed:
//
//  Current for each client request we have:
//      client1:  --------------> get---------------> Server
//                serialise---------<----de-serialize
//
//      client2:  --------------> get---------------> Server
//                serialise---------<----de-serialize
//
//      client3:  --------------> get---------------> Server
//                serialise---------<----de-serialize
//
// By caching the de-serialisation process, we can speed up the downloads.
// However whenever there is a state change we need to update the cache
//
//      client1:  --------------> get---------------> Server
//                serialise------<----de-serialisation
//
//      client2:  --------------> get---------------> Server
//                serialise---------<----return cache
//
//      client3:  --------------> get---------------> Server
//                serialise---------<----return cache
//================================================================================
class DefsCache {
public:
    DefsCache() = delete;

    // Server side
    static void update_cache_if_state_changed(Defs* defs);
    static void update_cache(Defs* defs);

    // Client side
    static defs_ptr restore_defs_from_string(const std::string&);
    static defs_ptr restore_defs_from_string(); // used in test

private:
    friend class SSyncCmd;
    friend class DefsCmd;

    static std::string full_server_defs_as_string_;
    static unsigned int state_change_no_;  // detect state change in defs across clients
    static unsigned int modify_change_no_; // detect state change in defs across clients
};

#endif /* ecflow_base_cts_DefsCache_HPP */
