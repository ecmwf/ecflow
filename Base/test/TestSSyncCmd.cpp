//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #40 $ 
//
// Copyright 2009-2017 ECMWF.
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
#include "CalendarUpdateParams.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;


BOOST_AUTO_TEST_SUITE( BaseTestSuite )

/// define a function which returns nothing, and takes a defs_ptr parameter
typedef boost::function<void (defs_ptr)> defs_change_cmd;

/// Re-use the same test scaffold to modify and then resync, by passing
/// in a function that will modify the defs
static void test_sync_scaffold(
         defs_change_cmd the_defs_change_command,
         const std::string& test_name,
         bool full_sync = false,
         bool start_with_begin = false)
{
	MyDefsFixture clientFixture;
	MyDefsFixture serverFixture;
	defs_ptr server_defs = serverFixture.create_defs();
	if ( start_with_begin ) server_defs->beginAll();
	server_defs->set_server().set_state( SState::HALTED);  // if defs default state is RUNNING, whereas for server it is HALTED

 	ServerReply server_reply;
   defs_ptr client_defs = clientFixture.create_defs();
   if ( start_with_begin ) client_defs->beginAll();
   client_defs->set_server().set_state( SState::HALTED); // if defs default state is RUNNING, whereas for server it is HALTED
	server_reply.set_client_defs(  client_defs );


   Ecf::set_debug_equality(true); // only has affect in DEBUG build
	BOOST_CHECK_MESSAGE( *server_defs == *server_reply.client_defs(),"Test:" << test_name << ": Starting point client and server defs should be the same");
   Ecf::set_debug_equality(false); // only has affect in DEBUG build

	// Get change number before any changes
	unsigned int client_state_change_no = Ecf::state_change_no();
	unsigned int client_modify_change_no = Ecf::modify_change_no();

	// make some change to the server
	{
		Ecf::set_server(true);

		the_defs_change_command(server_defs);

      std::string error_msg;
      BOOST_REQUIRE_MESSAGE( server_defs->checkInvariants(error_msg),"Test:" << test_name << ": Invariants failed: " << error_msg);
      BOOST_REQUIRE_MESSAGE( !(*server_reply.client_defs() == *server_defs),"Test:" << test_name << ": Expected client and server defs to differ\n" << *server_reply.client_defs() << "\n" << "server defs   = " << *server_defs);
		Ecf::set_server(false);
	}

	MockServer mock_server(server_defs);
	unsigned int client_handle = 0;
   SNewsCmd news_cmd(client_handle, client_state_change_no,  client_modify_change_no, &mock_server );
   SSyncCmd cmd(client_handle, client_state_change_no,  client_modify_change_no, &mock_server );

   std::string error_msg;
   BOOST_REQUIRE_MESSAGE( server_defs->checkInvariants(error_msg),"Test:" << test_name << ": Invariants failed: " << error_msg);
   BOOST_CHECK_MESSAGE( news_cmd.get_news(), "Test:" << test_name << ": Expected server to change");
   BOOST_CHECK_MESSAGE( cmd.do_sync( server_reply ),"Test:" << test_name << ": Expected server to change");
	BOOST_CHECK_MESSAGE( server_reply.in_sync(),     "Test:" << test_name << ": Expected server to change");
	BOOST_CHECK_MESSAGE( server_reply.full_sync() == full_sync,"Test:" << test_name << ": Expected sync not as expected");

	error_msg.clear();
   BOOST_REQUIRE_MESSAGE( server_reply.client_defs()->checkInvariants(error_msg),"Test:" << test_name << ": Invariants failed: " << error_msg);

   DebugEquality debug_equality;  // only has affect in DEBUG build
	BOOST_CHECK_MESSAGE( *server_defs == *server_reply.client_defs(),"Test:" << test_name << ": Server and client should be same after sync" );
//	if (! (*server_defs == *server_reply.client_defs()) ) {
//	   cout << "Server====================================================================\n";
//	   cout << server_defs;
//    cout << "Client====================================================================\n";
//    cout << server_reply.client_defs();
//	}
}

// The modifiers
void delete_some_attributes(defs_ptr defs)
{
	std::vector<Task*> tasks;
	defs->getAllTasks(tasks);
	BOOST_FOREACH(Task* task, tasks) {

		SuiteChanged1 changed(task->suite());

		/// Take a copy, of the objects we want to delete. since there are returned by reference
		std::vector<Event> events = task->events();
		std::vector<Meter> meters = task->meters();
		std::vector<Label> labels = task->labels();

		BOOST_FOREACH(const Event& e, events) { task->deleteEvent( e.name_or_number() );}
		BOOST_FOREACH(const Meter& m, meters) { task->deleteMeter( m.name() ); }
		BOOST_FOREACH(const Label& l, labels) { task->deleteLabel( l.name() ); }

		BOOST_REQUIRE_MESSAGE( task->events().empty(),"Expected all events to be deleted");
		BOOST_REQUIRE_MESSAGE( task->meters().empty(),"Expected all meters to be deleted");
		BOOST_REQUIRE_MESSAGE( task->labels().empty(),"Expected all labels to be deleted");
	}
}

void add_some_attributes(defs_ptr defs) {
	std::vector<task_ptr> tasks;
	defs->get_all_tasks(tasks);
	BOOST_FOREACH(task_ptr task, tasks) { SuiteChanged1 changed(task->suite()); task->addDay( DayAttr(DayAttr::TUESDAY) );}
}

void begin(defs_ptr defs) { defs->beginAll();} // reset all attributes

void add_alias(defs_ptr defs) {

   std::vector<task_ptr> tasks;
   defs->get_all_tasks(tasks);
   BOOST_REQUIRE_MESSAGE( !tasks.empty(), "Expected at least one task");

   SuiteChanged1 changed(tasks[0]->suite());
   tasks[0]->add_alias_only();
}

void remove_all_aliases(defs_ptr defs) {

   std::vector<alias_ptr> aliases;
   defs->get_all_aliases(aliases);
   BOOST_REQUIRE_MESSAGE( !aliases.empty(), "Expected at least one alias");

   BOOST_FOREACH(alias_ptr alias, aliases) {
      TestHelper::invokeRequest(defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::DELETE,alias->absNodePath())));
   }

   aliases.clear();
   defs->get_all_aliases(aliases);
   BOOST_REQUIRE_MESSAGE( aliases.empty(), "Expected at no  alias");
}

void remove_all_tasks(defs_ptr defs) {

	// Remove tasks should force a incremental sync
	std::vector<task_ptr> tasks;
	defs->get_all_tasks(tasks);
	BOOST_FOREACH(task_ptr task, tasks) { SuiteChanged1 changed(task->suite()); task->remove() ;}

	tasks.clear();
	defs->get_all_tasks(tasks);
	BOOST_REQUIRE_MESSAGE( tasks.empty(), "Failed to delete tasks");
}

void remove_a_family(defs_ptr defs) {

   // Remove tasks should force a incremental sync
   std::vector<Family*> vec;
   defs->getAllFamilies(vec);
   size_t family_size = vec.size();
   BOOST_REQUIRE_MESSAGE( !vec.empty(), "Expected at least one family");
   if (!vec.empty()) {
      SuiteChanged1 changed(vec[0]->suite());
      vec[0]->remove();
   }

   vec.clear();
   defs->getAllFamilies(vec);
   BOOST_REQUIRE_MESSAGE( vec.size() < family_size, "Failed to delete family");
}


void change_clock_gain(defs_ptr defs) {
  	BOOST_FOREACH(suite_ptr suite, defs->suiteVec()) {
		if (suite->clockAttr().get()) {
			SuiteChanged changed(suite);
 			suite->changeClockGain("100001");
		}
 	}
}
void change_clock_type_to_real(defs_ptr defs) {

   BOOST_FOREACH(suite_ptr suite, defs->suiteVec()) {
      if (suite->clockAttr().get()) {
         SuiteChanged changed(suite);
         suite->changeClockType("real");
      }
   }
}
void change_clock_type_to_hybrid(defs_ptr defs) {

   BOOST_FOREACH(suite_ptr suite, defs->suiteVec()) {
      if (suite->clockAttr().get()) {
         SuiteChanged changed(suite);
         suite->changeClockType("hybrid");
      }
   }
}
void change_clock_date(defs_ptr defs) {

   BOOST_FOREACH(suite_ptr suite, defs->suiteVec()) {
      if (suite->clockAttr().get()) {
         SuiteChanged changed(suite);
         suite->changeClockDate("1.1.2001");
      }
   }
}
void change_clock_sync(defs_ptr defs) {

   BOOST_FOREACH(suite_ptr suite, defs->suiteVec()) {
      if (suite->clockAttr().get()) {
         SuiteChanged changed(suite);
         suite->changeClockSync();
      }
   }
}


/// This has been split into two functions, as changing both together could mask an error
/// i.e found bug where we forgot to update state_change number when changing the limit
/// max value, however because we had, changed value as well it got masked.
void change_limit_max(defs_ptr defs) {

   BOOST_FOREACH(suite_ptr s, defs->suiteVec()) {
      std::vector<limit_ptr> theLimits =  s->limits();
      BOOST_FOREACH(limit_ptr l, theLimits) {
         //std::cout << "found " << l->toString() << "\n";
         TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LIMIT_MAX,l->name(),"90")));
         limit_ptr v = s->find_limit(l->name());
         BOOST_CHECK_MESSAGE( v.get() && v->theLimit() == 90, "expected to find limit with max value of 90");
      }
   }
}
void change_limit_value(defs_ptr defs) {

   BOOST_FOREACH(suite_ptr s, defs->suiteVec()) {
      std::vector<limit_ptr> theLimits =  s->limits();
      BOOST_FOREACH(limit_ptr l, theLimits) {
         TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LIMIT_VAL,l->name(),"33")));
         limit_ptr v = s->find_limit(l->name());
         BOOST_CHECK_MESSAGE( v.get() && v->value() == 33, "expected to find limit with value of 33");
      }
   }
}


void update_repeat(defs_ptr defs) {

   std::vector<Node*> nodes;
   defs->getAllNodes(nodes);

   BOOST_FOREACH(Node* n, nodes) {
      if (!n->repeat().empty()) {
         SuiteChanged1 changed(n->suite());
         n->increment_repeat();
      }
   }
}

void update_calendar(defs_ptr defs) {

   // The calendar is *only* updated if the suite have been begun. Hence make sure this test scaffold
   // starts the test, with all the suites in a begun state
   CalendarUpdateParams p( Calendar::second_clock_time(), minutes(1), true /* server running */, false/* for Test*/ );
   defs->updateCalendar(p);

   // Currently updating the calendar, does not cause change, Hence force a change
   BOOST_FOREACH(suite_ptr suite, defs->suiteVec()) {
      SuiteChanged changed(suite);
      suite->add_variable("name","value");
   }

   // Note: In the real server, persisting that calendar, the clock type is not persisted.
   //       i.e when we have hybrid calendar, when restored on the client side it will be 'real' clock since
   //       that is the default now. This is not correct and will fail invariant checking.
   //       however the memento should reset clock type on the calenadar form the clok attribute.
}


void delete_suite(defs_ptr defs) {
   std::vector<suite_ptr> vec =  defs->suiteVec();
   BOOST_REQUIRE_MESSAGE(!vec.empty(),"Expected suites");
   vec[0]->remove();
}

void set_server_state_shutdown(defs_ptr defs) {
   defs->set_server().set_state( SState::SHUTDOWN );
}
void set_server_state_running(defs_ptr defs) {
   defs->set_server().set_state( SState::RUNNING );
}

void add_server_variable(defs_ptr defs) {
   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"_fred_","value")));
}
void change_server_variable(defs_ptr defs) {
   // Because the scaffold create client/server defs each time.
   // To test change/delete variables we modify the default set
   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd("/",AlterCmd::VARIABLE,"ECF_TRIES","4")));
}
void delete_server_variable(defs_ptr defs) {
   // Because the scaffold create client/server defs each time.
   // To test change/delete variables we modify the default set
   // ***NOTE*** we can not delete server variables like ECF_TRIES, can only change them
   // However we can delete user variables added to the server
   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new AlterCmd("/",AlterCmd::DEL_VARIABLE,"MyDefsFixture_user_variable")));
}

void reorder_suites(defs_ptr defs) {
   std::vector<suite_ptr> suiteVec =  defs->suiteVec();
   BOOST_REQUIRE_MESSAGE(!suiteVec.empty(),"Expected suites");
   std::string path = "/" + suiteVec[0]->name();
   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new OrderNodeCmd(path,NOrder::ALPHA)));
}

void set_defs_flag(defs_ptr defs) {
   defs->flag().set(ecf::Flag::MESSAGE);
}

void set_defs_state(defs_ptr defs) {
   defs->set_state(NState::ABORTED);
}

BOOST_AUTO_TEST_CASE( test_ssync_cmd  )
{
	// To DEBUG: enable the defines in Memento.hpp
	cout << "Base:: ...test_ssync_cmd\n";
   test_sync_scaffold(update_repeat,"update_repeat");
   test_sync_scaffold(delete_some_attributes,"delete_some_attributes");
	test_sync_scaffold(add_some_attributes,"add_some_attributes");
   test_sync_scaffold(begin,"begin",true /* expect full_sync */);
   test_sync_scaffold(add_alias,"add_alias");
   test_sync_scaffold(remove_all_aliases,"remove_all_aliases");
   test_sync_scaffold(remove_all_tasks,"remove_all_tasks");
   test_sync_scaffold(remove_a_family,"remove_a_family");
   test_sync_scaffold(change_clock_gain,"change_clock_gain", true  /* expect full_sync */);
   test_sync_scaffold(change_clock_type_to_real,"change_clock_type_to_real", true  /* expect full_sync */);
   test_sync_scaffold(change_clock_type_to_hybrid,"change_clock_type_to_hybrid", true  /* expect full_sync */);
   test_sync_scaffold(change_clock_date,"change_clock_date", true  /* expect full_sync */);
   test_sync_scaffold(change_clock_sync,"change_clock_sync", true  /* expect full_sync */);
   test_sync_scaffold(update_calendar,"update_calendar", false/* expect full_sync */, true /* start test with begin */ );
   test_sync_scaffold(change_limit_max,"change_limit_max");
   test_sync_scaffold(change_limit_value,"change_limit_value");
   test_sync_scaffold(delete_suite,"delete_suite", true /* expect full_sync */);

   // Test Changes in Defs
   // The default server state is HALTED, hence setting to halted will not show a change
   test_sync_scaffold(set_server_state_shutdown,"set_server_state_shutdown");
   test_sync_scaffold(set_server_state_running,"set_server_state_running");

   test_sync_scaffold(add_server_variable,"add_server_variable");
   test_sync_scaffold(change_server_variable,"change_server_variable");
   test_sync_scaffold(delete_server_variable,"delete_server_variable");

   test_sync_scaffold(reorder_suites,"reorder_suites");

   test_sync_scaffold(set_defs_flag,"set_defs_flag");
   test_sync_scaffold(set_defs_state,"set_defs_state");
}

BOOST_AUTO_TEST_SUITE_END()


