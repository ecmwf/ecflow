/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_WhyCmd_HPP
#define ecflow_base_WhyCmd_HPP

///
/// \brief Client side command only.
///
/// \note Placed in this category, since the server does not need to link with it.
///

#include <string>

#include "ecflow/node/NodeFwd.hpp"

class WhyCmd {
public:
    WhyCmd() = delete;

    WhyCmd(defs_ptr defs, const std::string& absNodePath);

    WhyCmd(const WhyCmd&)            = delete;
    WhyCmd& operator=(const WhyCmd&) = delete;
    WhyCmd(WhyCmd&&)                 = delete;
    WhyCmd& operator=(WhyCmd&&)      = delete;

    ~WhyCmd() = default;

    /// Why the node is not running
    /// Return a '/n' separated string which lists the reasons why
    /// the provided node is not active.
    std::string why() const;

private:
    defs_ptr defs_;
    node_ptr node_;
};

#endif /* ecflow_base_WhyCmd_HPP */
