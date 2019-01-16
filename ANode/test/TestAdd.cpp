/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
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

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_add )
{
   cout << "ANode:: ...test_add\n";

   defs_ptr defs = Defs::create();
   task_ptr t1 = Task::create("t1");
   family_ptr f1 = Family::create("f1");
   suite_ptr s1 = Suite::create("s1");

   defs->addSuite(s1);
   s1->addFamily(f1);
   f1->addTask(t1);

   defs_ptr defs2 = Defs::create();
   suite_ptr s2 = Suite::create("s2");
   family_ptr f2 = Family::create("f2");

   // This should all fail since, they are already owned.
   // Otherwise we end up with two different container owning the same object
   BOOST_CHECK_THROW( s2->addFamily(f1),std::runtime_error);
   BOOST_CHECK_THROW( f2->addTask(t1),std::runtime_error);
   BOOST_CHECK_THROW( defs2->addSuite(s1),std::runtime_error);
}

BOOST_AUTO_TEST_CASE( test_add_error )
{
   cout << "ANode:: ...test_add_error\n";

   defs_ptr defs = Defs::create();
   suite_ptr s1  = defs->add_suite("s1");
   s1->add_task("t1");
   s1->add_family("f1");
   BOOST_CHECK_THROW( defs->add_suite("s1"),std::runtime_error); // duplicate suite
   BOOST_CHECK_THROW( s1->add_task("t1"),std::runtime_error);    // duplicate task
   BOOST_CHECK_THROW( s1->add_family("t1"),std::runtime_error);  // duplicate name
   BOOST_CHECK_THROW( s1->add_task("f1"),std::runtime_error);    // duplicate name
}

BOOST_AUTO_TEST_CASE( test_add_delete_time )
{
   cout << "ANode:: ...test_add_delete_time\n"; // ECFLOW-1260

   // Make sure that if we delete any time based attributes
   Defs defs;
   suite_ptr s1 = defs.add_suite("s1");
   task_ptr t1 = s1->add_task("t1");

   BOOST_REQUIRE_MESSAGE(!t1->hasTimeDependencies(), "Expected no time attributes");

   ecf::CronAttr cronAttr;
   ecf::TimeSlot start( 0, 0 );
   ecf::TimeSlot finish( 10, 0 );
   ecf::TimeSlot incr( 0, 5 );
   std::vector<int> weekdays;   for(int i=0;i<7;++i) weekdays.push_back(i);
   std::vector<int> daysOfMonth;for(int i=1;i<32;++i) daysOfMonth.push_back(i);
   std::vector<int> months;     for(int i=1;i<13;++i) months.push_back(i);
   cronAttr.addTimeSeries(start,finish,incr);
   cronAttr.addWeekDays( weekdays  );
   cronAttr.addDaysOfMonth(daysOfMonth);
   cronAttr.addMonths(  months );
   t1->addCron( cronAttr  );
   BOOST_CHECK_MESSAGE(t1->hasTimeDependencies(), "Expected time attributes");
   t1->deleteCron("");
   BOOST_CHECK_MESSAGE(!t1->hasTimeDependencies(), "Expected no time attributes");

   t1->addDate( DateAttr(1,2,2009) );
   BOOST_CHECK_MESSAGE(t1->hasTimeDependencies(), "Expected time attributes");
   t1->deleteDate("");
   BOOST_CHECK_MESSAGE(!t1->hasTimeDependencies(), "Expected no time attributes");

   t1->addDay( DayAttr(DayAttr::MONDAY) );
   BOOST_CHECK_MESSAGE(t1->hasTimeDependencies(), "Expected time attributes");
   t1->deleteDay("");
   BOOST_CHECK_MESSAGE(!t1->hasTimeDependencies(), "Expected no time attributes");

   t1->addTime( ecf::TimeAttr(ecf::TimeSlot(0,0),ecf::TimeSlot(10,1),ecf::TimeSlot(0,1),true) );
   BOOST_CHECK_MESSAGE(t1->hasTimeDependencies(), "Expected time attributes");
   t1->deleteTime("");
   BOOST_CHECK_MESSAGE(!t1->hasTimeDependencies(), "Expected no time attributes");

   t1->addToday( ecf::TodayAttr(ecf::TimeSlot(10,12)) );
   BOOST_CHECK_MESSAGE(t1->hasTimeDependencies(), "Expected time attributes");
   t1->deleteToday("");
   BOOST_CHECK_MESSAGE(!t1->hasTimeDependencies(), "Expected no time attributes");
}

BOOST_AUTO_TEST_SUITE_END()
