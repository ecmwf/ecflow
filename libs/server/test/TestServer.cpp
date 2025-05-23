/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/server/Server.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_Server)

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// make public the function we wish to test:
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class TestServer : public BasicServer {
public:
    explicit TestServer(boost::asio::io_context& io, ServerEnvironment& s) : BasicServer(io, s) {}
    ~TestServer() override = default;

    // abort server if check pt files exist, but can't be loaded
    //   bool load_check_pt_file_on_startup();
    //   void loadCheckPtFile();
    //   bool restore_from_checkpt(const std::string& filename, bool& failed);
    //   void set_server_state(SState::State);

    /// AbstractServer functions
    SState::State state() const override { return BasicServer::state(); }
    std::pair<std::string, std::string> hostPort() const override { return BasicServer::hostPort(); }
    defs_ptr defs() const override { return BasicServer::defs(); }
    // virtual void updateDefs(defs_ptr,bool force);
    void clear_defs() override { BasicServer::clear_defs(); }
    //   virtual void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
    //                               int check_pt_interval = 0,
    //                               int check_pt_save_time_alarm = 0);
    void restore_defs_from_checkpt() override { BasicServer::restore_defs_from_checkpt(); }
    void nodeTreeStateChanged() override { BasicServer::nodeTreeStateChanged(); }
    bool allowTaskCommunication() const override { return BasicServer::allowTaskCommunication(); }
    void shutdown() override { BasicServer::shutdown(); }
    void halted() override { BasicServer::halted(); }
    void restart() override { BasicServer::restart(); }

    bool reloadWhiteListFile(std::string& errorMsg) override { return BasicServer::reloadWhiteListFile(errorMsg); }
    bool reloadPasswdFile(std::string& errorMsg) override { return BasicServer::reloadPasswdFile(errorMsg); }

    bool authenticateReadAccess(const std::string& user, bool custom_user, const std::string& passwd) override {
        return BasicServer::authenticateReadAccess(user, custom_user, passwd);
    }
    bool authenticateReadAccess(const std::string& user,
                                bool custom_user,
                                const std::string& passwd,
                                const std::string& path) override {
        return BasicServer::authenticateReadAccess(user, custom_user, passwd, path);
    }
    bool authenticateReadAccess(const std::string& user,
                                bool custom_user,
                                const std::string& passwd,
                                const std::vector<std::string>& paths) override {
        return BasicServer::authenticateReadAccess(user, custom_user, passwd, paths);
    }

    bool authenticateWriteAccess(const std::string& user) override {
        return BasicServer::authenticateWriteAccess(user);
    }
    bool authenticateWriteAccess(const std::string& user, const std::string& path) override {
        return BasicServer::authenticateWriteAccess(user, path);
    }
    bool authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths) override {
        return BasicServer::authenticateWriteAccess(user, paths);
    }

    bool lock(const std::string& user) override { return BasicServer::lock(user); }
    void unlock() override { BasicServer::unlock(); }
    const std::string& lockedUser() const override { return BasicServer::lockedUser(); }
    //   virtual void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now, bool
    //   user_cmd_context) const;
    int poll_interval() const override { return BasicServer::poll_interval(); }
    //   virtual void debug_server_on();
    //   virtual void debug_server_off();
    //   virtual bool debug() const;
};

void test_the_server(const std::string& port) {
    std::string server_port = "--port=" + port;
    int argc                = 3;
    char* argv[]            = {const_cast<char*>("ServerEnvironment"),
                               const_cast<char*>(server_port.c_str()),
                               const_cast<char*>("--ecfinterval=12")};

    ServerEnvironment server_environment(argc, argv); // This can throw ServerEnvironmentException
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(server_environment.valid(errorMsg), errorMsg);

    {
        boost::asio::io_context io;
        TestServer theServer(io, server_environment); // This can throw exception, bind address in use.

        BOOST_REQUIRE_MESSAGE(theServer.defs(), "Expected defs to be created");

        const std::vector<Variable>& server_variables = theServer.defs()->server_state().server_variables();
        BOOST_REQUIRE_MESSAGE(!server_variables.empty(), "Expected defs to be updated with the server variables");

        // for(size_t i = 0; i < server_variables.size(); ++i)  cout << server_variables[i].dump() << "\n";
        const std::string& ecf_port = theServer.defs()->server_state().find_variable("ECF_PORT");
        BOOST_REQUIRE_MESSAGE(ecf_port == port,
                              "Expected port " << port << " but found " << ecf_port
                                               << " defs server variables, should be in sync with server");

        const std::string& interval = theServer.defs()->server_state().find_variable("ECF_INTERVAL");
        BOOST_REQUIRE_MESSAGE(interval == "12",
                              "Expected interval 12 but found "
                                  << interval << " defs server variables, should be in sync with server");
        BOOST_REQUIRE_MESSAGE(theServer.poll_interval() == 12,
                              "Expected poll interval 12 but found " << theServer.poll_interval());

        BOOST_REQUIRE_MESSAGE(theServer.state() == SState::HALTED,
                              "Expected halted at server start but found " << SState::to_string(theServer.state()));
        theServer.halted();
        BOOST_REQUIRE_MESSAGE(theServer.state() == SState::HALTED, "Expected halted ");
        theServer.shutdown();
        BOOST_REQUIRE_MESSAGE(theServer.state() == SState::SHUTDOWN, "Expected shutdown ");
        theServer.restart();
        BOOST_REQUIRE_MESSAGE(theServer.state() == SState::RUNNING, "Expected shutdown ");

        BOOST_REQUIRE_MESSAGE(theServer.lock("fred"), "Expected to lock user fred");
        BOOST_REQUIRE_MESSAGE(theServer.state() == SState::SHUTDOWN, "Locking should shutdown server");
        BOOST_REQUIRE_MESSAGE(theServer.lockedUser() == "fred",
                              "Expected locked user 'fred' but found " << theServer.lockedUser());
        theServer.unlock();
        BOOST_REQUIRE_MESSAGE(theServer.lockedUser().empty(),
                              "Expected no locked user but found " << theServer.lockedUser());
        BOOST_REQUIRE_MESSAGE(theServer.state() == SState::RUNNING, "Expected unlock to restart server ");
    }
}

BOOST_AUTO_TEST_CASE(test_server) {
    ECF_NAME_THIS_TEST();

    // Create a unique port number, allowing debug and release,gnu,clang,intel to run at the same time
    // Hence the lock file is not always sufficient.
    // ECF_FREE_PORT should be unique among  gnu,clang,intel, etc
    std::string the_port1 = "3144";
    if (auto port = ecf::environment::fetch("ECF_FREE_PORT"); port) { // from metabuilder, allow parallel tests
        the_port1 = port.value();
    }
    cout << "  Find free port to start server, starting with port " << the_port1 << "\n";

    auto the_port = ecf::convert_to<int>(the_port1);
    while (!EcfPortLock::is_free(the_port))
        the_port++;
    std::string port = ecf::convert_to<std::string>(the_port);
    EcfPortLock::create(port);
    cout << "  Found free port: " << port << " ";

    Host h;
    int count = 0;
    while (1) {
        try {
            test_the_server(port);
            cout << "\n";
            break;
        }
        catch (...) {
            count++;

            // cleanup
            fs::remove(h.ecf_log_file(port));
            EcfPortLock::remove(port);

            cout << " : port " << port << " is used, trying next port\n";

            the_port = ecf::convert_to<int>(port);
            the_port++;

            while (!EcfPortLock::is_free(the_port))
                the_port++;
            port = ecf::convert_to<std::string>(the_port);
            EcfPortLock::create(port);
            cout << "  Found free port: " << port << "\n";

            BOOST_REQUIRE_MESSAGE(count < 20, "Could not find new port after 20 attempts");
        }
    }

    // cleanup
    fs::remove(h.ecf_log_file(port));
    EcfPortLock::remove(port);

    /// Destroy Log singleton to avoid valgrind from complaining
    Log::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
