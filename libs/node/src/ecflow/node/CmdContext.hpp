/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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

    ~CmdContext();

    static bool in_command() { return in_command_; }

private:
    static bool in_command_;
};
} // namespace ecf

#endif /* ecflow_node_CmdContext_HPP */
