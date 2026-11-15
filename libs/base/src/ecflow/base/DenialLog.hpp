// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <string>

namespace ecf {

///
/// @brief Bounds the rate at which denied requests are recorded in the server log.
///
/// Denials are grouped by key (the requester and the command). The first denial of a key is recorded; the following
/// ones are suppressed until the period has elapsed, and the next record of that key reports how many were
/// suppressed.
///
/// The number of keys tracked is bounded. When the bound is reached, the keys whose period has elapsed are forgotten;
/// if none has, the denials of new keys share a single overflow key, limited in the same way, while the keys tracked
/// keep their own limits. A key that becomes tracked while the overflow key is within its period starts from the
/// overflow key's last record, so that no key is recorded twice within a period.
///
/// @invariant keys() <= the maximum number of keys given at construction.
///
class DenialLog {
public:
    using Clock = std::chrono::steady_clock;

    ///
    /// @brief Creates the log bookkeeping.
    ///
    /// @param period The minimal time between two records of the same key
    /// @param max_keys The number of keys tracked at most
    ///
    explicit DenialLog(Clock::duration period = std::chrono::seconds(10), std::size_t max_keys = 1000)
        : period_{period},
          max_keys_{max_keys} {}

    ///
    /// @brief Decides whether a denial is recorded.
    ///
    /// @param[in] key The key of the denial
    /// @param[in] now The time of the denial
    /// @param[out] suppressed When the denial is recorded, the number of denials suppressed since the last record of
    ///             its key, or of the overflow key
    /// @param[out] overflow When the denial is recorded, true when it was decided by the overflow key, whose count
    ///             covers the denials of every key it took
    /// @return True when the denial is to be recorded.
    ///
    bool record(const std::string& key, Clock::time_point now, std::size_t& suppressed, bool& overflow);

    ///
    /// @brief Decides whether a denial is recorded, as the other overload, without telling an overflow apart.
    ///
    /// @param[in] key The key of the denial
    /// @param[in] now The time of the denial
    /// @param[out] suppressed When the denial is recorded, the number of denials suppressed since the last record
    /// @return True when the denial is to be recorded.
    ///
    bool record(const std::string& key, Clock::time_point now, std::size_t& suppressed) {
        bool overflow = false;
        return record(key, now, suppressed, overflow);
    }

    ///
    /// @brief Returns the number of keys tracked.
    ///
    /// @return The number of keys.
    ///
    [[nodiscard]] std::size_t keys() const { return entries_.size(); }

    ///
    /// @brief Formats the record of a denial, as written in the server log.
    ///
    /// The record is a single line: `DENIED` followed by the fields user, host, command, request, reason and
    /// suppressed, as `name=value`, and by `overflow=yes` for a record decided by the overflow key. Every value but
    /// suppressed and overflow is quoted with `'`; a `'` or `\` inside a value is preceded by `\`, and a line break is
    /// replaced by a space.
    ///
    /// @param[in] user The requester
    /// @param[in] host The host of the requester, as given by the client
    /// @param[in] command The option of the request, e.g. "--suspend"
    /// @param[in] request The request, in short form, with its arguments
    /// @param[in] reason Why the request was denied
    /// @param[in] suppressed The denials of the same key left out since the previous record or, for an overflow, the
    ///            denials of every key taken by the overflow key
    /// @param[in] overflow True for a record decided by the overflow key
    /// @return The record, without line break.
    ///
    static std::string format(const std::string& user,
                              const std::string& host,
                              const std::string& command,
                              const std::string& request,
                              const std::string& reason,
                              std::size_t suppressed,
                              bool overflow = false);

private:
    struct Entry
    {
        Clock::time_point last;
        std::size_t suppressed{0};
    };

    Clock::duration period_;
    std::size_t max_keys_;
    std::map<std::string, Entry> entries_;
    std::optional<Entry> overflow_; // shared by the new keys while every key tracked is within its period

    bool limit(Entry& entry, Clock::time_point now, std::size_t& suppressed) const;
};

} // namespace ecf
