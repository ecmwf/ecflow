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

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_node_begin_reque_hybrid )
{
   cout << "ANode:: ...test_node_begin_reque_hybrid\n";

   // Create a suite with a *HYBRID* clock, and tasks with day,date and cron time attributes
   defs_ptr the_defs = Defs::create();
   suite_ptr s1 = the_defs->add_suite( "s1" ) ;
   s1->addClock( ClockAttr(true) );
   family_ptr f1  = s1->add_family( "f1" ) ;

   CronAttr cron;
   std::vector<int> week_days; week_days.push_back(0);  week_days.push_back(1);
   cron.addWeekDays(week_days);
   cron.add_time_series(10,10,true);

   task_ptr t1 = f1->add_task("t1"); t1->addDay( DayAttr(DayAttr::MONDAY));
   task_ptr t2 = f1->add_task("t2"); t2->addDate( DateAttr(1,1,2014));
   task_ptr t3 = f1->add_task("t3"); t3->addCron( cron );

   // begin the suite, Under hybrid clock, nodes with day,date and cron attributes should be marked as complete
   the_defs->beginAll();
   BOOST_CHECK_MESSAGE(t1->state() == NState::COMPLETE,"Expected node to be complete");
   BOOST_CHECK_MESSAGE(t2->state() == NState::COMPLETE,"Expected node to be complete");
   BOOST_CHECK_MESSAGE(t3->state() == NState::COMPLETE,"Expected node to be complete");

   // Change the node state, so that we can test re-queue
   t1->set_state(NState::QUEUED);
   t2->set_state(NState::QUEUED);
   t3->set_state(NState::QUEUED);
   BOOST_CHECK_MESSAGE(t1->state() == NState::QUEUED,"Expected node to be QUEUED");
   BOOST_CHECK_MESSAGE(t2->state() == NState::QUEUED,"Expected node to be QUEUED");
   BOOST_CHECK_MESSAGE(t3->state() == NState::QUEUED,"Expected node to be QUEUED");

   // Now re-queue all and make sure re-queue also Under hybrid clock, nodes with day,date and cron
   // attributes should be marked as complete
   the_defs->requeue();
   BOOST_CHECK_MESSAGE(t1->state() == NState::COMPLETE,"Expected node to be complete");
   BOOST_CHECK_MESSAGE(t2->state() == NState::COMPLETE,"Expected node to be complete");
   BOOST_CHECK_MESSAGE(t3->state() == NState::COMPLETE,"Expected node to be complete");
}

BOOST_AUTO_TEST_SUITE_END()
