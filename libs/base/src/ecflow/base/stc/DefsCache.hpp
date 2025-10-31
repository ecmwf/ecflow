/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_cts_DefsCache_HPP
#define ecflow_base_cts_DefsCache_HPP

#include <ecflow/core/Identity.hpp>

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
    DefsCache()                            = delete;
    DefsCache(const DefsCache&)            = delete;
    DefsCache& operator=(const DefsCache&) = delete;

    // Server side
    // static void update_cache_if_state_changed(Defs* defs);
    static void update_cache_if_state_changed(Defs* defs, const ecf::Identity& identity);
    // static void update_cache(Defs* defs);
    static void update_cache(Defs* defs, const ecf::Identity& identity);

    // Client side
    static defs_ptr restore_defs_from_string(const std::string&);
    static defs_ptr restore_defs_from_string(); // used in test

private:
    friend class SSyncCmd;
    friend class DefsCmd;

    static ecf::Identity identity_;
    static std::string full_server_defs_as_string_;
    static unsigned int state_change_no_;  // detect state change in defs across clients
    static unsigned int modify_change_no_; // detect state change in defs across clients
};

#endif /* ecflow_base_cts_DefsCache_HPP */
