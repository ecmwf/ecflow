//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $
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
#include "TestHelper.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_requeue_with_suspend )
{
   cout << "Base:: ...test_requeue_with_suspend\n";

   defs_ptr the_defs = Defs::create();
   suite_ptr s1 = the_defs->add_suite( "s1" ) ;
   family_ptr f1  = s1->add_family( "f1" ) ;
   task_ptr t1 = f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");
   task_ptr t3 = f1->add_task("t3");
   t3->addDefStatus(DState::SUSPENDED);

   the_defs->beginAll();

   // After begin/requeue we must honour defs status
   BOOST_CHECK_MESSAGE(t3->isSuspended(),"Expected node to be suspended");

   // Suspend all nodes
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,s1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,f1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,t1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,t2->absNodePath())));
   BOOST_CHECK_MESSAGE(s1->isSuspended(),"Expected node to be suspended");
   BOOST_CHECK_MESSAGE(f1->isSuspended(),"Expected node to be suspended");
   BOOST_CHECK_MESSAGE(t1->isSuspended(),"Expected node to be suspended");
   BOOST_CHECK_MESSAGE(t2->isSuspended(),"Expected node to be suspended");

   // Re-queue of the nodes, that are suspended, they should *stay* suspended
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(t1->absNodePath())));
   BOOST_CHECK_MESSAGE(t1->isSuspended(),"Expected node to stay suspended");

   // Now re-queue the top level suite, this is suspended.
   // This should stay suspended *BUT* child nodes which are suspend should be cleared
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(s1->absNodePath())));
   BOOST_CHECK_MESSAGE(s1->isSuspended(),"Suite should stay suspend");
   BOOST_CHECK_MESSAGE(!f1->isSuspended(),"Expected child nodes to be un-suspended");
   BOOST_CHECK_MESSAGE(!t1->isSuspended(),"Expected child nodes to be un-suspended");
   BOOST_CHECK_MESSAGE(!t2->isSuspended(),"Expected child nodes to be un-suspended");
   BOOST_CHECK_MESSAGE(t3->isSuspended(), "Requeue must honour def status");
}

BOOST_AUTO_TEST_CASE( test_requeue_family_clears_children_SUP_909 )
{
   cout << "Base:: ...test_requeue_family_clears_children_SUP_909\n";

   //   suite s1
   //    family f1
   //       task t1
   //          time 23:30
   //     endFamily
   //   endsuite
   // make sure time is set *before* 23:30, so that time dependency holds the task

   defs_ptr the_defs = Defs::create();
   suite_ptr suite = the_defs->add_suite( "s1" ) ;
   ClockAttr clockAttr(15,12,2010,false);
   clockAttr.set_gain(9/*hour*/,30/*minutes*/); // start at 09:30
   suite->addClock( clockAttr );

   family_ptr f1  = suite->add_family( "f1" ) ;
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( TimeAttr(23,30));

   const TimeSeries& theTime = t1->timeVec().back().time_series();

   the_defs->beginAll();

   BOOST_CHECK_MESSAGE(theTime.is_valid(), "Expected time to be holding");
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::COMPLETE);
   BOOST_CHECK_MESSAGE( !theTime.is_valid(), "Expected time to have expired");
   BOOST_CHECK_MESSAGE( t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set");

   // Now reque the family, this should clear the time, so that it now holding again
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(f1->absNodePath())));
   TestHelper::test_state(f1,NState::QUEUED);
   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE( theTime.is_valid(), "Expected time to be reset");
   BOOST_CHECK_MESSAGE( !t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
}

BOOST_AUTO_TEST_CASE( test_repeat_based_requeue_clears_children )
{
   cout << "Base:: ...test_repeat_based_requeue_clears_children\n";

   //   suite s1
   //    repeat day 1
   //    family f1
   //       task t1
   //          time 23:30
   //     endfamily
   //   endsuite
   // make sure time is set *before* 23:30, so that time dependency holds the task

   defs_ptr the_defs = Defs::create();
   suite_ptr suite = the_defs->add_suite( "s1" ) ;
   ClockAttr clockAttr(15,12,2010,false);
   clockAttr.set_gain(9/*hour*/,30/*minutes*/); // start at 09:30
   suite->addClock( clockAttr );

   suite->addRepeat( RepeatDay(1) );

   family_ptr f1  = suite->add_family( "f1" ) ;
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( TimeAttr(23,30));

   const TimeSeries& theTime = t1->timeVec().back().time_series();

   the_defs->beginAll();

   BOOST_CHECK_MESSAGE(theTime.is_valid(), "Expected time to be holding");

   // Forcing task t1 to complete, should cause the top, level repeat to REQEUE
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new ForceCmd(t1->absNodePath(),"complete",true /*recursive */, false /* set Repeat to last value */)));

   TestHelper::test_state(t1,NState::QUEUED);
   BOOST_CHECK_MESSAGE( theTime.is_valid(), "Expected time to be holding");
   BOOST_CHECK_MESSAGE( !t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear");
}


BOOST_AUTO_TEST_SUITE_END()
