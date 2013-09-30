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
#include <fstream>
#include <stdlib.h>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"

#include "Simulator.hpp"

#include "File.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestUtil.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;


BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

/// Use this class to test single simulation of definition file that we want to add
/// to Test Simulator. This is a separate exe

BOOST_AUTO_TEST_CASE( test_analysys )
{
   cout << "Simulator:: ...test_analysys\n";
   //suite suite
   //	family family
   //   	task t1
   //          trigger t2 == complete
   //   	task t2
   //          trigger t1 == complete
   //  	endfamily
   //endsuite

   // This simulation is expected to fail, since we have a deadlock/ race condition
   // It will prodice a defs.depth and defs.flat files. Make sure to remove them
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_analysys");
      family_ptr fam = suite->add_family("family");

      task_ptr task1 = fam->add_task("t1");
      task1->add_trigger( "t2 == complete" );

      task_ptr task2 = fam->add_task("t2");
      task2->add_trigger( "t1 == complete" );

      //		cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(!simulator.run(theDefs, TestUtil::testDataLocation("test_analysys.def") , errorMsg),errorMsg);

   //	cout << theDefs << "\n";
   boost::filesystem::remove("defs.depth");
   boost::filesystem::remove("defs.flat");

   /// Destroy singleton's to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

