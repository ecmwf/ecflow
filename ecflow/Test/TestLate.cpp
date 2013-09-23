//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
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
#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "TestFixture.hpp"
#include "ServerTestHarness.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "ClientInvoker.hpp"
#include "PrintStyle.hpp"
#include "ClientToServerCmd.hpp"
#include "DefsStructureParser.hpp"
#include "AssertTimer.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite  )

BOOST_AUTO_TEST_CASE( test_late )
{
   DurationTimer timer;
   cout << "Test:: ...test_late " << flush;
   TestClean clean_at_start_and_end;

   /// This test will sleep longer than the job submission interval
   /// which cause the task to be late
   /// as the active time has been set for 1 minute.
   /// The check for lateness is ONLY done are server poll time.
   /// Hence the task run time must be at least twice the poll time.
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_late");
      suite->add_variable("SLEEPTIME",boost::lexical_cast<std::string>(TestFixture::job_submission_interval()*2) ); // this will cause the late

      task_ptr task = suite->add_task("t1");
      ecf::LateAttr lateAttr;
      lateAttr.addComplete( ecf::TimeSlot(0,1), true);

      task->addLate( lateAttr );
   }

   ServerTestHarness serverTestHarness(false/*do verification*/,false/* dont do standard verification */);
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_late.def"));

   ClientInvoker theClient;
   theClient.sync_local();
   BOOST_CHECK_MESSAGE( theClient.defs(),"Expected defs");

   node_ptr node = theClient.defs()->findAbsNode("/test_late/t1");
   BOOST_CHECK_MESSAGE( node,"Expected task to be found");

   ecf::LateAttr* late = node->get_late();
   BOOST_CHECK_MESSAGE( late->isLate(),"Expected late to be set");
   BOOST_CHECK_MESSAGE( node->flag().is_set(ecf::Flag::LATE),"Expected late flag to be set");

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()
