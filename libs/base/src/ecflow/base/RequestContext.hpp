// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/node/EffectivePermissions.hpp"

class AbstractServer;

namespace ecf {

///
/// @brief Holds what a request needs to know about its requester, for the duration of the request.
///
/// The context is created when the server handles a request, once the requester is authenticated, and is passed to the
/// authorisation, to the execution of the command and to the construction of its reply. It ends with the request: no
/// requester is kept in global or static state.
///
/// @invariant The identity and the server outlive the context.
///
class RequestContext {
public:
    ///
    /// @brief Creates the context of a request.
    ///
    /// @param[in] identity The identity of the requester, as established by authentication
    /// @param[in] server The server handling the request
    ///
    RequestContext(const Identity& identity, AbstractServer& server);

    RequestContext(const RequestContext&)            = delete;
    RequestContext& operator=(const RequestContext&) = delete;

    ///
    /// @brief Returns the identity of the requester.
    ///
    /// @return The identity.
    ///
    [[nodiscard]] const Identity& identity() const { return identity_; }

    ///
    /// @brief Returns the server handling the request.
    ///
    /// @return The server.
    ///
    [[nodiscard]] AbstractServer& server() const { return server_; }

    ///
    /// @brief Returns the evaluator of the effective permissions of the requester.
    ///
    /// The evaluator is created on first use, from the definitions of the server at that time, and kept for the rest
    /// of the request.
    ///
    /// @return The evaluator.
    ///
    [[nodiscard]] const EffectivePermissions& permissions() const;

    ///
    /// @brief Returns the effective roles of the requester at a path, as the definitions are now.
    ///
    /// The roles are those of the node with the path or, when there is none, of its closest existing ancestor; the
    /// server for "/", an empty path, or a path without a suite. An attribute name after the node path (`path:name`)
    /// is ignored. The roles are evaluated afresh at each call, so that a request that changes the definitions (e.g.
    /// a sub-command of a group) is followed by an evaluation of the new definitions.
    ///
    /// @param[in] path The path addressed by the request
    /// @return The effective roles; empty for a requester the server attribute does not admit.
    /// @throws std::logic_error when the server holds no definitions, or Permissions do not apply.
    ///
    [[nodiscard]] Roles roles_at(const std::string& path) const;

private:
    const Identity& identity_;
    AbstractServer& server_;
    mutable std::optional<EffectivePermissions> permissions_;
};

} // namespace ecf
