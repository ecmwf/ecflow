// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Pid.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Signal.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

///
/// With the switch enabled, a task whose job cannot be spawned as its owner is aborted with the reason,
/// and a kill or status command is spawned as the user that requested it, or as the owner otherwise.
///

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_SpawnAsOwner)

namespace {

/// Restores the default of the singleton at the end of a test
struct WithSpawnAsOwner
{
    explicit WithSpawnAsOwner(bool enabled) { System::instance()->set_spawn_as_owner(enabled); }
    ~WithSpawnAsOwner() { System::instance()->set_spawn_as_owner(false); }
};

/// A definition with one task, its script written under the test data directory
struct WithScriptedTask
{
    WithScriptedTask() {
        suite = Suite::create(Pid::unique_name("test_spawn_as_owner"));
        task  = suite->add_task("t1");
        defs.addSuite(suite);
        ecf_home = File::test_data("libs/node/test/data", "libs/node");
        defs.server_state().add_or_update_user_variables(ecf::environment::ECF_HOME, ecf_home);
        defs.server_state().add_or_update_user_variables(ecf::environment::ECF_JOB_CMD, "%ECF_JOB%");
        defs.server_state().add_or_update_user_variables(ecf::environment::ECF_KILL_CMD, "kill -15 %ECF_RID%");
        defs.beginAll();
        script_location = ecf_home + task->absNodePath() + File::ECF_EXTN();
        File::createMissingDirectories(script_location);
        std::string error;
        File::create(script_location, "#!/bin/sh\necho spawn as owner\n", error);
    }
    ~WithScriptedTask() { fs::remove_all(ecf_home + suite->absNodePath()); }

    Defs defs;
    suite_ptr suite;
    task_ptr task;
    std::string ecf_home;
    std::string script_location;
};

} // namespace

BOOST_AUTO_TEST_CASE(test_a_task_whose_owner_cannot_be_resolved_is_aborted_with_the_reason) {
    ECF_NAME_THIS_TEST();
    WithSpawnAsOwner enabled(true);
    WithScriptedTask scripted;

    scripted.task->set_owner("no_such_user_ecflow_test");
    JobsParam jobsParam(60, true /* create jobs */, true /* spawn them */);
    BOOST_CHECK_MESSAGE(!scripted.task->submitJob(jobsParam), "Expected the submission to be refused");
    BOOST_CHECK_MESSAGE(scripted.task->state() == NState::ABORTED,
                        "Expected the task aborted, state " << NState::toString(scripted.task->state()));
    const std::string& reason = scripted.task->abortedReason();
    BOOST_CHECK_MESSAGE(reason.find("Refused to spawn") != std::string::npos &&
                            reason.find("no_such_user_ecflow_test") != std::string::npos,
                        "Expected the reason to name the refusal and the user, got: " << reason);
    BOOST_CHECK_MESSAGE(System::instance()->process() == 0, "Expected nothing to be spawned");
}

BOOST_AUTO_TEST_CASE(test_a_task_without_an_owner_is_aborted_with_the_reason) {
    ECF_NAME_THIS_TEST();
    WithSpawnAsOwner enabled(true);
    WithScriptedTask scripted;

    JobsParam jobsParam(60, true /* create jobs */, true /* spawn them */);
    BOOST_CHECK_MESSAGE(!scripted.task->submitJob(jobsParam), "Expected the submission to be refused");
    BOOST_CHECK_MESSAGE(scripted.task->state() == NState::ABORTED, "Expected the task aborted");
    BOOST_CHECK_MESSAGE(scripted.task->abortedReason().find("no owner") != std::string::npos,
                        "Expected the reason to say no owner is recorded, got: " << scripted.task->abortedReason());
}

BOOST_AUTO_TEST_CASE(test_kill_is_spawned_as_the_requester_or_as_the_owner) {
    ECF_NAME_THIS_TEST();
    WithSpawnAsOwner enabled(true);
    WithScriptedTask scripted;
    scripted.task->set_owner("owner_ecflow_test");

    // The requester is used when given: the refusal names the requester
    try {
        scripted.task->kill("12345", "requester_ecflow_test");
        BOOST_CHECK_MESSAGE(false, "Expected the kill to be refused");
    }
    catch (const std::runtime_error& e) {
        std::string what = e.what();
        BOOST_CHECK_MESSAGE(what.find("ECF_KILL_CMD") != std::string::npos &&
                                what.find("requester_ecflow_test") != std::string::npos,
                            "Expected the refusal to name the requester, got: " << what);
    }
    BOOST_CHECK(scripted.task->get_flag().is_set(ecf::Flag::KILLCMD_FAILED));

    // Without a requester (an automatic kill), the owner is used
    try {
        scripted.task->kill("12345", "");
        BOOST_CHECK_MESSAGE(false, "Expected the kill to be refused");
    }
    catch (const std::runtime_error& e) {
        std::string what = e.what();
        BOOST_CHECK_MESSAGE(what.find("owner_ecflow_test") != std::string::npos,
                            "Expected the refusal to name the owner, got: " << what);
    }
}

BOOST_AUTO_TEST_CASE(test_nothing_is_refused_when_the_switch_is_off) {
    ECF_NAME_THIS_TEST();
    WithSpawnAsOwner disabled(false);
    WithScriptedTask scripted;

    // The job is spawned as the server, whatever the owner; wait for the child to finish
    scripted.task->set_owner("no_such_user_ecflow_test");
    JobsParam jobsParam(60, true /* create jobs */, true /* spawn them */);
    BOOST_CHECK_MESSAGE(scripted.task->submitJob(jobsParam),
                        "Expected the submission to succeed: " << jobsParam.getErrorMsg());
    BOOST_CHECK_MESSAGE(scripted.task->state() == NState::SUBMITTED, "Expected the task submitted");
    while (System::instance()->process() != 0) {
        Signal unblock_on_desctruction_then_reblock;
        System::instance()->processTerminatedChildren();
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
