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

// In the test case we will dynamically create all the test data.
// The data is created dynamically so that we can stress test the server
// This test does not have any time dependencies in the def file.

BOOST_AUTO_TEST_CASE( test_events )
{
   DurationTimer timer;
   cout << "Test:: ...test_events "<< flush;
   TestClean clean_at_start_and_end;

   // Create the defs file corresponding to the text below
   // ECF_HOME variable is automatically added by the test harness.
   // ECF_INCLUDE variable is automatically added by the test harness.
   // SLEEPTIME variable is automatically added by the test harness.
   // ECF_CLIENT_EXE_PATH variable is automatically added by the test harness.
   //                     This is substituted in sms includes
   //                     Allows test to run without requiring installation

   //# Note: we have to use relative paths, since these tests are relocatable
   // suite test_events
   //   family family1
   //       task a
   //         event 1 myEvent
   //         meter myMeter 0 100
   //       task b
   //          trigger a == complete
   //    endfamily
   //    family family2
   //          task aa
   //             trigger ../family1/a:myMeter >= 90 and ../family1/a:myEvent
   //          task bb
   //             trigger ../family1/a:myMeter >= 50  or  ../family1/a:myEvent
   //     endfamily
   // endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_events" );
      {
         family_ptr fam = suite->add_family("family1");
         task_ptr task_a = fam->add_task("a");
         task_ptr task_b = fam->add_task("b");
         task_a->addMeter( Meter("myMeter",0,100,100) );             // ServerTestHarness will add correct ecf
         task_a->addEvent( Event(1,"myEvent") );                     // ServerTestHarness will add correct ecf
         task_a->addLabel( Label("task_a_label","Label1") );         // ServerTestHarness will add correct ecf
         task_a->addVerify( VerifyAttr(NState::COMPLETE,1) );

         task_b->add_trigger( "a == complete" );
         task_b->addLabel( Label("task_b_label","label1 label2") );  // ServerTestHarness will add correct ecf
         task_b->addVerify( VerifyAttr(NState::COMPLETE,1) );
      }
      {
         family_ptr fam2 = suite->add_family("family2");
         task_ptr task_aa = fam2->add_task("aa");
         task_ptr task_bb = fam2->add_task("bb");
         task_aa->add_trigger(  "../family1/a:myMeter >= 90 and ../family1/a:myEvent");
         task_aa->addVerify( VerifyAttr(NState::COMPLETE,1) );
         task_bb->add_trigger(  "../family1/a:myMeter >= 50  or  ../family1/a:myEvent" );
         task_bb->addVerify( VerifyAttr(NState::COMPLETE,1) );
      }
      //cout << theDefs;
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness(false);
   serverTestHarness.run(theDefs,ServerTestHarness::testDataDefsLocation("test_events.def"));

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()
