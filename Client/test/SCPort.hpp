/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_test_SCPort_HPP
#define ecflow_client_test_SCPort_HPP

#include <string>

namespace ecf {

// If two different process/servers both try to use the same port number, you
// get an "Address in use" error, even if one the process is dead. This is
// because the kernel does not immediately release the resource and there is
// time out period. To get round this we will start the new server/client
// and use a different port number.

class SCPort {
public:
    /// Check ECF_HOST and ECF_PORT first, otherwise calls next_only
    static std::string next();

    /// make sure we have a unique port, each time next_only() is called;
    static std::string next_only(bool debug = false);

private:
    SCPort()  = delete;
    ~SCPort() = delete;

    static int thePort_;
};

} // namespace ecf

#endif /* ecflow_client_test_SCPort_HPP */
