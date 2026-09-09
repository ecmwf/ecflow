/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_base_cts_CtsCmdRegistry_HPP
#define ecflow_base_cts_CtsCmdRegistry_HPP

#include <vector>

#include <boost/program_options.hpp>

#include "ecflow/base/Cmd.hpp"

class AbstractClientEnv;

///
/// \brief Registration of all the client to server commands
///
/// This allows us to parse the arg associated with commands in this category.
/// The idea is to keep new commands functionality in one place.
/// Any new client to server commands that are created must be added to this class.
///

class CtsCmdRegistry {
public:
    explicit CtsCmdRegistry(bool addGroupCmd = true);

    CtsCmdRegistry(const CtsCmdRegistry&)            = delete;
    CtsCmdRegistry& operator=(const CtsCmdRegistry&) = delete;
    CtsCmdRegistry(CtsCmdRegistry&&)                 = delete;
    CtsCmdRegistry& operator=(CtsCmdRegistry&&)      = delete;

    ~CtsCmdRegistry() = default;

    /// These option describe the arguments for each of the commands
    /// They also can be presented to the user via --help option.
    void addAllOptions(boost::program_options::options_description& desc) const;
    void addCmdOptions(boost::program_options::options_description& desc) const;

    /// Parse arguments given in 'vm' and use that to create a command
    /// that will be sent to the server. Will throw std::runtime_error for errors
    /// Returns true if command line argument specified via 'vm', matches one of the
    /// registered command.
    /// *** This allows us to distinguish between where we match with a registered
    /// *** command, but do *NOT* set Cmd_ptr, ie since its a client specific command
    /// *** i.e there is no need to send it to the server
    bool parse(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const;

private:
    std::vector<Cmd_ptr> vec_;

    void addHelpOption(boost::program_options::options_description& desc) const;
};

#endif /* ecflow_base_cts_CtsCmdRegistry_HPP */
