//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
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

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "MyDefsFixture.hpp"
#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "System.hpp"
#include "PrintStyle.hpp"
#include "ChangeMgrSingleton.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

static defs_ptr create_defs()
{
   defs_ptr theDefs = Defs::create();
   suite_ptr suite = theDefs->add_suite( "s1" ) ;
   family_ptr f1  = suite->add_family( "f1" ) ;
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( TimeAttr(10,30));
   t1->add_alias_only();
   task_ptr t2 = f1->add_task("t2");
   t2->add_alias_only();
   return theDefs;
}

BOOST_AUTO_TEST_CASE( test_force_cmd )
{
   cout << "Base:: ...test_force_cmd\n";
   defs_ptr the_defs = create_defs();
   the_defs->beginAll();
   node_ptr s1 = the_defs->findAbsNode("/s1");
   node_ptr f1 = the_defs->findAbsNode("/s1/f1");
   node_ptr t1 = the_defs->findAbsNode("/s1/f1/t1");
   node_ptr t2 = the_defs->findAbsNode("/s1/f1/t2");

   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::COMPLETE);
   BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
   BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");
   BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");

   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t2->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t2,NState::COMPLETE);
   BOOST_CHECK_MESSAGE(!t2->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set since there are NO time depedencies");
   BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");
   BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be NOT set");


   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(s1->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(s1,NState::COMPLETE);
   TestHelper::test_state(f1,NState::COMPLETE);
   TestHelper::test_state(t1,NState::COMPLETE);

   int clear_suspended_in_child_nodes = 0;
   s1->requeue(true,clear_suspended_in_child_nodes);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");
   BOOST_CHECK_MESSAGE(!t2->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");
   BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");
   BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear after requeue");

   node_ptr alias = the_defs->findAbsNode("/s1/f1/t1/alias0");
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(alias->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(alias,NState::COMPLETE);
}


static void doForce(MockServer& mockServer,
         Node* node,
         const std::string& stateOrEvent,
         const std::vector<Node*>& nodes)
{
   ForceCmd cmd(node->absNodePath(), stateOrEvent, true /*recursive */, true /* set Repeat to last value */);
   cmd.setup_user_authentification();
   STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
   BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to force for node " << node->debugNodePath());

   for(size_t i = 0; i < nodes.size(); i++) {
      if (NState::isValid(stateOrEvent)) {
         NState::State state = NState::toState(stateOrEvent);

         // Force Cmd recursive does **NOT** apply to aliases.
         if (nodes[i]->isAlias()) {
            // The alias should still be in default QUEUED state
            BOOST_CHECK_MESSAGE( nodes[i]->state() == NState::QUEUED, "Expected state NState::QUEUED for alias but found " << NState::toString(nodes[i]->state()) << " for alias " << nodes[i]->debugNodePath());
         }
         else {
            BOOST_CHECK_MESSAGE( nodes[i]->state() == state, "Expected state " << NState::toString(state) << " but found " << NState::toString(nodes[i]->state()) << " for node " << nodes[i]->debugNodePath());
         }
      }
      else  BOOST_CHECK_MESSAGE(false, "oops");

      if (!(nodes[i]->repeat().empty())) {
         BOOST_CHECK_MESSAGE( !nodes[i]->repeat().valid(), "Expected repeat to be set to last value. ie in valid");
      }
   }
}

BOOST_AUTO_TEST_CASE( test_force_cmd_recursive )
{
   cout << "Base:: ...test_force_cmd_recursive\n";

   defs_ptr the_defs = create_defs();
   node_ptr suite = the_defs->findAbsNode("/s1");
   std::vector<Node*> nodes;
   suite->getAllNodes(nodes);

   MockServer mockServer(the_defs);
   std::vector< std::string > all_states = NState::allStates();
   BOOST_FOREACH(const std::string& state, all_states) {
      doForce(mockServer,suite.get(),state,nodes);
   }
}

BOOST_AUTO_TEST_CASE( test_force_cmd_bubbles_up_state_changes )
{
   cout << "Base:: ...test_force_cmd_bubbles_up_state_changes\n";

   defs_ptr the_defs = create_defs();
   std::vector<Node*> nodes;
   std::vector<Task*> tasks;
   the_defs->getAllNodes(nodes);
   the_defs->getAllTasks(tasks);
   node_ptr suite = the_defs->findAbsNode("/s1");

   MockServer mockServer(the_defs);

   std::vector< std::string > all_states = NState::allStates();
   BOOST_FOREACH(const std::string& state, all_states) {

      // 		cout << "Setting all tasks to state " << state << "\n";
      BOOST_FOREACH(Task* task, tasks) {
         ForceCmd cmd(task->absNodePath(), state, false /*recursive */, false /* set Repeat to last value */);
         cmd.setup_user_authentification();
         STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
         BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to force for node " << task->debugNodePath());
      }

      // Check that state change set on task has bubbled up to the suite.
      // Since the state has been set on *all* tasks
      // 		cout << "Suite state = " << NState::toString(suite->state()) << "\n";
      NState::State expected_state = NState::toState(state);
      BOOST_CHECK_MESSAGE( suite->state() == expected_state, "Expected state " << NState::toString(expected_state) << " but found " << NState::toString(suite->state()) << " for suite " << suite->debugNodePath());
   }
}

BOOST_AUTO_TEST_CASE( test_force_cmd_alias_does_not_bubble_up_state_changes )
{
   cout << "Base:: ...test_force_cmd_alias_does_not_bubble_up_state_changes\n";

   defs_ptr the_defs = create_defs();
   std::vector<Node*> nodes;
   std::vector<alias_ptr> aliases;
   the_defs->getAllNodes(nodes);
   the_defs->get_all_aliases(aliases);
   node_ptr suite = the_defs->findAbsNode("/s1");

   // initialize by setting all nodes to state QUEUED
   BOOST_FOREACH(Node* n, nodes) { n->set_state(NState::QUEUED); }

   MockServer mockServer(the_defs);
   std::vector< std::string > all_states = NState::allStates();
   BOOST_FOREACH(const std::string& state, all_states) {

      BOOST_FOREACH(alias_ptr alias, aliases) {
         ForceCmd cmd(alias->absNodePath(), state, false /*recursive */, false /* set Repeat to last value */);
         cmd.setup_user_authentification();
         STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
         BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to force for alias " << alias->debugNodePath());
      }

      // Check that state change set on alias has *NOT* bubbled up to the suite.
      BOOST_CHECK_MESSAGE( suite->state() == NState::QUEUED, "Expected suite to have state QUEUED but found " << NState::toString(suite->state()));
   }
}


BOOST_AUTO_TEST_CASE( test_force_events )
{
   cout << "Base:: ...test_force_events\n";

   MyDefsFixture fixtureDef;
   MockServer mockServer(&fixtureDef.defsfile_);

   node_ptr suite = fixtureDef.fixtureDefsFile().findAbsNode("/suiteName");
   BOOST_REQUIRE_MESSAGE( suite.get(), "Could not find suite");
   std::vector<Node*> nodes;
   suite->getAllNodes(nodes);

   /// Set and clear events
   BOOST_FOREACH(Node* node, nodes) {
      BOOST_FOREACH(const Event& e, node->events()) {
         std::string path = node->absNodePath() + ":" + e.name_or_number();
         ForceCmd cmd(path, Event::SET(), false /*recursive */, false /* set Repeat to last value */);
         cmd.setup_user_authentification();
         STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
         BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to force event for node " << node->debugNodePath());
      }
      BOOST_FOREACH(const Event& e, node->events()) {
         BOOST_CHECK_MESSAGE(e.value(), "Event not set as expected for node " << node->debugNodePath());
      }
   }
   BOOST_FOREACH(Node* node, nodes) {
      BOOST_FOREACH(const Event& e, node->events()) {
         std::string path = node->absNodePath() + ":" + e.name_or_number();
         ForceCmd cmd(path, Event::CLEAR(), false /*recursive */, false /* set Repeat to last value */);
         cmd.setup_user_authentification();
         STC_Cmd_ptr returnCmd  = cmd.handleRequest( &mockServer );
         BOOST_REQUIRE_MESSAGE(returnCmd->ok(),"Failed to force event for node " << node->debugNodePath());
      }
      BOOST_FOREACH(const Event& e, node->events()) {
         BOOST_CHECK_MESSAGE(!e.value(), "Event not cleared as expected for node " << node->debugNodePath());
      }
   }
}


BOOST_AUTO_TEST_CASE( test_force_interactive )
{
   // This test is custom. When the user interactively forces a node to the
   // complete state, and that node has pending time activities. The default
   // behaviour, is the node is re-queued again, and hence the propagation up the
   // node tree does not happen. This is test checks that the node does *not* exhibit
   // this functionality. What we want is that task is set to complete, without
   // forcing a re-queue, this is then propagated up the node tree. Which forces the
   // family to complete, and hence update the repeat variable.
   cout << "Base:: ...test_force_interactive\n";

   //   suite s1
   //     family daily
   //       repeat date YMD 20101215 20101217 1
   //       task t1
   //         time 11:30
   //     endfamily
   //   endsuite
   // make sure time is set *before* 11:30, so that time dependency holds the task
   defs_ptr the_defs = Defs::create();
   suite_ptr suite = Suite::create( "s1" ) ;
   ClockAttr clockAttr(15,12,2010,false);
   clockAttr.set_gain(1/*hour*/,0/*minutes*/);
   suite->addClock( clockAttr );
   family_ptr f1  = Family::create( "daily" ) ;
   f1->addRepeat( RepeatDate("date", 20101215, 20101217) );
   task_ptr t1 = Task::create("t1" );
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(11,30)) );
   f1->addTask( t1 );
   suite->addFamily( f1 );
   the_defs->addSuite( suite );

   // before test flags should be clear
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");
   BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, before test");
   BOOST_CHECK_MESSAGE(!suite->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, before test");

   /// begin the suite
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new BeginCmd("s1",false)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_REQUIRE_MESSAGE(f1->repeat().value() == 20101215,"Repeat value expected is 20101215 but found " << f1->repeat().value());


   // Force the task t1 to complete state.
   // Since task t1 is complete, the family 'daily' should complete.
   // This will cause the repeat to take the next value anf forcing a requeue
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_REQUIRE_MESSAGE(f1->repeat().value() == 20101216,"Repeat value expected is 20101216 but found " << f1->repeat().value());

   // Reque should mean flag was cleared + on suite should never get set, since flag is stopped at repeat
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, since task was REQUED");
   BOOST_CHECK_MESSAGE(!f1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, since family was REQUED");
   BOOST_CHECK_MESSAGE(!suite->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear, since repeat should stop flag propagation up node tree");

   // Force the task t1 to complete again
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_REQUIRE_MESSAGE(f1->repeat().value() == 20101217,"Repeat value expected is 20101217 but found " << f1->repeat().value());

   // Do for the last time,
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::COMPLETE);
   TestHelper::test_state(f1,NState::COMPLETE);
   TestHelper::test_state(suite,NState::COMPLETE);

   // Flag propagation should stop at the repeat
   BOOST_CHECK_MESSAGE(!suite->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP *NOT* to be set");
}

BOOST_AUTO_TEST_CASE( test_force_interactive_next_time_slot )
{
   // This test is custom. When the user interactively forces a node to the complete state,
   // But where the user has a time range. In this case the node should complete and then
   // requee and miss the next time slot. If this is repeated, eventually we should reach the
   // end of the time slot. In which case the node should *not* reque and stay complete
   //
   // When the node is then requeed check that the next time slot has been correctly reset.
   cout << "Base:: ...test_force_interactive_next_time_slot\n";

   //   suite s1
   //       task t1
   //          time 10:00 14:00 01:00
   //   endsuite
   // make sure time is set *before* 11:30, so that time dependency holds the task
   Defs the_defs;
   suite_ptr suite = the_defs.add_suite("s1");
   task_ptr t1 = suite->add_task("t1");
   t1->addTime( TimeAttr(TimeSlot(10,0),TimeSlot(14,0),TimeSlot(1,0)) );
   ClockAttr clockAttr(15,12,2010,false);
   clockAttr.set_gain(10/*hour*/,30/*minutes*/); // start at 10:30
   suite->addClock( clockAttr );

   // before test flags should be clear
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

   /// begin the suite
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new BeginCmd("s1",false)));
   TestHelper::test_state(t1,NState::QUEUED);
//   PrintStyle::setStyle(PrintStyle::STATE);
//   cout << the_defs << "\n";

   // since we started at 10:30 the next time slot should be 11:00
   const TimeSlot& next_time_slot  = t1->timeVec().back().time_series().get_next_time_slot();
   BOOST_CHECK_MESSAGE( next_time_slot == TimeSlot(11,0),"Expected next time slot of 11:00 but found " << next_time_slot.toString());

   // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
   // It should also advance the next time slot
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( next_time_slot == TimeSlot(12,0),"Expected next time slot of 12:00 but found " << next_time_slot.toString());

   // Repeat, to make sure next_time_slot is advanced
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( next_time_slot == TimeSlot(13,0),"Expected next time slot of 13:00 but found " << next_time_slot.toString());

   // Repeat, to make sure next_time_slot is advanced
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( next_time_slot == TimeSlot(14,0),"Expected next time slot of 14:00 but found " << next_time_slot.toString());

   // Repeat, ** THIS time we have *exceeded* the time range, it should no longer requeue, and should stay complete
   //         Additionally since there is no reque we expect NO_REQUE_IF_SINGLE_TIME_DEP to remain set
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::COMPLETE);
   BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
   BOOST_CHECK_MESSAGE( next_time_slot == TimeSlot(15,0),"Expected next time slot of 15:00 but found " << next_time_slot.toString());

   /// we will now Re-queue, Since the time is still 10:30, we expect next_time slot to be 11:00 and not 10:00
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new RequeueNodeCmd(t1->absNodePath())));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( next_time_slot == TimeSlot(11,0),"After Re-queue, Expected next time slot of 11:00 but found " << next_time_slot.toString());
}

BOOST_AUTO_TEST_CASE( test_force_interactive_next_time_slot_2 )
{
   // This test is custom. When the user interactively forces a node to the complete state,
   // But where the user set of time slots. In this case the node should complete and then
   // requee and miss the next time. If this is repeated, eventually we should reach the
   // end of the time slot. In which case the node should *not* re-queue and stay complete
   //
   // When the node is then re-queued check that the time has been correctly reset.
   cout << "Base:: ...test_force_interactive_next_time_slot_2\n";

   //   suite s1
   //       task t1
   //          time 10:00
   //          time 11:00
   //          time 12:00
   //          time 13:00
   //   endsuite
   Defs the_defs;
   suite_ptr suite = the_defs.add_suite("s1");
   task_ptr t1 = suite->add_task("t1");
   t1->addTime( TimeAttr(10,0) );
   t1->addTime( TimeAttr(11,0) );
   t1->addTime( TimeAttr(12,0) );
   t1->addTime( TimeAttr(13,0) );
   ClockAttr clockAttr(15,12,2010,false);
   clockAttr.set_gain(10/*hour*/,30/*minutes*/); // *start* at 10:30
   suite->addClock( clockAttr );

   // before test flags should be clear
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

   /// begin the suite
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new BeginCmd("s1",false)));
   TestHelper::test_state(t1,NState::QUEUED);
//   PrintStyle::setStyle(PrintStyle::STATE);
//   cout << the_defs << "\n";

   // get all the time attributes
   const TimeSeries& ts_10 = t1->timeVec()[0].time_series();
   const TimeSeries& ts_11 = t1->timeVec()[1].time_series();
   const TimeSeries& ts_12 = t1->timeVec()[2].time_series();
   const TimeSeries& ts_13 = t1->timeVec()[3].time_series();
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be in-valid since we started clock at 10:30 ");
   BOOST_CHECK_MESSAGE( ts_11.is_valid(),  "Expected time 11 to be valid since we started at 10:30");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid since we started at 10:30");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid since we started at 10:30");

   const TimeSlot& time_10 = t1->timeVec()[0].time_series().get_next_time_slot();
   const TimeSlot& time_11 = t1->timeVec()[1].time_series().get_next_time_slot();
   const TimeSlot& time_12 = t1->timeVec()[2].time_series().get_next_time_slot();
   const TimeSlot& time_13 = t1->timeVec()[3].time_series().get_next_time_slot();
   BOOST_CHECK_MESSAGE( time_10 == TimeSlot(10,0),"Expected next time slot of 10:00 but found " << time_10.toString());
   BOOST_CHECK_MESSAGE( time_11 == TimeSlot(11,0),"Expected next time slot of 11:00 but found " << time_11.toString());
   BOOST_CHECK_MESSAGE( time_12 == TimeSlot(12,0),"Expected next time slot of 12:00 but found " << time_12.toString());
   BOOST_CHECK_MESSAGE( time_13 == TimeSlot(13,0),"Expected next time slot of 13:00 but found " << time_13.toString());


   // Force the task t1 to complete state. Since we have a future time dependency, we should get re-queued again
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be in-valid since we started clock at 10:30 ");
   BOOST_CHECK_MESSAGE( !ts_11.is_valid(),  "Expected time 11 to be valid");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid");

   // Repeat
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be in-valid due to force cmd");
   BOOST_CHECK_MESSAGE( !ts_11.is_valid(), "Expected time 11 to be in-valid due to force cmd");
   BOOST_CHECK_MESSAGE( !ts_12.is_valid(), "Expected time 12 to be in-valid due to force cmd");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid");

   // Repeat *last* time, since all times have expired, we expect task to complete.
   // Addtionally since we have *not* re-queued the flag NO_REQUE_IF_SINGLE_TIME_DEP should have remained set
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::COMPLETE);
   BOOST_CHECK_MESSAGE( t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be in-valid due to force cmd");
   BOOST_CHECK_MESSAGE( !ts_11.is_valid(), "Expected time 11 to be in-valid due to force cmd");
   BOOST_CHECK_MESSAGE( !ts_12.is_valid(), "Expected time 12 to be in-valid due to force cmd");
   BOOST_CHECK_MESSAGE( !ts_13.is_valid(), "Expected time 13 to be in-valid due to force cmd");

   /// we will now Re-queue, Since the time is still 10:30, we expect valid from 11:00 and not 10:00
   /// We should also have cleared NO_REQUE_IF_SINGLE_TIME_DEP
   TestHelper::invokeRequest(&the_defs,Cmd_ptr( new RequeueNodeCmd(t1->absNodePath())));
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be in valid since we started clock at 10:30 ");
   BOOST_CHECK_MESSAGE( ts_11.is_valid(),  "Expected time 11 to be valid");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid");

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
   ChangeMgrSingleton::destroy();
}
BOOST_AUTO_TEST_SUITE_END()
