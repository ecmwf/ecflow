// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ecflow/core/os/AccountDatabase.hpp"

namespace ecf::test::scaffold {

///
/// @brief An in-memory account database, for tests that need several accounts without creating them.
///
class FakeAccountDatabase : public ecf::os::AccountDatabase {
public:
    ///
    /// @brief Adds a group.
    ///
    /// @param[in] name The group name.
    /// @param[in] gid The group identifier.
    /// @return This database, to chain calls.
    ///
    FakeAccountDatabase& add_group(const std::string& name, gid_t gid) {
        groups_[name] = ecf::os::Group{name, gid};
        return *this;
    }

    ///
    /// @brief Adds a user account.
    ///
    /// @param[in] name The user name.
    /// @param[in] uid The user identifier.
    /// @param[in] gid The primary group identifier.
    /// @param[in] supplementary The supplementary group identifiers.
    /// @return This database, to chain calls.
    ///
    FakeAccountDatabase&
    add_user(const std::string& name, uid_t uid, gid_t gid, const std::vector<gid_t>& supplementary = {}) {
        users_[name]         = ecf::os::Account{name, uid, gid, "/home/" + name, "/bin/sh"};
        supplementary_[name] = supplementary;
        return *this;
    }

    std::optional<ecf::os::Account> find_user(const std::string& name) const override {
        auto found = users_.find(name);
        return found == users_.end() ? std::nullopt : std::optional<ecf::os::Account>(found->second);
    }

    std::optional<ecf::os::Group> find_group(const std::string& name) const override {
        auto found = groups_.find(name);
        return found == groups_.end() ? std::nullopt : std::optional<ecf::os::Group>(found->second);
    }

    std::vector<gid_t> groups_of(const ecf::os::Account& account) const override {
        std::vector<gid_t> groups{account.gid};
        if (auto found = supplementary_.find(account.name); found != supplementary_.end()) {
            for (auto gid : found->second) {
                if (std::find(groups.begin(), groups.end(), gid) == groups.end()) {
                    groups.push_back(gid);
                }
            }
        }
        return groups;
    }

private:
    std::map<std::string, ecf::os::Account> users_;
    std::map<std::string, ecf::os::Group> groups_;
    std::map<std::string, std::vector<gid_t>> supplementary_;
};

} // namespace ecf::test::scaffold
