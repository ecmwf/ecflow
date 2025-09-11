/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_test_InvokeServer_HPP
#define ecflow_http_test_InvokeServer_HPP

#include <iostream>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/server/Server.hpp"
#include "ecflow/server/ServerEnvironment.hpp"

class InvokeServer {
public:
    InvokeServer(bool use_http_backend = false) : use_http_backend_(use_http_backend) {
        std::string port;
        ecf::environment::get(ecf::environment::ECF_PORT, port);
        /// Remove check pt and backup check pt file, else server will load it & remove log file
        ecf::Host h;
        fs::remove(h.ecf_checkpt_file(port));
        fs::remove(h.ecf_backup_checkpt_file(port));
        fs::remove(h.ecf_log_file(port));

        std::string theServerInvokePath = ecf::File::find_ecf_server_path();

        BOOST_REQUIRE_MESSAGE(!theServerInvokePath.empty(), "The server executable could not found");
        BOOST_REQUIRE_MESSAGE(fs::exists(theServerInvokePath),
                              "The server executable does not exist at: " << theServerInvokePath);

        BOOST_TEST_MESSAGE("Using eclow_server from " << theServerInvokePath);

        std::string cmd = theServerInvokePath;
        if (use_http_backend_) {
            cmd += " --http";
        }

        std::thread t([&] { system(cmd.c_str()); });

        t.detach();

        sleep(1);

        ClientInvoker theClient("localhost", port);
        if (use_http_backend_) {
            theClient.enable_http();
        }
        if (theClient.wait_for_server_reply()) {
            theClient.restartServer();
            server_started = true;
        }
        else {
            server_started = false;
        }
    }

    ~InvokeServer() {
        std::string port;
        ecf::environment::get(ecf::environment::ECF_PORT, port);

        BOOST_TEST_MESSAGE("*****InvokeServer:: Closing server on port " << port);
        {
            ClientInvoker theClient("localhost", port);
            if (use_http_backend_) {
                theClient.enable_http();
            }
            BOOST_REQUIRE_NO_THROW(theClient.terminateServer());
            BOOST_REQUIRE_MESSAGE(theClient.wait_for_server_death(), "Failed to terminate server after 60 seconds\n");
        }

        ecf::Host h;
        fs::remove(h.ecf_log_file(port));
        BOOST_CHECK_MESSAGE(!fs::exists(h.ecf_log_file(port)), "log file " << h.ecf_log_file(port) << " not deleted\n");

        fs::remove(h.ecf_checkpt_file(port));
        fs::remove(h.ecf_backup_checkpt_file(port));
        BOOST_CHECK_MESSAGE(!fs::exists(h.ecf_checkpt_file(port)),
                            "file " << h.ecf_checkpt_file(port) << " not deleted\n");
        BOOST_CHECK_MESSAGE(!fs::exists(h.ecf_backup_checkpt_file(port)),
                            "file " << h.ecf_backup_checkpt_file(port) << " not deleted\n");
    }

    bool server_started{false};
    bool use_http_backend_{false};
};

#endif /* ecflow_http_test_InvokeServer_HPP */
