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
#include "PrintStyle.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_repeat_day_time_combination_in_hierarchy )
{
   cout << "ANode:: ...test_repeat_day_time_combination_in_hierarchy \n";
   // Create the suite, starting on a sunday
   // suite s1
   //  family f
   //    repeat integer rep 0 1
   //    family f1
   //       day monday
   //       task t1
   //         time 10:00  # should run, monday at 10:00 twice, due to repeat
   //         time 11:00  # should run, monday at 11:00 twice, due to repeat
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2019,8,4),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   family_ptr f = suite->add_family("f");
   family_ptr f1 = f->add_family("f1");
   f1->addRepeat(RepeatInteger("rep",0,1,1));
   f1->addDay( DayAttr(DayAttr::MONDAY) );
   task_ptr t1 = f1->add_task("t1");
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(11,0)) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time1 = boost::posix_time::ptime(date(2019,8,5),time_duration(10,0,0));  // Monday & 10
   boost::posix_time::ptime expected_time2 = boost::posix_time::ptime(date(2019,8,5),time_duration(11,0,0));  // Monday & 11
   boost::posix_time::ptime expected_time3 = boost::posix_time::ptime(date(2019,8,12),time_duration(10,0,0)); // Monday & 10
   boost::posix_time::ptime expected_time4 = boost::posix_time::ptime(date(2019,8,12),time_duration(11,0,0)); // Monday & 11
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 212; m++) {  // run for 9 days

      //const std::vector<DayAttr>& days = f1->days();
      //const std::vector<ecf::TimeAttr>& times = t1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << " " << days[0].dump() << " " << times[0].dump() << " rep: " << f1->repeat().value() << "\n";

      Jobs jobs(&defs);
      JobsParam jobsParam;
      jobs.generate(jobsParam);

      if (jobsParam.submitted().size() ) {
         submitted++;
         //cout << "   submitted at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";

         // 1st Run: Monday at 10:00 am
         if ( submitted == 1)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time1,"\nExpected to submit at " << expected_time1 << " only, but also found " << suite->calendar().suiteTime());

         // 2nd Run: Monday at 11:00 am
         if ( submitted == 2)
             BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time2,"\nExpected to submit at " << expected_time2 << " only, but also found " << suite->calendar().suiteTime());

         // 3rd Run: Monday at 10:00 am
         if ( submitted == 3)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time3,"\nExpected to submit at " << expected_time3 << " only, but also found " << suite->calendar().suiteTime());

         // 4th Run: Monday at 11:00 am
          if ( submitted == 4)
             BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time4,"\nExpected to submit at " << expected_time4 << " only, but also found " << suite->calendar().suiteTime());


         t1->set_state(NState::COMPLETE); // cause repeat to loop
         //cout << "   set_complete t1 at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 4,"Expected four submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_repeat_time_day_combination_in_hierarchy )
{
   cout << "ANode:: ...test_repeat_time_day_combination_in_hierarchy \n";
   // Create the suite, starting on a sunday
   // suite s1
   //  family f
   //    repeat integer rep 0 1
   //    family f1
   //       time 10:00  # should run, monday at 10:00 twice, due to repeat
   //       task t1
   //          day monday
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   boost::posix_time::ptime the_time = boost::posix_time::ptime(date(2019,8,4),time_duration(0,0,0)); //sunday
   suite->addClock( ClockAttr(the_time) );
   family_ptr f = suite->add_family("f");
   family_ptr f1 = f->add_family("f1");
   f1->addRepeat(RepeatInteger("rep",0,3,1));  // four iteration
   f1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   task_ptr t1 = f1->add_task("t1");
   t1->addDay( DayAttr(DayAttr::MONDAY) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   boost::posix_time::ptime expected_time1 = boost::posix_time::ptime(date(2019,8,5),time_duration(0,0,0));   // Monday & 00:00
   boost::posix_time::ptime expected_time2 = boost::posix_time::ptime(date(2019,8,5),time_duration(10,0,0));  // Monday & 10
   boost::posix_time::ptime expected_time3 = boost::posix_time::ptime(date(2019,8,12),time_duration(0,0,0));  // Monday & 00:00
   boost::posix_time::ptime expected_time4 = boost::posix_time::ptime(date(2019,8,12),time_duration(10,0,0)); // Monday & 10
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 212; m++) {  // run for 9 days

      //const std::vector<DayAttr>& days = t1->days();
      //const std::vector<ecf::TimeAttr>& times = f1->timeVec();
      //cout << "Suite time " << suite->calendar().suiteTime() << " dayChanged:" << suite->calendar().dayChanged() << " " << days[0].dump() << " " << times[0].dump() << " rep: " << f1->repeat().value() << "\n";

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

         // 1st Run: Monday at 00:00 am
         if ( submitted == 3)
            BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time3,"\nExpected to submit at " << expected_time3 << " only, but also found " << suite->calendar().suiteTime());

         // 2nd Run: Monday at 10:00 am
         if ( submitted == 4)
             BOOST_CHECK_MESSAGE( suite->calendar().suiteTime() == expected_time4,"\nExpected to submit at " << expected_time4 << " only, but also found " << suite->calendar().suiteTime());

         t1->set_state(NState::COMPLETE); // cause repeat to loop
         //cout << "   set_complete t1 at " << suite->calendar().suiteTime() << " " << days[0].dump() << " " << times[0].dump() << "\n";
         //PrintStyle style(PrintStyle::MIGRATE);
         //cout << defs << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 4 ,"Expected four submission but found " << submitted);
}

BOOST_AUTO_TEST_CASE( test_repeat_with_impossible_day_combination_in_hierarchy  )
{
   cout << "ANode:: ...test_repeat_with_impossible_day_combination_in_hierarchy\n";
   // Create the suite, starting on a sunday
   //   suite s1
   //     clock real 04.08.2019            # Sunday
   //     family f
   //       repeat integer rep 0 1
   //       family f1
   //          day monday
   //          task t1
   //               day tuesday            # cant be free on Monday and Tuesday,
   //               time 10:00
   //               verify complete:1
   Defs  defs;
   suite_ptr suite = defs.add_suite("s1");
   suite->addClock( ClockAttr( boost::posix_time::ptime(date(2019,8,4),time_duration(0,0,0)) ) );//sunday
   family_ptr f = suite->add_family("f");
   family_ptr f1 = f->add_family("f1");
   f1->addRepeat(RepeatInteger("rep",0,1,1));  // 2 iteration
   f1->addDay( DayAttr(DayAttr::MONDAY) );
   task_ptr t1 = f1->add_task("t1");
   t1->addDay( DayAttr(DayAttr::TUESDAY) );
   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(10,0)) );
   defs.beginAll();

   CalendarUpdateParams calUpdateParams( hours(1) );
   //cout << defs << "\n";

   int submitted = 0;
   for(int m=1; m < 212; m++) {  // run for 9 days

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

         t1->set_state(NState::COMPLETE); // cause repeat to loop
         //PrintStyle style(PrintStyle::MIGRATE);
         //cout << defs << "\n";
      }

      defs.updateCalendar(calUpdateParams);
   }
   BOOST_CHECK_MESSAGE( submitted == 0 ,"Expected 0 submission but found " << submitted);
}

BOOST_AUTO_TEST_SUITE_END()
