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

BOOST_AUTO_TEST_CASE( test_repeat_date_for_loop2  )
{
   cout << "Simulator:: ...test_repeat_date_for_loop2\n";

   //suite suite
   // clock real <todays date>
   // repeat date YMD 20091001  20091005 1  # yyyymmdd
   // family family
   //     repeat date YMD 20091001  20091005 1  # yyyymmdd
   //    task t
   //       time 10:00
   //       time 11:00
   //    endfamily
   //endsuite

   // Each task should be run 5 * 5 * 2 = 50 times, ie every day from from 1st Oct -> 5 Oct 5*5 times * 2 time slots
   Defs theDefs;
   {
      // start at specific time other wise time dependent checks will not verify
      suite_ptr suite = theDefs.add_suite("test_repeat_date_for_loop2");
      suite->addRepeat( RepeatDate("YMD",20091001,20091005,1));  // repeat contents 5 times
      suite->addVerify( VerifyAttr(NState::COMPLETE,5) );

      ClockAttr clockAttr;
      clockAttr.date(1,10,2009);
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family( "family" );
      fam->addRepeat( RepeatDate("YMD",20091001,20091005,1));  // repeat contents 5 times
      fam->addVerify( VerifyAttr(NState::COMPLETE,25) );

      task_ptr task = fam->add_task("t");
      task->addTime( ecf::TimeAttr( TimeSlot(10,0) ) );
      task->addTime( ecf::TimeAttr( TimeSlot(11,0) ) );
      task->addVerify( VerifyAttr(NState::COMPLETE,50) );     // task should complete 50 times

      // cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_repeat_date_for_loop2.def"), errorMsg),errorMsg);
}


//BOOST_AUTO_TEST_CASE( test_single_from_file  )
//{
//   cout << "Simulator:: ...test_single_from_file\n";
//
////std::string path = File::test_data("CSim/test/data/good_defs/operations/loop.def","CSim");
//   std::string path = File::test_data("CSim/test/data/good_defs/ECFLOW-130/radarlvl2.def","CSim");
//
//   Simulator simulator;
//   std::string errorMsg;
//   bool passed = simulator.run(path, errorMsg);
//
//   BOOST_REQUIRE_MESSAGE(passed, path << " failed simulation \n" << errorMsg);
//}


BOOST_AUTO_TEST_SUITE_END()

