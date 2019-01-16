//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"

#include "ClientToServerCmd.hpp"
#include "TestHelper.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "CalendarUpdateParams.hpp"
#include "Pid.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;


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

BOOST_AUTO_TEST_CASE( test_ecflow_359 )
{
   cout << "Base:: ...test_ECFLOW-359\n";

   //   suite s1
   //     family f1
   //       repeat date YMD 20090916 20090928
   //       family parent
   //          task dummy
   //             time 16:00
   //       endfamily
   //       task complete
   //          defstatus complete

   //   set defstatus complete /s1/f1/parent/dummy
   //   requeue s1/f1/parent
   //   f1 gets set complete through status inheritance
   //   YMD is not increment at the moment...

   defs_ptr the_defs = Defs::create();
   suite_ptr suite = the_defs->add_suite( "s1" ) ;
   family_ptr f1 = suite->add_family("f1");
   f1->addRepeat( RepeatDate("YMD",20090916,20090928,1) );

   family_ptr parent  = f1->add_family( "parent" ) ;
   task_ptr dummy = parent->add_task("dummy");
   dummy->addTime( TimeAttr(16,0));

   task_ptr complete = f1->add_task("complete");
   complete->addDefStatus( DState::COMPLETE);

//   PrintStyle style(PrintStyle::STATE); cout << the_defs;

   the_defs->beginAll();

//   cout << "----->set defstatus complete /s1/f1/parent/dummy\n";
   BOOST_CHECK_MESSAGE( dummy->defStatus() == DState::QUEUED, "Expected dstate to be queued but found " << DState::toString( dummy->dstate()));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new AlterCmd(dummy->absNodePath(),AlterCmd::DEFSTATUS,"complete")));
   BOOST_CHECK_MESSAGE( dummy->defStatus() == DState::COMPLETE, "Expected dstate to be complete but found " << DState::toString( dummy->dstate()));
//   cout << the_defs;

//   cout << "----->requeue parent since all children are now complete, we expect the repeat on the f1 to increment\n";
   BOOST_CHECK_MESSAGE( f1->repeat().value() ==  20090916, "Expected repeat value of 20090916 but found " << f1->repeat().value());
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(parent->absNodePath())));
   BOOST_CHECK_MESSAGE( f1->repeat().value() ==  20090917, "Expected repeat value of 20090917 but found " << f1->repeat().value());
//   cout << the_defs;

//   cout << "----->requeue again\n";
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(parent->absNodePath())));
   BOOST_CHECK_MESSAGE( f1->repeat().value() ==  20090918, "Expected repeat value of 20090918 but found " << f1->repeat().value());
//   cout << the_defs;
}


BOOST_AUTO_TEST_CASE( test_ecflow_428 )
{
   cout << "Base:: ...test_ECFLOW-428\n";
   //   suite s1
   //     family f1
   //       family f2
   //          task t1
   //          task t2

   // Set t1 to aborted, then call reque aborted on suite s1
   // This should result ALL nodes to be in a queued state

   defs_ptr the_defs = Defs::create();
   suite_ptr suite = the_defs->add_suite( "s1" ) ;
   suite->addDefStatus(DState::SUSPENDED);
   family_ptr f1 = suite->add_family("f1");
   family_ptr f2 = f1->add_family("f1");
   task_ptr t1 = f2->add_task("t1");
   task_ptr t2 = f2->add_task("t2");

//   PrintStyle style(PrintStyle::STATE); cout << the_defs;

   the_defs->beginAll();

   // set t1 to aborted state
   TestHelper::invokeRequest(the_defs.get(),
                             Cmd_ptr( new ForceCmd(t1->absNodePath(), "aborted", false /*recursive */, false /* set Repeat to last value */)));
   TestHelper::test_state(t1,NState::ABORTED);
   TestHelper::test_state(t2,NState::QUEUED);
   TestHelper::test_state(f1,NState::ABORTED);
   TestHelper::test_state(f2,NState::ABORTED);
   TestHelper::test_state(suite,NState::ABORTED);

   // Now reque aborted tasks for suite
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(suite->absNodePath(), RequeueNodeCmd::ABORT)));

   // *Suite* should now be queued
   TestHelper::test_state(suite,NState::QUEUED);
   TestHelper::test_state(f1,NState::QUEUED);
   TestHelper::test_state(f2,NState::QUEUED);
   TestHelper::test_state(t1,NState::QUEUED);
   TestHelper::test_state(t2,NState::QUEUED);
}


BOOST_AUTO_TEST_CASE( test_repeat_based_requeue_resets_relative_duration )
{
   cout << "Base:: ...test_repeat_based_requeue_resets_relative_duration\n";

   //   suite ecflow_1182
   //    family f
   //      repeat integer HYEAR 1993 2017 1
   //      time +00:01
   //      task uk2fdb

   defs_ptr the_defs = Defs::create();
   suite_ptr suite = the_defs->add_suite( "ecflow_1182" ) ;
   family_ptr f = suite->add_family("f");
   f->addRepeat( RepeatInteger("rep",1993,2017) );
   f->addTime( TimeAttr("+00:01"));

   task_ptr t1 = f->add_task("uk2fsb");

   TimeSeries& theTime = const_cast<TimeSeries&>(f->timeVec().back().time_series());

   the_defs->beginAll();
   BOOST_CHECK_MESSAGE(theTime.is_valid(), "Expected time to be holding");
   
   // forward time, so that time expires
   // The calendar is *only* updated if the suite have been begun. Hence make sure this test scaffold
   // starts the test, with all the suites in a begun state
   boost::posix_time::ptime time_now = Calendar::second_clock_time();
   {
      CalendarUpdateParams cal( time_now , minutes(1), true /* server running */, false/* for Test*/ );
      the_defs->updateCalendar(cal); //cout << suite->calendar().toString() << "\n";
      time_now += minutes(1);
   }
   {
      CalendarUpdateParams cal( time_now , minutes(1), true /* server running */, false/* for Test*/ );
      the_defs->updateCalendar(cal); // cout << suite->calendar().toString() << "\n";
   }
   theTime.requeue(suite->calendar(),true); //will expire time
   BOOST_CHECK_MESSAGE(!theTime.is_valid(), "Expected time to be expired");

   // Now re-queue the suite/family
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(suite->absNodePath())));
   BOOST_CHECK_MESSAGE(theTime.is_valid(), "Expected time to be valid");

   //PrintStyle style(PrintStyle::MIGRATE);
   //cout << the_defs;
}

BOOST_AUTO_TEST_CASE( test_reque_with_repeat_and_defstatus_complete )
{
   cout << "Base:: ...test_reque_with_repeat_and_defstatus_complete\n";
   // This will test that when we have a family with a repeat AND defstatus complete
   // We ONLY log the state change complete in the log file when re-queuing

   //   suite test_reque_with_repeat_and_defstatus_complete
   //     family f1
   //       repeat date YMD 20000101 20100101 1
   //       task t1
   //       task t2
   ///      defstatus complete

   defs_ptr the_defs = Defs::create();
   suite_ptr suite = the_defs->add_suite( "test_reque_with_repeat_and_defstatus_complete" ) ;
   family_ptr f1 = suite->add_family("f1");
   f1->addRepeat(RepeatDate("YMD",20090531,20101231,1));
   f1->addDefStatus(DState::COMPLETE);
   task_ptr t1 = f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");

   //PrintStyle style(PrintStyle::STATE); cout << the_defs;

   the_defs->beginAll();

   // Create a log file with a unique name, to avoid problems when running in paralle
   // This test relies on log file contents to be flushed.
   std::string log_file = "Base/test/test_reque_with_repeat_and_defstatus_complete_";
   log_file += Pid::getpid(); // can throw
   log_file += ".log";
   log_file = File::test_data(log_file,"Base");

   Log::create(log_file);

   // Re-queue the family. In past we queued all nodes, then set to complete
   // We still do this, but we now longer LOG the setting the the queued state
   // Hence in the log file we only expected to see nodes in the complete state
   // See: ECFLOW-1239. When dealing with thousands of nodes, this was causing performance problems
   TestHelper::invokeRequest(the_defs.get(), Cmd_ptr( new RequeueNodeCmd(f1->absNodePath())));
   TestHelper::test_state(t1,NState::COMPLETE);
   TestHelper::test_state(t2,NState::COMPLETE );
   TestHelper::test_state(f1,NState::COMPLETE);
   TestHelper::test_state(suite,NState::COMPLETE);

   // This should also flush the log file.
   Log::destroy();

   // Open the log file and look for 'queued:' if we find it then this is a regression.
   std::string error_msg;
   std::string log_contents = File::get_first_n_lines(log_file,1000, error_msg);
   BOOST_REQUIRE_MESSAGE(error_msg.empty(), "could not open log file " << log_file);
   BOOST_CHECK_MESSAGE(log_contents.find("complete:") != std::string::npos,"defstatus complete failed.\n'" << log_contents << "'");
   BOOST_CHECK_MESSAGE(log_contents.find("queued:") == std::string::npos,"RequeNode with repeat and defstatus complete should only log the complete state change");

   fs::remove(log_file);
}

BOOST_AUTO_TEST_SUITE_END()
