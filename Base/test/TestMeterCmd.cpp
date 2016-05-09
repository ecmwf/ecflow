 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <iostream>
#include <fstream>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestHelper.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_meter_cmd )
{
   cout << "Base:: ...test_meter_cmd\n";
   // Create the defs file.
   // suite suite
   //    family f
   //          task t1
   //              meter m 0 100 100
   //    endfamily
   // endsuite
   Defs defs;
   string suite_f_t1 = "suite/f/t1";
   task_ptr t1 = Task::create("t1");
   std::string meter_name = "m";
   {
      t1->addMeter( Meter(meter_name,0,100,100));
      suite_ptr s = Suite::create("suite");
      family_ptr f = Family::create("f");
      f->addTask( t1 );
      s->addFamily(f);
      defs.addSuite( s );
   }

   // Meter which doesn't exist should be silently ignore
   int meter_value = 10;
   TestHelper::invokeRequest(&defs,
                             Cmd_ptr( new MeterCmd(suite_f_t1,
                                                   Submittable::DUMMY_JOBS_PASSWORD(),
                                                   Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),
                                                   1,"FRED",meter_value)),
                             false  /* expect change number not to change */
   );

   /// Test setting meter value
   TestHelper::invokeRequest(&defs,Cmd_ptr( new MeterCmd(suite_f_t1,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,meter_name,meter_value)));
   const Meter& the_meter = t1->findMeter(meter_name);
   BOOST_CHECK_MESSAGE( !the_meter.empty(),  "Meter not found");
   BOOST_CHECK_MESSAGE( the_meter.value() == meter_value,  "Expected meter value " << meter_value << " but found " << the_meter.value());


   /// Set a meter value less than the current value.
   meter_value = 9;
   TestHelper::invokeRequest(&defs,Cmd_ptr( new MeterCmd(suite_f_t1,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,meter_name,meter_value)));
   BOOST_CHECK_MESSAGE( the_meter.value() == meter_value,  "Expected meter value " << meter_value << " but found " << the_meter.value());

   /// Set a meter value greater than max meter, this should be ignored
   /// Avoid failing task, if meter,  value is out of range. Just log a warning message
   meter_value = 2000;
   TestHelper::invokeRequest(&defs,Cmd_ptr( new MeterCmd(suite_f_t1,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,meter_name,meter_value))
                             ,false  /* expect change number not to change */);
   BOOST_CHECK_MESSAGE( the_meter.value() == 9,  "Expected meter value 9 but found " << the_meter.value());


   /// Set to valid value
   meter_value = 20;
   TestHelper::invokeRequest(&defs,Cmd_ptr( new MeterCmd(suite_f_t1,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,meter_name,meter_value)));
   BOOST_CHECK_MESSAGE( the_meter.value() == meter_value,  "Expected meter value " << meter_value << " but found " << the_meter.value());

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

