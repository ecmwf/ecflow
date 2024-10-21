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

    /**
     * Check if there is *no* ecFlow server running on the given port.
     *
     * The check is performed by connecting to the port (on localhost) and issuing a ping request.
     *
     * \returns false, if: (1) the server answers with a ping response;
     *                     (2) the request is accepted, but fails due to failed authentication;
     *                     (3) there is a mismatch communication protocol (4.x vs 5.x)
     *          true, otherwise
     */
    static bool is_free_port(int port, bool debug = false);

    static std::string find_available_port(const std::string& port);

    // find free port on local host. Not 100% accurate, use in test
    [[deprecated]] static std::string find_free_port(int seed_port_number, bool debug = false);

private:
    SCPort()  = delete;
    ~SCPort() = delete;

    static int thePort_;
};

} // namespace ecf

#endif /* ecflow_client_test_SCPort_HPP */
