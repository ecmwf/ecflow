/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "PrintStyle.hpp"
#include "CalendarUpdateParams.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_ECFLOW_417_real_clock )
{
   // Make sure reque resets calendar according to the clock attribute *FOR* a real clock
   cout << "ANode:: ...test_ECFLOW_417_real_clock  \n";

   defs_ptr defs = Defs::create();
   suite_ptr s1 = defs->add_suite("s1");
   s1->addRepeat(RepeatDay(1));
   s1->addClock( ClockAttr(1,10,2015,false/*real clock*/) );
   task_ptr t1 = s1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(16,0),false) );

   defs->beginAll();

   // ECF_DATE = year/month/day of month
   {
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151001","expected 20151001 but found " << ecf_date.theValue());
   }

   // now requeue and date should be reset
   {
      defs->requeue();
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151001","expected 20151001 but found " << ecf_date.theValue());
   }

   // Now change the clock date to 10.10.2015.
   {
      s1->changeClockDate("10.10.2015");
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151010","expected 20151010 but found " << ecf_date.theValue());
   }

   // now requeue
   {
      defs->requeue();
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151010","expected 20151010 but found " << ecf_date.theValue());
   }
}

BOOST_AUTO_TEST_CASE( test_ECFLOW_417_hybrid_clock )
{
   // ECFLOW-417
   // For a suite with a hybrid clock *AND* repeat day. requue should update calendar date, by the repeat day interval
   cout << "ANode:: ...test_ECFLOW_417_hybrid_clock  \n";

   defs_ptr defs = Defs::create();
   suite_ptr s1 = defs->add_suite("s1");
   s1->addRepeat(RepeatDay(1));
   s1->addClock( ClockAttr(1,10,2015,true/*hybrid*/) );
   task_ptr t1 = s1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(16,0),false) );

   defs->beginAll();

   // ECF_DATE = year/month/day of month
   {
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151001","expected 20151001 but found " << ecf_date.theValue());
   }

   // now requeue and date should be incremented
   {
      defs->requeue();
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151002","expected 20151002 but found " << ecf_date.theValue());
   }

   // now requeue again and date should be incremented
   {
      defs->requeue();
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151003","expected 20151003 but found " << ecf_date.theValue());
   }


   // Now change the clock date to 10.10.2015. We expect date to decremented to 09.10.2015,
   // Since user should always re-queue after changing suite clock attributes.
   // This will get us back to the original date set by the user.
   {
      s1->changeClockDate("10.10.2015");
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151009","expected 20151009 but found " << ecf_date.theValue());
   }

   // now requeue and date should be incremented
   {
      defs->requeue();
      const Variable& ecf_date = s1->findGenVariable("ECF_DATE");
      BOOST_CHECK_MESSAGE(!ecf_date.empty(),"Did not find ECF_DATE");
      BOOST_CHECK_MESSAGE(ecf_date.theValue() == "20151010","expected 20151010 but found " << ecf_date.theValue());
   }

   // Now update calendar for more than 24 hours, and calendar date should *NOT* change for hybrid
   {
      boost::posix_time::ptime time_now = s1->calendar().suiteTime();
      boost::posix_time::time_duration serverPollPeriod = boost::posix_time::time_duration(0,1,0,0);
      std::string expectedDate = "2015-Oct-10";

      for(int hour=1; hour <= 60; hour++) {

         // Update calendar every hour, for 60 hours
         time_now += hours(1);
         CalendarUpdateParams param(time_now, serverPollPeriod, true, /* serverRunning */ false /* forTest */ );

         defs->updateCalendar( param );

         // cout << "hour = " << hour << " timeAfterUpdate " << to_simple_string(s1->calendar().suiteTime()) <<  "\n";

         std::string actualDate = to_simple_string(s1->calendar().suiteTime().date());
         BOOST_CHECK_MESSAGE( actualDate == expectedDate,"Expected '" << expectedDate << "' but found " << actualDate << " at hour " << hour);
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
