/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ZombieUtil.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/Child.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/DurationTimer.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

///
/// \note This is used to INVOKE a SINGLE test.
/// Making it easier for Easier for debugging and development
///

// ****************************************************************
// In the test below we create two sets of process. One of these
// being a zombie. The test below create a list of user Zombies.
//   In the cases where these test fail
//   The dump of zombies shows that only One call was made.(i.e the initial creation of the Zombie)
//   and that the proces-id is not defined.
//   This implies that at test start, that not all process were started.
//   ??Note sure why?? This can be seen by enabling debug in TaskCmds which
//   shows that the Task INIT command was never received ???
// Updated these test to detect STALE *user* zombies & remove them
// i.e. where we have created a user Zombie but there is no associated process ??
// *******************************************************************

#define DO_TEST1 1
#define DO_TEST2 1
#define DO_TEST3 1
#define DO_TEST4 1
#define DO_TEST5 1
#define DO_TEST6 1
#define DO_TEST7 1
#define DO_TEST8 1
#define DO_TEST9 1
#define DO_TEST10 1
#define DO_TEST11 1
#define DO_TEST12 1

static bool ecf_debug_enabled = false; // allow environment(ECF_DEBUG_ZOMBIES) to enable debug

BOOST_AUTO_TEST_SUITE(S_Test)

BOOST_AUTO_TEST_SUITE(T_Zombies)

enum WaitType { SINGLE, ALL };
static int timeout      = 32;
static int NUM_OF_TASKS = 5;

static std::string dump_tasks(const vector<Task*>& tasks) {
    std::stringstream ss;
    ss << "     task status: no of tasks(" << tasks.size() << ")\n";
    for (const Task* task : tasks) {
        ss << "      " << task->absNodePath() << " " << NState::toString(task->state())
           << " passwd:" << task->jobsPassword() << " pid:" << task->process_or_remote_id()
           << " flag:" << task->get_flag().to_string();
        if (task->state() == NState::ABORTED)
            ss << " " << task->abortedReason();
        ss << "\n";
    }
    ss << "\n";
    return ss.str();
}

static std::string dump_task_status() {
    TestFixture::client().sync_local();
    defs_ptr defs = TestFixture::client().defs();
    vector<Task*> tasks;
    defs->getAllTasks(tasks);
    return dump_tasks(tasks);
}

static void dump_zombies() {
    TestFixture::client().zombieGet();
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
    std::cout << Zombie::pretty_print(zombies, 6);
    std::cout << dump_task_status();
}

static bool waitForTaskStates(WaitType num_of_tasks, NState::State state1, NState::State state2, int max_time_to_wait) {
    std::string wait_type_str = (num_of_tasks == SINGLE) ? "SINGLE" : "ALL";
    if (ecf_debug_enabled) {
        if (num_of_tasks == SINGLE) {
            if (state1 == state2)
                std::cout << "\n   Waiting for SINGLE task to reach state " << NState::toString(state1) << "\n";
            else
                std::cout << "\n   Waiting for SINGLE task to reach state " << NState::toString(state1) << " || "
                          << NState::toString(state2) << "\n";
        }
        else {
            if (state1 == state2)
                std::cout << "\n   Waiting for ALL tasks to reach state " << NState::toString(state1) << "\n";
            else
                std::cout << "\n   Waiting for ALL tasks to reach state " << NState::toString(state1) << " || "
                          << NState::toString(state2) << "\n";
        }
    }
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0,
                              "waitForTaskStates:*error* sync_local failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        defs_ptr defs = TestFixture::client().defs();
        vector<Task*> tasks;
        defs->getAllTasks(tasks);
        if (num_of_tasks == SINGLE) {
            for (Task* task : tasks) {
                if (task->state() == state1 || task->state() == state2) {
                    if (ecf_debug_enabled) {
                        if (task->state() == state1)
                            std::cout << "    Found at least one Task with state " << NState::toString(state1)
                                      << " returning\n";
                        if (state1 != state2 && task->state() == state2)
                            std::cout << "    Found at least one Task with state " << NState::toString(state2)
                                      << " returning\n";
                        dump_zombies();
                    }
                    return true;
                }
            }
        }
        else {
            size_t count = 0;
            for (Task* task : tasks) {
                if (task->state() == state1 || task->state() == state2)
                    count++;
            }
            if (count == tasks.size()) {
                if (ecf_debug_enabled) {
                    if (state2 == state1)
                        std::cout << "    All tasks(" << tasks.size() << ") have reached state "
                                  << NState::toString(state1) << " returning\n";
                    else
                        std::cout << "    All tasks(" << tasks.size() << ") have reached state "
                                  << NState::toString(state1) << " || " << NState::toString(state2) << " returning\n";
                    dump_tasks(tasks);
                    dump_zombies();
                }
                return true;
            }
        }

        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            if (ecf_debug_enabled) {
                dump_zombies();
                dump_tasks(tasks);
                std::cout << "waitForTaskState " << wait_type_str << " reach state " << NState::toString(state1)
                          << " || " << NState::toString(state2) << " Test taking longer than time constraint of "
                          << assertTimer.timeConstraint() << " returning false\n";
            }
            break;
        }
        sleep(1);
    }
    dump_zombies();
    return false;
}

static bool waitForTaskState(WaitType wait_type, NState::State state1, int max_time_to_wait) {
    return waitForTaskStates(wait_type, state1, state1, max_time_to_wait);
}

static bool waitForZombieCreation(size_t no_of_zombies, int max_time_to_wait) {
    if (ecf_debug_enabled)
        std::cout << "\n   Waiting for " << no_of_zombies
                  << " zombies to be created. max_time_to_wait:" << max_time_to_wait << "s\n";

    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0,
                              "zombieGet failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        if (zombies.size() >= no_of_zombies) {
            if (ecf_debug_enabled) {
                std::cout << Zombie::pretty_print(zombies, 3);
                std::cout << "     Found " << no_of_zombies << " zombies. returning after " << assertTimer.duration()
                          << "s\n";
            }
            return true;
        }
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            if (zombies.size() > 0) {
                if (ecf_debug_enabled) {
                    std::cout << "   *Timeout* out found only " << zombies.size() << " zombies."
                              << " Quit waiting.\n";
                    std::cout << Zombie::pretty_print(zombies, 3);
                }
                return true;
            }

            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < assertTimer.timeConstraint(),
                                  "\n*error* waitForZombieCreation expected "
                                      << no_of_zombies << " zombies but found "
                                      << TestFixture::client().server_reply().zombies().size()
                                      << " : Test taking longer than time constraint of "
                                      << assertTimer.timeConstraint() << "s aborting\n"
                                      << dump_task_status());
            break;
        }
        sleep(1);
    }
    return false;
}

static void remove_stale_zombies() {
    // Remove those zombies that have only *ZERO* calls hence PID is empty
    // There is no associated process/child command that has updated the zombie
    if (ecf_debug_enabled)
        cout << "\n   remove_stale_zombies\n";

    BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0,
                          "*error* zombieGet failed should return 0\n"
                              << TestFixture::client().errorMsg());
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
    for (const Zombie& z : zombies) {
        if (z.process_or_remote_id().empty()) {
            cout << "\n      **** removing_stale_zombies ***** " << z << "\n";
            cout << "        Typically happens when a submitted job, never creates a process ?\n";
            cout << "        Since the process never communicated back, the pid is empty.\n";
            TestFixture::client().zombieRemove(z);
        }
    }
}

static void wait_for_zombies_of_type(Child::ZombieType zt, int no_of_tasks, int max_time_to_wait) {
    if (ecf_debug_enabled)
        cout << "\n   wait_for_zombies_of_type: " << Child::to_string(zt) << " no_of_tasks:" << no_of_tasks
             << " max_time_to_wait:" << max_time_to_wait << "s\n";

    std::vector<Zombie> zombies;
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0,
                              "*error* zombieGet failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        zombies           = TestFixture::client().server_reply().zombies();
        int no_of_zombies = 0;
        for (const Zombie& z : zombies) {
            if (z.type() == zt)
                no_of_zombies++;
        }
        if (no_of_zombies >= no_of_tasks)
            break;

        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            // BOOST_WARN_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
            std::cout << "wait_for_zombies_of_type. Expected " << no_of_tasks << " " << Child::to_string(zt)
                      << " zombies, but found " << no_of_zombies << "\nTest taking longer than time constraint of "
                      << assertTimer.timeConstraint() << "\n"
                      << Zombie::pretty_print(zombies, 6) << "\n"
                      << dump_task_status() << "\n"
                      << "... breaking out\n";
            return;
        }
        sleep(1);
    }

    if (ecf_debug_enabled) {
        cout << Zombie::pretty_print(zombies, 6) << "\n";
        cout << dump_task_status();
    }
}

static void check_expected_no_of_zombies(size_t expected) {
    if (ecf_debug_enabled)
        cout << "   check_expected_no_of_zombies: " << expected << "\n";
    TestFixture::client().zombieGet();
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();

    BOOST_CHECK_MESSAGE(zombies.size() == expected,
                        "*error* Expected " << expected << " zombies but got " << zombies.size());
    if (zombies.size() != expected) {
        std::cout << Zombie::pretty_print(zombies, 6);
    }
}

static void check_expected_no_of_zombies_range(size_t min_expected, size_t max_expected) {
    if (ecf_debug_enabled)
        cout << "   check_expected_no_of_zombies_range: min:" << min_expected << " max:" << max_expected << "\n";
    TestFixture::client().zombieGet();
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();

    bool in_range = (zombies.size() >= min_expected && zombies.size() <= max_expected);
    BOOST_CHECK_MESSAGE(in_range,
                        "*error* Expected range(" << min_expected << "-" << max_expected << ") zombies but got "
                                                  << zombies.size());
    if (!in_range) {
        std::cout << Zombie::pretty_print(zombies, 6);
    }
}

static int check_at_least_one_zombie() {
    TestFixture::client().zombieGet();
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
    if (ecf_debug_enabled) {
        std::cout << "\n   check_at_least_one_zombie: actual " << zombies.size() << "\n";
        std::cout << Zombie::pretty_print(zombies, 6);
    }
    BOOST_CHECK_MESSAGE(zombies.size() >= 1, "*error* Expected at least one zombies but got nothing\n");
    return zombies.size();
}

static bool wait_for_zombie_termination(int max_time_to_wait) {
    if (ecf_debug_enabled)
        std::cout << "\n   wait_for_zombie_termination: max_time_to_wait:" << max_time_to_wait << "\n";

    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        TestFixture::client().zombieGet();
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        if (zombies.empty()) {
            if (ecf_debug_enabled)
                cout << "      zombies empty. Waited for " << assertTimer.duration() << "s returning\n";
            break;
        }

        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            if (ecf_debug_enabled) {
                std::cout << "      wait_for_zombie_termination: taking longer than time constraint of "
                          << assertTimer.timeConstraint() << " returning\n";
                std::cout << Zombie::pretty_print(zombies, 6);
            }
            return false;
        }
        sleep(1);
    }
    return true;
}

static void wait_for_zombies_child_cmd(WaitType wait_type,
                                       ecf::Child::CmdType child_cmd,
                                       int max_time_to_wait,
                                       bool do_delete = false) {
    if (ecf_debug_enabled) {
        std::cout << "\n   Waiting for ";
        if (wait_type == SINGLE)
            std::cout << "SINGLE";
        else
            std::cout << "ALL";
        std::cout << "  child (" << Child::to_string(child_cmd) << ") cmd; ";
        if (do_delete)
            std::cout << " then DELETE zombies ";
        std::cout << "=============================================================================================\n";
    }

    bool child_type_found = false;
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        TestFixture::client().zombieGet();
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        if (ecf_debug_enabled)
            std::cout << "    Get zombies returned " << zombies.size() << "\n";

        if (zombies.empty() && child_type_found && do_delete) {
            if (ecf_debug_enabled)
                std::cout << "   No zombies. (do_delete) was set returning\n";
            return;
        }

        size_t completed_zombies = 0;
        for (const Zombie& z : zombies) {
            if (ecf_debug_enabled)
                std::cout << "   " << z << "\n";
            if (z.last_child_cmd() == child_cmd) {
                child_type_found = true;
                completed_zombies++;
                if (wait_type == SINGLE) {
                    if (ecf_debug_enabled) {
                        std::cout << "   " << z << "\n";
                        std::cout << "   Found SINGLE zombie of correct child type returning\n";
                    }
                    return;
                }
            }
        }

        if (completed_zombies == zombies.size() && child_type_found) {
            if (ecf_debug_enabled) {
                std::cout << "   Found ALL(" << completed_zombies << ") zombies of child type "
                          << Child::to_string(child_cmd) << "\n";
                std::cout << Zombie::pretty_print(zombies, 6);
            }
            if (do_delete) {
                for (const Zombie& z : zombies) {
                    if (ecf_debug_enabled)
                        std::cout << "   deleteing " << z << "\n";
                    TestFixture::client().zombieRemove(z);
                }
            }
            return;
        }

        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            std::cout << Zombie::pretty_print(zombies, 3);
            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < assertTimer.timeConstraint(),
                                  "*error* wait_for_zombies_child_cmd Test wait "
                                      << assertTimer.duration() << " taking longer than time constraint of "
                                      << assertTimer.timeConstraint() << " aborting\n");
        }
        sleep(1);
    }
}

static void wait_for_no_zombies(int max_time_to_wait) {
    if (ecf_debug_enabled)
        std::cout << "\n   wait_for_no_zombies, for " << max_time_to_wait << "s\n";

    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        TestFixture::client().zombieGet();
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        if (ecf_debug_enabled) {
            std::cout << "      Get zombies returned " << zombies.size() << "\n";
            if (!zombies.empty())
                std::cout << Zombie::pretty_print(zombies, 6) << "\n";
        }
        if (zombies.empty()) {
            if (ecf_debug_enabled)
                std::cout << "      returning after " << assertTimer.duration() << "s\n";
            return;
        }

        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            std::cout << "\n" << Zombie::pretty_print(zombies, 6);
            BOOST_REQUIRE_MESSAGE(assertTimer.duration() < assertTimer.timeConstraint(),
                                  "*error* wait_for_no_zombies Test wait "
                                      << assertTimer.duration() << " taking longer than time constraint of "
                                      << assertTimer.timeConstraint() << " aborting\n");
        }
        sleep(2);
    }
}

static void populate_defs(Defs& theDefs, const std::string& suite_name) {
    suite_ptr suite = theDefs.add_suite(suite_name);
    suite->addVariable(Variable("SLEEPTIME", "5")); // sleep for longer than normal to allow for creation of zombies
    family_ptr family = suite->add_family("f");
    for (int i = 0; i < NUM_OF_TASKS; i++) {
        family->add_task("t" + ecf::convert_to<std::string>(i));
    }
    suite->add_variable("CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL", "_any_");
}

static void
create_and_start_test(Defs& theDefs, const std::string& suite_name, const std::string& create_zombies_with) {
    if (ecf_debug_enabled) {
        std::cout << "   create_and_start_test  " << suite_name << "  using  " << create_zombies_with << "\n";
    }

    /// Avoid side effects from previous test, by removing all zombies
    TestFixture::client().zombieGet();
    if (!TestFixture::client().server_reply().zombies().empty()) {
        (void)ZombieUtil::do_zombie_user_action(
            User::REMOVE, TestFixture::client().server_reply().zombies().size(), timeout);
    }

    if (ecf_debug_enabled)
        std::cout << "   creating server test harness\n";

    // The test harness will create corresponding directory structure & default ecf file
    ServerTestHarness serverTestHarness;
    serverTestHarness.run(theDefs,
                          ServerTestHarness::testDataDefsLocation(suite_name + ".def"),
                          1 /*timeout*/,
                          false /* don't wait for test to finish */);

    // Wait for a single task to reach state submitted or active, before creating zombies
    waitForTaskStates(SINGLE, NState::SUBMITTED, NState::ACTIVE, timeout);

    // ******************************************************************************
    // IMPORTANT: If Job generation takes to long i.e >= next poll then it will TIMEOUT.
    //            Hence use the number of submitted/active tasks, as the expected number of zombies.
    //            This must be done before creating zombies by changing state.
    // *******************************************************************************
    size_t number_submitted_or_active = 0;
    TestFixture::client().sync_local();
    defs_ptr defs = TestFixture::client().defs();
    vector<Task*> tasks;
    defs->getAllTasks(tasks);
    for (const auto& task : tasks) {
        if (task->state() == NState::SUBMITTED || task->state() == NState::ACTIVE)
            number_submitted_or_active++;
    }
    if (ecf_debug_enabled)
        std::cout << "   found " << number_submitted_or_active << " in submitted or active state\n";

    if (create_zombies_with == "delete") {
        if (ecf_debug_enabled)
            std::cout << "   create USER zombies by deleting all the nodes in the server\n";
        TestFixture::client().delete_all(true /* force */);
    }
    else if (create_zombies_with == "begin") {
        /// Begin with force, Should create user zombies. WILL only catch those task that are submitted
        /// It may well be that job submission, may only get a subset, others, which come for submission *later*
        /// on will be ECF zombies.
        /// Note: If we wait for SUMBITTED, then we get the case, where we have two task jobs
        ///       with same password, BUT different process id ??????
        ///       i.e The begin has regenerate the job file, so we get job started twice.
        ///       This should be trapped by the server, as Task should be active
        if (ecf_debug_enabled) {
            std::cout
                << "   Calling begin_all_suites, now have 2 sets of jobs, The first/original set are now zombies\n";
            std::cout << "   - Second set of jobs can fail on creation with (Text File busy),\n";
            std::cout
                << "     since server is creating a new job file, whilst zombie process in running the same job file\n";
            std::cout << "   - The first set can fail, because process not created, but server and has created 2nd job "
                         "file(same path),\n";
            std::cout << "     but not made it executable yet. Hence expect (exited with status 126) i.e exe' found "
                         "but not executable\n";
        }
        TestFixture::client().set_throw_on_error(false);
        if (TestFixture::client().begin_all_suites(true /* force */) == 1) {
            std::cout << "   Begin raised exception: because " << TestFixture::client().errorMsg() << " ***\n";
        }
        TestFixture::client().set_throw_on_error(true);
    }
    else if (create_zombies_with == "queued") {
        if (ecf_debug_enabled) {
            std::cout << "   Creating zombies by setting all tasks to queued\n";
            std::cout
                << "   This should increment the try number, avoiding (Text File busy) and (exited with status 126)\n";
            std::cout
                << "   Zombies(first set) will have try_no:1 and the queued tasks(second set) will have try number2\n";
        }
        std::vector<Task*> tasks;
        theDefs.getAllTasks(tasks);
        std::vector<std::string> paths;
        for (auto task : tasks)
            paths.emplace_back(task->absNodePath());

        TestFixture::client().set_throw_on_error(false);
        if (TestFixture::client().force(paths, "queued") == 1) {
            std::cout << "   force queued raised exception: because " << TestFixture::client().errorMsg() << " ***\n";
        }
        TestFixture::client().set_throw_on_error(true);
    }
    else if (create_zombies_with == "complete") {

        if (ecf_debug_enabled)
            std::cout << "   create USER zombies by calling complete\n";
        std::string path = "/" + suite_name;
        TestFixture::client().force(path, "complete", true /*recursive*/);
    }
    else if (create_zombies_with == "aborted") {

        if (ecf_debug_enabled)
            std::cout << "   create USER zombies by calling abort\n";
        std::string path = "/" + suite_name;
        TestFixture::client().force(path, "aborted", true /*recursive*/);
    }
    else {
        BOOST_REQUIRE_MESSAGE(false, "*error* Create zombies via " << create_zombies_with << " unrecognised");
    }

    /// When jobs try to communicate with server via child commands they will block the Child commands
    if (waitForZombieCreation(number_submitted_or_active, timeout)) {

        /// Check we have zombies and they are of type USER
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        BOOST_CHECK_MESSAGE(zombies.size() > 0, "*error* NO zombies created");
        for (const Zombie& z : zombies) {
            if (create_zombies_with == "begin") {
                BOOST_CHECK_MESSAGE(z.type() == Child::USER,
                                    "*error* Creating zombies via begin, Expected 'user' zombie type but got: " << z);
            }
            else if (create_zombies_with == "queued") {
                BOOST_CHECK_MESSAGE(z.type() == Child::USER,
                                    "*error* Creating zombies via queued, Expected 'user' zombie type but got: " << z);
            }
            else if (create_zombies_with == "delete") {
                BOOST_CHECK_MESSAGE(
                    z.type() == Child::USER || z.type() == Child::PATH,
                    "*error* Creating zombies via delete, Expected 'user | path' zombie type but got: " << z);
            }
            else if (create_zombies_with == "complete") {
                BOOST_CHECK_MESSAGE(z.type() == Child::USER, "*error* Expected 'user' zombie type but got: " << z);
            }
            else if (create_zombies_with == "aborted") {
                BOOST_CHECK_MESSAGE(z.type() == Child::USER, "*error* Expected 'user' zombie type but got: " << z);
            }
            else {
                BOOST_CHECK_MESSAGE(z.type() == Child::ECF, "*error* Expected 'ecf' zombie type but got: " << z);
            }
        }
    }
    if (ecf_debug_enabled) {
        cout << dump_task_status();
    }
}

static void create_and_start_test(const std::string& suite_name,
                                  const std::string& create_zombies_with,
                                  bool add_delay_before_init = false) {

    if (ecf_debug_enabled) {
        TestFixture::client().get_log_path();
        std::cout << "\n   Creating defs, log path " << TestFixture::client().get_string() << "\n";
    }
    Defs theDefs;
    populate_defs(theDefs, suite_name);
    if (add_delay_before_init) {
        suite_ptr suite = theDefs.findSuite(suite_name);
        suite->add_variable("ADD_DELAY_BEFORE_INIT", "sleep 1"); // add delay before --init
        if (ecf_debug_enabled)
            std::cout << "   adding a delay of 1 second before --init is called\n";
    }
    create_and_start_test(theDefs, suite_name, create_zombies_with);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(enable_debug_for_ECF_TRY_NO_Greater_than_one) {
    BOOST_CHECK_MESSAGE(true, "dummy test");

    if (ecf::environment::has("ECF_DEBUG_ZOMBIES")) {
        ecf_debug_enabled = true;
        ECF_NAME_THIS_TEST();
    }
}

#ifdef DO_TEST1
BOOST_AUTO_TEST_CASE(test_path_zombie_creation) {
    ECF_NAME_THIS_TEST();

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    if (ecf_debug_enabled)
        cout << "\n";
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    // User zombies will be converted to path zombies by the server
    create_and_start_test("test_path_zombie_creation", "delete"); // create zombie via delete

    // The job generation can timeout, hence we may not have submitted the full number of tasks & hence expected zombies
    // can vary
    check_expected_no_of_zombies_range(1, NUM_OF_TASKS);

    // The server will automatically change existing zombie to be of type PATH
    // when no task nodes exists
    // *Note* in test environment the client invoker will try connecting to the server
    // ****** for 5 seconds, after that an error is returned. This will cause the
    // ****** job to abort.
    wait_for_zombies_of_type(Child::PATH, NUM_OF_TASKS, timeout);

    // Fob all the zombies. This will UNBLOCK the child commands allowing them to complete
    // Fobing does *NOT* alter node tree state, however COMPLETE should auto delete the zombie
    // Hence after this command, the number of fobed zombies may *NOT* be the same
    // as the number of tasks. Since the fobed zombies are auto deleted when a complete
    // child command is received.
    int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
    BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0, "*error* Expected some fobed zombies but found none ?");

    // Wait for zombies to be deleted in the server
    if (!wait_for_zombie_termination(timeout)) {
        remove_stale_zombies();
    }

    // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST2
BOOST_AUTO_TEST_CASE(test_user_zombies_for_delete_fob) {
    ECF_NAME_THIS_TEST();

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    if (ecf_debug_enabled)
        cout << "\n";
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    // User zombies will be converted to path zombies by the server
    create_and_start_test(
        "test_user_zombies_for_delete_fob", "delete", true /* add a delay before init */); // create zombie via delete

    // expect at least one zombie
    (void)check_at_least_one_zombie();

    // Fob all the zombies. This will UNBLOCK the child commands allowing them to finish
    // Fobing does *NOT* alter node tree state, however COMPLETE should auto delete the zombie
    // Hence after this command, the number of fobed zombies may *NOT* be the same
    // as the number of tasks. Since the fobed zombies are auto deleted when a complete
    // child command is recieved.
    int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
    BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0, "*error* Expected some fobed zombies but found none ?");

    // Wait for zombies to be deleted in the server
    if (!wait_for_zombie_termination(timeout)) {
        remove_stale_zombies(); // see notes above
    }

    // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST3
BOOST_AUTO_TEST_CASE(test_user_zombies_for_delete_fail) {
    ECF_NAME_THIS_TEST();

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    if (ecf_debug_enabled)
        cout << "\n";
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    // User zombies will be converted to path zombies by the server
    create_and_start_test("test_user_zombies_for_delete_fail", "delete");

    check_at_least_one_zombie();

    // Fail all the zombies. This will UNBLOCK and terminate the child commands allowing them to finish
    int no_of_failed_zombies = ZombieUtil::do_zombie_user_action(User::FAIL, NUM_OF_TASKS, timeout);
    BOOST_CHECK_MESSAGE(no_of_failed_zombies > 0, "*error* Expected > 0 Failed zombies but found none");

    check_at_least_one_zombie();

    // Wait for zombies to abort, then remove all the zombies
    wait_for_zombies_child_cmd(ALL, ecf::Child::ABORT, timeout, true /* delete */);

    check_expected_no_of_zombies(0);
    cout << timer.duration() << "s\n";
}
#endif

// test_zombies_attr_for_begin/f/t1 user 26 ZzD/ycIW <pid> 1 calls(1) BLOCK INIT

#ifdef DO_TEST4
BOOST_AUTO_TEST_CASE(test_user_zombies_for_begin) {
    ECF_NAME_THIS_TEST();

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    if (ecf_debug_enabled)
        cout << "\n";
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    // Note: if when we start to create zombies and some tasks are still queued *NOT* all will be zombies.
    create_and_start_test("test_user_zombies_for_begin", "begin");

    /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete or abort
    /// The second set can still abort, if the first set are busy with job file. look for '(Text file busy)'
    /// Previously we had tried again, but fix for ECFLOW-1216, means we now don't try again,hence allow abort
    BOOST_REQUIRE_MESSAGE(waitForTaskStates(ALL, NState::COMPLETE, NState::ABORTED, timeout),
                          "*error* Expected non-zombie tasks to complete or abort");

    // Note: if when we started to create zombies and some tasks are still queued *NOT* all will be zombies.
    int no_of_zombies = check_at_least_one_zombie();

    // Fob all the zombies. This will UNBLOCK the child commands allowing them to finish
    // Hence after this command, the number of fobed zombies may *NOT* be the same
    // as the number of tasks. Since the fobed zombies are auto deleted when a complete
    // child command is received.
    //
    /// When we have two sets of completes, we just fob, automatically. See TaskCmd::authenticate
    int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, no_of_zombies, timeout);
    BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0, "*error* Expected some fobed zombies but found none ?");

    // Fobing does *NOT* alter node tree state, however child COMPLETE should auto delete the zombie
    if (!wait_for_zombie_termination(timeout)) {
        remove_stale_zombies(); // see notes above
    }

    // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST5
BOOST_AUTO_TEST_CASE(test_zombies_attr) {
    ECF_NAME_THIS_TEST();

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    std::string suite_name = "test_zombies_attr";
    if (ecf_debug_enabled)
        cout << "\n";
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    create_and_start_test(
        suite_name, "queued", true /* add a delay before init */); // create zombies re-queuing submitted/active tasks

    /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete or abort
    /// Creating zombies by queuing submitted/active tasks, increments try number of the second set.
    /// This avoid text file busy,(i.e by using begin/requeue creating a job file, whilst its already running)
    BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL, NState::COMPLETE, timeout),
                          "*error* Expected non-zombie tasks to complete");

    check_at_least_one_zombie();

    /// *** Fobbing will not change state of the node tree ****
    if (ecf_debug_enabled)
        std::cout << "   Add a zombie attribute 'user:fob::' to the suite, which fobs all child commands\n";
    TestFixture::client().alter("/" + suite_name, "add", "zombie", "user:fob::");

    // Fobbing causes auto deletion of zombies, when the Child complete is reached
    if (!wait_for_zombie_termination(timeout)) {
        remove_stale_zombies(); // see notes above
    }

    // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST6
BOOST_AUTO_TEST_CASE(test_user_zombies_for_adopt) {
    ECF_NAME_THIS_TEST();

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    std::string suite_name = "test_user_zombies_for_adopt";
    if (ecf_debug_enabled)
        cout << "\n";
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    // Using "queued" to create zombies will mean we update try_no, zombies will not share output.
    // This avoid'(Text file busy)'
    create_and_start_test(suite_name, "queued", true /* add delay before init */);

    /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
    BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL, NState::COMPLETE, timeout),
                          "*error* Expected non-zombie tasks to complete");

    check_at_least_one_zombie();

    /// Adopt all the zombies. This will UNBLOCK the child commands allowing them to finish
    /// This test below fail on AIX, its too fast , task's may already be adopted and hence don't fail
    int no_of_adopted_zombied = ZombieUtil::do_zombie_user_action(User::ADOPT, NUM_OF_TASKS, timeout);
    if (ecf_debug_enabled)
        cout << "   found " << no_of_adopted_zombied << " zombies for adoption\n";

    /// The blocked zombies are free, start with blocked init command
    (void)waitForTaskState(SINGLE, NState::ACTIVE, timeout);

    /// Now wait for all tasks to complete
    BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL, NState::COMPLETE, timeout),
                          "*error* Expected zombie tasks to complete");

    remove_stale_zombies(); // see notes above

    // After adoption the zombies should be removed
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST7
BOOST_AUTO_TEST_CASE(test_zombies_attr_for_adopt) {
    std::string suite_name = "test_zombies_attr_for_adopt";
    ECF_NAME_THIS_TEST(<< ", using suite: " << suite_name);

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    create_and_start_test(suite_name, "queued", true /* add a delay before init */);

    /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
    BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL, NState::COMPLETE, timeout),
                          "*error* Expected non-zombie tasks to complete");

    // expected 5 zombies, ie because we have NUM_OF_TASKS tasks. These should all be blocking
    check_at_least_one_zombie();

    if (ecf_debug_enabled)
        std::cout << "   Add a zombie attribute 'user:adopt::' to the suite, which *ADOPTS* all zombies allowing them "
                     "to complete\n";
    TestFixture::client().alter("/" + suite_name, "add", "zombie", "user:adopt::");

    if (ecf_debug_enabled)
        dump_zombies();

    /// The blocked zombies are free, start with blocked init command
    /// This may fail on AIX, its too fast , task's may already be complete, dont fail
    (void)waitForTaskState(SINGLE, NState::ACTIVE, timeout);

    /// Now wait for all tasks to complete. ** They may be complete from last process set **
    BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL, NState::COMPLETE, timeout),
                          "*error* Expected all zombie task to complete after adopt");

    remove_stale_zombies(); // see notes above

    // After adoption the zombies should be removed
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST8
BOOST_AUTO_TEST_CASE(test_user_zombie_creation_via_complete) {
    std::string suite_name = "test_zombies_attr_for_adopt";
    ECF_NAME_THIS_TEST(<< ", using suite: " << suite_name);

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    create_and_start_test(suite_name, "complete");

    /// Since we have set tasks to complete, we should only have *ONE* set of zombies
    check_at_least_one_zombie();

    // Fob all the zombies child commands allowing them to finish
    (void)ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);

    // Wait for zombies to complete, they should get removed automatically
    wait_for_no_zombies(timeout);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST9
BOOST_AUTO_TEST_CASE(test_user_zombie_creation_via_abort) {
    std::string suite_name = "test_user_zombie_creation_via_abort";
    ECF_NAME_THIS_TEST(<< ", using suite: " << suite_name);

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    create_and_start_test(suite_name, "aborted");

    /// Since we have set tasks to complete, we should only have *ONE* set of zombies
    check_at_least_one_zombie();

    // Fob all the zombies child commands allowing them to finish
    (void)ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);

    // Wait for zombies to complete, they should get removed automatically
    wait_for_no_zombies(timeout);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST10
BOOST_AUTO_TEST_CASE(test_zombie_inheritance) {
    std::string suite_name = "test_zombie_inheritance";
    ECF_NAME_THIS_TEST(<< ", using suite: " << suite_name);

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // Add zombie attribute, make sure it inherited
    Defs theDefs;
    populate_defs(theDefs, suite_name);
    suite_ptr suite = theDefs.findSuite(suite_name);
    suite->addZombie(ZombieAttr(ecf::Child::USER, std::vector<ecf::Child::CmdType>(), ecf::User::FOB, -1));
    suite->addZombie(ZombieAttr(ecf::Child::ECF, std::vector<ecf::Child::CmdType>(), ecf::User::FOB, -1));
    suite->addZombie(ZombieAttr(ecf::Child::ECF_PID, std::vector<ecf::Child::CmdType>(), ecf::User::FOB, -1));
    suite->addZombie(ZombieAttr(ecf::Child::ECF_PID_PASSWD, std::vector<ecf::Child::CmdType>(), ecf::User::FOB, -1));
    suite->addZombie(ZombieAttr(ecf::Child::ECF_PASSWD, std::vector<ecf::Child::CmdType>(), ecf::User::FOB, -1));
    suite->addZombie(ZombieAttr(ecf::Child::PATH, std::vector<ecf::Child::CmdType>(), ecf::User::FOB, -1));

    create_and_start_test(theDefs, suite_name, "complete");

    /// Since we have set tasks to complete, we should only have *ONE* set of zombies
    // expect NUM_OF_TASKS zombies, ie because we have NUM_OF_TASKS tasks
    TestFixture::client().set_throw_on_error(true);
    TestFixture::client().zombieGet();
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
    BOOST_CHECK_MESSAGE(!zombies.empty(), "No zombies found");
    for (const Zombie& z : zombies) {
        BOOST_CHECK_MESSAGE(z.user_action() == ecf::User::FOB,
                            "*error* Expected zombies with user action of type FOB but found "
                                << User::to_string(z.user_action()));
        break;
    }

    // Wait for zombies to complete, they should get removed automatically
    wait_for_no_zombies(timeout);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST11
static int wait_for_killed_zombies(int no_of_tasks, int max_time_to_wait) {
    if (ecf_debug_enabled)
        std::cout << "\n   wait_for_killed_zombies\n";
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (1) {
        int killed = 0;
        TestFixture::client().zombieGet();
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        for (const Zombie& z : zombies) {
            if (z.kill())
                killed++;
        }
        if (ecf_debug_enabled)
            std::cout << "   found " << killed << " killed zombies\n";

        if (killed == no_of_tasks)
            return killed;

        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            cout << "   wait_for_killed_zombies Test wait " << assertTimer.duration()
                 << " taking longer than time constraint of " << assertTimer.timeConstraint() << " breaking out\n"
                 << Zombie::pretty_print(zombies, 6) << "\n";
            return killed;
        }
        sleep(2);
    }
    return 0;
}

BOOST_AUTO_TEST_CASE(test_zombie_kill) {
    std::string suite_name = "test_zombie_kill";
    ECF_NAME_THIS_TEST(<< ", using suite: " << suite_name);

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // This command creates user zombies up front, these may not have a pid, if task in submitted state
    create_and_start_test(suite_name, "complete");

    check_at_least_one_zombie();

    // kill all the zombies, i.e kill -15 on the script
    // This will be trapped by the signal and hence will call abort
    (void)ZombieUtil::do_zombie_user_action(User::KILL, NUM_OF_TASKS, timeout);

    // wait for kill zombies. This should eventually lead to process terminating
    int killed = wait_for_killed_zombies(NUM_OF_TASKS, timeout);
    BOOST_CHECK_MESSAGE(killed > 0, "*error* Expected " << NUM_OF_TASKS << " killed ");

    {
        // wait for process to be killed: killing is a separate process, we could well
        // have got to the complete, before the process is killed.
        // Once the complete is fobed it terminate the process.
        AssertTimer assertTimer(timeout, false); // Bomb out after n seconds, fall back if test fail
        while (1) {
            int completed = 0;
            int aborted   = 0;
            TestFixture::client().zombieGet();
            std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
            for (const Zombie& z : zombies) {
                if (z.last_child_cmd() == ecf::Child::ABORT)
                    aborted++;
                if (z.last_child_cmd() == ecf::Child::COMPLETE)
                    completed++;
            }
            if (aborted + completed == NUM_OF_TASKS)
                break;
            if (assertTimer.duration() >= assertTimer.timeConstraint()) {
                cout << "   wait for for abort:  found " << aborted + completed
                     << " aborted & completed tasks. Test wait " << assertTimer.duration()
                     << " taking longer than time constraint of " << assertTimer.timeConstraint() << " breaking out\n"
                     << Zombie::pretty_print(zombies, 6) << "\n";
                break;
            }
            sleep(1);
        }
    }
    {
        bool task_became_blocked = false;
        // wait for process to be die
        AssertTimer assertTimer(timeout, false); // Bomb out after n seconds, fall back if test fail
        while (1) {
            TestFixture::client().zombieGet();
            std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
            for (const Zombie& z : zombies) {
                if (z.block()) { // something went wrong, fob so don't leave process hanging
                    TestFixture::client().zombieFob(z);
                    task_became_blocked = true;
                    cout << "Zombies blocking ?? " << z << "\n";
                }
            }
            if (assertTimer.duration() >= assertTimer.timeConstraint()) {
                if (task_became_blocked) {
                    cout << "   Task became blocked, fobing" << assertTimer.duration()
                         << " taking longer than time constraint of " << assertTimer.timeConstraint()
                         << " breaking out\n"
                         << Zombie::pretty_print(zombies, 6) << "\n";
                }
                break;
            }
            sleep(1);
        }
    }

    // remove the killed zombies
    (void)ZombieUtil::do_zombie_user_action(User::REMOVE, NUM_OF_TASKS, timeout, false);

    wait_for_no_zombies(timeout);

    cout << timer.duration() << "s\n";
}
#endif

#ifdef DO_TEST12

static void remove_all_user_zombies() {
    if (ecf_debug_enabled) {
        cout << "\n   remove_all_user_zombies\n";
        dump_zombies();
    }

    int removed_count = 0;
    AssertTimer assertTimer(timeout, false); // Bomb out after n seconds, fall back if test fail
    while (removed_count < NUM_OF_TASKS) {
        TestFixture::client().zombieGet();
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        for (const Zombie& z : zombies) {
            if (z.type() == ecf::Child::USER) {
                TestFixture::client().zombieRemove(z); // This should be immediate, and is not remembered
                removed_count++;
            }
        }
        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {
            cout << "      removed " << removed_count
                 << " user zombies. Quit waiting, to remove all zombies of type USER\n";
            cout << Zombie::pretty_print(zombies, 6);
            return;
        }
        sleep(1);
    }
    if (ecf_debug_enabled)
        dump_zombies();
}
BOOST_AUTO_TEST_CASE(test_ecf_zombie_type_creation) {
    std::string suite_name = "test_ecf_zombie_type_creation";
    ECF_NAME_THIS_TEST(<< ", using suite: " << suite_name);

    if (ecf_debug_enabled)
        std::cout << "\n\n=============================================================================\n";
    DurationTimer timer;
    TestClean clean_at_start_and_end;

    // some systems are really quick, where tasks are already active, when we make them zombies, then the next
    // client command in the zombie is: --complete. This distorts the test. Since if the real task is complete and
    // we get a zombie with --complete, the server lets it through and the zombie process terminates.
    // To get round this we will introduce a delay, before we get --init
    create_and_start_test(suite_name, "queued", true /* add a delay before init */);

    /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
    BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL, NState::COMPLETE, timeout),
                          "*error* Expected non-zombie tasks to complete");

    // wait and remove all *USER* zombies.
    remove_all_user_zombies();

    // wait of at least *ONE* zombie of type *ECF*
    wait_for_zombies_of_type(Child::ECF, NUM_OF_TASKS, timeout);

    int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
    BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0, "*error* Expected some fobed zombies but found none ?");

    // Fobing does *NOT* alter node tree state, however child COMPLETE should auto delete the zombie
    if (!wait_for_zombie_termination(timeout)) {
        remove_stale_zombies(); // see notes above
    }

    // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was received
    check_expected_no_of_zombies(0);

    cout << timer.duration() << "s\n";
}
#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
