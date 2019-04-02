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
   //    repeat date YMD 20090916 20200916
   //    family f
   //          edit var2 var2
   //          task t1
   //              edit var1 var1
   //              meter m 0 100 100
   //              event event
   //              trigger t2 == complete
   //          task t2
   //              label name value new_value
   //              label name2 value2
   //    endfamily
   //    task task
   // endsuite
   Defs defs;
   string suite_f_t1 = "suite/f/t1";
   task_ptr t1 = Task::create("t1");
   task_ptr t2;
   task_ptr task = Task::create("task");
   string suite_task = "suite/task";
   std::string meter_name = "m";
   std::string event_name = "event";
   std::string label_name = "name";
   {
      t1->addMeter( Meter(meter_name,0,100,100));
      t1->addEvent( Event(event_name));
      t1->add_variable("var1","var1");

      suite_ptr s = defs.add_suite("suite");
      s->addRepeat( RepeatDate("YMD",20090916,20200916,1) );
      family_ptr f = s->add_family("f");
      f->add_variable("var2","var2");
      f->addTask( t1 );
      t2 = f->add_task("t2");
      t2->add_label(label_name,"value","");
      s->addTask(task);
   }

   defs.beginAll(); // this will clear label new_value


   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("state","/suite/f/t11","","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("dstate","/suite/f/t11","","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite/f/t1","eventxx","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite",event_name,"/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("event","xxxx/f/t1",event_name,"/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("meter","/suite/f/t1","meterxx","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("meter","/suite",meter_name,"/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("label","/suite/f/t1",label_name,"/suite/f/t1"))); // no label on t1
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("label","/suite/f/t2","fred","/suite/f/t2")));     // wrong label name
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite","1 == ","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","1 == ","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","XXXX","/suite/f/t1")));
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("xxxxx","/suite/f/t1",event_name,"/suite/f/t1")));

   // t3 does not exist
   TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","t3 == complete","/suite/f/t1")));

   // QueryCmd is read only, hence change numbers should not change
   std::string res;
   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("state","/suite","","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "queued","expected query state to return queued but found: " << res);

   res  = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("dstate","/suite/f","","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "queued","expected query state to return queued but found: " << res);

   // Note: we pick a task outside of a repeat, since setting a task to complete, inside a repeat will cause it to requeue
   // Avoid using ForceCmd to avoid side affects
   std::vector<std::string> states = NState::allStates();
   for(const auto & state : states) {
      task->setStateOnly( NState::toState(state));
      res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("state", task->absNodePath(),"",task->absNodePath())), false);
      BOOST_CHECK_MESSAGE(res == state ,"expected query state to return " << state << " but found: " << res);
   }

   TestHelper::invokeRequest(&defs,Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND, task->absNodePath())));
   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("dstate", task->absNodePath(),"",task->absNodePath())), false);
   BOOST_CHECK_MESSAGE(res == "suspended" ,"expected query state to return suspend but found: " << res);


   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("event","/suite/f/t1",event_name,"/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "clear","expected query event to return clear but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("meter","/suite/f/t1",meter_name,"/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "0","expected query meter to return 0 but found: " << res);


   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("label","/suite/f/t2",label_name,"/suite/f/t2")), false);
   BOOST_CHECK_MESSAGE(res == "value","expected query label to return 'value' but found: " << res);
   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new LabelCmd("/suite/f/t2",Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,label_name,"new_value")));
   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("label","/suite/f/t2",label_name,"/suite/f/t2")), false);
   BOOST_CHECK_MESSAGE(res == "new_value","expected query label to return 'new_value' but found: " << res);


   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","t2 == complete","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "false","expected query trigger to return false but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","1 == 1","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "true","expected query trigger to return true but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","/suite/f/t1:var1 == 0","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "true","expected query trigger to return true but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","/suite/f:var2 == 0","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "true","expected query trigger to return true but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","/suite/f/t1:m == 0","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "true","expected query trigger to return true but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite/f/t1","/suite/f/t1:event","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "false","expected query trigger to return false but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("trigger","/suite","/suite:YMD == 20090916","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "true","expected query trigger to return true but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","var1","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "var1","expected query variable to return var2 but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","var2","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "var2","expected query variable to return var2 but found: " << res);

   res = TestHelper::invokeRequest(&defs,Cmd_ptr( new QueryCmd("variable","/suite/f/t1","YMD","/suite/f/t1")), false);
   BOOST_CHECK_MESSAGE(res == "20090916","expected query variable to return 20090916 but found: " << res);

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
