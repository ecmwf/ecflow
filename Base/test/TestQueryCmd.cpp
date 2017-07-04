 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>
#include <iostream>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestHelper.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_query_cmd )
{
   cout << "Base:: ...test_query_cmd\n";
   // Create the defs file.
   // suite suite
   //    family f
   //          task t1
   //              meter m 0 100 100
   //              event event
   //              trigger t2 == complete
   //          task t2
   //    endfamily
   // endsuite
   Defs defs;
   string suite_f_t1 = "suite/f/t1";
   task_ptr t1 = Task::create("t1");
   std::string meter_name = "m";
   std::string event_name = "event";
   {
      t1->addMeter( Meter(meter_name,0,100,100));
      t1->addEvent( Event(event_name));
      t1->add_variable("var1","var1");

      suite_ptr s = defs.add_suite("suite");
      s->addRepeat( RepeatDate("YMD",20090916,20090916,1) );
      family_ptr f = s->add_family("f");
      f->add_variable("var2","var2");
      f->addTask( t1 );
      f->add_task("t2");
   }

   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite/f/t1","eventxx","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite",event_name,"/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","xxxx/f/t1",event_name,"/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite/f/t1",event_name,"xxx/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("meter","/suite/f/t1","meterxx","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("meter","/suite",meter_name,"/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite","1 == ","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","1 == ","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","XXXX","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("xxxxx","/suite/f/t1",event_name,"/suite/f/t1")));

   // t3 does not exist
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","t3 == complete","/suite/f/t1")));

   // QueryCmd is read only, hence change numbers should not change
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite/f/t1",event_name,"/suite/f/t1")), false);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("meter","/suite/f/t1",meter_name,"/suite/f/t1")), false);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","t2 == complete","/suite/f/t1")), false);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","1 == 1","/suite/f/t1")), false);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","var1","/suite/f/t1")), false);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","var2","/suite/f/t1")), false);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","YMD","/suite/f/t1")), false);

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

