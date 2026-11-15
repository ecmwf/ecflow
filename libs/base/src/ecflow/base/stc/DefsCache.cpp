// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/stc/DefsCache.hpp"

#include <iterator>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Visibility.hpp"

// =====================================================================================================
// #define DEBUG_SERVER_SYNC 1
// #define DEBUG_CLIENT_SYNC 1
#ifdef DEBUG_SERVER_SYNC
    #include <iostream>
#endif

// ===========================================================================================
// CACHE: the deserialization costs, so that if multiple clients request the full defs
//        we can improve the performance, by only performing that once for each state change.
std::map<std::string, DefsCache::Entry> DefsCache::entries_;
std::weak_ptr<Defs> DefsCache::defs_;
std::shared_ptr<const std::string> DefsCache::last_;

namespace {

const std::string& key_of(const ecf::Visibility* visibility) {
    static const std::string everything = "*";
    return visibility ? visibility->key() : everything;
}

} // namespace

std::shared_ptr<const std::string> DefsCache::update_cache_if_state_changed(const defs_ptr& defs,
                                                                            const ecf::Visibility* visibility) {
    const bool same_defs = !defs_.expired() && !defs_.owner_before(defs) && !defs.owner_before(defs_);
    if (same_defs) {
        if (auto found = entries_.find(key_of(visibility)); found != entries_.end() &&
                                                            found->second.state_change_no == Ecf::state_change_no() &&
                                                            found->second.modify_change_no == Ecf::modify_change_no()) {
#ifdef DEBUG_SERVER_SYNC
            cout << ": *cache* up to date";
#endif
            last_ = found->second.text;
            return last_;
        }
    }
    return update_cache(defs, visibility);
}

std::shared_ptr<const std::string> DefsCache::update_cache(const defs_ptr& defs, const ecf::Visibility* visibility) {
#ifdef DEBUG_SERVER_SYNC
    cout << ": *updating* cache";
#endif
    // Entries built from other definitions, or before the change numbers moved, are dropped
    const bool same_defs = !defs_.expired() && !defs_.owner_before(defs) && !defs.owner_before(defs_);
    if (!same_defs) {
        entries_.clear();
        defs_ = defs;
    }
    for (auto it = entries_.begin(); it != entries_.end();) {
        const bool stale = it->second.state_change_no != Ecf::state_change_no() ||
                           it->second.modify_change_no != Ecf::modify_change_no();
        it = stale ? entries_.erase(it) : std::next(it);
    }

    auto text = std::make_shared<std::string>();
    defs->write_to_string(*text, PrintStyle::NET, visibility);
    auto& entry            = entries_[key_of(visibility)];
    entry.text             = std::move(text);
    entry.state_change_no  = Ecf::state_change_no();
    entry.modify_change_no = Ecf::modify_change_no();
    last_                  = entry.text;
    return last_;
}

std::size_t DefsCache::entries() {
    return entries_.size();
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
    return restore_defs_from_string(last_ ? *last_ : std::string{});
}
