// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_Scaffold)

///
/// @brief Verifies repeated waits and termination after natural exit without reaping twice.
///
BOOST_AUTO_TEST_CASE(test_process_wait_caches_exit_status) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    Process process("/bin/sh", {"-c", "exit 23"});
    BOOST_CHECK(process.is_running());
    BOOST_CHECK_EQUAL(process.wait(), 23);
    BOOST_CHECK(!process.is_running());
    BOOST_CHECK_EQUAL(process.wait(), 23);
    BOOST_CHECK_EQUAL(process.terminate(), 0);
    BOOST_CHECK_EQUAL(process.wait(), 23);
}

///
/// @brief Verifies that an object without a child can be terminated, but not waited for or asked for its id.
///
BOOST_AUTO_TEST_CASE(test_process_without_child) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    Process empty;
    BOOST_CHECK(!empty.is_running());
    BOOST_CHECK_EQUAL(empty.terminate(), 0);
    BOOST_CHECK_THROW(empty.wait(), std::logic_error);
    BOOST_CHECK_THROW(empty.pid(), std::logic_error);

    Process owner("/bin/sh", {"-c", "exit 0"});
    Process moved(std::move(owner));
    BOOST_CHECK(!owner.is_running()); // NOLINT(bugprone-use-after-move): the moved-from state is under test
    BOOST_CHECK_THROW(owner.wait(), std::logic_error);
    BOOST_CHECK_EQUAL(moved.wait(), 0);
}

///
/// @brief Verifies termination followed by repeated waits and cleanup without a second native reap.
///
BOOST_AUTO_TEST_CASE(test_process_terminate_then_wait) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    Process process("/bin/sh", {"-c", "exec sleep 30"});
    static_assert(noexcept(process.terminate()));
    BOOST_REQUIRE_EQUAL(process.terminate(), 0);
    const int status = process.wait();
    BOOST_CHECK_NE(status, 0);
    BOOST_CHECK_EQUAL(process.wait(), status);
    BOOST_CHECK_EQUAL(process.terminate(), 0);
    BOOST_CHECK_EQUAL(process.wait(), status);
}

///
/// @brief Verifies that termination cooperates with a concurrent waiter instead of racing to reap the child.
///
/// The waiter usually, but not always, blocks before termination starts; the iterations cover both orders. Both
/// requests run on detached threads with bounded waits, so that a deadlock fails the test instead of hanging it.
///
BOOST_AUTO_TEST_CASE(test_process_terminate_with_pending_wait) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    // Runs the request on a detached thread, which shares ownership of the process in case the test gives up
    const auto run_detached = [](const std::shared_ptr<Process>& process, std::function<int(Process&)> request) {
        std::packaged_task<int()> task([process, request = std::move(request)] { return request(*process); });
        auto result = task.get_future();
        std::thread(std::move(task)).detach();
        return result;
    };

    for (int iteration = 0; iteration < 8; ++iteration) {
        BOOST_TEST_CONTEXT("Wait/terminate interleaving " << iteration) {
            auto process     = std::make_shared<Process>("/bin/sh", std::vector<std::string>{"-c", "exec sleep 30"});
            auto completion  = run_detached(process, [](Process& p) { return p.wait(); });
            auto termination = run_detached(process, [](Process& p) { return p.terminate(); });

            BOOST_REQUIRE(termination.wait_for(server_startup_timeout) == std::future_status::ready);
            BOOST_REQUIRE_EQUAL(termination.get(), 0);
            BOOST_REQUIRE(completion.wait_for(server_startup_timeout) == std::future_status::ready);
            const int status = completion.get();
            BOOST_CHECK_NE(status, 0);
            BOOST_CHECK_EQUAL(process->wait(), status);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_creating_host) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    {
        auto host = MakeHost{}.create();
        BOOST_CHECK(host.is_valid());
        BOOST_CHECK(host.value() == Host::resolve_hostname());
    }

    {
        const std::string custom_name = "custom";
        auto host                     = MakeHost{}.with(custom_name).create();
        BOOST_CHECK(host.is_valid());
        BOOST_CHECK(host.value() == custom_name);
    }

    {
        BOOST_CHECK_EXCEPTION(
            [[maybe_unused]] auto host = MakeHost{}.with(std::string{}).create(),
            std::runtime_error,
            [](const std::runtime_error& e) { return std::string(e.what()) == "host name cannot be empty\n"; });
    }
}

BOOST_AUTO_TEST_CASE(test_locking_known_tcp_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::test::scaffold;

    auto port_a = MakePort{}.with(SpecificPortValue{24444}).create();
    auto port_b = MakePort{}.with(SpecificPortValue{24445}).create();
    auto port_c = MakePort{}.with(SpecificPortValue{24446}).create();

    BOOST_TEST_MESSAGE("Acquired port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_b.value() << " at " << port_b.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_c.value() << " at " << port_c.lock_location());

    BOOST_CHECK(port_a.value() == 24444);
    BOOST_CHECK(port_b.value() == 24445);
    BOOST_CHECK(port_c.value() == 24446);
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
    BOOST_CHECK(fs::exists(port_c.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_locking_automatic_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::test::scaffold;

    auto port_a = MakePort{}.with(AutomaticPortValue{}).create();
    auto port_b = MakePort{}.with(AutomaticPortValue{24444}).create();
    auto port_c = MakePort{}.with(AutomaticPortValue{24444}).create();

    BOOST_TEST_MESSAGE("Acquired port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_b.value() << " at " << port_b.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_c.value() << " at " << port_c.lock_location());

    BOOST_CHECK(port_a.value() != port_b.value());
    BOOST_CHECK(port_a.value() != port_c.value());
    BOOST_CHECK(port_b.value() != port_c.value());
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
    BOOST_CHECK(fs::exists(port_c.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_locking_known_udp_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::test::scaffold;

    // Occupy UDP port 24447, as a running ecflow_udp would
    boost::asio::io_context io;
    boost::asio::ip::udp::socket occupied(io, {boost::asio::ip::udp::v4(), 24447});

    // A UDP reservation skips the occupied port, while a TCP reservation of the same number is unaffected
    auto port_a = MakePort{}.with(AutomaticPortValue{24447, Transport::UDP}).create();
    BOOST_CHECK_THROW(MakePort{}.with(SpecificPortValue{24447, Transport::UDP}).create(), MakePort::UnableToLockPort);
    auto port_b = MakePort{}.with(SpecificPortValue{24447, Transport::TCP}).create();

    BOOST_TEST_MESSAGE("Acquired UDP port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired TCP port: " << port_b.value() << " at " << port_b.lock_location());

    BOOST_CHECK(port_a.value() == 24448);
    BOOST_CHECK(port_b.value() == 24447);
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_setting_environment_variables) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    {
        std::string name     = "";
        std::string expected = "value";
        BOOST_CHECK_EXCEPTION([[maybe_unused]] auto variable = MakeEnvironmentVariable{}.with(name, expected).create(),
                              MakeEnvironmentVariable::InvalidVariableName,
                              [](const MakeEnvironmentVariable::InvalidVariableName& e) {
                                  return std::string(e.what()) == "Environment variable name cannot be empty";
                              });
    }

    std::string name = "FOOLPROOF_VARIABLE";
    {
        std::string expected = "";
        auto variable        = MakeEnvironmentVariable{}.with(name, expected).create();
        auto actual          = EnvironmentVariable::get_environment_variable(name);

        BOOST_CHECK(variable.value() == expected);
        BOOST_CHECK(actual.has_value() && (actual.value() == expected));
    }
    auto retrieved = EnvironmentVariable::get_environment_variable(name);
    BOOST_CHECK(!retrieved.has_value());

    {
        std::string expected = "value";
        auto variable        = MakeEnvironmentVariable{}.with(name, expected).create();
        auto actual          = EnvironmentVariable::get_environment_variable(name);

        BOOST_CHECK(variable.value() == expected);
        BOOST_CHECK(actual.has_value() && (actual.value() == expected));
    }
    retrieved = EnvironmentVariable::get_environment_variable(name);
    BOOST_CHECK(!retrieved.has_value());
}

BOOST_AUTO_TEST_CASE(test_creating_working_directory) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    BOOST_CHECK(!cwd.path().string().empty());
    BOOST_CHECK(fs::exists(cwd.path()));
    BOOST_CHECK(fs::is_directory(cwd.path()));
    BOOST_CHECK(cwd.path().is_absolute());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
