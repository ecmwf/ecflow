#define BOOST_TEST_MODULE TestSingleSimulator
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #6 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/// Use this class to test single simulation of definition file that we want to add
/// to Test Simulator. This is a separate exe
//============================================================================
#include "Simulator.hpp"

#include "File.hpp"
#include "Log.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestUtil.hpp"

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

//BOOST_AUTO_TEST_CASE( test_repeat_with_cron  )
//{
//   cout << "Simulator:: ...test_repeat_with_cron\n";
////    suite s
////    clock real <today date>
////      family f
////      repeat date YMD 20091001  20091004 1  # yyyymmdd
////       family plot
////          complete plot/finish == complete
////
////          task finish
////             trigger 1 == 0    # stops task from running
////             complete checkdata::done or checkdata == complete
////
////          task checkdata
////             event done
////             cron <today date> + 2 minutes     # cron that run forever
////      endfamily
////   endfamily
//// endsuite
//
//   Defs theDefs;
//   {
//      boost::posix_time::ptime   theLocalTime =  Calendar::second_clock_time();
//      boost::posix_time::ptime   time_plus_2_minute =  theLocalTime +  minutes(2);
//      ClockAttr clockAttr(theLocalTime, false/* real clock*/);
//
//      suite_ptr suite = theDefs.add_suite("test_repeat_with_cron");
//      suite->addClock( clockAttr );
//
//      family_ptr f = suite->add_family( "f" );
//      f->addRepeat( RepeatDate("YMD",20091001,20091004,1));  // repeat contents 4 times
//      f->addVerify( VerifyAttr(NState::COMPLETE,4) );
//
//      family_ptr family_plot = f->add_family( "plot" );
//      family_plot->add_complete(  "plot/finish ==  complete");
//      family_plot->addVerify( VerifyAttr(NState::COMPLETE,4) );
//
//
//      task_ptr task_finish = family_plot->add_task("finish");
//      task_finish->add_trigger(  "1 == 0");
//      task_finish->add_complete( "checkdata:done or checkdata == complete" );
//      task_finish->addVerify( VerifyAttr(NState::COMPLETE,8) );
//
//      task_ptr task_checkdata = family_plot->add_task("checkdata");
//      task_checkdata->addEvent( Event(1,"done"));
//
//      CronAttr cronAttr;
//      cronAttr.addTimeSeries( ecf::TimeSlot(time_plus_2_minute.time_of_day()) );
//      task_checkdata->addCron( cronAttr  );
//      task_checkdata->addVerify( VerifyAttr(NState::COMPLETE,8) );
//
////    cout << theDefs << "\n";
//   }
//
//   Simulator simulator;
//   std::string errorMsg;
//   BOOST_REQUIRE_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_repeat_with_cron.def"), errorMsg),errorMsg);
//}


BOOST_AUTO_TEST_CASE( test_single_from_file  )
{
   cout << "Simulator:: ...test_single_from_file\n";

   std::string path = File::test_data("CSim/test/data/good_defs/operations/loop.def","CSim");

   Simulator simulator;
   std::string errorMsg;
   bool passed = simulator.run(path, errorMsg);

   BOOST_REQUIRE_MESSAGE(passed, path << " failed simulation \n" << errorMsg);
}


BOOST_AUTO_TEST_SUITE_END()

