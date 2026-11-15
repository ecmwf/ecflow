// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/base/DenialLog.hpp"

namespace ecf {

namespace {

// Quotes a value of a record, so that the record can be parsed
std::string quoted(const std::string& value) {
    std::string result = "'";
    for (char c : value) {
        if (c == '\'' || c == '\\') {
            result += '\\';
        }
        result += (c == '\n' || c == '\r') ? ' ' : c;
    }
    return result + "'";
}

} // namespace

bool DenialLog::limit(Entry& entry, Clock::time_point now, std::size_t& suppressed) const {
    if (now - entry.last < period_) {
        ++entry.suppressed;
        return false;
    }
    suppressed       = entry.suppressed;
    entry.last       = now;
    entry.suppressed = 0;
    return true;
}

bool DenialLog::record(const std::string& key, Clock::time_point now, std::size_t& suppressed, bool& overflow) {
    overflow = false;
    if (auto found = entries_.find(key); found != entries_.end()) {
        return limit(found->second, now, suppressed);
    }

    if (entries_.size() >= max_keys_) {
        for (auto it = entries_.begin(); it != entries_.end();) {
            it = (now - it->second.last >= period_) ? entries_.erase(it) : std::next(it);
        }
    }
    if (entries_.size() >= max_keys_) {
        // Every key tracked is within its period: the new keys share the overflow key
        overflow = true;
        if (!overflow_) {
            overflow_  = Entry{now, 0};
            suppressed = 0;
            return true;
        }
        return limit(*overflow_, now, suppressed);
    }

    if (overflow_ && now - overflow_->last < period_) {
        // The key may have been taken by the overflow key: it starts from the overflow key's last record
        auto& entry = entries_.emplace(key, Entry{overflow_->last, 0}).first->second;
        return limit(entry, now, suppressed);
    }

    entries_.emplace(key, Entry{now, 0});
    suppressed = 0;
    return true;
}

std::string DenialLog::format(const std::string& user,
                              const std::string& host,
                              const std::string& command,
                              const std::string& request,
                              const std::string& reason,
                              std::size_t suppressed,
                              bool overflow) {
    return "DENIED user=" + quoted(user) + " host=" + quoted(host) + " command=" + quoted(command) +
           " request=" + quoted(request) + " reason=" + quoted(reason) + " suppressed=" + std::to_string(suppressed) +
           (overflow ? " overflow=yes" : "");
}

} // namespace ecf
