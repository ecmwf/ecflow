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
   std::vector<std::string> q1_items; q1_items.push_back("s1"); q1_items.push_back("s2"); q1_items.push_back("s3");
   QueueAttr q1("q1",q1_items);
   s->add_queue(q1);
   const QueueAttr& q1_ref = s->find_queue("q1");
   BOOST_REQUIRE_MESSAGE( !q1_ref.empty(),  "queue not found");
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 0,  "Expected to 0 index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "s1",  "Expected to s1 but found " << q1_ref.value());

   family_ptr f = s->add_family("f");
   std::vector<std::string> q2_items; q2_items.push_back("f1"); q2_items.push_back("f2"); q2_items.push_back("f3");
   QueueAttr q2("q2",q2_items);
   f->add_queue(q2);
   const QueueAttr& q2_ref = f->find_queue("q2");
   BOOST_REQUIRE_MESSAGE( !q2_ref.empty(),  "queue not found");
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 0,  "Expected to 0 index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "f1",  "Expected to f1 but found " << q2_ref.value());

   task_ptr t = f->add_task("t");
   std::vector<std::string> q3_items; q3_items.push_back("t1"); q3_items.push_back("t2"); q3_items.push_back("t3");
   QueueAttr q3("q3",q3_items);
   t->add_queue(q3);
   const QueueAttr& q3_ref = t->find_queue("q3");
   BOOST_REQUIRE_MESSAGE( !q3_ref.empty(),  "queue not found");
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 0,  "Expected to 0 index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "t1",  "Expected to t1 but found " << q3_ref.value());

   //cout << defs;

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Update q1, this on the suite. hence we should find it up the hierarchy
   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1")));
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 1,  "Expected 1 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "s2",  "Expected to s2 but found " << q1_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1")));
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 2,  "Expected 2 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "s3",  "Expected to s3 but found " << q1_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1")));
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 3,  "Expected 3 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q1_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
                                                          "q1")));
   BOOST_CHECK_MESSAGE( q1_ref.index_or_value() == 4,  "Expected 4 for index but found " << q1_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q1_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q1_ref.value());


   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Update q2, this on the family, In this we sill specify path to the queue
   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","/suite/f")));
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 1,  "Expected 1 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "f2",  "Expected to f2 but found " << q2_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","/suite/f")));
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 2,  "Expected 2 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "f3",  "Expected to f3 but found " << q2_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","/suite/f")));
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 3,  "Expected 3 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q2_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q2","/suite/f")));
   BOOST_CHECK_MESSAGE( q2_ref.index_or_value() == 4,  "Expected 4 for index but found " << q2_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q2_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q2_ref.value());

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Update q3, this on the task
   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3")));
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 1,  "Expected 1 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "t2",  "Expected to t2 but found " << q3_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3")));
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 2,  "Expected 2 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "t3",  "Expected to t3 but found " << q3_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3")));
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 3,  "Expected 3 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q3_ref.value());

   TestHelper::invokeRequest(&defs, Cmd_ptr( new QueueCmd(suite_f_t,Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,
         "q3")));
   BOOST_CHECK_MESSAGE( q3_ref.index_or_value() == 4,  "Expected 4 for index but found " << q3_ref.index_or_value());
   BOOST_CHECK_MESSAGE( q3_ref.value() == "<NULL>",  "Expected to <NULL> but found " << q3_ref.value());

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
