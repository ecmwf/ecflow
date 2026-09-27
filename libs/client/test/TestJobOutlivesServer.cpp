// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <csignal>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/EcfPortLock.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

namespace {

const std::string suite_name = "test_job_outlives_server";

///
/// @brief The ECF_HOME of the test, holding the script of a job that outlives the server that submitted it.
///
/// The job writes its process identifier to a file, then sleeps for longer than the test lasts. The directory is
/// removed, and the job killed, when the fixture is destroyed, even when the test fails.
///
struct OutlivingJob
{
    OutlivingJob() {
        fs::remove_all(home);
        fs::create_directories(home / suite_name);
        std::ofstream script(home / suite_name / "t.ecf");
        script << "echo $$ > %ECF_HOME%/job.pid\n"
               << "exec sleep 300\n";
    }

    OutlivingJob(const OutlivingJob&)            = delete;
    OutlivingJob& operator=(const OutlivingJob&) = delete;

    ~OutlivingJob() { stop(); }

    /// @brief Kills the job, if it runs, and removes the directory.
    void stop() {
        if (pid > 0) {
            ::kill(pid, SIGKILL);
            pid = 0;
        }
        std::error_code ignored;
        fs::remove_all(home, ignored);
    }

    /// @brief Returns the definition of a suite whose only task runs the job.
    defs_ptr defs() const {
        defs_ptr defs   = Defs::create();
        suite_ptr suite = defs->add_suite(suite_name);
        suite->add_variable("ECF_HOME", home.string());
        suite->add_task("t");
        return defs;
    }

    /// @brief Waits for the job to start, and records its process identifier; returns false on time out.
    bool wait_for_start(std::chrono::seconds time_out) {
        const fs::path pid_file = home / "job.pid";
        const auto deadline     = std::chrono::steady_clock::now() + time_out;
        while (std::chrono::steady_clock::now() < deadline) {
            std::ifstream in(pid_file);
            if (in >> pid && pid > 0) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        return false;
    }

    /// @brief Describes the task and its job, to explain why the job did not start.
    std::string diagnose(const ClientInvoker& client) const {
        std::ostringstream out;
        if (client.getDefs() == 0 && client.defs()) {
            if (node_ptr task = client.defs()->findAbsNode("/" + suite_name + "/t")) {
                out << "task state " << NState::toString(task->state());
                if (auto submittable = task->isSubmittable(); submittable && !submittable->abortedReason().empty()) {
                    out << ", aborted: " << submittable->abortedReason();
                }
            }
        }
        else {
            out << "the definition could not be retrieved: " << client.errorMsg();
        }
        for (const auto& entry : fs::directory_iterator(home / suite_name)) {
            if (entry.path().extension() == ".1") {
                std::ifstream in(entry.path());
                out << "; job output: " << std::string(std::istreambuf_iterator<char>(in), {});
            }
        }
        return out.str();
    }

    /// @brief Tells whether the job is still running.
    bool alive() const { return pid > 0 && ::kill(pid, 0) == 0; }

    // In the working directory of the test, as for the other tests of the server, rather than in the temporary
    // directory of the system, which may be mounted without permission to execute (the job file is executed)
    fs::path home = fs::current_path() / ("ecflow_job_outlives_server_" + std::to_string(::getpid()));
    pid_t pid     = 0;
};

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_JobOutlivesServer)

///
/// A job inherits none of the descriptors of the server that spawns it, in particular not its listening socket:
/// once the server stops, a job still running does not keep the port, and a new server can bind it again.
///
BOOST_AUTO_TEST_CASE(test_server_restarts_on_its_port_while_a_job_outlives_it) {
    ECF_NAME_THIS_TEST();

    if (!ClientEnvironment::hostSpecified().empty()) {
        // The test starts and stops the server itself
        std::cout << "Ignoring test when ECF_HOST specified..." << std::endl;
        return;
    }

    OutlivingJob job;
    const std::string port = SCPort::next();

    {
        InvokeServer server("Client:: ...test_server_restarts_on_its_port_while_a_job_outlives_it", port);
        BOOST_REQUIRE_MESSAGE(server.server_started(), "Server failed to start on " << server.host() << ":" << port);

        ClientInvoker client(server.host(), port);
        client.set_throw_on_error(false);
        BOOST_REQUIRE_MESSAGE(client.restartServer() == 0, "restart failed: " << client.errorMsg());
        BOOST_REQUIRE_MESSAGE(client.load(job.defs()) == 0, "load failed: " << client.errorMsg());
        BOOST_REQUIRE_MESSAGE(client.begin(suite_name) == 0, "begin failed: " << client.errorMsg());

        BOOST_REQUIRE_MESSAGE(job.wait_for_start(std::chrono::seconds(60)),
                              "The job did not start: " << job.diagnose(client));
    } // the server is terminated here, while the job goes on

    BOOST_REQUIRE_MESSAGE(job.alive(), "The job ended with the server, so that it does not outlive it");

    auto server = std::make_unique<InvokeServer>(
        "Client:: ...test_server_restarts_on_its_port_while_a_job_outlives_it (again)", port);
    const bool restarted = server->server_started();
    const bool job_alive = job.alive();
    if (!restarted) {
        // The harness cannot stop a server that never started: its destructor would abort the test run. The object
        // is released without being destroyed, and the job, which may hold the port, and the lock of the port are
        // cleaned up here instead
        (void)server.release();
        job.stop();
        ecf::test::scaffold::EcfPortLock::remove(port);
    }
    BOOST_CHECK_MESSAGE(
        restarted, "A new server failed to start on port " << port << " while the job of the previous one still runs");
    BOOST_CHECK_MESSAGE(job_alive, "The job ended before the new server started");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
