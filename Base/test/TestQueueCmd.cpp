 //============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $
//
// Copyright 2009-2019 ECMWF.
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

BOOST_AUTO_TEST_CASE( test_queue_cmd )
{
   cout << "Base:: ...test_queue_cmd\n";
   // Create the defs file.
   // suite suite
   //    queue q1 s1 s2 s3
   //    family f
   //          queue q2 f1 f2 f3
   //          task t
   //             queue q3 t1 t2 t3
   //    endfamily
   // endsuite
   Defs defs;
   string suite_f_t = "/suite/f/t";

   suite_ptr s = defs.add_suite("suite");
   QueueAttr q1("q1", {"s1","s2","s3"});
   s->add_queue(q1);
   QueueAttr& q1_ref = s->findQueue("q1");
   BOOST_REQUIRE_MESSAGE( !q1_ref.empty(),  "queue not found");
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 0,  "Expected to 0 index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "s1",  "Expected to s1 but found " << q1_ref.value());

   family_ptr f = s->add_family("f");
   QueueAttr q2("q2", {"f1","f2","f3"});
   f->add_queue(q2);
   QueueAttr& q2_ref = f->findQueue("q2");
   BOOST_REQUIRE_MESSAGE( !q2_ref.empty(),  "queue not found");
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 0,  "Expected to 0 index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "f1",  "Expected to f1 but found " << q2_ref.value());

   task_ptr t = f->add_task("t");
   QueueAttr q3("q3",{"t1","t2","t3"} );
   t->add_queue(q3);
   QueueAttr& q3_ref = t->findQueue("q3");
   BOOST_REQUIRE_MESSAGE( !q3_ref.empty(),  "queue not found");
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 0,  "Expected to 0 index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "t1",  "Expected to t1 but found " << q3_ref.value());

   //cout << defs;

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Update q1, this on the suite. hence we should find it up the hierarchy
   std::string step = TestHelper::invokeRequest(&defs,
                             Cmd_ptr( new QueueCmd(
                                      suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1","active")));
   BOOST_CHECK_MESSAGE( step == "s1", "Expected step s1 but found " << step);
   BOOST_CHECK_MESSAGE( NState::ACTIVE == q1_ref.state(step), "Expected ACTIVE step but found " << NState::toString(q1_ref.state(step)));
   q1_ref.aborted(step);
   BOOST_CHECK_MESSAGE( NState::ABORTED == q1_ref.state(step), "Expected ABORTED step but found " << NState::toString(q1_ref.state(step)));
   q1_ref.complete(step);
   BOOST_CHECK_MESSAGE( NState::COMPLETE == q1_ref.state(step), "Expected COMPLETE step but found " << NState::toString(q1_ref.state(step)));
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 1,  "Expected 1 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "s2",  "Expected s2 for value but found " << q1_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1","active")));
   BOOST_CHECK_MESSAGE( step == "s2", "Expected step s2 but found " << step);
   BOOST_CHECK_MESSAGE( NState::ACTIVE == q1_ref.state(step), "Expected ACTIVE step but found " << NState::toString(q1_ref.state(step)));
   q1_ref.aborted(step);
   BOOST_CHECK_MESSAGE( NState::ABORTED == q1_ref.state(step), "Expected ABORTED step but found " << NState::toString(q1_ref.state(step)));
   BOOST_CHECK_MESSAGE( q1_ref.no_of_aborted() == "1", "Expected  1 aborted step but found " << q1_ref.no_of_aborted() );
   q1_ref.complete(step);
   BOOST_CHECK_MESSAGE( q1_ref.no_of_aborted() == "", "Expected *NO* aborted step but found " << q1_ref.no_of_aborted() );
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 2,  "Expected 2 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "s3",  "Expected to s3 but found " << q1_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1","active")));
   BOOST_CHECK_MESSAGE( step == "s3", "Expected step s3 but found " << step);
   BOOST_CHECK_MESSAGE( NState::ACTIVE == q1_ref.state(step), "Expected ACTIVE step but found " << NState::toString(q1_ref.state(step)));
   q1_ref.aborted(step);
   BOOST_CHECK_MESSAGE( NState::ABORTED == q1_ref.state(step), "Expected ABORTED step but found " << NState::toString(q1_ref.state(step)));
   q1_ref.complete(step);
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 3,  "Expected 3 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q1_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1","active")),false);
   BOOST_CHECK_MESSAGE( step == "<NULL>", "Expected step <NULL> but found " << step);
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 3,  "Expected 3 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q1_ref.value());

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Update q2, this on the family, In this we sill specify path to the queue
   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","active","","/suite/f")));
   BOOST_CHECK_MESSAGE( step == "f1", "Expected step f1 but found " << step);
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 1,  "Expected 1 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "f2",  "Expected to f2 but found " << q2_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","active","","/suite/f")));
   BOOST_CHECK_MESSAGE( step == "f2", "Expected step f2 but found " << step);
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 2,  "Expected 2 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "f3",  "Expected to f3 but found " << q2_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","active","","/suite/f")));
   BOOST_CHECK_MESSAGE( step == "f3", "Expected step f3 but found " << step);
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 3,  "Expected 3 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q2_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","active","","/suite/f")),false);
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 3,  "Expected 3 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q2_ref.value());

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Update q3, this on the task
   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3","active")));
   BOOST_CHECK_MESSAGE( step == "t1", "Expected step t1 but found " << step);
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 1,  "Expected 1 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "t2",  "Expected to t2 but found " << q3_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3","active")));
   BOOST_CHECK_MESSAGE( step == "t2", "Expected step t2 but found " << step);
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 2,  "Expected 2 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "t3",  "Expected to t3 but found " << q3_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3","active")));
   BOOST_CHECK_MESSAGE( step == "t3", "Expected step t3 but found " << step);
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 3,  "Expected 3 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q3_ref.value());

   step = TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3","active")),false);
   BOOST_CHECK_MESSAGE( step == "<NULL>", "Expected step <NULL> but found " << step);
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 3,  "Expected 3 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q3_ref.value());

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
