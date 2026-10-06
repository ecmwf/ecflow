// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "LocalAvisoServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::SCPort;
using ecf::System;
using ecf::test::LocalAvisoServer;

namespace {

using namespace std::chrono_literals;

///
/// @brief Starts a local Aviso server, on a port reserved from the base given by the test environment.
///
struct WithLocalAvisoServer
{
    WithLocalAvisoServer()
        : port{ecf::test::scaffold::MakePort{}
                   .with(ecf::test::scaffold::AutomaticPortValue{base_port()})
                   .create_owned()},
          server{static_cast<int>(port->value())},
          auth{fs::temp_directory_path() / ("ecflow_aviso_auth_" + std::to_string(port->value()) + ".json")} {
        std::ofstream{auth} << R"({ "email": "user@host.int", "key": "abc" })";
        server.require_bearer_auth("abc");
    }

    ~WithLocalAvisoServer() { fs::remove(auth); }

    static ecf::test::scaffold::Port::port_t base_port() {
        const char* base = std::getenv("ECF_TEST_AVISO_PORT_BASE");
        if (base == nullptr) {
            throw std::runtime_error(
                "ECF_TEST_AVISO_PORT_BASE is not defined (it is set by the CMake test definition)");
        }
        return static_cast<ecf::test::scaffold::Port::port_t>(std::stoul(base));
    }

    std::unique_ptr<ecf::test::scaffold::Port> port;
    LocalAvisoServer server;
    fs::path auth;
};

const std::string listener = R"({ "event": "test_event", "request": { "date": "20261006" } })";

defs_ptr
make_defs(const std::string& url, const fs::path& auth, ecf::AvisoAttr::revision_t revision, bool collapse = false) {
    defs_ptr defs   = Defs::create();
    suite_ptr suite = defs->add_suite("s");
    suite->addVariable(Variable("ECF_AVISO_URL", url));
    suite->addVariable(Variable("ECF_AVISO_AUTH", auth.string()));
    task_ptr task = suite->add_task("t");
    task->addAviso(
        ecf::AvisoAttr{nullptr, "A", listener, "%ECF_AVISO_URL%", revision, "%ECF_AVISO_AUTH%", "", collapse});
    return defs;
}

///
/// @brief Synchronises with the server until the predicate holds for the task, or the timeout expires.
///
bool wait_for_task(ClientInvoker& client,
                   const std::function<bool(const Task&)>& predicate,
                   std::chrono::milliseconds timeout = 10s) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (client.sync_local() == 0 && client.defs()) {
            if (auto node = client.defs()->findAbsNode("/s/t"); node && node->isTask()) {
                if (predicate(*node->isTask())) {
                    return true;
                }
            }
        }
        std::this_thread::sleep_for(100ms);
    }
    return false;
}

bool is_released(const Task& task) {
    return task.state() != NState::QUEUED;
}

ecf::AvisoAttr::revision_t revision_of(ClientInvoker& client) {
    client.sync_local();
    return client.defs()->findAbsNode("/s/t")->avisos().front().revision();
}

void start(ClientInvoker& client, const defs_ptr& defs) {
    BOOST_REQUIRE_MESSAGE(client.load(defs) == 0, "load failed: " << client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.restartServer() == 0, "restart failed: " << client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.begin("s") == 0, "begin failed: " << client.errorMsg());
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_FIXTURE_TEST_SUITE(T_AvisoAgainstServer, WithLocalAvisoServer)

BOOST_AUTO_TEST_CASE(releases_task_when_notification_is_published) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...releases_task_when_notification_is_published", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to start on port " << invokeServer.port());

    ClientInvoker client(invokeServer.host(), invokeServer.port());
    client.set_throw_on_error(false);
    start(client, make_defs(server.url(), auth, 0));

    BOOST_REQUIRE(server.wait_for_requests(1, 10s));
    BOOST_CHECK(!wait_for_task(client, is_released, 1s));

    server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");

    BOOST_CHECK_MESSAGE(wait_for_task(client, is_released), "Expected the task to be released by the notification");
    BOOST_CHECK_EQUAL(revision_of(client), 1u);

    System::destroy();
}

BOOST_AUTO_TEST_CASE(waits_for_new_notification_after_requeue) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...waits_for_new_notification_after_requeue", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to start on port " << invokeServer.port());

    ClientInvoker client(invokeServer.host(), invokeServer.port());
    client.set_throw_on_error(false);
    start(client, make_defs(server.url(), auth, 0));

    BOOST_REQUIRE(server.wait_for_requests(1, 10s));
    server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");
    BOOST_REQUIRE(wait_for_task(client, is_released));

    // Once requeued, the task resumes after the notification already consumed, and waits for a new one
    BOOST_REQUIRE_MESSAGE(client.requeue("/s/t") == 0, "requeue failed: " << client.errorMsg());
    BOOST_REQUIRE(server.wait_for_requests(2, 10s));
    BOOST_CHECK(!wait_for_task(client, is_released, 1s));

    server.publish("test_event", R"({ "date": "20261006", "time": "1800" })");
    BOOST_CHECK_MESSAGE(wait_for_task(client, is_released), "Expected the task to be released by the new notification");
    BOOST_CHECK_EQUAL(revision_of(client), 2u);

    System::destroy();
}

BOOST_AUTO_TEST_CASE(resumes_after_stored_revision) {
    ECF_NAME_THIS_TEST();

    // Notifications 1 and 2 were consumed before the definition was stored (e.g. in a checkpoint); 3 was not
    for (int i = 0; i < 3; ++i) {
        server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");
    }

    InvokeServer invokeServer("Client:: ...resumes_after_stored_revision", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to start on port " << invokeServer.port());

    ClientInvoker client(invokeServer.host(), invokeServer.port());
    client.set_throw_on_error(false);
    start(client, make_defs(server.url(), auth, 2));

    BOOST_CHECK_MESSAGE(wait_for_task(client, is_released), "Expected the task to be released by notification 3");
    BOOST_CHECK_EQUAL(revision_of(client), 3u);

    System::destroy();
}

BOOST_AUTO_TEST_CASE(releases_task_once_per_notification) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...releases_task_once_per_notification", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to start on port " << invokeServer.port());

    ClientInvoker client(invokeServer.host(), invokeServer.port());
    client.set_throw_on_error(false);
    start(client, make_defs(server.url(), auth, 0));

    BOOST_REQUIRE(server.wait_for_requests(1, 10s));
    for (int i = 0; i < 3; ++i) {
        server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");
    }

    // A burst of three notifications releases the task three times, once per notification, in order
    for (ecf::AvisoAttr::revision_t expected = 1; expected <= 3; ++expected) {
        BOOST_REQUIRE_MESSAGE(wait_for_task(client, is_released), "Expected release by notification " << expected);
        BOOST_CHECK_EQUAL(revision_of(client), expected);
        BOOST_REQUIRE_MESSAGE(client.requeue("/s/t") == 0, "requeue failed: " << client.errorMsg());
    }
    BOOST_CHECK(!wait_for_task(client, is_released, 1s));

    System::destroy();
}

BOOST_AUTO_TEST_CASE(releases_task_once_for_all_notifications_when_collapsing) {
    ECF_NAME_THIS_TEST();

    InvokeServer invokeServer("Client:: ...releases_task_once_for_all_notifications_when_collapsing", SCPort::next());
    BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to start on port " << invokeServer.port());

    ClientInvoker client(invokeServer.host(), invokeServer.port());
    client.set_throw_on_error(false);

    start(client, make_defs(server.url(), auth, 0, true));
    BOOST_REQUIRE(server.wait_for_requests(1, 10s));

    // The task is suspended while the burst arrives, so that it is evaluated once all three are received
    BOOST_REQUIRE_MESSAGE(client.suspend("/s/t") == 0, "suspend failed: " << client.errorMsg());
    for (int i = 0; i < 3; ++i) {
        server.publish("test_event", R"({ "date": "20261006", "time": "1800" })");
    }
    std::this_thread::sleep_for(1s);
    BOOST_REQUIRE_MESSAGE(client.resume("/s/t") == 0, "resume failed: " << client.errorMsg());

    BOOST_REQUIRE_MESSAGE(wait_for_task(client, is_released), "Expected the task to be released by the burst");
    BOOST_CHECK_EQUAL(revision_of(client), 3u);

    BOOST_REQUIRE_MESSAGE(client.requeue("/s/t") == 0, "requeue failed: " << client.errorMsg());
    BOOST_CHECK(!wait_for_task(client, is_released, 1s));

    System::destroy();
}

BOOST_AUTO_TEST_CASE(continues_releasing_once_per_notification_after_server_restart) {
    ECF_NAME_THIS_TEST();

    const std::string port = SCPort::next();

    {
        // The first server is released by notification 1, and checkpoints with notifications 2 and 3 not consumed
        InvokeServer invokeServer("Client:: ...continues_releasing_once_per_notification_after_server_restart",
                                  port,
                                  false /* disable_job_generation */,
                                  true /* remove_checkpt_file_before_server_start */,
                                  false /* remove_checkpt_file_after_server_exit */);
        BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to start on port " << port);

        ClientInvoker client(invokeServer.host(), invokeServer.port());
        client.set_throw_on_error(false);
        start(client, make_defs(server.url(), auth, 0));

        BOOST_REQUIRE(server.wait_for_requests(1, 10s));
        for (int i = 0; i < 3; ++i) {
            server.publish("test_event", R"({ "date": "20261006", "time": "1200" })");
        }

        BOOST_REQUIRE_MESSAGE(wait_for_task(client, is_released), "Expected release by notification 1");
        BOOST_CHECK_EQUAL(revision_of(client), 1u);

        BOOST_REQUIRE_MESSAGE(client.haltServer() == 0, "halt failed: " << client.errorMsg());
        BOOST_REQUIRE_MESSAGE(client.requeue("/s/t") == 0, "requeue failed: " << client.errorMsg());
        BOOST_REQUIRE_MESSAGE(client.checkPtDefs() == 0, "checkpoint failed: " << client.errorMsg());
    }

    {
        // The second server loads the checkpoint, and is released by notifications 2 and 3, in turn
        InvokeServer invokeServer("Client:: ...continues_releasing_once_per_notification_after_server_restart (2)",
                                  port,
                                  false /* disable_job_generation */,
                                  false /* remove_checkpt_file_before_server_start */,
                                  true /* remove_checkpt_file_after_server_exit */);
        BOOST_REQUIRE_MESSAGE(invokeServer.server_started(), "Server failed to restart on port " << port);

        ClientInvoker client(invokeServer.host(), invokeServer.port());
        client.set_throw_on_error(false);
        BOOST_REQUIRE_MESSAGE(client.restartServer() == 0, "restart failed: " << client.errorMsg());

        BOOST_CHECK_EQUAL(revision_of(client), 1u);
        for (ecf::AvisoAttr::revision_t expected = 2; expected <= 3; ++expected) {
            BOOST_REQUIRE_MESSAGE(wait_for_task(client, is_released),
                                  "Expected release by notification " << expected << " after the restart");
            BOOST_CHECK_EQUAL(revision_of(client), expected);
            BOOST_REQUIRE_MESSAGE(client.requeue("/s/t") == 0, "requeue failed: " << client.errorMsg());
        }
        BOOST_CHECK(!wait_for_task(client, is_released, 1s));
    }

    System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
