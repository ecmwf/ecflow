/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_server_ServerOptions_HPP
#define ecflow_server_ServerOptions_HPP

///
/// \brief This class will parse the server arguments.
/// It will update the ServerEnvironment
///

#include <boost/program_options.hpp>

#include "ecflow/core/CommandLine.hpp"

class ServerEnvironment;

class ServerOptions {
public:
    ServerOptions(const CommandLine& cl, ServerEnvironment*);

    // Disable copy (and move) semantics
    ServerOptions(const ServerOptions&)                  = delete;
    const ServerOptions& operator=(const ServerOptions&) = delete;
    ServerOptions(ServerOptions&&)                       = delete;
    ServerOptions& operator=(ServerOptions&&)            = delete;

    ~ServerOptions() = default;

    /// return true if help selected, else false
    bool help_option() const;
    bool version_option() const;

private:
    boost::program_options::variables_map vm_;
};

#endif /*ecflow_server_ServerOptions_HPP */
