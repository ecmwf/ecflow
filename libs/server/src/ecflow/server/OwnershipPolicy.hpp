// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <sys/types.h>

#include "ecflow/core/os/AccountDatabase.hpp"
#include "ecflow/core/os/OwnerEligibility.hpp"
#include "ecflow/node/Ownership.hpp"

class Node;

namespace ecf {

class Identity;
class OwnerAttr;

///
/// @brief Decides who may create, update and delete the Owner of a node.
///
/// @details This is the single place for the ownership rules, applied after the existing authorisation and in the
///          same way whether ownership is enabled or not:
///          - create: only the user named in the new Owner, on a node (not an alias) without Owner;
///          - update: only the user of the current Owner, to any eligible user;
///          - delete: only the user of the current Owner.
///          Commands bringing definitions in or taking them out (load, replace, plug) are checked as a whole:
///          every Owner they add must name the requester, even when identical to the existing one, and every Owner
///          they remove must belong to the requester.
///          Every Owner introduced must name an eligible account (see os::OwnerEligibility). Requesters are compared
///          literally, by user name, with the user of an Owner.
///
/// @invariant The account database is never null.
///
class OwnershipPolicy {
public:
    ///
    /// @brief Creates the policy of a server running under the given account.
    ///
    /// @param[in] accounts The account database used to check the Owners introduced; must not be null.
    /// @param[in] server_uid The user identifier of the server account.
    /// @throws std::invalid_argument if @p accounts is null.
    ///
    OwnershipPolicy(std::shared_ptr<const os::AccountDatabase> accounts, uid_t server_uid);

    ///
    /// @brief Checks that the requester may give the node its first Owner.
    ///
    /// @param[in] requester The identity attached to the command.
    /// @param[in] node The node receiving the Owner.
    /// @param[in] value The new Owner.
    /// @throws std::runtime_error with the reason, if the operation is refused, including when the account database
    ///         cannot be queried.
    ///
    void check_create(const Identity& requester, const Node& node, const OwnerAttr& value) const;

    ///
    /// @brief Checks that the requester may replace the Owner of the node.
    ///
    /// @param[in] requester The identity attached to the command.
    /// @param[in] node The node holding the Owner.
    /// @param[in] value The new Owner.
    /// @throws std::runtime_error with the reason, if the operation is refused, including when the account database
    ///         cannot be queried.
    ///
    void check_update(const Identity& requester, const Node& node, const OwnerAttr& value) const;

    ///
    /// @brief Checks that the requester may remove the Owner of the node.
    ///
    /// @param[in] requester The identity attached to the command.
    /// @param[in] node The node holding the Owner.
    /// @throws std::runtime_error with the reason, if the operation is refused.
    ///
    void check_delete(const Identity& requester, const Node& node) const;

    ///
    /// @brief Checks that the requester may make all the changes of a command bringing definitions in or taking
    ///        them out.
    ///
    /// @param[in] requester The identity attached to the command.
    /// @param[in] changes The Owners added and removed by the command.
    /// @param[in] command The command, as named to users (e.g. "load").
    /// @throws std::runtime_error with the reason of the first change refused, if any change is refused, including
    ///         when the account database cannot be queried.
    ///
    void check_change_set(const Identity& requester, const ownership::ChangeSet& changes, const char* command) const;

    ///
    /// @brief Sets whether ownership is enabled on the server.
    ///
    /// @details The rules are the same in both modes. When ownership is disabled (the default), the Owners brought
    ///          in are reported (see report_addition()), as the commands of their nodes run as the server account.
    ///
    /// @param[in] enabled Whether ownership is enabled.
    ///
    void set_enabled(bool enabled) { enabled_ = enabled; }

    ///
    /// @brief Reports an Owner brought in, by a warning in the server log when ownership is disabled (R11).
    ///
    /// @details Called by the commands once their change is made, so that only Owners actually introduced are
    ///          reported. Does nothing when ownership is enabled.
    ///
    /// @param[in] value The Owner brought in.
    /// @param[in] path The path of the node holding it.
    /// @param[in] operation The operation, as named to users (e.g. "create").
    ///
    void report_addition(const OwnerAttr& value, const std::string& path, const char* operation) const;

    ///
    /// @brief Reports the Owners brought in by a command, as report_addition() does for each of them.
    ///
    /// @param[in] changes The Owners added and removed by the command, once its change is made.
    /// @param[in] command The command, as named to users (e.g. "load").
    ///
    void report_additions(const ownership::ChangeSet& changes, const char* command) const;

private:
    void check_requester_is_current_owner(const Identity& requester, const Node& node, const char* operation) const;

    /// Extension point for administrators, allowed to manage any Owner (question Q1, deferred): none for now
    [[nodiscard]] bool is_administrator(const Identity&) const { return false; }

    void check_eligible(const OwnerAttr& value, const std::string& path, const char* operation) const;

    std::shared_ptr<const os::AccountDatabase> accounts_;
    uid_t server_uid_;
    bool enabled_{false};
};

} // namespace ecf
