//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #26 $
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
#include <fstream>
#include <stdlib.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "ServerTestHarness.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite )


BOOST_AUTO_TEST_CASE( test_clk_sync )
{
   // This test is used to test sync'ing of the suite calendars
   // The default clock type is *real*. We will create a suite with a hybrid clock attribute
   // For the suite calendar, we do not persist the clock type(hybrid/real), since this can be
   // obtained from the clock attribute. Hence a hybrid calendar in the server, will arrive as
   // real calendar at the client side. (i.e via the memento). It is then up to the client
   // to update the calendar with clock type stored in the clock attribute.
   // See:void Suite::set_memento( const SuiteCalendarMemento* memento )
   // This test(implicitly) will check that after an incremental sync that suite calendar and
   // suite clock attribute, that both are of the same clock type.
   // This is done in ServerTestHarness via invariant checking.

   DurationTimer timer;
   cout << "Test:: ...test_clk_sync "<< flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in ecf includes
   //                     Allows test to run without requiring installation

   //# Note: we have to use relative paths, since these tests are relocatable
   // suite test_clk_sync
   //   clocl hybrid
   //   task a
   //      meter myMeter 0 100
   // endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_clk_sync" );
      suite->addClock(ClockAttr(true)); // add hybrid clock
      task_ptr task_a = suite->add_task("a");
      task_a->addMeter( Meter("myMeter",0,100,100) );
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness(false);
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_clk_sync.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()
