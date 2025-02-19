/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
    ClientOptions(ClientOptions&&)                 = delete;
    ClientOptions& operator=(const ClientOptions&) = delete;
    ClientOptions& operator=(ClientOptions&&)      = delete;
    ~ClientOptions();

    /// parse the arguments and create the client request that is to be sent
    /// to the server. Will throw std::runtime_error if invalid arguments specified
    Cmd_ptr parse(const CommandLine& cl, ClientEnvironment*) const;

private:
    CtsCmdRegistry cmdRegistry_;
    boost::program_options::options_description* desc_;
};

#endif /* ecflow_client_ClientOptions_HPP */
