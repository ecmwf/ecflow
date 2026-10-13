// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <boost/test/unit_test.hpp>

#include "TestHelper.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/base/cts/user/EditScriptCmd.hpp"
#include "ecflow/base/cts/user/ForceCmd.hpp"
#include "ecflow/base/cts/user/LoadDefsCmd.hpp"
#include "ecflow/base/cts/user/ReplaceNodeCmd.hpp"
#include "ecflow/base/cts/user/RequeueNodeCmd.hpp"
#include "ecflow/base/cts/user/RunNodeCmd.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::TestLog;

///
/// The user whose command puts a task in a state from which a job is spawned becomes the owner of
/// that task, exposed as the generated variable ECF_OWNER.
///

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_OwnerCmd)

namespace {

template <typename Cmd, typename... Args>
Cmd_ptr as_user(const std::string& user, Args&&... args) {
    Cmd_ptr cmd = std::make_shared<Cmd>(std::forward<Args>(args)...);
    cmd->setup_user_authentification(user, "");
    return cmd;
}

void check_owner(const Submittable* task, const std::string& expected) {
    BOOST_REQUIRE(task);
    BOOST_CHECK_MESSAGE(task->owner() == expected,
                        task->absNodePath() << ": expected owner '" << expected << "', got '" << task->owner() << "'");
    std::string value;
    BOOST_CHECK_MESSAGE(task->findParentVariableValue("ECF_OWNER", value) && value == expected,
                        task->absNodePath() << ": expected ECF_OWNER '" << expected << "', got '" << value << "'");
}

void check_owner(const task_ptr& task, const std::string& expected) {
    check_owner(task.get(), expected);
}

const Submittable* task_at(const defs_ptr& defs, const std::string& path) {
    node_ptr node = defs->findAbsNode(path);
    return node ? node->isSubmittable() : nullptr;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_load_and_replace_record_the_owner) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_load_and_replace_record_the_owner.log");

    defs_ptr server_defs = Defs::create();

    // The user loading a suite owns every task of it
    defs_ptr loaded = Defs::create();
    loaded->add_suite("s1")->add_family("f1")->add_task("t1");
    TestHelper::invokeRequest(server_defs.get(), as_user<LoadDefsCmd>("xyza", loaded, false));
    check_owner(task_at(server_defs, "/s1/f1/t1"), "xyza");

    // The user replacing a node owns every task below it, the rest is untouched
    defs_ptr replacement = Defs::create();
    family_ptr rf        = replacement->add_suite("s1")->add_family("f2");
    rf->add_task("t2");
    TestHelper::invokeRequest(server_defs.get(), as_user<ReplaceNodeCmd>("xyzb", "/s1/f2", true, replacement, false));
    check_owner(task_at(server_defs, "/s1/f2/t2"), "xyzb");
    check_owner(task_at(server_defs, "/s1/f1/t1"), "xyza");
}

BOOST_AUTO_TEST_CASE(test_requeue_records_the_owner) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_requeue_records_the_owner.log");

    defs_ptr defs = Defs::create();
    family_ptr f1 = defs->add_suite("s1")->add_family("f1");
    task_ptr t1   = f1->add_task("t1");
    task_ptr t2   = f1->add_task("t2");
    defs->beginAll();
    BOOST_CHECK_MESSAGE(t1->owner().empty(), "Expected no owner before any user command");

    // A requeue of a family sets the owner of every task below it
    TestHelper::invokeRequest(defs.get(), as_user<RequeueNodeCmd>("xyza", f1->absNodePath()));
    check_owner(t1, "xyza");
    check_owner(t2, "xyza");

    // A later requeue of one task by another user replaces the owner of that task only.
    // The tasks were submitted by the job generation that followed the first requeue, hence force.
    TestHelper::invokeRequest(defs.get(), as_user<RequeueNodeCmd>("xyzb", t2->absNodePath(), RequeueNodeCmd::FORCE));
    check_owner(t1, "xyza");
    check_owner(t2, "xyzb");
}

BOOST_AUTO_TEST_CASE(test_requeue_abort_records_the_owner_of_the_aborted_tasks_only) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_requeue_abort_records_the_owner_of_the_aborted_tasks_only.log");

    defs_ptr defs = Defs::create();
    family_ptr f1 = defs->add_suite("s1")->add_family("f1");
    task_ptr t1   = f1->add_task("t1");
    task_ptr t2   = f1->add_task("t2");
    t1->addDefStatus(DState::SUSPENDED);
    t2->addDefStatus(DState::SUSPENDED);
    defs->beginAll();
    TestHelper::invokeRequest(defs.get(), as_user<RequeueNodeCmd>("xyza", f1->absNodePath()));
    check_owner(t1, "xyza");
    check_owner(t2, "xyza");

    // Only the aborted task is requeued, and only it changes owner
    TestHelper::invokeRequest(defs.get(), as_user<ForceCmd>("xyza", t2->absNodePath(), "aborted", false, false));
    TestHelper::invokeRequest(defs.get(), as_user<RequeueNodeCmd>("xyzb", f1->absNodePath(), RequeueNodeCmd::ABORT));
    check_owner(t1, "xyza");
    check_owner(t2, "xyzb");
}

BOOST_AUTO_TEST_CASE(test_force_queued_recursive_records_the_owner_of_every_task) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_force_queued_recursive_records_the_owner_of_every_task.log");

    defs_ptr defs = Defs::create();
    family_ptr f1 = defs->add_suite("s1")->add_family("f1");
    task_ptr t1   = f1->add_task("t1");
    task_ptr t2   = f1->add_family("f2")->add_task("t2");
    t1->addDefStatus(DState::SUSPENDED);
    t2->addDefStatus(DState::SUSPENDED);
    defs->beginAll();

    // The container forwards the owner to every task below it
    TestHelper::invokeRequest(defs.get(),
                              as_user<ForceCmd>("xyza", f1->absNodePath(), "queued", true /*recursive*/, false));
    check_owner(t1, "xyza");
    check_owner(t2, "xyza");
}

BOOST_AUTO_TEST_CASE(test_run_and_force_queued_record_the_owner) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_run_and_force_queued_record_the_owner.log");

    defs_ptr defs = Defs::create();
    family_ptr f1 = defs->add_suite("s1")->add_family("f1");
    task_ptr t1   = f1->add_task("t1");
    task_ptr t2   = f1->add_task("t2");
    t1->addDefStatus(DState::SUSPENDED);
    t2->addDefStatus(DState::SUSPENDED);
    defs->beginAll();

    // --run (in test mode, no job is spawned) sets the owner
    TestHelper::invokeRequest(defs.get(), as_user<RunNodeCmd>("xyza", t1->absNodePath(), false, true /*test*/));
    check_owner(t1, "xyza");

    // --force queued sets the owner; a force to another state leaves it
    TestHelper::invokeRequest(defs.get(), as_user<ForceCmd>("xyzb", t2->absNodePath(), "complete", false, false));
    BOOST_CHECK_MESSAGE(t2->owner().empty(), "A force to complete must not set the owner, got " << t2->owner());
    TestHelper::invokeRequest(defs.get(), as_user<ForceCmd>("xyzb", t2->absNodePath(), "queued", false, false));
    check_owner(t2, "xyzb");
    TestHelper::invokeRequest(defs.get(), as_user<ForceCmd>("xyzc", t2->absNodePath(), "aborted", false, false));
    check_owner(t2, "xyzb");
}

BOOST_AUTO_TEST_CASE(test_automatic_requeue_keeps_the_owner) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_automatic_requeue_keeps_the_owner.log");

    defs_ptr defs = Defs::create();
    family_ptr f1 = defs->add_suite("s1")->add_family("f1");
    f1->addRepeat(RepeatInteger("R", 1, 3));
    task_ptr t1 = f1->add_task("t1");
    t1->addDefStatus(DState::SUSPENDED);
    defs->beginAll();

    TestHelper::invokeRequest(defs.get(), as_user<ForceCmd>("xyza", t1->absNodePath(), "queued", false, false));
    check_owner(t1, "xyza");

    // Completing the task increments the repeat and requeues the family: no user acts, the owner stays
    TestHelper::invokeRequest(defs.get(), as_user<ForceCmd>("xyzb", t1->absNodePath(), "complete", false, false));
    BOOST_CHECK_MESSAGE(f1->repeat().value() == 2, "Expected the repeat to advance, value " << f1->repeat().value());
    BOOST_CHECK_MESSAGE(t1->state() == NState::QUEUED,
                        "Expected the task requeued, state " << NState::toString(t1->state()));
    check_owner(t1, "xyza");
}

BOOST_AUTO_TEST_CASE(test_edit_script_alias_records_the_owner) {
    ECF_NAME_THIS_TEST();
    TestLog test_log("test_edit_script_alias_records_the_owner.log");

    defs_ptr defs = Defs::create();
    task_ptr t1   = defs->add_suite("s1")->add_family("f1")->add_task("t1");
    t1->addDefStatus(DState::SUSPENDED);
    defs->beginAll();

    // An alias created from a task, without running it, belongs to the user creating it
    std::vector<std::string> script{"echo alias"};
    NameValueVec variables;
    TestHelper::invokeRequest(
        defs.get(),
        as_user<EditScriptCmd>("xyzb", t1->absNodePath(), variables, script, true /*alias*/, false /*run*/));
    BOOST_REQUIRE_MESSAGE(t1->aliases().size() == 1, "Expected one alias, got " << t1->aliases().size());
    alias_ptr alias = t1->aliases().front();
    BOOST_CHECK_MESSAGE(alias->owner() == "xyzb", "Expected owner xyzb on the alias, got " << alias->owner());
    std::string value;
    BOOST_CHECK_MESSAGE(alias->findParentVariableValue("ECF_OWNER", value) && value == "xyzb",
                        "Expected ECF_OWNER xyzb on the alias, got " << value);
    BOOST_CHECK_MESSAGE(t1->owner().empty(), "The task itself keeps its owner, got " << t1->owner());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
