/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_client_test_InvokeServer_HPP
#define ecflow_client_test_InvokeServer_HPP

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Str.hpp"

class InvokeServer {
public:
    InvokeServer()                    = delete;
    InvokeServer(const InvokeServer&) = delete;
    explicit InvokeServer(const std::string& msg,
                          const std::string& port                      = ecf::Str::DEFAULT_PORT_NUMBER(),
                          bool disable_job_generation                  = false,
                          bool remove_checkpt_file_before_server_start = true,
                          bool remove_checkpt_file_after_server_exit   = true)
        : port_(port),
          host_(ClientEnvironment::hostSpecified()),
          remove_checkpt_file_after_server_exit_(remove_checkpt_file_after_server_exit) {
        if (host_.empty()) {
            if (!msg.empty()) {
                std::cout << msg << "   port(" << port_ << ")";
#ifdef ECF_OPENSSL
                if (ecf::environment::has(ecf::environment::ECF_SSL)) {
                    std::cout << " (ssl)";
                }
#endif
                std::cout << std::endl;
            }

            doStart(msg, port_, server_started_, disable_job_generation, remove_checkpt_file_before_server_start);
        }
        else {
            // Start of test, clear any existing defs on remote/localhost server
            // Assuming this has been started on input port
            std::string test_name = msg;
            test_name += " on ";
            test_name += host_;
            test_name += ecf::Str::COLON();
            test_name += port_;

            std::cout << test_name << std::endl;

            ClientInvoker theClient(host_, port_);
            theClient.logMsg(test_name);
            BOOST_REQUIRE_MESSAGE(theClient.delete_all() == 0,
                                  CtsApi::to_string(CtsApi::delete_node())
                                      << " failed should return 0. Should Delete ALL existing defs in the server\n"
                                      << theClient.errorMsg());
            server_started_ = true;
        }
    }

    InvokeServer(const std::string& port,
                 bool stop_ambiguaty,
                 bool disable_job_generation                  = false,
                 bool remove_checkpt_file_before_server_start = true,
                 bool remove_checkpt_file_after_server_exit   = true)
        : port_(port),
          remove_checkpt_file_after_server_exit_(remove_checkpt_file_after_server_exit) {
        // host_ is empty.
        doStart("", port_, server_started_, disable_job_generation, remove_checkpt_file_before_server_start);
    }

    ~InvokeServer() {
        // This will also remove the generated files.
        // Will only terminate local server, host_ is *EMPTY* for local server, using two constructors above
        if (host_.empty()) {
            doEnd(ecf::Str::LOCALHOST(),
                  port_,
                  remove_checkpt_file_after_server_exit_,
                  remove_log_file_after_server_exit_);
        }
    }

    InvokeServer& operator=(const InvokeServer&) = delete;

    const std::string& port() const { return port_; }
    const std::string& host() const {
        if (host_.empty())
            return ecf::Str::LOCALHOST();
        return host_;
    }

    std::string ecf_log_file() const { return host_name_.ecf_log_file(port_); }
    std::string ecf_checkpt_file() const { return host_name_.ecf_checkpt_file(port_); }
    std::string ecf_backup_checkpt_file() const { return host_name_.ecf_backup_checkpt_file(port_); }
    bool server_started() const { return server_started_; }
    void keep_log_file_after_server_exit() { remove_log_file_after_server_exit_ = false; }

private:
    static void doStart(const std::string& msg,
                        const std::string& port,
                        bool& server_started,
                        bool disable_job_generation                  = false,
                        bool remove_checkpt_file_before_server_start = true) {
        /// Remove check pt and backup check pt file, else server will load it & remove log file
        ecf::Host h;
        if (remove_checkpt_file_before_server_start) {
            fs::remove(h.ecf_checkpt_file(port));
            fs::remove(h.ecf_backup_checkpt_file(port));
        }
        fs::remove(h.ecf_log_file(port));

        // start the server in the background
        std::string theServerInvokePath = ecf::File::find_ecf_server_path();
        BOOST_REQUIRE_MESSAGE(!theServerInvokePath.empty(),
                              "InvokeServer::doStart: `ecflow_server` executable must be available");
        BOOST_REQUIRE_MESSAGE(fs::exists(theServerInvokePath),
                              "InvokeServer::doStart: `ecflow_server` executable available at:" << theServerInvokePath);

        // Create a port file. To avoid creating multiple servers on the same port number
        ecf::EcfPortLock::create(port);

        // Make sure server starts in the background to avoid hanging test
        theServerInvokePath += " --port=" + port;
        if (disable_job_generation) {
            theServerInvokePath += " --dis_job_gen";
        }
        theServerInvokePath += " &";

        // std::cout << "InvokeServer::doStart port = " << port << " server path = " <<  theServerInvokePath << "\n";
        (void)system(theServerInvokePath.c_str());

        // Allow time for server process to kick in.
        ClientInvoker theClient(ecf::Str::LOCALHOST(), port);
        if (theClient.wait_for_server_reply()) {
            server_started = true;
            if (!msg.empty())
                theClient.logMsg(msg);
        }
        else {
            server_started = false;
        }
    }

    static void doEnd(const std::string& host,
                      const std::string& port,
                      bool remove_checkpt_file_after_server_exit,
                      bool remove_log_file_after_server_exit) {
        //    std::cout << "*****InvokeServer::doEnd    Closing server on  " << host << ecf::Str::COLON() << port <<
        //    "\n";
        {
            ClientInvoker theClient(host, port);
            BOOST_REQUIRE_NO_THROW(theClient.terminateServer());
            BOOST_REQUIRE_MESSAGE(theClient.wait_for_server_death(), "Failed to terminate server after 60 seconds\n");
        }

        // remove port file. This prevented multiple different process from opening servers with same port number
        ecf::EcfPortLock::remove(port);

        // Remove generated file comment for debug
        ecf::Host h;
        if (remove_log_file_after_server_exit) {
            fs::remove(h.ecf_log_file(port));
            BOOST_CHECK_MESSAGE(!fs::exists(h.ecf_log_file(port)),
                                "log file " << h.ecf_log_file(port) << " not deleted\n");
        }

        if (remove_checkpt_file_after_server_exit) {
            fs::remove(h.ecf_checkpt_file(port));
            fs::remove(h.ecf_backup_checkpt_file(port));
            BOOST_CHECK_MESSAGE(!fs::exists(h.ecf_checkpt_file(port)),
                                "file " << h.ecf_checkpt_file(port) << " not deleted\n");
            BOOST_CHECK_MESSAGE(!fs::exists(h.ecf_backup_checkpt_file(port)),
                                "file " << h.ecf_backup_checkpt_file(port) << " not deleted\n");
        }
    }

private:
    std::string port_;
    std::string host_;
    ecf::Host host_name_;
    bool remove_checkpt_file_after_server_exit_{true};
    bool remove_log_file_after_server_exit_{true};
    bool server_started_{false};
};

#endif /* ecflow_client_test_InvokeServer_HPP */
