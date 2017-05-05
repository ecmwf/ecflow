//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This is used to INVOKE a SINGLE test.
//               Making it easier for Easier for debugging and development
//============================================================================
#include <iostream>
#include <limits> // for std::numeric_limits<int>::max()

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
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

std::vector<std::string> to_string_vec(const std::vector<suite_ptr>& sv) {
   std::vector<std::string> r; r.reserve(sv.size());
   for(size_t i = 0; i < sv.size(); i++) r.push_back(sv[i]->name());
   return r;
}

std::vector<std::string> to_string_vec(const std::vector<node_ptr>& sv) {
   std::vector<std::string> r; r.reserve(sv.size());
   for(size_t i = 0; i < sv.size(); i++) r.push_back(sv[i]->name());
   return r;
}

void test_ordering() {

   TestFixture::client().set_throw_on_error(true);
   std::vector<std::string> str_a_b_c; str_a_b_c.push_back("a"); str_a_b_c.push_back("b"); str_a_b_c.push_back("c");
   std::vector<std::string> str_c_b_a; str_c_b_a.push_back("c"); str_c_b_a.push_back("b"); str_c_b_a.push_back("a");
   std::vector<std::string> str_b_c_a; str_b_c_a.push_back("b"); str_b_c_a.push_back("c"); str_b_c_a.push_back("a");
   std::vector<std::string> str_b_a_c; str_b_a_c.push_back("b"); str_b_a_c.push_back("a"); str_b_a_c.push_back("c");
   std::vector<std::string> str_a_c_b; str_a_c_b.push_back("a"); str_a_c_b.push_back("c"); str_a_c_b.push_back("b");
   TestFixture::client().sync_local();  // First sync_local will do a full sync
   {
      // TEST SUITE ORDERING
      TestFixture::client().order("/a",NOrder::toString(NOrder::ORDER));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()) ==  str_c_b_a, "Order not as expected");


      TestFixture::client().order("/a",NOrder::toString(NOrder::ALPHA));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()) ==  str_a_b_c, "Order not as expected");


      TestFixture::client().order("/a",NOrder::toString(NOrder::BOTTOM));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()) ==  str_b_c_a, "Order not as expected");

      TestFixture::client().order("/a",NOrder::toString(NOrder::ALPHA));
      TestFixture::client().order("/a",NOrder::toString(NOrder::DOWN));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()) ==  str_b_a_c, "Order not as expected");

      TestFixture::client().order("/a",NOrder::toString(NOrder::ALPHA));
      TestFixture::client().order("/c",NOrder::toString(NOrder::UP));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()) ==  str_a_c_b, "Order not as expected");
   }
   {
      // TEST FAMILY ordering
      TestFixture::client().order("/a/a",NOrder::toString(NOrder::ORDER));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()[0]->nodeVec()) ==  str_c_b_a, "Order not as expected");


      TestFixture::client().order("/a/a",NOrder::toString(NOrder::ALPHA));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()[0]->nodeVec()) ==  str_a_b_c, "Order not as expected");


      TestFixture::client().order("/a/a",NOrder::toString(NOrder::BOTTOM));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()[0]->nodeVec()) ==  str_b_c_a, "Order not as expected");

      TestFixture::client().order("/a/a",NOrder::toString(NOrder::ALPHA));
      TestFixture::client().order("/a/a",NOrder::toString(NOrder::DOWN));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()[0]->nodeVec()) ==  str_b_a_c, "Order not as expected");

      TestFixture::client().order("/a/a",NOrder::toString(NOrder::ALPHA));
      TestFixture::client().order("/a/c",NOrder::toString(NOrder::UP));
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental sync");
      BOOST_CHECK_MESSAGE(to_string_vec(TestFixture::client().defs()->suiteVec()[0]->nodeVec()) ==  str_a_c_b, "Order not as expected");
   }
}

BOOST_AUTO_TEST_CASE( test_change_order )
{
   DurationTimer timer;
   cout << "Test:: ...test_change_order " << flush;
   TestClean clean_at_start_and_end;

   /// NOT: We *DONT* need to run the jobs for this TEST
   std::vector<std::string> str_a_b_c; str_a_b_c.push_back("a"); str_a_b_c.push_back("b"); str_a_b_c.push_back("c");
   Defs theDefs; {
      for(size_t s = 0; s < str_a_b_c.size(); s++) {
         suite_ptr suite = theDefs.add_suite(str_a_b_c[s]);
         suite->addDefStatus(DState::SUSPENDED); // NO NEED to run jobs for this test:
         for(size_t f = 0; f < str_a_b_c.size(); f++) {
            family_ptr fam = suite->add_family(str_a_b_c[f]);
            for(size_t t = 0; t < str_a_b_c.size(); t++) {
               fam->add_task(str_a_b_c[t]);
            }
         }
      }
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
            ServerTestHarness::testDataDefsLocation("test_change_order.def"),
            1 /*timeout*/,
            false/* don't wait for test to finish */);


   test_ordering();

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}


BOOST_AUTO_TEST_CASE( test_handle_change_order )
{
   DurationTimer timer;
   cout << "Test:: ...test_handle_change_order " << flush;
   TestClean clean_at_start_and_end;

   std::vector<std::string> str_a_b_c; str_a_b_c.push_back("a"); str_a_b_c.push_back("b"); str_a_b_c.push_back("c");
   std::vector<std::string> suite_a_b_c_d_e; suite_a_b_c_d_e.push_back("a"); suite_a_b_c_d_e.push_back("b"); suite_a_b_c_d_e.push_back("c");
   suite_a_b_c_d_e.push_back("d"); suite_a_b_c_d_e.push_back("e");
   Defs theDefs; {
      for(size_t s = 0; s < suite_a_b_c_d_e.size(); s++) {
         suite_ptr suite = theDefs.add_suite(suite_a_b_c_d_e[s]);
         suite->addDefStatus(DState::SUSPENDED); // NO NEED to run jobs for this test:
         for(size_t f = 0; f < str_a_b_c.size(); f++) {
            family_ptr fam = suite->add_family(str_a_b_c[f]);
            for(size_t t = 0; t < str_a_b_c.size(); t++) {
               fam->add_task(str_a_b_c[t]);
            }
         }
      }
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
            ServerTestHarness::testDataDefsLocation("test_change_order.def"),
            1 /*timeout*/,
            false/* don't wait for test to finish */);


   TestFixture::client().set_throw_on_error( true );
   TestFixture::client().sync_local();  // First sync_local will do a full sync
   {
      // register suites a,b,c
      std::vector<std::string> suites_a_b_c; suites_a_b_c.push_back("a"); suites_a_b_c.push_back("b"); suites_a_b_c.push_back("c");
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_a_b_c);

      // Check the sync_local() does a full sync for our handle
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().in_sync(),"Expected to be in sync after syn_local()");
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");
      BOOST_CHECK_MESSAGE(TestFixture::client().defs()->suiteVec().size() == 3,"Expected sync to return 3 suites.");
   }

   // Do same test, this time sync_local will only return suite a,b,c (i.e suite d,e not registered and hence left out)
   // Ording should still be applied to the whole suite.
   test_ordering();

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}
BOOST_AUTO_TEST_SUITE_END()
