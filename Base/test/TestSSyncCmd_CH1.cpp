//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include <boost/function.hpp>

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "MyDefsFixture.hpp"
#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "SSyncCmd.hpp"
#include "SNewsCmd.hpp"
#include "Ecf.hpp"
#include "NodeFwd.hpp"
#include "SuiteChanged.hpp"

using namespace std;
using namespace ecf;

/// This test, is used to check sync with the client handles.
/// The client handles will register interest in a set of suites
/// When calling sync, check will only receive notification on our
/// our suites
// In particular when a set of suites are registered to a handle
// and we add/delete/check point/order the server passes back to the client
// the a *new* defs with all the suites corresponding to the handle
// There is a complication however, the suite are not copied/cloned
// This causes a problem, since if we create a new defs, and add
// the suites, the suite defs pointer is *NOW* corrupted, as it will point
// to the new defs. This is not an issue in real apps, since searisation
// fixes up these ptrs.

// The client handle commands do not change state & modify change number, hence need to bypass these checks
static bool bypass_state_modify_change_check = false;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

static defs_ptr create_client_defs(defs_ptr defs)
{
   for(size_t j = 0; j < 5; j++)  {
      suite_ptr suite = defs->add_suite( "s" + boost::lexical_cast<std::string>(j) );
      family_ptr f = suite->add_family("f");
      f->add_task("t");
      if (j == 0) {
         suite->addLimit( Limit("suiteLimit",10) );
         suite->addRepeat( RepeatDate("YMD",20090916,20090916,1) );
      }
   }
   return defs;
}

static defs_ptr create_server_defs()
{
   defs_ptr defs = Defs::create();

   // Create server defs, with a port other than default.
   // This allows additional testing. i.e server variables
   std::vector<Variable>  server_variables;
   ServerState::setup_default_server_variables(server_variables,"4000");
   defs->set_server().set_server_variables(server_variables);

   // ensure client/server start out the same
   return create_client_defs(defs);
}

/// define a function which returns nothing, and takes a defs_ptr parameter
typedef boost::function<bool (defs_ptr)> defs_change_cmd;

/// Re-use the same test scaffold to modify and then resync, by passing
/// in a function that will modify the defs
void test_sync_scaffold( defs_change_cmd the_defs_change_command, const std::string& test_name, bool full_sync = false)
{
   // Create the defs
   defs_ptr server_defs = create_server_defs();
   ServerReply server_reply;
   server_reply.set_client_defs( create_client_defs(Defs::create()) );

   Ecf::set_debug_equality(true); // only has affect in DEBUG build
   BOOST_CHECK_MESSAGE( *server_defs == *server_reply.client_defs(),
                        test_name << ": Starting point client and server defs should be the same : "
                        << "SERVER\n" << server_defs
                        << "CLIENT\n" << server_reply.client_defs());
   Ecf::set_debug_equality(false);

   // set handle and change numbers, before any changes
   Ecf::set_state_change_no(0);
   Ecf::set_modify_change_no(0);
   unsigned int client_state_change_no = Ecf::state_change_no();
   unsigned int client_modify_change_no = Ecf::modify_change_no();
   unsigned int client_handle = 0;

   bool expected_change;

   // make some change to the server
   {
      /// create client handle which references suites s0 and s4, in the server defs
      /// Registering suites should change handle_changed boolean
      std::vector<std::string> suite_names; suite_names.push_back("s0"); suite_names.push_back("s4");
      TestHelper::invokeRequest(server_defs.get(),Cmd_ptr( new ClientHandleCmd(suite_names,false)),bypass_state_modify_change_check);

      BOOST_CHECK_MESSAGE(server_defs->client_suite_mgr().clientSuites().size() == 1,test_name << ": Expected 1 Client suites but found " <<server_defs->client_suite_mgr().clientSuites().size());
      client_handle = server_defs->client_suite_mgr().clientSuites().front().handle();
      BOOST_CHECK_MESSAGE( client_handle == 1,"" );

      /// Check that handle_changed set, required for syncing, i.e needed for full sync , without change state/modify numbers
      BOOST_CHECK_MESSAGE(server_defs->client_suite_mgr().handle_changed(client_handle) == true,"Expected handle_changed to be set when suites are registered ");

      /// called create Defs should clear the flag. make sure server state, is synced
      defs_ptr the_client_defs = server_defs->client_suite_mgr().create_defs(client_handle,server_defs);
      BOOST_CHECK_MESSAGE(the_client_defs->suiteVec().size() == 2  ,test_name << ": Expected 2 suites");
      BOOST_CHECK_MESSAGE(server_defs->client_suite_mgr().handle_changed(client_handle) == false,test_name << ": Expected handle_changed to be cleared after create_defs()");
      Ecf::set_debug_equality(true);
      BOOST_CHECK_MESSAGE( server_defs->server().compare(the_client_defs->server() ), test_name << ": Server state does not match");
      Ecf::set_debug_equality(false);

      Ecf::set_server(true);
      expected_change = the_defs_change_command(server_defs);
      std::string error_msg;
      BOOST_REQUIRE_MESSAGE( server_defs->checkInvariants(error_msg) , test_name << ": Invariants failed: " << error_msg);
      Ecf::set_server(false);

      /// Call create defs again, after change in server defs, check server state is synced
      the_client_defs = server_defs->client_suite_mgr().create_defs(client_handle,server_defs);
      Ecf::set_debug_equality(true);
      BOOST_CHECK_MESSAGE( server_defs->server().compare(the_client_defs->server() ), test_name << ": Server state does not match");
      Ecf::set_debug_equality(false);
   }

   MockServer mock_server(server_defs);
   SNewsCmd news_cmd(client_handle, client_state_change_no,  client_modify_change_no, &mock_server );
   SSyncCmd cmd(client_handle, client_state_change_no,  client_modify_change_no, &mock_server );

   if ( expected_change ) {
      BOOST_CHECK_MESSAGE( news_cmd.get_news(),         test_name << " : get_news : Expected server to change");
      BOOST_CHECK_MESSAGE( cmd.do_sync( server_reply ), test_name << " : do_sync : Expected server to change");
      BOOST_CHECK_MESSAGE( server_reply.in_sync(),      test_name << " : in_sync : Expected client server to be in sync");
      BOOST_CHECK_MESSAGE( server_reply.full_sync() == full_sync,test_name << ": Expected sync not as expected. client: " << server_reply.full_sync() << " full_sync: " << full_sync);
      BOOST_CHECK_MESSAGE( server_defs->state() == server_reply.client_defs()->state(),test_name << ": Expected server State(" << NState::toString(server_defs->state()) << ") to be same as client state(" << NState::toString(server_reply.client_defs()->state()) << ")");
      if (full_sync) {
         Ecf::set_debug_equality(true);
         BOOST_CHECK_MESSAGE( server_defs->server().compare(server_reply.client_defs()->server()),test_name << ": Server state does not match");
         Ecf::set_debug_equality(false);
      }
      else {
      }

      // * Note we expect client defs to fail invariant checking when doing a full sync with handles
      // * Under real server this should be ok since, we fix up the defs ptr, during serialisation

      // * note. We can't really compare server and client defs, since when we sync with
      // * with handles, we only return a sub set of the suites, in our handle

      // DO a sync again. hence we should expect no changes
      server_reply.clear_for_invoke(false);
      Ecf::set_server(true);
      /* server side */ SNewsCmd news_cmd1(client_handle, server_reply.client_defs()->state_change_no(),  server_reply.client_defs()->modify_change_no(), &mock_server );
      /* server side */ SSyncCmd cmd1(client_handle, server_reply.client_defs()->state_change_no(),  server_reply.client_defs()->modify_change_no(), &mock_server );
      Ecf::set_server(false);
      /* client side */ BOOST_CHECK_MESSAGE( !news_cmd1.get_news(),         test_name << ": Expected no changes to client, we should be in sync");
      /* client side */ BOOST_CHECK_MESSAGE( !cmd1.do_sync( server_reply ), test_name << ": Expected no changes to client, we should be in sync");
   }
   else {

      BOOST_CHECK_MESSAGE( !news_cmd.get_news(), test_name << ": Expected no change");
      BOOST_CHECK_MESSAGE( !cmd.do_sync( server_reply ), test_name << ": Expected no change");
      BOOST_CHECK_MESSAGE( !server_reply.in_sync(),      test_name << ": Expected no change");
      BOOST_CHECK_MESSAGE( !(*server_defs == *server_reply.client_defs()), test_name << ": Server and client defs expected to differ" );
   }
}


static bool set_server_state_shutdown(defs_ptr defs) {
   defs->set_server().set_state( SState::SHUTDOWN );
   return true; // expect changes
}
static bool set_server_state_running(defs_ptr defs) {
   defs->set_server().set_state( SState::RUNNING );
   return true; // expect changes
}

static bool change_suites_s3_outside_of_handle(defs_ptr defs)
{
   /// Make a state change to suites s3, in the server. This is *not* in the client handle
   /// Note: we do *NOT* make a state change as this will be propagated to the defs, and hence will be synced
   /// We *NEED* MockSuiteChangedServer, so that change is propagated to the suite.
   MockSuiteChangedServer mockServer(defs->findSuite("s3")); // Increment suite state/modify change number
   defs->findSuite("s3")->suspend();  // small scale state change
   return false;                    // expect no changes
}

static bool change_suites_s3_outside_of_handle_add_variable(defs_ptr defs)
{
   /// make a state change to suites s3, in the server. This is *not* in the client handle
   /// We *NEED* MockSuiteChangedServer, so that change is propagated to the suite.
   MockSuiteChangedServer mockServer(defs->findSuite("s3")); // Increment suite state/modify change number
   defs->findSuite("s3")->add_variable("Var","value");       // small scale state change
   return false;                                             // expect no changes in sync
}

static bool add_task_to_suite_s3(defs_ptr defs)
{
   /// make a modify change to suites s3, in the server. This is *not* in the client handle
   MockSuiteChangedServer mockServer(defs->findSuite("s3")); // Increment suite state/modify change number
   defs->findSuite("s3")->addTask( Task::create("s3_task")); // small scale change
   return false;                                             //  expect no changes, since suite s3 not in handle
}

static bool delete_task_on_suite_s3(defs_ptr defs)
{
   /// make a modify change to suites s3, in the server. This is *not* in the client handle
   MockSuiteChangedServer mockServer(defs->findSuite("s3")); // Increment suite state/modify change number
   defs->findAbsNode("/s3/f/t")->remove(); // small scale change
   return false;                           // expect no changes, since suite s3 not in handle
}
static bool delete_family_on_suite_s3(defs_ptr defs)
{
   /// make a modify change to suites s3, in the server. This is *not* in the client handle
   MockSuiteChangedServer mockServer(defs->findSuite("s3")); // Increment suite state/modify change number
   defs->findAbsNode("/s3/f")->remove(); // small scale change
   return false;                         // expect no changes, since suite s3 not in handle
}

static bool change_state_of_s4(defs_ptr defs)
{
   /// Ok now make state change to s4, which **is** in the handle
   /// We need MockSuiteChangedServer, so that change is propagated to the suite.
   MockSuiteChangedServer mockServer(defs->findSuite("s4")); // Increment suite state/modify change number
   defs->findSuite("s4")->set_state(NState::SUBMITTED);      // small scale change
   return true;                                             // expect changes
}

static bool add_variable_to_suite_s4(defs_ptr defs)
{
   /// Ok now make state change to s4, which **is** in the handle
   /// We need MockSuiteChangedServer, so that change is propagated to the suite.
   MockSuiteChangedServer mockServer(defs->findSuite("s4")); // Increment suite state/modify change number
   defs->findSuite("s4")->add_variable("Var","value");       // small scale state change
   return true;                                             // expect changes
}

static bool add_server_user_variables(defs_ptr defs)
{
   // Change server. This is outside of any suites
   std::vector<Variable> user_variables;
   user_variables.push_back(Variable("a","b"));
   user_variables.push_back(Variable("c","d"));
   defs->set_server().set_user_variables(user_variables);
   return true; // expect change
}

static bool add_task_to_suite_s4(defs_ptr defs)
{
   /// Ok now make modify change to s4, which **is** in the handle
   MockSuiteChangedServer mockServer(defs->findSuite("s4")); // Increment suite state/modify change number
   defs->findSuite("s4")->addTask( Task::create("s4_task")); // small scale change
   return true;                                             // expect changes
}

static bool delete_task_on_suite_s4(defs_ptr defs)
{
   /// Ok now make modify change to s4, which **is** in the handle
   MockSuiteChangedServer mockServer(defs->findSuite("s4")); // Increment suite state/modify change number
   defs->findAbsNode("/s4/f/t")->remove();                   // small scale change
   return true;                                             // expect changes, since suite s4 is in handle
}

static bool delete_family_on_suite_s4(defs_ptr defs)
{
   /// Ok now make modify change to s4, which **is** in the handle
   MockSuiteChangedServer mockServer(defs->findSuite("s4")); // Increment suite state/modify change number
   defs->findAbsNode("/s4/f")->remove();                     // small scale change
   return true;                                             // expect changes, since suite s4 is in handle
}

static bool change_order_of_s4_top(defs_ptr defs) {
   /// Ok make modify change to s4, which **is** in the handle
   suite_ptr s4 = defs->findSuite("s4");
   MockSuiteChangedServer mockServer(s4); // Increment suite state/modify change number
   defs->order(s4.get(),NOrder::TOP) ;    // small scale scale change
   return true;
}

static bool change_order_of_s4_bottom(defs_ptr defs) {
   /// Ok make modify change to s4, which **is** in the handle
   suite_ptr s4 = defs->findSuite("s4");
   MockSuiteChangedServer mockServer(s4);   // Increment suite state/modify change number
   defs->order(s4.get(),NOrder::BOTTOM) ;   // small scale change
   return true;
}

static bool delete_suite_s4(defs_ptr defs)
{
   /// Ok now make delete s4 which, which **is** in the handle
   MockSuiteChangedServer mockServer(defs->findSuite("s4")); // Increment suite state/modify change number
   defs->findSuite("s4")->remove();            // large scale change
   return true;                                // expect changes
}

static bool set_defs_state(defs_ptr defs)
{
   defs->set_state( NState::ABORTED );         // changes the defs state
   return true;                               // expect changes
}

static bool set_defs_state_2(defs_ptr defs)
{
   defs->set_state( NState::ABORTED );         // changes the defs state
   return delete_suite_s4( defs );            // large scale change
}

static bool add_server_variable(defs_ptr defs) {
   // Change defs server state. small scale change
   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"_fred_","value")));
   return true;
}

static bool change_server_variable(defs_ptr defs) {
   // Because the scaffold create client/server defs each time.
   // To test change/delete variables we modify the default set
   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd("/",AlterCmd::VARIABLE,"ECF_TRIES","4")));
   return true;
}

// ===============================================================================
// The modifiers, do this for suite s0 which is in a handle
// ===============================================================================
static bool s0_delete_some_attributes(defs_ptr defs) {

   /// Ok now make state change to s4, which **is** in the handle
   /// We need MockSuiteChangedServer, so that change is propagated to the suite.
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   std::vector<Task*> tasks;
   suite->getAllTasks(tasks);
   BOOST_REQUIRE_MESSAGE( !tasks.empty(), "Expected at least one task");

   BOOST_FOREACH(Task* task, tasks) {
      SuiteChanged1 changed(task->suite());
      task->addMeter(Meter("meter",0,100));
   }
   return true;
}

static bool s0_add_some_attributes(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   std::vector<task_ptr> tasks;
   suite->get_all_tasks(tasks);
   BOOST_REQUIRE_MESSAGE( !tasks.empty(), "Expected at least one task");

   BOOST_FOREACH(task_ptr task, tasks) { SuiteChanged1 changed(suite.get()); task->addDay( DayAttr(DayAttr::TUESDAY) );}
   return true;
}

static bool s0_begin(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   suite->begin();
   return true;
}

static bool s0_add_alias(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   std::vector<task_ptr> tasks;
   suite->get_all_tasks(tasks);
   BOOST_REQUIRE_MESSAGE( !tasks.empty(), "Expected at least one task");

   SuiteChanged1 changed(tasks[0]->suite());
   tasks[0]->add_alias_only();
   return true;
}

static bool s0_remove_all_tasks(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number


   // Remove tasks should force a incremental sync
   std::vector<task_ptr> tasks;
   suite->get_all_tasks(tasks);
   BOOST_REQUIRE_MESSAGE( !tasks.empty(), "Expected at least one task");
   BOOST_FOREACH(task_ptr task, tasks) { SuiteChanged1 changed(task->suite()); task->remove() ;}

   tasks.clear();
   suite->get_all_tasks(tasks);
   BOOST_REQUIRE_MESSAGE( tasks.empty(), "Failed to delete tasks");
   return true;
}

static bool s0_remove_a_family(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   std::vector<Family*> vec;
   suite->getAllFamilies(vec);
   size_t family_size = vec.size();
   BOOST_REQUIRE_MESSAGE( !vec.empty(), "Expected at least one family");
   if (!vec.empty()) {
      SuiteChanged1 changed(vec[0]->suite());
      vec[0]->remove();
   }

   vec.clear();
   suite->getAllFamilies(vec);
   BOOST_REQUIRE_MESSAGE( vec.size() < family_size, "Failed to delete family");
   return true;
}


static bool s0_change_clock_gain(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   suite->changeClockGain("100001");
   return true;
}

static bool s0_change_clock_type_to_real(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   suite->changeClockType("hybrid");
   return true;
}

static bool s0_change_clock_date(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");

   suite->changeClockDate("1.1.2001");
   return true;
}

static bool s0_change_clock_sync(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number

   suite->changeClockSync();
   return true;
}

/// This has been split into two functions, as changing both together could mask an error
/// i.e found bug where we forgot to update state_change number when changing the limit
/// max value, however because we had, changed value as well it got masked.
static bool s0_change_limit_max(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   // Note: we ONLY* need MockSuiteChangedServer, when we make changes via functions and not commands

   std::vector<limit_ptr> theLimits =  suite->limits();
   BOOST_REQUIRE_MESSAGE( !theLimits.empty(),"The limit are empty on suite s0 " << defs);
   BOOST_FOREACH(limit_ptr l, theLimits) {
      //std::cout << "found " << l->toString() << "\n";
      TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd(suite->absNodePath(),AlterCmd::LIMIT_MAX,l->name(),"90")));
      limit_ptr v = suite->find_limit(l->name());
      BOOST_CHECK_MESSAGE( v.get() && v->theLimit() == 90, "expected to find limit with max value of 90");
   }
   return true;
}

static bool s0_change_limit_value(defs_ptr defs) {
   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   // Note: we ONLY* need MockSuiteChangedServer, when we make changes via functions and not commands

   std::vector<limit_ptr> theLimits =  suite->limits();
   BOOST_FOREACH(limit_ptr l, theLimits) {
      TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd(suite->absNodePath(),AlterCmd::LIMIT_VAL,l->name(),"33")));
      limit_ptr v = suite->find_limit(l->name());
      BOOST_CHECK_MESSAGE( v.get() && v->value() == 33, "expected to find limit with value of 33");
   }
   return true;
}

static bool s0_update_repeat(defs_ptr defs) {

   suite_ptr suite = defs->findSuite("s0");
   BOOST_REQUIRE_MESSAGE( suite,"Could not find suite");
   MockSuiteChangedServer mockServer(suite); // Increment suite state/modify change number


   SuiteChanged1 changed(suite.get());
   suite->increment_repeat();
   return true;
}

// ===============================================

BOOST_AUTO_TEST_CASE( test_ssync_using_handle  )
{
   cout << "Base:: ...test_ssync_using_handle\n";

   // =======================================================================================
   // Note: where we update Suite::modify_change_no()  we should *EXPECT* a full sync
   // =======================================================================================

   // test_sync_scaffold will created 5 suites  s0,s1,s2,s3,s4,s5 and add suites s0 & s4 to a handle
   // The following test will perform changes in/out of handles

   {  // Change defs state in the presence of handles. These should sync regardless of handles
      // The default server state is HALTED, hence setting to halted will not show a change
      test_sync_scaffold(set_server_state_shutdown,"set_server_state_shutdown");
      test_sync_scaffold(set_server_state_running,"set_server_state_running");

      test_sync_scaffold(add_server_variable,"add_server_variable");
      test_sync_scaffold(change_server_variable,"change_server_variable");

      test_sync_scaffold(set_defs_state,"set_defs_state");
      test_sync_scaffold(set_defs_state_2,"set_defs_state_2",true /* expect a full sync */);
   }

   test_sync_scaffold(change_suites_s3_outside_of_handle,"change_suites_s3_outside_of_handle");
   test_sync_scaffold(change_suites_s3_outside_of_handle_add_variable,"change_suites_s3_outside_of_handle_add_variable");
   test_sync_scaffold(add_task_to_suite_s3,"add_task_to_suite_s3");
   test_sync_scaffold(delete_task_on_suite_s3,"delete_task_on_suite_s3");
   test_sync_scaffold(delete_family_on_suite_s3,"delete_family_on_suite_s3");

   test_sync_scaffold(change_order_of_s4_top,"change_order_of_s4_top");    // change order is an incremental sync
   test_sync_scaffold(change_order_of_s4_bottom,"change_order_of_s4_bottom"); // change order is an incremental sync

   test_sync_scaffold(change_state_of_s4,"change_state_of_s4");
   test_sync_scaffold(add_variable_to_suite_s4,"add_variable_to_suite_s4");
   test_sync_scaffold(add_server_user_variables,"add_server_user_variables");
   test_sync_scaffold(add_task_to_suite_s4,"add_task_to_suite_s4");
   test_sync_scaffold(delete_task_on_suite_s4,"delete_task_on_suite_s4");
   test_sync_scaffold(delete_family_on_suite_s4,"delete_family_on_suite_s4");
   test_sync_scaffold(delete_suite_s4,"delete_suite_s4", true /* expect a full sync */);


   test_sync_scaffold(s0_delete_some_attributes,"s0_delete_some_attributes");
   test_sync_scaffold(s0_add_some_attributes,"s0_add_some_attributes");
   test_sync_scaffold(s0_add_alias,"s0_add_alias");
   test_sync_scaffold(s0_update_repeat,"s0_update_repeat");
   test_sync_scaffold(s0_change_limit_max,"s0_change_limit_max");
   test_sync_scaffold(s0_change_limit_value,"s0_change_limit_value");
   test_sync_scaffold(s0_begin,"s0_begin", true/* expect a full sync */);

   test_sync_scaffold(s0_remove_all_tasks,"s0_remove_all_tasks" );
   test_sync_scaffold(s0_remove_a_family,"s0_remove_a_family");

   test_sync_scaffold(s0_change_clock_gain,"s0_change_clock_gain", true/* expect a full sync */);
   test_sync_scaffold(s0_change_clock_type_to_real,"s0_change_clock_type_to_real", true/* expect a full sync */);
   test_sync_scaffold(s0_change_clock_date,"s0_change_clock_date", true/* expect a full sync */);
   test_sync_scaffold(s0_change_clock_sync,"s0_change_clock_sync", true/* expect a full sync */);
}



static defs_ptr create_the_server_defs()
{
   defs_ptr defs = create_server_defs();
   std::vector<suite_ptr> suite_vec =  defs->suiteVec();
   for(size_t j = 0; j < suite_vec.size(); j++)  {
      suite_vec[j]->set_state_change_no(j);
      suite_vec[j]->set_modify_change_no(j);
   }
   return defs;
}

BOOST_AUTO_TEST_CASE( test_ssync_full_sync_using_handle  )
{
   /// This test checks that when user has registered with all the suites.
   /// Syncing should use the global change numbers
   /// **ADDITIONALLY* the newsCmd  must reflect this.
   /// This is handled in ClientSuites::create_defs, in that we *MUST* update the
   /// local change numbers to be the same as the global change numbers
   /// This is important since the NewsCmd must be in *SYNC* with SYNCCmd

   cout << "Base:: ...test_ssync_full_sync_using_handle\n";
   // Create the server defs with some changes, in state & modify numbers
   defs_ptr server_defs = create_the_server_defs();

   // Create Client defs, without any changes
   ServerReply server_reply;
   server_reply.set_client_defs( create_client_defs(Defs::create()) );

   // Server & client should be the same, since we ignore change numbers in the comparison
   Ecf::set_debug_equality(true); // only has affect in DEBUG build
   BOOST_CHECK_MESSAGE( *server_defs == *server_reply.client_defs(), "Starting point client and server defs should be the same"
                        << "SERVER\n" << server_defs
                        << "CLIENT\n" << server_reply.client_defs());
   Ecf::set_debug_equality(false);

   /// register interest in **ALL** the suites
   std::vector<std::string> suite_names;
   suite_names.push_back("s0");
   suite_names.push_back("s1");
   suite_names.push_back("s2");
   suite_names.push_back("s3");
   suite_names.push_back("s4");
   TestHelper::invokeRequest(server_defs.get(),Cmd_ptr( new ClientHandleCmd(suite_names,false)),bypass_state_modify_change_check);

   /// make sure handle created.
   BOOST_CHECK_MESSAGE(server_defs->client_suite_mgr().clientSuites().size() == 1,"Expected 1 Client suites but found " <<server_defs->client_suite_mgr().clientSuites().size());
   unsigned int client_handle = server_defs->client_suite_mgr().clientSuites().front().handle();
   BOOST_CHECK_MESSAGE( client_handle == 1,"" );

   // make sure server suites have different state/modify numbers from the client
   {
      unsigned int server_state_change_no = 0;
      unsigned int server_modify_change_no = 0;
      server_defs->client_suite_mgr().max_change_no(client_handle,server_state_change_no, server_modify_change_no);
      BOOST_CHECK_MESSAGE(server_state_change_no == 4,"" );
      BOOST_CHECK_MESSAGE(server_modify_change_no == 4,"" );

      // *MAKE* sure global change numbers are different to handle suite change numbers
      // This is the key part of this test.
      // Since syncing should transfer these change numbers to the client
      Ecf::set_state_change_no(server_state_change_no+10);
      Ecf::set_modify_change_no(server_modify_change_no+20);
   }

   // Now sync from server
   Ecf::set_server(true);
   MockServer mock_server(server_defs);
   /* server side */ SNewsCmd news_cmd(client_handle, server_reply.client_defs()->state_change_no(),  server_reply.client_defs()->modify_change_no(), &mock_server );
   /* server side */ SSyncCmd cmd(client_handle, server_reply.client_defs()->state_change_no(),  server_reply.client_defs()->modify_change_no(), &mock_server );
   Ecf::set_server(false);

   // make sure SSyncCmd updated the server change numbers, to use global change numbers
   unsigned int server_state_change_no = 0;
   unsigned int server_modify_change_no = 0;
   {
      server_defs->client_suite_mgr().max_change_no(client_handle,server_state_change_no, server_modify_change_no);
      BOOST_CHECK_MESSAGE(server_state_change_no == 14,"" );
      BOOST_CHECK_MESSAGE(server_modify_change_no == 24,"" );
   }

   // SYNC the client, via do_sync( this should transfer change numbers to the client )
   BOOST_CHECK_MESSAGE( news_cmd.get_news(), "Expected server to change");
   BOOST_CHECK_MESSAGE( cmd.do_sync( server_reply ), "Expected server to change");
   BOOST_CHECK_MESSAGE( server_reply.in_sync(),      "Expected server to change");
   BOOST_CHECK_MESSAGE( server_defs->state() == server_reply.client_defs()->state(),"Expected server State(" << NState::toString(server_defs->state()) << ") to be same as client state(" << NState::toString(server_reply.client_defs()->state()) << ")");

   // After do_sync client and server change number should be in sync
   BOOST_CHECK_MESSAGE(server_reply.client_defs()->state_change_no() == server_state_change_no,"Expected " << server_reply.client_defs()->state_change_no() << " state change number but found " << server_state_change_no);
   BOOST_CHECK_MESSAGE(server_reply.client_defs()->modify_change_no() == server_modify_change_no,"Expected " << server_reply.client_defs()->modify_change_no() << " modify change number but found " << server_modify_change_no);

   // Do final sync, there should not be ANY changes
   Ecf::set_server(true);
   /* server side */ SNewsCmd news_cmd1(client_handle, server_reply.client_defs()->state_change_no(),  server_reply.client_defs()->modify_change_no(), &mock_server );
   /* server side */ SSyncCmd cmd1(client_handle, server_reply.client_defs()->state_change_no(),  server_reply.client_defs()->modify_change_no(), &mock_server );
   Ecf::set_server(false);
   /* client side */ BOOST_CHECK_MESSAGE( !news_cmd1.get_news(),         "Expected no changes to client, we should be in sync");
   /* client side */ BOOST_CHECK_MESSAGE( !cmd1.do_sync( server_reply ), "Expected no changes to client, we should be in sync");
}

BOOST_AUTO_TEST_SUITE_END()
