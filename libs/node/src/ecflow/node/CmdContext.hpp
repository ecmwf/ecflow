/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_node_CmdContext_HPP
#define ecflow_node_CmdContext_HPP

///
/// \brief This class allow client to determine whether they are in a middle of a command.
///

namespace ecf {

class CmdContext {
public:
    CmdContext();

    // Disable copy (and move) semantics
    CmdContext(const CmdContext&)                  = delete;
    const CmdContext& operator=(const CmdContext&) = delete;
    CmdContext(CmdContext&&)                       = delete;
    CmdContext& operator=(CmdContext&&)            = delete;

    ~CmdContext();

    static bool in_command() { return in_command_; }

private:
    static bool in_command_;
};
} // namespace ecf

#endif /* ecflow_node_CmdContext_HPP */
