// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iostream>
#include <pwd.h>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Signal.hpp"
#include "ecflow/node/SpawnIdentity.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_System)

BOOST_AUTO_TEST_CASE(test_system) {
    ECF_NAME_THIS_TEST();

    std::string file = "test_system.log";
    std::string cmd  = "echo sat siri akal dunyia > " + file;

    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(System::instance()->spawn(System::ECF_STATUS_CMD, cmd, "", errorMsg),
                          "System::instance()->spawn() failed: " << errorMsg);
    while (System::instance()->process() != 0) {
        // Capture child process termination. Child sends SIGNAl SIGCHLD, caught by parent
        Signal unblock_on_desctruction_then_reblock;
        // sleep(1); // Need to wait for child termination()
        System::instance()->processTerminatedChildren();
    }

    BOOST_CHECK_MESSAGE(fs::exists(file), "Expected cmd(" << cmd << ") to produce a file " << file);

    fs::remove(file); // Remove the file. Comment out for debugging
}

namespace {

/// Waits for every spawned process to terminate, as the server does between requests
void wait_for_children() {
    while (System::instance()->process() != 0) {
        Signal unblock_on_desctruction_then_reblock;
        System::instance()->processTerminatedChildren();
    }
}

/// Restores the default of the singleton at the end of a test
struct WithSpawnAsOwner
{
    explicit WithSpawnAsOwner(bool enabled) { System::instance()->set_spawn_as_owner(enabled); }
    ~WithSpawnAsOwner() { System::instance()->set_spawn_as_owner(false); }
};

std::string first_line_of(const std::string& path) {
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    return line;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_spawn_as_owner_refuses_what_cannot_be_resolved) {
    ECF_NAME_THIS_TEST();
    WithSpawnAsOwner enabled(true);

    struct Refusal
    {
        std::string user;
        std::string expected;
    };
    for (const Refusal& refusal :
         {Refusal{"", "no owner"}, Refusal{"no_such_user_ecflow_test", "unknown"}, Refusal{"root", "root"}}) {
        std::string file = "test_spawn_as_owner_refused.log";
        std::string cmd  = "echo should not run > " + file;
        std::string errorMsg;
        BOOST_CHECK_MESSAGE(!System::instance()->spawn(System::ECF_JOB_CMD, cmd, "/s/t", refusal.user, errorMsg),
                            "Expected the spawn for '" << refusal.user << "' to be refused");
        BOOST_CHECK_MESSAGE(errorMsg.find("Refused to spawn") != std::string::npos &&
                                errorMsg.find("/s/t") != std::string::npos &&
                                errorMsg.find(refusal.expected) != std::string::npos,
                            "Expected the reason to name the task and '" << refusal.expected << "', got: " << errorMsg);
        BOOST_CHECK_MESSAGE(System::instance()->process() == 0, "Expected nothing to be spawned");
        BOOST_CHECK_MESSAGE(!fs::exists(file), "Expected the command not to run");
        fs::remove(file);
    }
}

BOOST_AUTO_TEST_CASE(test_spawn_as_owner_off_ignores_the_user) {
    ECF_NAME_THIS_TEST();
    WithSpawnAsOwner disabled(false);

    // Without the switch the user is not even looked at: an unknown one runs the command as the server
    std::string file = "test_spawn_as_owner_off.log";
    std::string cmd  = "echo ran > " + file;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(
        System::instance()->spawn(System::ECF_STATUS_CMD, cmd, "", "no_such_user_ecflow_test", errorMsg),
        "spawn failed: " << errorMsg);
    wait_for_children();
    BOOST_CHECK_MESSAGE(first_line_of(file) == "ran", "Expected the command to run as the server");
    fs::remove(file);
}

BOOST_AUTO_TEST_CASE(test_spawn_as_owner_runs_as_the_current_user_when_it_is_the_owner) {
    ECF_NAME_THIS_TEST();

    // Switching to the account the server already runs as needs no privilege, hence runs everywhere
    if (geteuid() == 0) {
        std::cout << "  Skipped: running as root, whose identity is refused by design\n";
        return;
    }
    WithSpawnAsOwner enabled(true);
    struct passwd* me = getpwuid(geteuid());
    BOOST_REQUIRE(me && me->pw_name);

    std::string file = "test_spawn_as_owner_self.log";
    std::string cmd  = "echo $(id -u) $HOME $USER $LOGNAME > " + file;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(System::instance()->spawn(System::ECF_STATUS_CMD, cmd, "", me->pw_name, errorMsg),
                          "spawn failed: " << errorMsg);
    wait_for_children();
    std::string expected = std::to_string(geteuid()) + " " + me->pw_dir + " " + me->pw_name + " " + me->pw_name;
    BOOST_CHECK_MESSAGE(first_line_of(file) == expected,
                        "Expected '" << expected << "', got '" << first_line_of(file) << "'");
    fs::remove(file);
}

BOOST_AUTO_TEST_CASE(test_spawn_as_owner_switches_to_another_user_as_root) {
    ECF_NAME_THIS_TEST();

    // Only root can switch to another account: this case is exercised where the tests run as root
    if (geteuid() != 0) {
        std::cout << "  Skipped: not running as root, another account cannot be switched to\n";
        return;
    }
    WithSpawnAsOwner enabled(true);
    std::string error;
    auto nobody = resolve_spawn_identity("nobody", error);
    BOOST_REQUIRE_MESSAGE(nobody, "Expected the account 'nobody': " << error);

    std::string file = "/tmp/test_spawn_as_owner_nobody.log";
    std::string cmd  = "echo $(id -u) $HOME > " + file;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(System::instance()->spawn(System::ECF_STATUS_CMD, cmd, "", "nobody", errorMsg),
                          "spawn failed: " << errorMsg);
    wait_for_children();
    std::string expected = std::to_string(nobody->uid) + " " + nobody->home;
    BOOST_CHECK_MESSAGE(first_line_of(file) == expected,
                        "Expected '" << expected << "', got '" << first_line_of(file) << "'");
    fs::remove(file);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
