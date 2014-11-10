#define BOOST_TEST_MODULE TEST_ZOMBIES
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #58 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This is used to INVOKE a SINGLE test.
//               Making it easier for Easier for debugging and development
//============================================================================
#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "TestFixture.hpp"
#include "ServerTestHarness.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "PrintStyle.hpp"
#include "AssertTimer.hpp"
#include "Child.hpp"
#include "ZombieUtil.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

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
// i.e. where we have created a user Zombie but there is associated process ??
// *******************************************************************

//#define DEBUG_ZOMBIE 1
#define DO_TEST1 1
#define DO_TEST2 1
#define DO_TEST3 1
#define DO_TEST4 1
#define DO_TEST5 1
#define DO_TEST6 1
#define DO_TEST7 1
#define DO_TEST8 1
#define DO_TEST9 1
//#define DO_TEST10 1 Need to make reliable
#define DO_TEST11 1

static bool ecf_debug_enabled = false; // allow environment to enable debug


BOOST_GLOBAL_FIXTURE( TestFixture );

BOOST_AUTO_TEST_SUITE( TestSuite  )

enum WaitType { SINGLE, ALL };
static int timeout = 30;
static int NUM_OF_TASKS = 5;

static void dump_zombies()
{
   TestFixture::client().zombieGet();
   std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
   std::cout <<  Zombie::pretty_print( zombies , 3);
}

static void dump_tasks(const vector<Task*>& tasks) {
   BOOST_FOREACH(const Task* task, tasks) {
      std::cout << "    " << task->absNodePath() << " " << NState::toString(task->state()) << " " << task->jobsPassword() << " " << task->process_or_remote_id() << "\n";
   }
   std::cout << "\n";
}

static bool waitForTaskStates(WaitType num_of_tasks,NState::State state1,NState::State state2, int max_time_to_wait)
{
   std::string wait_type_str = (num_of_tasks == SINGLE ) ? "SINGLE" : "ALL";
   if (ecf_debug_enabled) {
      if ( num_of_tasks == SINGLE) {
         if (state1 == state2) std::cout << "\n   Waiting for SINGLE task to reach state " << NState::toString(state1) << "\n";
         else                  std::cout << "\n   Waiting for SINGLE task to reach state " << NState::toString(state1) << " || " << NState::toString(state2)  << "\n";
      }
      else {
         if (state1 == state2)  std::cout << "\n   Waiting for ALL tasks to reach state " << NState::toString(state1) << "\n";
         else                   std::cout << "\n   Waiting for ALL tasks to reach state " << NState::toString(state1) << " || " << NState::toString(state2)  << "\n";
      }
   }
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(TestFixture::client().sync_local() == 0, "waitForTaskStates: sync_local failed should return 0\n" << TestFixture::client().errorMsg());
      defs_ptr defs = TestFixture::client().defs();
      vector<Task*> tasks; defs->getAllTasks(tasks);
      if (num_of_tasks == SINGLE) {
         BOOST_FOREACH(Task* task, tasks) {
            if (task->state() == state1 || task->state() == state2 ) {
               if (ecf_debug_enabled) {
                  if (task->state() == state1) std::cout << "    Found at least one Task with state " << NState::toString(state1) << " returning\n";
                  if (state1 != state2 && task->state() == state2) std::cout << "    Found at least one Task with state " << NState::toString(state2) << " returning\n";
                  dump_zombies();
               }
               return true;
            }
         }
      }
      else {
         size_t count = 0;
         BOOST_FOREACH(Task* task, tasks) { if (task->state() == state1 || task->state() == state2)  count++; }
         if (count == tasks.size()) {
            if (ecf_debug_enabled) {
               if (state2 == state1) std::cout << "    All tasks(" << tasks.size() << ") have reached state " << NState::toString(state1) << " returning\n";
               else                  std::cout << "    All tasks(" << tasks.size() << ") have reached state " << NState::toString(state1) << " || " <<  NState::toString(state2) <<  " returning\n";
               dump_tasks(tasks);
            }
            return true;
         }
      }

      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         if (ecf_debug_enabled) {
            dump_zombies();
            dump_tasks(tasks);
            std::cout << "waitForTaskState " << wait_type_str << " reach state " << NState::toString(state1) << " || " << NState::toString(state2)
                  << " Test taking longer than time constraint of " << assertTimer.timeConstraint() <<  " returning false\n";
         }
         break;
      }
      sleep(1);
   }
   return false;
}

static bool waitForTaskState(WaitType wait_type,NState::State state1, int max_time_to_wait)
{
   return waitForTaskStates(wait_type, state1, state1, max_time_to_wait);
}

static bool waitForZombieCreation(size_t no_of_zombies, int max_time_to_wait)
{
   if (ecf_debug_enabled) std::cout << "\n   Waiting for " << no_of_zombies << " zombies to be created\n";

   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0, "zombieGet failed should return 0\n" << TestFixture::client().errorMsg());
      std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
      if (zombies.size() == no_of_zombies) {
         if (ecf_debug_enabled) {
            std::cout <<  Zombie::pretty_print( zombies , 3);
            std::cout << "    Found " << no_of_zombies << " zombies. returning.\n";
         }
         return true;
      }
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {

         if (zombies.size() > 0) {
            if (ecf_debug_enabled) {
               std::cout << "   Timeout out found only " << zombies.size()  << " zombies." << "\n";
               std::cout <<  Zombie::pretty_print( zombies , 3);
               std::cout << "    Found " << no_of_zombies << " zombies. returning.\n";
            }
            return true;
         }

         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "waitForZombieCreation expected " << no_of_zombies
                  << " zombies but found " << TestFixture::client().server_reply().zombies().size()
                  << " : Test taking longer than time constraint of "
                  << assertTimer.timeConstraint() << " aborting\n"
                  << Zombie::pretty_print( zombies , 3));
         break;
      }
      sleep(1);
   }
   return false;
}

static void remove_stale_zombies()
{
   // Remove those zombies that have only *ONE* call
   // There is no associated process/child command that has updated the zombie
   if (ecf_debug_enabled) cout << "\n   remove_stale_zombies \n";

   BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0, "zombieGet failed should return 0\n" << TestFixture::client().errorMsg());
   std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
   BOOST_FOREACH(const Zombie& z, zombies) {
      if (z.calls() == 1) {
         cout << "\n      **** removing_stale_zombies ***** " << z << "\n";
         cout << "        Typically happens when a submitted job, never creates a process ?\n";
         cout << "        Since the process never communicated back, the pid is empty.\n";
         TestFixture::client().zombieRemove(z);
      }
   }
}

static void wait_for_path_zombies(int no_of_tasks, int max_time_to_wait)
{
   if (ecf_debug_enabled) cout << "\n   wait_for_path_zombies\n";

   int no_of_path_zombies = 0;
   std::vector<Zombie> zombies;
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0, "zombieGet failed should return 0\n" << TestFixture::client().errorMsg());
      zombies = TestFixture::client().server_reply().zombies();
      BOOST_FOREACH(const Zombie& z, zombies) {
         if (z.type() == Child::PATH ) {
            no_of_path_zombies++;
         }
      }
      if (no_of_path_zombies >= no_of_tasks) break;
      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         // BOOST_WARN_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
         std::cout << "remove_user_zombies_and_wait_for_path_zombies. Expected "
                            << no_of_tasks << " PATH zombies, but found " << no_of_path_zombies
                            << "\nTest taking longer than time constraint of " << assertTimer.timeConstraint()
                            << "\n"
                            << Zombie::pretty_print( zombies , 6) << "\n... breaking out\n";
         break;
      }
      sleep(1);
   }

   if (ecf_debug_enabled) cout << Zombie::pretty_print( zombies , 6) << "\n";
}


static void check_expected_no_of_zombies(size_t expected)
{
   TestFixture::client().zombieGet();
   std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();

   BOOST_CHECK_MESSAGE(zombies.size() == expected ,"Expected " << expected << " zombies but got " << zombies.size());
   if ( zombies.size() != expected) {
      std::cout <<  Zombie::pretty_print( zombies , 3);
      return;
   }
}

static void check_at_least_one_zombie()
{
   TestFixture::client().zombieGet();
   std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
   BOOST_CHECK_MESSAGE(zombies.size() >= 1 ,"Expected at least one zombies but got nothing\n");
}

static bool wait_for_zombie_termination(int max_time_to_wait)
{
   if (ecf_debug_enabled) std::cout << "\n   wait_for_zombie_termination\n";

   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      TestFixture::client().zombieGet();
      std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
      if (zombies.empty() ) break;

      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         if (ecf_debug_enabled)  {
            std::cout << "wait_for_zombie_termination: taking longer than time constraint of " << assertTimer.timeConstraint() << " returning\n";
            std::cout <<  Zombie::pretty_print( zombies , 3);
         }
         return false;
      }
      sleep(1);
   }
   return true;
}

static void wait_for_zombies_child_cmd(WaitType wait_type,ecf::Child::CmdType child_cmd,int max_time_to_wait, bool do_delete = false)
{
   if (ecf_debug_enabled) {
      std::cout << "\n   Waiting for ";
      if (wait_type == SINGLE) std::cout << "SINGLE";
      else                     std::cout << "ALL";
      std::cout << "  child (" << Child::to_string(child_cmd) << ") cmd; ";
      if (do_delete) std::cout << " then DELETE zombies ";
      std::cout << "=============================================================================================\n";
   }

   bool child_type_found = false;
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      TestFixture::client().zombieGet();
      std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
      if (ecf_debug_enabled) std::cout << "    Get zombies returned " << zombies.size() << "\n";

      if (zombies.empty() && child_type_found && do_delete) {
         if (ecf_debug_enabled) std::cout << "   No zombies. (do_delete) was set returning\n";
         return;
      }

      size_t completed_zombies = 0;
      BOOST_FOREACH(const Zombie& z, zombies) {
         if (ecf_debug_enabled) std::cout << "   " << z << "\n";
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

      if ( completed_zombies == zombies.size() && child_type_found) {
         if (ecf_debug_enabled) {
            std::cout << "   Found ALL(" << completed_zombies << ") zombies of child type " << Child::to_string(child_cmd) << "\n";
            std::cout << Zombie::pretty_print( zombies , 3);
         }
         if (do_delete) {
            BOOST_FOREACH(const Zombie& z, zombies) {
               if (ecf_debug_enabled) std::cout << "   deleteing " << z << "\n";
               TestFixture::client().zombieRemove(z);
            }
         }
         return;
      }

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         std::cout << Zombie::pretty_print( zombies , 3);
         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "wait_for_zombies_child_cmd Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " aborting\n");
      }
      sleep(1);
   }
}

static void wait_for_no_zombies(int max_time_to_wait)
{
   if (ecf_debug_enabled) std::cout << "\n   wait_for_no_zombies\n";

   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      TestFixture::client().zombieGet();
      std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
      if (ecf_debug_enabled) {
         std::cout << "      Get zombies returned " << zombies.size() << "\n";
         std::cout << Zombie::pretty_print( zombies , 6) << "\n";
      }
      if (zombies.empty()) return;

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         std::cout << "\n" << Zombie::pretty_print( zombies , 3);
         BOOST_REQUIRE_MESSAGE(assertTimer.duration() <  assertTimer.timeConstraint(),
                  "wait_for_no_zombies Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " aborting\n");
      }
      sleep(2);
   }
}

static void populate_defs(Defs& theDefs,const std::string& suite_name) {
   suite_ptr suite =  theDefs.add_suite(suite_name);
   suite->addVariable(Variable("SLEEPTIME","5")); // sleep for longer than normal to allow for creation of zombies
   family_ptr family = suite->add_family("f");
   for (int i = 0; i < NUM_OF_TASKS; i++) {
      family->add_task( "t" + boost::lexical_cast<std::string>(i) );
   }
   suite->add_variable("CHECK_TASK_DURATION_LESS_THAN_SERVER_POLL","_any_");
}


static void create_and_start_test(Defs& theDefs, const std::string& suite_name, const std::string& create_zombies_with) {
   if (ecf_debug_enabled) {
      std::cout << "\n\n=============================================================================\n";
      std::cout << "create_and_start_test  " << suite_name << "  using  " << create_zombies_with << "\n";
   }

   /// Avoid side effects from previous test, by removing all zombies
   TestFixture::client().zombieGet();
   if (!TestFixture::client().server_reply().zombies().empty()) {
      (void)ZombieUtil::do_zombie_user_action(User::REMOVE,TestFixture::client().server_reply().zombies().size(), timeout);
   }

   if (ecf_debug_enabled) std::cout << "   creating server test harness\n";

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation(suite_name + ".def"),
                         1 /*timeout*/,
                         false/* don't wait for test to finish */);

   // Wait for a single task to reach state submitted or active, before creating zombies
   waitForTaskStates(SINGLE,NState::SUBMITTED,NState::ACTIVE, timeout);

   // ******************************************************************************
   // IMPORTANT: Since 4.0.5, if Job generation takes to long i.e >= next poll
   //            then it will TIMEOUT. Hence we may no get the number of zombies
   //            that we expected
   // *******************************************************************************

   if (create_zombies_with == "delete") {
      if (ecf_debug_enabled) std::cout << "   create USER zombies by deleting all the nodes in the server\n";
      TestFixture::client().delete_all( true /* force */);
   }
   else if (create_zombies_with == "begin") {
      /// Begin with force, Should create user zombies. WILL only catch those task that are submitted
      /// It may well be that job submission, may only get a subset, others, which come for submission *later*
      /// on will be ECF zombies.
      /// Note: If we wait for SUMBITTED, then we get the case, where we have two task jobs
      ///       with same password, BUT different process id ??????
      ///       i.e The begin has regenerate the job file, so we get job started twice.
      ///       This should be trapped by the server, as Task should be active
      if (ecf_debug_enabled) std::cout << "   Calling begin_all_suites, now have 2 sets of jobs, The first/original set are now zombies\n";
      TestFixture::client().set_throw_on_error(false);
      if (TestFixture::client().begin_all_suites( true /* force */) == 1) {
         std::cout << "   Begin raised exception: because " << TestFixture::client().errorMsg() << " ***\n";
      }
      TestFixture::client().set_throw_on_error(true);
   }
   else if (create_zombies_with == "complete") {

      if (ecf_debug_enabled) std::cout << "   create USER zombies by calling complete\n";
      std::string path = "/" + suite_name;
      TestFixture::client().force(path,"complete",true/*recursive*/);
   }
   else if (create_zombies_with == "aborted") {

      if (ecf_debug_enabled) std::cout << "   create USER zombies by calling abort\n";
      std::string path = "/" + suite_name;
      TestFixture::client().force(path,"aborted",true/*recursive*/);
   }
   else {
      BOOST_REQUIRE_MESSAGE(false,"Create zombies via " << create_zombies_with << " unrecognised");
   }

   /// Wait for all task zombies
   vector<Task*> tasks;
   theDefs.getAllTasks(tasks);

   /// When jobs try to communicate with server via child commands they will block the Child commands
   if (waitForZombieCreation(tasks.size(),timeout)) {

      /// Check we have zombies and they are of type USER
      std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
      BOOST_CHECK_MESSAGE(zombies.size() > 0," NO zombies created");
      BOOST_FOREACH(const Zombie& z, zombies) {
         if (create_zombies_with == "begin") {
            BOOST_CHECK_MESSAGE(z.type() == Child::USER,"Creating zombies via begin, Expected 'user' zombie type but got: " << z);
         }
         else if (create_zombies_with == "delete") {
            BOOST_CHECK_MESSAGE(z.type() == Child::USER || z.type() == Child::PATH,"Creating zombies via delete, Expected 'user | path' zombie type but got: " << z);
         }
         else {
            BOOST_CHECK_MESSAGE(z.type() == Child::ECF,"Expected 'ecf' zombie type but got: " << z);
         }
      }
   }
}

static void create_and_start_test(const std::string& suite_name, const std::string& create_zombies_with) {

   if (ecf_debug_enabled) std::cout << "   Creating defs\n";
   Defs theDefs;
   populate_defs(theDefs,suite_name);
   create_and_start_test(theDefs,suite_name,create_zombies_with );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE( enable_debug_for_ECF_TRY_NO_Greater_than_one)
{
   BOOST_CHECK_MESSAGE(!ecf_debug_enabled ,"dummy test");

   if (getenv("ECF_DEBUG_ZOMBIES")) {
      ecf_debug_enabled = true;
      cout << "Test:: ... debug_enabled" << endl;
   }
}

#ifdef DO_TEST1
BOOST_AUTO_TEST_CASE(test_path_zombie_creation)
{
   DurationTimer timer;
   cout << "Test:: ...test_path_zombie_creation " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   // User zombies will be converted to path zombies by the server
   create_and_start_test("test_path_zombie_creation","delete"); // create zombie via delete

   // expect NUM_OF_TASKS zombies, ie because we have NUM_OF_TASKS tasks
   check_expected_no_of_zombies(NUM_OF_TASKS);

   // The server will automatically change existing zombie to be of type PATH
   // when no task nodes exists
   // *Note* in test environment the client invoker will try connecting to the server
   // ****** for 5 seconds, after that an error is returned. This will cause the
   // ****** job to abort.
   wait_for_path_zombies(NUM_OF_TASKS,timeout);

   // Fob all the zombies. This will UNBLOCK the child commands allowing them to complete
   // Fobing does *NOT* alter node tree state, however COMPLETE should auto delete the zombie
   // Hence after this command, the number of fobed zombies may *NOT* be the same
   // as the number of tasks. Since the fobed zombies are auto deleted when a complete
   // child command is recieved.
   int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0,"Expected some fobed zombies but found none ?");

   // Wait for zombies to be deleted in the server
   if (!wait_for_zombie_termination(timeout)) {
      remove_stale_zombies();
   }

   // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
   check_expected_no_of_zombies(0);

   cout << timer.duration() << "\n";
}
#endif

#ifdef DO_TEST2
BOOST_AUTO_TEST_CASE( test_user_zombies_for_delete_fob )
{
   DurationTimer timer;
   cout << "Test:: ...test_user_zombies_for_delete_fob " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   // User zombies will be converted to path zombies by the server
   create_and_start_test("test_user_zombies_for_delete_fob","delete"); // create zombie via delete

   // expect 5 zombies, ie because we have NUM_OF_TASKS tasks
   check_expected_no_of_zombies(NUM_OF_TASKS);

   // Fob all the zombies. This will UNBLOCK the child commands allowing them to finish
   // Fobing does *NOT* alter node tree state, however COMPLETE should auto delete the zombie
   // Hence after this command, the number of fobed zombies may *NOT* be the same
   // as the number of tasks. Since the fobed zombies are auto deleted when a complete
   // child command is recieved.
   int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0,"Expected some fobed zombies but found none ?");

   // Wait for zombies to be deleted in the server
   if (!wait_for_zombie_termination(timeout)) {
      remove_stale_zombies();  // see notes above
   }

   // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
   check_expected_no_of_zombies(0);

   cout << timer.duration() << "\n";
}
#endif


#ifdef DO_TEST3
BOOST_AUTO_TEST_CASE( test_user_zombies_for_delete_fail )
{
   DurationTimer timer;
   cout << "Test:: ...test_user_zombies_for_delete_fail " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   // User zombies will be converted to path zombies by the server
   create_and_start_test("test_user_zombies_for_delete_fail","delete");

   check_at_least_one_zombie();

   // Fail all the zombies. This will UNBLOCK and terminate the child commands allowing them to finish
   int no_of_failed_zombies = ZombieUtil::do_zombie_user_action(User::FAIL, NUM_OF_TASKS, timeout);
   BOOST_CHECK_MESSAGE(no_of_failed_zombies >0,"Expected > 0 Failed zombies but found none");

   check_at_least_one_zombie();

   // Wait for zombies to abort, then remove all the zombies
   wait_for_zombies_child_cmd(ALL,ecf::Child::ABORT,timeout, true /* delete */);

   check_expected_no_of_zombies(0);
   cout << timer.duration() << "\n";
}
#endif

//test_zombies_attr_for_begin/f/t1 user 26 ZzD/ycIW <pid> 1 calls(1) BLOCK INIT

#ifdef DO_TEST4
BOOST_AUTO_TEST_CASE( test_user_zombies_for_begin )
{
   DurationTimer timer;
   cout << "Test:: ...test_user_zombies_for_begin " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test("test_user_zombies_for_begin","begin");

   /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
   BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL,NState::COMPLETE,timeout),"Expected non-zombie tasks to complete");

   check_at_least_one_zombie();

   // Fob all the zombies. This will UNBLOCK the child commands allowing them to finish
   // Hence after this command, the number of fobed zombies may *NOT* be the same
   // as the number of tasks. Since the fobed zombies are auto deleted when a complete
   // child command is received. 
   int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   BOOST_CHECK_MESSAGE(no_of_fobed_zombies > 0,"Expected  some fobed zombies but found none ?");

   // Fobing does *NOT* alter node tree state, however child COMPLETE should auto delete the zombie
   if (!wait_for_zombie_termination(timeout)) {
      remove_stale_zombies();  // see notes above
   }

   // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
   check_expected_no_of_zombies(0);

   cout << timer.duration() << "\n";
}
#endif


#ifdef DO_TEST5
BOOST_AUTO_TEST_CASE( test_zombies_attr_for_begin )
{
   DurationTimer timer;
   std::string suite_name  = "test_zombies_attr_for_begin";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test(suite_name,"begin"); // create zombies via begin force

   /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
   BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL,NState::COMPLETE,timeout),"Expected non-zombie tasks to complete");

   check_at_least_one_zombie();

   /// *** Fobbing will not change state of the node tree ****
   if (ecf_debug_enabled) std::cout << "   Add a zombie attribute 'user:fob::' to the suite, which fobs all child commands\n";
   TestFixture::client().alter("/" + suite_name,"add","zombie","user:fob::");

   // Fobbing causes auto deletion of zombies, when the Child complete is reached
   if (!wait_for_zombie_termination(timeout)) {
      remove_stale_zombies();  // see notes above
   }

   // The fob should have forced removal of zombies, in the server. when the COMPLETE child command was recieved
   check_expected_no_of_zombies(0);

   cout << timer.duration() << "\n";
}
#endif



#ifdef DO_TEST6
BOOST_AUTO_TEST_CASE( test_user_zombies_for_adopt )
{
   DurationTimer timer;
   std::string suite_name  = "test_user_zombies_for_adopt";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test(suite_name,"begin");

   /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
   BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL,NState::COMPLETE,timeout),"Expected non-zombie tasks to complete");

   check_at_least_one_zombie();

   /// Adopt all the zombies. This will UNBLOCK the child commands allowing them to finish
   /// This test below fail on AIX, its too fast , task's may already be adopted and hence don't fail
   int no_of_adopted_zombied = ZombieUtil::do_zombie_user_action(User::ADOPT,  NUM_OF_TASKS, timeout);
   if (ecf_debug_enabled) cout << "   found " << no_of_adopted_zombied << " zombies for adoption\n";

   /// The blocked zombies are free, start with blocked init command
   /// This may fail on AIX, its too fast , task's may already be complete, hence don't fail
   (void)waitForTaskState(SINGLE,NState::ACTIVE,timeout);

   /// Now wait for all tasks to complete
   BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL,NState::COMPLETE,timeout),"Expected zombie tasks to complete");


   remove_stale_zombies();  // see notes above

   // After adoption the zombies should be removed
   check_expected_no_of_zombies(0);

   cout << timer.duration() << "\n";
}
#endif


#ifdef DO_TEST7
BOOST_AUTO_TEST_CASE( test_zombies_attr_for_adopt )
{
   DurationTimer timer;
   std::string suite_name  = "test_zombies_attr_for_adopt";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test(suite_name,"begin");

   /// We have two *sets* of jobs, Wait for ALL the tasks(non zombies) to complete
   BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL,NState::COMPLETE,timeout),"Wait for all non-zombie tasks to complete failed");

   // expected 5 zombies, ie because we have NUM_OF_TASKS tasks. These should all be blocking
   check_at_least_one_zombie();

   if (ecf_debug_enabled) std::cout << "   Add a zombie attribute 'user:adopt::' to the suite, which *ADOPTS* all zombies allowing them to complete\n";
   TestFixture::client().alter("/" + suite_name,"add","zombie","user:adopt::");

   if (ecf_debug_enabled) dump_zombies();

   /// The blocked zombies are free, start with blocked init command
   /// This may fail on AIX, its too fast , task's may already be complete, dont fail
   (void)waitForTaskState(SINGLE,NState::ACTIVE,timeout);

   /// Now wait for all tasks to complete. ** They may be complete from last process set **
   BOOST_REQUIRE_MESSAGE(waitForTaskState(ALL,NState::COMPLETE,timeout),"Expected all zombie task to complete after adopt");

   remove_stale_zombies();  // see notes above

   // After adoption the zombies should be removed
   check_expected_no_of_zombies(0);

   cout << timer.duration() << "\n";
}
#endif


#ifdef DO_TEST8
BOOST_AUTO_TEST_CASE( test_ecf_zombie_creation_via_complete )
{
   DurationTimer timer;
   std::string suite_name  = "test_ecf_zombie_creation_via_complete";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test(suite_name,"complete");

   /// Since we have set tasks to complete, we should only have *ONE* set of zombies
   check_at_least_one_zombie();

   // Fob all the zombies child commands allowing them to finish
   (void) ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   // int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   // BOOST_CHECK_MESSAGE(no_of_fobed_zombies == NUM_OF_TASKS,"Expected " << NUM_OF_TASKS << " Fobed zombies but found " << no_of_fobed_zombies);

   // Wait for zombies to complete, they should get removed automatically
   wait_for_no_zombies( timeout);

   cout << timer.duration() << "\n";
}
#endif

#ifdef DO_TEST9
BOOST_AUTO_TEST_CASE( test_ecf_zombie_creation_via_abort )
{
   DurationTimer timer;
   std::string suite_name  = "test_ecf_zombie_creation_via_abort";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test(suite_name,"aborted");

   /// Since we have set tasks to complete, we should only have *ONE* set of zombies
   check_at_least_one_zombie();

   // Fob all the zombies child commands allowing them to finish
   (void) ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   // int no_of_fobed_zombies = ZombieUtil::do_zombie_user_action(User::FOB, NUM_OF_TASKS, timeout);
   // BOOST_CHECK_MESSAGE(no_of_fobed_zombies == NUM_OF_TASKS,"Expected " << NUM_OF_TASKS << " Fobed zombies but found " << no_of_fobed_zombies);

   // Wait for zombies to complete, they should get removed automatically
   wait_for_no_zombies(timeout);

   cout << timer.duration() << "\n";
}
#endif


#ifdef DO_TEST10
BOOST_AUTO_TEST_CASE( test_zombie_inheritance )
{
   DurationTimer timer;
   std::string suite_name  = "test_zombie_inheritance";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // Add zombie attribute, make sure it inherited
   Defs theDefs;
   populate_defs(theDefs,suite_name);
   suite_ptr suite = theDefs.findSuite(suite_name);
   suite->addZombie( ZombieAttr(ecf::Child::USER, std::vector<ecf::Child::CmdType>(), ecf::User::FOB,-1) );
   suite->addZombie( ZombieAttr(ecf::Child::ECF, std::vector<ecf::Child::CmdType>(), ecf::User::FOB,-1) );
   suite->addZombie( ZombieAttr(ecf::Child::PATH, std::vector<ecf::Child::CmdType>(), ecf::User::FOB,-1) );

   create_and_start_test(theDefs,suite_name,"complete" );

   /// Since we have set tasks to complete, we should only have *ONE* set of zombies
   // expect NUM_OF_TASKS zombies, ie because we have NUM_OF_TASKS tasks
   TestFixture::client().set_throw_on_error(true);
   TestFixture::client().zombieGet();
   std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
   BOOST_CHECK_MESSAGE(!zombies.empty(),"No zombies found");
   BOOST_FOREACH(const Zombie& z, zombies) {
      BOOST_CHECK_MESSAGE(z.user_action() == ecf::User::FOB, "Expected zombies with user action of type FOB but found " << User::to_string(z.user_action()));
      break;
   }

   // Wait for zombies to complete, they should get removed automatically
   wait_for_no_zombies(timeout);

   cout << timer.duration() << "\n";
}
#endif


#ifdef DO_TEST11
static int wait_for_killed_zombies(int no_of_tasks, int max_time_to_wait)
{
   if (ecf_debug_enabled) std::cout << "\n   wait_for_killed_zombies\n";
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      int killed = 0;
      TestFixture::client().zombieGet();
      std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
      BOOST_FOREACH(const Zombie& z, zombies) {
         if (z.kill()) killed++;
      }
      if (ecf_debug_enabled) std::cout << "   found " << killed << " killed zombies\n";

      if (killed == no_of_tasks)  return killed;

      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         cout <<  "wait_for_killed_zombies Test wait " << assertTimer.duration() <<
                  " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                  " breaking out\n" << Zombie::pretty_print( zombies , 3) << "\n";
         return killed;
      }
      sleep(2);
   }
   return 0;
}

BOOST_AUTO_TEST_CASE( test_zombie_kill )
{
   DurationTimer timer;
   std::string suite_name  = "test_zombie_kill";
   cout << "Test:: ..." << suite_name << " " << flush;
   TestClean clean_at_start_and_end;

   // This command creates user zombies up front, these may not have a pid, if task in submitted state
   create_and_start_test(suite_name,"complete");

   check_at_least_one_zombie();

   // kill all the zombies, i.e kill -15 on the script
   // This will be trapped by the signal and hence will call abort
   (void) ZombieUtil::do_zombie_user_action(User::KILL, NUM_OF_TASKS, timeout);

   // wait for kill zombies. This should eventually lead to process terminating
   int killed = wait_for_killed_zombies(NUM_OF_TASKS,timeout);
   BOOST_CHECK_MESSAGE(killed > 0,"Expected " <<  NUM_OF_TASKS << " killed ");

   {
      // wait for process to be killed: killing is a separate process, we could well
      // have got to the complete, before the process is killed.
      // Once the complete is fobed it terminate the process.
      AssertTimer assertTimer(timeout,false); // Bomb out after n seconds, fall back if test fail
      while (1) {
          int completed = 0; int aborted = 0;
          TestFixture::client().zombieGet();
          std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
          BOOST_FOREACH(const Zombie& z, zombies) {
             if (z.last_child_cmd() == ecf::Child::ABORT) aborted++;
             if (z.last_child_cmd() == ecf::Child::COMPLETE) completed++;
          }
          if (aborted + completed == NUM_OF_TASKS) break;
          if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
             cout <<  "wait for for abort:  found " << aborted+completed<< " aborted & completed tasks. Test wait "
                      << assertTimer.duration() <<
                      " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                      " breaking out\n" << Zombie::pretty_print( zombies , 3) << "\n";
             break;
          }
          sleep(1);
       }
   }
   bool task_became_blocked = false;
   {
      // wait for process to be die
      AssertTimer assertTimer(timeout,false); // Bomb out after n seconds, fall back if test fail
      while (1) {
          TestFixture::client().zombieGet();
          std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
          BOOST_FOREACH(const Zombie& z, zombies) {
             if (z.block()) {  // something went wrong, fob so don't leave process hanging
                TestFixture::client().zombieFob(z);
                task_became_blocked = true;
                cout << "Zombies blocking ?? " << z << "\n";
             }
          }
          if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
             if ( task_became_blocked ) {
                cout <<  "Task became blocked, fobing" << assertTimer.duration() <<
                         " taking longer than time constraint of " << assertTimer.timeConstraint() <<
                         " breaking out\n" << Zombie::pretty_print( zombies , 3) << "\n";
             }
             break;
          }
          sleep(1);
       }
   }

   // remove the killed zombies
   (void) ZombieUtil::do_zombie_user_action(User::REMOVE,NUM_OF_TASKS, timeout,false);

   wait_for_no_zombies(timeout);

   cout << timer.duration() << "\n";
}
#endif

BOOST_AUTO_TEST_SUITE_END()
