// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string>

#include "InfoProvider.hpp"

class InfoPanelItem;

class OverviewProvider : public InfoProvider {
public:
    explicit OverviewProvider(InfoPresenter* owner);

    // From VInfoVisitor
    void visit(VInfoServer*) override;
    void visit(VInfoNode*) override;
    void visit(VInfoAttribute*) override;

    // From VTaskObserver
    void taskChanged(VTask_ptr) override;

protected:
    void serverInfo(VInfoServer*, std::stringstream& f);
    void nodeInfo(VInfoNode*, std::stringstream& f);

    ///
    /// @brief Returns the effective roles of every user at a path, as "<user>:<roles>, ...".
    ///
    /// The roles are computed on the client's copy of the definitions, with the rules of the server
    /// (ecf::effective_permissions_of_every_user()). Nothing is returned on a server without Permissions, when the
    /// server denies access, or when the path is not found in the copy.
    ///
    /// @param[in] server The server holding the copy
    /// @param[in] path The path of a node, or "/" for the server
    /// @return The list of the effective roles, or nothing
    ///
    static std::optional<std::string> effectivePermissions(ServerHandler* server, const std::string& path);
};
