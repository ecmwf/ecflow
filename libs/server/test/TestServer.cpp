// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/OwnerAttr.hpp"
#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/server/Server.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/FakeAccountDatabase.hpp"
#include "ecflow/test/scaffold/FakePrivileges.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_Server)

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// make public the function we wish to test:
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class TestServer : public BasicServer {
public:
    explicit TestServer(boost::asio::io_context& io, ServerEnvironment& s)
        : BasicServer(io, s) {}
    ~TestServer() override = default;

    // abort server if check pt files exist, but cannot be loaded
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

    std::vector<std::string> args = {"ServerEnvironment", "--port=" + port, "--ecfinterval=12"};
    ServerEnvironment server_environment(args); // This can throw ServerEnvironmentException
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

        // The ownership rules use the OS account database, and take the effective user as the server account
        {
            auto defs    = Defs::create(); // kept alive: the suite refers back to it
            auto suite   = defs->add_suite("s");
            auto refusal = [&](const std::string& name) {
                try {
                    theServer.ownership().check_create(Identity::make_user(name, ""), *suite, OwnerAttr::create(name));
                }
                catch (const std::runtime_error& e) {
                    return std::string(e.what());
                }
                return std::string();
            };
            BOOST_CHECK_MESSAGE(refusal("root").find("superuser") != std::string::npos, refusal("root"));
            if (const passwd* pw = ::getpwuid(::geteuid()); pw != nullptr && pw->pw_uid != 0) {
                BOOST_CHECK_MESSAGE(refusal(pw->pw_name).find("server account") != std::string::npos,
                                    refusal(pw->pw_name));
            }
        }

        // The protocol resolved by the environment must be the one reported by the statistics, since that is
        // what users are shown to confirm how the server is being reached.
        const std::string expected_protocol =
            std::string{ecf::Enumerate<ecf::Protocol>::to_string(server_environment.protocol()).value()};
        BOOST_REQUIRE_MESSAGE(theServer.stats().protocol_ == expected_protocol,
                              "Expected the statistics to report protocol '" << expected_protocol << "' but found '"
                                                                             << theServer.stats().protocol_ << "'");

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

    // Reserve a port, so that debug and release, gnu, clang, intel, etc. builds can run at the same time.
    // ECF_FREE_PORT (from metabuilder) seeds the search, and should be unique among those builds.
    ecf::test::scaffold::Port::port_t seed_port = 3144;
    if (auto port = ecf::environment::fetch("ECF_FREE_PORT"); port) {
        seed_port = ecf::convert_to<ecf::test::scaffold::Port::port_t>(port.value());
    }
    std::cout << "  Find free port to start server, starting with port " << seed_port << "\n";

    auto port = ecf::test::scaffold::MakePort{}.with(ecf::test::scaffold::AutomaticPortValue{seed_port}).create();
    std::string the_port = ecf::convert_to<std::string>(port.value());
    std::cout << "  Found free port: " << the_port << "\n";

    test_the_server(the_port);

    // cleanup (the port lock is released when `port` goes out of scope)
    Host h;
    fs::remove(h.ecf_log_file(the_port));

    /// Destroy Log singleton to avoid valgrind from complaining
    Log::destroy();
}

namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

BOOST_AUTO_TEST_CASE(test_server_warns_of_owners_when_ownership_is_disabled) {
    ECF_NAME_THIS_TEST();

    auto port           = ecf::test::scaffold::MakePort{}.with(ecf::test::scaffold::AutomaticPortValue{3144}).create();
    const auto the_port = ecf::convert_to<std::string>(port.value());
    Host host;
    {
        ServerEnvironment server_environment(std::vector<std::string>{"ServerEnvironment", "--port=" + the_port});
        std::string error;
        BOOST_REQUIRE_MESSAGE(server_environment.valid(error), error);

        // A checkpoint holding an Owner, restored at start-up
        Defs defs;
        defs.add_suite("s")->addOwner(OwnerAttr::create("alice"));
        defs.write_to_checkpt_file(server_environment.checkPtFilename());

        boost::asio::io_context io;
        TestServer server(io, server_environment);
        BOOST_REQUIRE(server.defs()->findAbsNode("/s")->get_owner());
        BOOST_CHECK(!server.defs()->server_state().server_owner());
        const auto log = read_file(Log::instance()->path());
        BOOST_CHECK_MESSAGE(log.find("Ownership is disabled, but the definitions hold Owners") != std::string::npos,
                            log);
        fs::remove(server_environment.checkPtFilename());
    }
    fs::remove(host.ecf_log_file(the_port));
    Log::destroy();
}

#ifdef ECF_OWNERSHIP_UNDER_TEST
BOOST_AUTO_TEST_CASE(test_server_owner_set_on_the_definitions) {
    ECF_NAME_THIS_TEST();

    auto port           = ecf::test::scaffold::MakePort{}.with(ecf::test::scaffold::AutomaticPortValue{3144}).create();
    const auto the_port = ecf::convert_to<std::string>(port.value());
    Host host;
    {
        ServerEnvironment server_environment(
            std::vector<std::string>{"ServerEnvironment", "--port=" + the_port, "--ownership_owner=ecflow-default"});
        ecf::test::scaffold::FakeAccountDatabase accounts;
        accounts.add_group("users", 100).add_user("ecflow-default", 1000, 100);
        ecf::test::scaffold::FakePrivileges privileges;
        privileges.has_capabilities = true;
        std::string error;
        BOOST_REQUIRE_MESSAGE(server_environment.valid(error, accounts, privileges), error);

        boost::asio::io_context io;
        TestServer server(io, server_environment);
        auto check_server_owner = [&server](const std::string& when) {
            const auto* owner = server.defs()->server_state().server_owner();
            BOOST_REQUIRE_MESSAGE(owner, "No server Owner " << when);
            BOOST_CHECK_EQUAL(owner->value(), "ecflow-default");
        };
        check_server_owner("after start-up");
        BOOST_CHECK(server.stats().ownership_.find("server Owner ecflow-default") != std::string::npos);

        // Never checkpointed, and set again after a restore
        defs_ptr loaded = Defs::create();
        loaded->add_suite("s");
        server.updateDefs(loaded, false);
        check_server_owner("after a load");
        BOOST_REQUIRE(server.checkPtDefs());
        server.clear_defs(); // as --restore_from_checkpt requires
        server.restore_defs_from_checkpt();
        BOOST_CHECK(server.defs()->findAbsNode("/s"));
        check_server_owner("after a restore");
        fs::remove(server_environment.checkPtFilename());
        fs::remove(server_environment.oldCheckPtFilename());
    }
    fs::remove(host.ecf_log_file(the_port));
    Log::destroy();
}
#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
