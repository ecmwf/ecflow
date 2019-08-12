/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

#include "Task.hpp"
#include "Family.hpp"
#include "Suite.hpp"
#include "Defs.hpp"
#include "Jobs.hpp"
#include "JobsParam.hpp"
#include "CalendarUpdateParams.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

// See ECFLOW-337
// See ECFLOW-833. Also See Note: ACore/doc/TimeDependencies.ddoc

BOOST_AUTO_TEST_CASE( test_day_time_combination )
{
   cout << "ANode:: ...test_day_time_combination\n";
   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 7.6.2015  # sunday
   //  task t1              # run task monday at 10, Day acts like a guard, time if ignored until day is free
   //      day monday
   //      time 10:00
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2015,6,7),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   task_ptr t1 = suite->add_task("t1");
   t1->addDay( DayAttr(DayAttr::MONDAY) );
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );

   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time = boost::posix_time::ptime(date(2015,6,8),time_duration(10,0,0)); //Monday & 10
   //cout << "expected_time =  " << expected_time << "\n";

   int submitted = 0;
   for(int m=1; m < 120; m++) {  // run for 5 days

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "submitted at " << suite->calendar().suiteTime() << "\n";
         BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time,"\nExpected to submit at " << expected_time << " only, but also found " << suite->calendar().suiteTime());
      }

      defs.updateCalendar(calUpdateParams);
      if ( suite->calendar().dayChanged() ) {
         Node::Requeue_args args;
         t1->requeue(args);
      }
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged: " <<   suite->calendar().dayChanged() << "\n";
   }
   BOOST_CHECK_MESSAGE( submitted == 1 ,"Expected one submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_date_time_combination )
{
   cout << "ANode:: ...test_date_time_combination\n";
   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 7.6.2015  # sunday
   //  task t1              # run task monday at 10, Date acts like a guard, time if ignored until date is free
   //      date 8.6.2015
   //      time 10:00
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2015,6,7),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   task_ptr t1 = suite->add_task("t1");
   t1->addDate( DateAttr(8,6,2015) );   // Monday
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time = boost::posix_time::ptime(date(2015,6,8),time_duration(10,0,0)); // Monday & 10
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 100; m++) {

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "submitted at " << suite->calendar().suiteTime() << "\n";
         BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time,"\nExpected to submit at " << expected_time << " only, but also found " << suite->calendar().suiteTime());
      }

      defs.updateCalendar(calUpdateParams);
      if ( suite->calendar().dayChanged() ) {
         Node::Requeue_args args;
         t1->requeue(args);
      }
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged: " <<   suite->calendar().dayChanged() << "\n";
   }
   BOOST_CHECK_MESSAGE( submitted == 1 ,"Expected one submission but found " << submitted);
}


BOOST_AUTO_TEST_CASE( test_day_time_combination_in_hierarchy )
{
   cout << "ANode:: ...test_day_time_combination_in_hierarchy\n";
   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 7.6.2015 # sunday
   //  family f1
   //    day monday     # day attribute prevents time from being free
   //    task t1
   //      time 10:00   # should run ONCE, monday at 10:00
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2015,6,7),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   family_ptr f1 = suite->add_family("f1");
   f1->addDay( DayAttr(DayAttr::MONDAY) );
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time = boost::posix_time::ptime(date(2015,6,8),time_duration(10,0,0)); //Monday & 10
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 120; m++) {  // run for 5 days

      //const std::vector<DayAttr>& days = f1->days();
      //const std::vector<ecf::TimeAttr>& times = t1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << " " << days[0].dump() << " " << times[0].dump() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "   submitted at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";

         // 1st Run: Monday at 10:00 am
         if ( submitted == 1)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time,"\nExpected to submit at " << expected_time << " only, but also found " << suite->calendar().suiteTime());

         Node::Requeue_args args;
         f1->requeue(args);
         //cout << "   requeue f1 at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 1 ,"Expected one submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_time_day_combination_in_hierarchy )
{
   cout << "ANode:: ...test_time_day_combination_in_hierarchy\n";
   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 7.6.2015 # sunday
   //  family f1
   //    time 10:00
   //    task t1      # task should run twice, Monday morning, and Monday at 10:00
   //      day monday
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2015,6,7),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   family_ptr f1 = suite->add_family("f1");
   f1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   task_ptr t1 = f1->add_task("t1");
   t1->addDay( DayAttr(DayAttr::MONDAY) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time1 = boost::posix_time::ptime(date(2015,6,8),time_duration(0,0,0)); //Monday & 00:00
   boost::posix_time::ptime expected_time2 = boost::posix_time::ptime(date(2015,6,8),time_duration(10,0,0)); //Monday & 10
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 120; m++) {  // run for 5 days
      //const std::vector<DayAttr>& days = t1->days();
      //const std::vector<ecf::TimeAttr>& times = f1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << " " << days[0].dump() << " " << times[0].dump() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "   submitted at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";

         // 1st Run: Monday at 00:00 am
         if ( submitted == 1)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time1,"\nExpected to submit at " << expected_time1 << " only, but also found " << suite->calendar().suiteTime());

         // 2nd Run: Monday at 10:00 am
         if ( submitted == 2)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time2,"\nExpected to submit at " << expected_time2 << " only, but also found " << suite->calendar().suiteTime());

         Node::Requeue_args args;
         f1->requeue(args);
         //cout << "   requeue f1 at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 2 ,"Expected two submission but found " << submitted);
}


BOOST_AUTO_TEST_CASE( test_date_time_combination_in_hierarchy )
{
   cout << "ANode:: ...test_date_time_combination_in_hierarchy\n";

   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 7.6.2015 # sunday
   //  family f1
   //    date 8.6.15  # monday date now guards the time
   //    task t1
   //      time 10:00
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2015,6,7),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   family_ptr f1 = suite->add_family("f1");
   f1->addDate( DateAttr(8,6,2015) );   // Monday
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time = boost::posix_time::ptime(date(2015,6,8),time_duration(10,0,0)); // Monday & 10
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 100; m++) {

      //const std::vector<DateAttr>& dates = f1->dates();
      //const std::vector<ecf::TimeAttr>& times = t1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << " " << dates[0].dump() << " " << times[0].dump() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "  submitted at " << suite->calendar().suiteTime() << " " << dates[0].dump() << " " << times[0].dump() << "\n";

         // New to ecflow 5.0, parent date will guard the time. i.e will not let time be free until date is satisfied.
         if ( submitted == 1)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time,"\nExpected to submit at " << expected_time << " only, but also found " << suite->calendar().suiteTime());

         Node::Requeue_args args;
         f1->requeue(args);
         //cout << "  requeue at " << suite->calendar().suiteTime() << " " << dates[0].dump() << " " << times[0].dump() << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 1 ,"Expected one submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_time_date_combination_in_hierarchy )
{
   cout << "ANode:: ...test_time_date_combination_in_hierarchy \n";
   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 7.6.2015 # sunday
   //  family f1
   //    time 10:00
   //    task t1
   //       date 8.6.15  # monday, run once at Monday morning, and gain Monday at 10:00
   //
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2015,6,7),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   family_ptr f1 = suite->add_family("f1");
   f1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   task_ptr t1 = f1->add_task("t1");
   t1->addDate( DateAttr(8,6,2015) );   // Monday
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time1 = boost::posix_time::ptime(date(2015,6,8),time_duration(0,0,0)); // Monday & 00:00
   boost::posix_time::ptime expected_time2 = boost::posix_time::ptime(date(2015,6,8),time_duration(10,0,0)); // Monday & 10
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 100; m++) {

      //const std::vector<DateAttr>& dates = t1->dates();
      //const std::vector<ecf::TimeAttr>& times = f1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << " " << dates[0].dump() << " " << times[0].dump() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "  submitted at " << suite->calendar().suiteTime() << " " << dates[0].dump() << " " << times[0].dump() << "\n";

         // 1st Run: MONDAY Morning 00:00 time was free from sunday
         if ( submitted == 1)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time1,"\nExpected to submit at " << expected_time1 << " only, but also found " << suite->calendar().suiteTime());

         // 2nd Run: Monday at 10:00
         if ( submitted == 2)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time2,"\nExpected to submit at " << expected_time2 << " only, but also found " << suite->calendar().suiteTime());

         Node::Requeue_args args;
         f1->requeue(args);
         //cout << "  requeue at " << suite->calendar().suiteTime() << " " << dates[0].dump() << " " << times[0].dump() << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 2 ,"Expected two submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_impossible_day_combination )
{
   cout << "ANode:: ...test_impossible_day_combination\n";
   // Create the suite, starting on a sunday
   // suite s1
   //  clock real 4.8.2019 # sunday
   //  family f1
   //    day monday
   //    task t1        # task should never run, can't be Sunday/Monday at same time.
   //      day tuesday
   //      time 10:00
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   suite->addClock( ClockAttr( boost::posix_time::ptime(date(2019,8,4),time_duration(0,0,0)) ) );//sunday
   family_ptr f1 = suite->add_family("f1");
   f1->addDay( DayAttr(DayAttr::MONDAY) );
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   t1->addDay( DayAttr(DayAttr::TUESDAY) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 100; m++) {
      //const std::vector<DateAttr>& fdates = f1->dates();
      //const std::vector<DateAttr>& dates = t1->dates();
      //const std::vector<ecf::TimeAttr>& times = t1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << "\n   "
      //         << fdates[0].dump() << " " << dates[0].dump() << " " << times[0].dump() << " rep: " << f1->repeat().value() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << " submitted " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << "\n   "
         //         << fdates[0].dump() << " " << dates[0].dump() << " " << times[0].dump() << " rep: " << f1->repeat().value() << "\n";
         Node::Requeue_args args;
         t1->requeue(args);
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 0,"Expected zero submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_impossible_date_combination )
{
   cout << "ANode:: ...test_impossible_date_combination\n";

   // Create the suite, starting on a sunday
   // suite s1
   //  family f1
   //    date 5.8.2019   # monday
   //    task t1         # task should never run, can't be Sunday/Monday at same time.
   //      date 6.8.2019 # tuesday
   //      time 10:00
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   suite->addClock( ClockAttr( boost::posix_time::ptime(date(2019,8,4),time_duration(0,0,0)) ) );//sunday
   family_ptr f1 = suite->add_family("f1");
   f1->addDate( DateAttr(5,8,2019) );   // Monday
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   t1->addDate( DateAttr(6,8,2019) );   // Tuesday
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 100; m++) {
      //const std::vector<DayAttr>& fdays = f1->days();
      //const std::vector<DayAttr>& days = t1->days();
      //const std::vector<ecf::TimeAttr>& times = t1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << "\n   "
      //         << fdays[0].dump() << " " << days[0].dump() << " " << times[0].dump() << " rep: " << f1->repeat().value() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << " submitted at " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << "\n   "
         //         << fdays[0].dump() << " " << days[0].dump() << " " << times[0].dump() << " rep: " << f1->repeat().value() << "\n";
         Node::Requeue_args args;
         t1->requeue(args);
      }
      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 0,"Expected zero submission but found " << submitted);
}

BOOST_AUTO_TEST_SUITE_END()
