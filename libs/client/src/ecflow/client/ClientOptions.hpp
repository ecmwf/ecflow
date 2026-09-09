/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_client_ClientOptions_HPP
#define ecflow_client_ClientOptions_HPP

#include "ecflow/base/cts/CtsCmdRegistry.hpp"
class ClientEnvironment;
class CommandLine;

///
/// \brief Will parse the client argument line, and construct a command that will be sent to the server.
///
/// The environment must be read in before the program options. The program options
/// will construct the commands, some of which require the environment
/// We could have just done this as last part of constructor. However, we need a
/// separation between reading the environment and reading the option for:
/// a/ testing purposes. i.e. as this allows us to inject/override the task path
///    read in from the environment.
/// b/ override host and port number.
/// will throw std::runtime_error for invalid arguments
///

class ClientOptions {
public:
    /// Will create command register, & ask each cmd to describe their arguments
    ClientOptions();

    ClientOptions(const ClientOptions&)            = delete;
    ClientOptions& operator=(const ClientOptions&) = delete;
    ClientOptions(ClientOptions&&)                 = delete;
    ClientOptions& operator=(ClientOptions&&)      = delete;

    ~ClientOptions();

    ///
    /// @brief Parse the command line arguments and create the client request that is to be sent to the server.
    ///
    /// @param cl The command line arguments to parse
    /// @param environment The client environment, which may be used by some commands to construct the client request
    /// @return A pointer to the client request to be sent to the server.
    ///         The pointer can be nullptr if the command is client specific (e.g. --help, --version)
    ///         and does not need to send a request to the server.
    /// @throw std::runtime_error if invalid arguments are specified
    ///
    Cmd_ptr parse(const CommandLine& cl, ClientEnvironment* environment) const;

private:
    CtsCmdRegistry cmdRegistry_;
    boost::program_options::options_description* desc_;
};

#endif /* ecflow_client_ClientOptions_HPP */
