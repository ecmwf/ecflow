//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $ 
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

/// Simulate definition files that are created on then fly. This us to validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

BOOST_AUTO_TEST_CASE( test_autocancel_ast_node_reset )
{
   cout << "Simulator:: ...test_autocancel_ast_node_reset\n";

   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autocancel at hour resolution
   Defs theDefs;
   {
      std::auto_ptr< Suite > suite( new Suite( "s1" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );

      std::auto_ptr< Task > task( new Task( "t" ) );
      task->add_trigger( "/s2/family/t == complete" );
      fam->addTask( task );

      std::auto_ptr< Task > task2( new Task( "t2" ) );
      task2->add_trigger( "/s3/family/t == complete" );
      task2->add_complete( "/s2/family == complete" );
      fam->addTask( task2 );

      std::auto_ptr< Task > task3( new Task( "t3" ) );
      task3->add_trigger( "/s3 == complete" );
      task3->add_complete( "/s2 == complete");
      fam->addTask( task3 );

      suite->addFamily( fam );
      theDefs.addSuite( suite );
      //		cout << theDefs << "\n";
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "s2" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      fam->addTask( task );
      suite->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(1,0), false));
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "s3" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      fam->addTask( task );
      suite->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(1,0), false));
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }

   // Check number of AST nodes. The AST should be created on the fly
   std::set<Node*> theSet;
   theDefs.getAllAstNodes(theSet);
   BOOST_CHECK_MESSAGE(theSet.size() == 5,"Expected to have 5 AST nodes in trigger/complete expressions but found " << theSet.size());

   // Run the simulator
   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_autocancel_ast_node_reset.def"), errorMsg),errorMsg);

   // Auto cancel should delete suite s2 and s3, leaving one suite i.e s1
   BOOST_CHECK_MESSAGE(theDefs.suiteVec().size() == 1,"Expected to have 1 suites but found " << theDefs.suiteVec().size());

   // The references to nodes in suites s2, s3 should have been cleared in suite s1
   {
      std::set<Node*> theSet;
      theDefs.getAllAstNodes(theSet);
      BOOST_CHECK_MESSAGE(theSet.size() == 0,"Expected to have 0 AST nodes in trigger/complete expressions but found " << theSet.size());
   }
}


BOOST_AUTO_TEST_CASE( test_autocancel_suite )
{
   cout << "Simulator:: ...test_autocancel_suite\n";

   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autocancel at hour resolution
   Defs theDefs;
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_10_hours_relative" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      suite->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(10,0), true));
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_1_hours_real" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      suite->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(1,0), false));
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_1_day_relative" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      suite->addAutoCancel( ecf::AutoCancelAttr(1) );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
      //    	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_autocancel_suite.def"), errorMsg),errorMsg);

   // make sure autocancel deletes the suite.
   BOOST_CHECK_MESSAGE(theDefs.suiteVec().size() == 0,"Expected to have 0 suites but found " << theDefs.suiteVec().size());
}

BOOST_AUTO_TEST_CASE( test_autocancel_family_and_task )
{
   cout << "Simulator:: ...test_autocancel_family_and_task\n";

   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autocancel at hour resolution
   Defs theDefs;
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_9_10_hours_relative" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      fam->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(10,0), true));
      std::auto_ptr< Task > task( new Task( "t" ) );
      task->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(9,0), true));
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
      //		cout << theDefs << "\n";
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_1_2_hours_real" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      fam->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(2,0), false));
      std::auto_ptr< Task > task( new Task( "t" ) );
      task->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(1,0), false));
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_1_2_day_relative" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      task->addAutoCancel( ecf::AutoCancelAttr(1) );
      fam->addTask( task );
      fam->addAutoCancel( ecf::AutoCancelAttr(2) );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
      //    	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_autocancel_family_and_task.def"), errorMsg),errorMsg);

   // make sure autocancel deletes the families.
   std::vector<Family*> famVec;
   theDefs.getAllFamilies(famVec);
   BOOST_CHECK_MESSAGE(famVec.size() == 0,"Expected to have 0 families but found " << famVec.size());
}

BOOST_AUTO_TEST_CASE( test_autocancel_task )
{
   cout << "Simulator:: ...test_autocancel_task\n";

   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autocancel at hour resolution
   Defs theDefs;
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_10_hours_relative" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      task->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(10,0), true));
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_1_hours_real" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      task->addAutoCancel( ecf::AutoCancelAttr( ecf::TimeSlot(1,0), false));
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
   }
   {
      std::auto_ptr< Suite > suite( new Suite( "test_autocancel_1_day_relative" ) );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite->addClock( clockAttr );
      std::auto_ptr< Family > fam( new Family( "family" ) );
      std::auto_ptr< Task > task( new Task( "t" ) );
      task->addAutoCancel( ecf::AutoCancelAttr(1) );
      fam->addTask( task );
      suite->addFamily( fam );
      theDefs.addSuite( suite );
      //    	cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_autocancel_task.def"), errorMsg),errorMsg);

   // make sure autocancel deletes the tasks and leaves families intact.
   std::vector<task_ptr> vec;
   theDefs.get_all_tasks(vec);

   std::vector<Family*> famVec;
   theDefs.getAllFamilies(famVec);

   BOOST_CHECK_MESSAGE(famVec.size() == 3,"Expected to have 3 families but found " << famVec.size());
   BOOST_CHECK_MESSAGE(vec.size() == 0,"Expected to have 0 tasks but found " << vec.size());
}

BOOST_AUTO_TEST_SUITE_END()

