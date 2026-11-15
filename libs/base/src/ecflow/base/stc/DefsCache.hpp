// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>

#include "ecflow/node/NodeFwd.hpp"

namespace ecf {
class Visibility;
}

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
//
// With Permissions attributes, the definitions sent depend on the nodes the requester can read (R5.6): the cache
// holds one entry per set of readable nodes, identified by the key of its visibility; requesters reading the same
// nodes share an entry. Every entry is valid until the state or modify change numbers change.
//

class DefsCache {
public:
    DefsCache() = delete;

    ///
    /// @brief Returns the definitions as sent to the requesters of a visibility, built again when the change numbers
    ///        moved since the entry was built.
    ///
    /// @param[in] defs The definitions of the server
    /// @param[in] visibility The nodes the requester can read; every node when null
    /// @return The serialised definitions, shared with the cache.
    ///
    static std::shared_ptr<const std::string> update_cache_if_state_changed(const defs_ptr& defs,
                                                                            const ecf::Visibility* visibility);

    ///
    /// @brief Builds the definitions as sent to the requesters of a visibility, and stores them in the cache.
    ///
    /// @param[in] defs The definitions of the server
    /// @param[in] visibility The nodes the requester can read; every node when null
    /// @return The serialised definitions, shared with the cache.
    ///
    static std::shared_ptr<const std::string> update_cache(const defs_ptr& defs, const ecf::Visibility* visibility);

    ///
    /// @brief Returns the number of entries of the cache, for tests and diagnostics.
    ///
    /// @return The number of entries.
    ///
    static std::size_t entries();

    // Client side
    static defs_ptr restore_defs_from_string(const std::string&);
    static defs_ptr restore_defs_from_string(); // used in test: the entry built or used last

private:
    struct Entry
    {
        std::shared_ptr<const std::string> text;
        unsigned int state_change_no{0};
        unsigned int modify_change_no{0};
    };

    static std::map<std::string, Entry> entries_;
    static std::weak_ptr<Defs> defs_;                // the definitions the entries were built from
    static std::shared_ptr<const std::string> last_; // the entry built or used last
};
