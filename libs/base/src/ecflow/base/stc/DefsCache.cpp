/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/DefsCache.hpp"

#include <ecflow/node/formatter/DefsWriter.hpp>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"

// =====================================================================================================
// #define DEBUG_SERVER_SYNC 1
// #define DEBUG_CLIENT_SYNC 1
#ifdef DEBUG_SERVER_SYNC
    #include <iostream>
#endif

using namespace std;
using namespace boost;

// ===========================================================================================
// CACHE: the deserialization costs, so that if multiple clients request the full defs
//        we can improve the performance, by only performing that once for each state change.
std::string DefsCache::full_server_defs_as_string_ = "";
unsigned int DefsCache::state_change_no_           = 0;
unsigned int DefsCache::modify_change_no_          = 0;
ecf::Identity DefsCache::identity_                 = ecf::Identity::make_none();

// void DefsCache::update_cache_if_state_changed(Defs* defs) {
//     // See if there was a state change *OR* if cache is empty
//     if (state_change_no_ != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no() ||
//         full_server_defs_as_string_.empty()) {
//         update_cache(defs);
//     }
// #ifdef DEBUG_SERVER_SYNC
//     else {
//         cout << ": *cache* up to date";
//     }
// #endif
// }

void DefsCache::update_cache_if_state_changed(Defs* defs, const ecf::Identity& identity) {
    if (state_change_no_ != Ecf::state_change_no() || modify_change_no_ != Ecf::modify_change_no() ||
        full_server_defs_as_string_.empty() || identity.username() != identity_.username()) {
        update_cache(defs, identity);
    }
}

// void DefsCache::update_cache(Defs* defs) {
// #ifdef DEBUG_SERVER_SYNC
//     cout << ": *updating* cache";
// #endif
//     defs->write_to_string(full_server_defs_as_string_, PrintStyle::NET); // update cache
//     state_change_no_  = Ecf::state_change_no();
//     modify_change_no_ = Ecf::modify_change_no();
// }

void DefsCache::update_cache(Defs* defs, const ecf::Identity& identity) {
#ifdef DEBUG_SERVER_SYNC
    cout << ": *updating* cache";
#endif
    auto ctx = ecf::Context::make_for(PrintStyle::NET, std::make_optional(identity.username()));
    defs->write_to_string(full_server_defs_as_string_, ctx); // update cache
    state_change_no_  = Ecf::state_change_no();
    modify_change_no_ = Ecf::modify_change_no();
}

defs_ptr DefsCache::restore_defs_from_string(const std::string& archive_data) {
#ifdef DEBUG_CLIENT_SYNC
    cout << ": DefsCache::restore_defs_from_string: archive_data.size(" << archive_data.size() << ")";
#endif

    defs_ptr defs = Defs::create();
    try {
        // cout << "archive_data:\n" << archive_data << "\n";
        defs->restore_from_string(archive_data);
    }
    catch (std::exception& e) {
        ecf::LogToCout logToCout;
        LOG(ecf::Log::ERR, "DefsCache::restore_defs_from_string " << e.what());
        throw;
    }

#ifdef DEBUG_CLIENT_SYNC
    if (defs.get()) {
        cout << ": valid defs";
    }
    else {
        cout << ": *empty* defs?";
    }
#endif
    return defs;
}

defs_ptr DefsCache::restore_defs_from_string() {
    // Used in Test when no client/server
    return restore_defs_from_string(full_server_defs_as_string_);
}
