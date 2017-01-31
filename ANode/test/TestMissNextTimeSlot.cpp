//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $
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

#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_miss_next_time_slot )
{
   cout << "ANode:: ...test_miss_next_time_slot\n";

   // Start TIME at 9:30
   // This test is custom. When the user interactively forces a node to the complete state,
   // But where the user set of time slots. In this case the node should complete and then
   // requee and miss the next time. If this is repeated, eventually we should reach the
   // end of the time slot. In which case the node should *not* re-queue and stay complete
   //
   // When the node is then re-queued check that the time has been correctly reset.

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
   clockAttr.set_gain(9/*hour*/,30/*minutes*/); // *start* at 9:30
   suite->addClock( clockAttr );

   suite->begin();

   // get all the time attributes
   const TimeSeries& ts_10 = t1->timeVec()[0].time_series();
   const TimeSeries& ts_11 = t1->timeVec()[1].time_series();
   const TimeSeries& ts_12 = t1->timeVec()[2].time_series();
   const TimeSeries& ts_13 = t1->timeVec()[3].time_series();
   BOOST_CHECK_MESSAGE( ts_10.is_valid(),  "Expected time 10 to be valid since we started at 9:30 ");
   BOOST_CHECK_MESSAGE( ts_11.is_valid(),  "Expected time 11 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid since we started at 9:30");

   const TimeSlot& time_10 = t1->timeVec()[0].time_series().get_next_time_slot();
   const TimeSlot& time_11 = t1->timeVec()[1].time_series().get_next_time_slot();
   const TimeSlot& time_12 = t1->timeVec()[2].time_series().get_next_time_slot();
   const TimeSlot& time_13 = t1->timeVec()[3].time_series().get_next_time_slot();
   BOOST_CHECK_MESSAGE( time_10 == TimeSlot(10,0),"Expected next time slot of 10:00 but found " << time_10.toString());
   BOOST_CHECK_MESSAGE( time_11 == TimeSlot(11,0),"Expected next time slot of 11:00 but found " << time_11.toString());
   BOOST_CHECK_MESSAGE( time_12 == TimeSlot(12,0),"Expected next time slot of 12:00 but found " << time_12.toString());
   BOOST_CHECK_MESSAGE( time_13 == TimeSlot(13,0),"Expected next time slot of 13:00 but found " << time_13.toString());

   // before test flags should be clear
   BOOST_CHECK_MESSAGE(!t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be clear,before test");

   t1->miss_next_time_slot();
   BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set,");
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be expired");
   BOOST_CHECK_MESSAGE( ts_11.is_valid(),  "Expected time 11 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid since we started at 9:30");

   // Calling miss_next_time_slot again, should have *NO* effect, since its only takes affect when NO_REQUE_IF_SINGLE_TIME_DEP is clear
   t1->miss_next_time_slot();
   BOOST_CHECK_MESSAGE(t1->get_flag().is_set(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP),"Expected ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP to be set,");
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be expired");
   BOOST_CHECK_MESSAGE( ts_11.is_valid(),  "Expected time 11 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid since we started at 9:30");

   // Clear the flag and call miss_next_time_slot, this time an additional time slot should have expired
   t1->flag().clear(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP);
   t1->miss_next_time_slot();
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be expired");
   BOOST_CHECK_MESSAGE( !ts_11.is_valid(), "Expected time 11 to be expired");
   BOOST_CHECK_MESSAGE( ts_12.is_valid(),  "Expected time 12 to be valid since we started at 9:30");
   BOOST_CHECK_MESSAGE( ts_13.is_valid(),  "Expected time 13 to be valid since we started at 9:30");

   // call twice more, to expire all time slots
   t1->flag().clear(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP);
   t1->miss_next_time_slot();
   t1->flag().clear(ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP);
   t1->miss_next_time_slot();
   BOOST_CHECK_MESSAGE( !ts_10.is_valid(), "Expected time 10 to be expired");
   BOOST_CHECK_MESSAGE( !ts_11.is_valid(), "Expected time 11 to be expired");
   BOOST_CHECK_MESSAGE( !ts_12.is_valid(), "Expected time 12 to be expired");
   BOOST_CHECK_MESSAGE( !ts_13.is_valid(), "Expected time 13 to be expired");
}

BOOST_AUTO_TEST_SUITE_END()
