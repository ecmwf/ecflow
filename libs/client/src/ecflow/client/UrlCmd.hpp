/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_client_UrlCmd_HPP
#define ecflow_client_UrlCmd_HPP

#include <string>

#include "ecflow/node/NodeFwd.hpp"

///
/// \note Client side command only.
/// \note Placed in this category, since the server does not need to link with it.
///

class UrlCmd {
public:
    UrlCmd() = delete;

    UrlCmd(const UrlCmd&)            = delete;
    UrlCmd& operator=(const UrlCmd&) = delete;
    UrlCmd(UrlCmd&&)                 = delete;
    UrlCmd& operator=(UrlCmd&&)      = delete;

    /// Will throw std::runtime_error if defs or node path is not correct
    UrlCmd(defs_ptr defs, const std::string& absNodePath);

    ~UrlCmd() = default;

    /// Will throw std::runtime_error if url cannot be formed
    std::string getUrl() const;

    /// Execute the url command
    void execute() const;

private:
    defs_ptr defs_;
    Node* node_;
};

#endif /* ecflow_client_UrlCmd_HPP */
