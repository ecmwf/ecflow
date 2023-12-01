/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
    WhyCmd()              = delete;
    WhyCmd(const WhyCmd&) = delete;
    WhyCmd(WhyCmd&&)      = delete;

    WhyCmd(defs_ptr defs, const std::string& absNodePath);

    /// Why the node is not running
    /// Return a '/n' separated string which lists the reasons why
    /// the provided node is not active.
    std::string why() const;

private:
    defs_ptr defs_;
    node_ptr node_;
};

#endif /* ecflow_base_WhyCmd_HPP */
