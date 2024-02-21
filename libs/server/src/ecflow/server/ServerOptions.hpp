/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_server_ServerOptions_HPP
#define ecflow_server_ServerOptions_HPP

///
/// \brief This class will parse the server arguments.
/// It will update the ServerEnvironment
///

#include <boost/program_options.hpp>
class ServerEnvironment;

class ServerOptions {
public:
    ServerOptions(int argc, char* argv[], ServerEnvironment*);
    // Disable copy (and move) semantics
    ServerOptions(const ServerOptions&)                  = delete;
    const ServerOptions& operator=(const ServerOptions&) = delete;

    /// return true if help selected, else false
    bool help_option() const;
    bool version_option() const;

private:
    boost::program_options::variables_map vm_;
};

#endif /*ecflow_server_ServerOptions_HPP */
