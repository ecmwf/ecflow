//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $ 
//
// Copyright 2009-2019 ECMWF.
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

#include "TestFixture.hpp"
#include "ServerTestHarness.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "DurationTimer.hpp"
#include "PrintStyle.hpp"
#include "ClientToServerCmd.hpp"
#include "AssertTimer.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( TestSuite  )

//static void dump_suites_and_handles(ClientInvoker& theClient, const std::string& title)
//{
//   std::cout << title;
//   TestFixture::client().suites();
//   {
//      const std::vector<std::string>& suites =  TestFixture::client().server_reply().get_string_vec();
//      BOOST_FOREACH(const std::string& suite, suites) { std::cout << "\n---> " << suite; }
//      std::cout << "\n";
//
//      const std::vector<std::pair<unsigned int, std::vector<std::string> > >& handles = TestFixture::client().server_reply().get_client_handle_suites();
//      std::pair<unsigned int, std::vector<std::string> > int_str_pair;
//      for(size_t i =0; i < handles.size(); i++) {
//          std::cout << "handle: " << handles[i].first << " : ";
//          BOOST_FOREACH(const std::string& suite, handles[i].second) {
//            std::cout << suite << " ";
//         }
//         std::cout << "\n";
//      }
//   }
//   std::cout << "\n";
//}

BOOST_AUTO_TEST_CASE( test_handle )
{
   DurationTimer timer;
   cout << "Test:: ...test_handle " << flush;
   TestClean clean_at_start_and_end;

   Defs theDefs; {
      for(int s = 0; s < 7; s++) {
         suite_ptr suite = theDefs.add_suite("s" + boost::lexical_cast<std::string>(s));
         suite->addDefStatus(DState::SUSPENDED); // NO NEED to run jobs for this test:
         for(int t = 0; t < 2; t++) { suite->add_task("t" + boost::lexical_cast<std::string>(t)); }
      }
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                        ServerTestHarness::testDataDefsLocation("test_handle.def"),
                        1 /*timeout*/,
                        false/* don't wait for test to finish */);

   std::vector<std::string> suites_s0_s1_s2; suites_s0_s1_s2.push_back("s0"); suites_s0_s1_s2.push_back("s1"); suites_s0_s1_s2.push_back("s2");
   std::vector<std::string> suites_s3_s4; suites_s3_s4.push_back("s3"); suites_s3_s4.push_back("s4");
   std::vector<std::string> suites_s0_s1_s2_s3_s4; suites_s0_s1_s2_s3_s4.push_back("s0"); suites_s0_s1_s2_s3_s4.push_back("s1"); suites_s0_s1_s2_s3_s4.push_back("s2"),suites_s0_s1_s2_s3_s4.push_back("s3"),suites_s0_s1_s2_s3_s4.push_back("s4");;
   std::vector<std::pair<unsigned int, std::vector<std::string> > > ch_suites;

   {
      // register suites s0,s1,s2
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);

      // get the handle associated with the registered suites
      unsigned int client_handle = TestFixture::client().server_reply().client_handle();

      // Check the sync_local() does a full sync for our handle
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().in_sync(),"Expected to be in sync after syn_local()");
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");

      // Now check the suites were registered, correctly
      TestFixture::client().ch_suites();
      ch_suites = TestFixture::client().server_reply().get_client_handle_suites();
      BOOST_CHECK_MESSAGE(!ch_suites.empty(),"Expected to have registered suites");
      BOOST_CHECK_MESSAGE(ch_suites.size() == 1,"Expected to have registered a single set");
      BOOST_CHECK_MESSAGE(ch_suites[0].first == client_handle,"Expected first client handle to be: " << client_handle << ", but found " << ch_suites[0].first);
      BOOST_CHECK_MESSAGE(ch_suites[0].second == suites_s0_s1_s2,"Expected suites s0,s1,s2");

      // Now drop the handle and Check handle was dropped
      TestFixture::client().ch_drop(client_handle);
      TestFixture::client().ch_suites();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   {
      // register suites s0,s1,s2
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);

      // get the handle associated with the registered suites
      unsigned int client_handle = TestFixture::client().server_reply().client_handle();
      BOOST_CHECK_MESSAGE(client_handle == 1,"Expected to have handle 1");

      // Check the sync_local() does a full sync for our handle. *THIS* should also clear the handle_changed flag
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().in_sync(),"Expected to be in sync after syn_local()");
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");


      // add additional suites & check suites were added to our handle. Sync_local should returna full_sync()
      std::vector<std::string> added_suites; added_suites.push_back("s3"); added_suites.push_back("s4");
      TestFixture::client().ch_add(client_handle,added_suites);

      TestFixture::client().ch_suites();
      ch_suites = TestFixture::client().server_reply().get_client_handle_suites();
      BOOST_CHECK_MESSAGE(!ch_suites.empty(),"Expected to have registered suites");
      BOOST_CHECK_MESSAGE(ch_suites.size() == 1,"Expected to have registered a single set");
      BOOST_CHECK_MESSAGE(ch_suites[0].first == client_handle,"Expected first client handle to be: " << client_handle << ", but found " << ch_suites[0].first);
      BOOST_CHECK_MESSAGE(ch_suites[0].second == suites_s0_s1_s2_s3_s4,"Expected suites s0,s1,s2,s3,s4");

      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after adding suites.");


      // remove the added suites, and check they were removed
      TestFixture::client().ch_remove(client_handle,added_suites);

      TestFixture::client().ch_suites();
      ch_suites = TestFixture::client().server_reply().get_client_handle_suites();
      BOOST_CHECK_MESSAGE(!ch_suites.empty(),"Expected to have registered suites");
      BOOST_CHECK_MESSAGE(ch_suites.size() == 1,"Expected to have registered a single set");
      BOOST_CHECK_MESSAGE(ch_suites[0].first == client_handle,"Expected first client handle to be: " << client_handle << ", but found " << ch_suites[0].first);
      BOOST_CHECK_MESSAGE(ch_suites[0].second == suites_s0_s1_s2,"Expected suites s0,s1,s2");

      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after removing suites");

      // Now drop the handle and check handle was dropped
      TestFixture::client().ch_drop(client_handle);
      TestFixture::client().ch_suites();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   {
      // register suites s0,s1,s2
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);

      // register suite s3,s4
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s3_s4);

      // get all the registered suites
      TestFixture::client().ch_suites();
      ch_suites = TestFixture::client().server_reply().get_client_handle_suites();
      BOOST_CHECK_MESSAGE(!ch_suites.empty(),"Expected to have registered suites");
      BOOST_CHECK_MESSAGE(ch_suites.size() == 2,"Expected to have 2 registered sets");
      BOOST_CHECK_MESSAGE(ch_suites[0].first == 1,"Expected first client handle to be 1, but found " << ch_suites[0].first);
      BOOST_CHECK_MESSAGE(ch_suites[0].second == suites_s0_s1_s2,"Expected suites s0,s1,s2, in first handle");
      BOOST_CHECK_MESSAGE(ch_suites[1].first == 2,"Expected second client handle to be 2, but found " << ch_suites[1].first);
      BOOST_CHECK_MESSAGE(ch_suites[1].second == suites_s3_s4,"Expected suites s3,s4 , in second handle");

      // Drop all handles
      for(size_t i = 0; i < ch_suites.size(); i++) TestFixture::client().ch_drop(ch_suites[i].first);
      TestFixture::client().ch_suites();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_CASE( test_handle_sync )
{
   DurationTimer timer;
   cout << "Test:: ...test_handle_sync " << flush;
   TestClean clean_at_start_and_end;

   Defs theDefs; {
      for(int s = 0; s < 7; s++) {
         suite_ptr suite = theDefs.add_suite("s" + boost::lexical_cast<std::string>(s));
         suite->addDefStatus(DState::SUSPENDED);
         for(int t = 0; t < 2; t++) { suite->add_task("t" + boost::lexical_cast<std::string>(t)); }
      }
   }

   // The test harness will create corresponding directory structure & default ecf file
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(theDefs,
                         ServerTestHarness::testDataDefsLocation("test_handle_sync.def"),
                         1 /*timeout*/,
                         false/* don't wait for test to finish */);

   std::vector<std::string> suites_s0_s1_s2; suites_s0_s1_s2.push_back("s0"); suites_s0_s1_s2.push_back("s1"); suites_s0_s1_s2.push_back("s2");
   std::vector<std::pair<unsigned int, std::vector<std::string> > > ch_suites;

   {
      // register suites s0,s1,s2
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);
      unsigned int client_handle = TestFixture::client().server_reply().client_handle();
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().get_news(),"Expected news after registering");
      TestFixture::client().sync_local(); // sync for any changes to get full update before test starts
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");
      BOOST_CHECK_MESSAGE(TestFixture::client().defs()->suiteVec().size() == 3,"Expected 3 suites back from sync, after registering 3 suites " << *TestFixture::client().defs());


      // make a change to a suite not in our handle, that does not cause state propagation.
      // State propagation changes the defs state. The defs state is sync regardless
      TestFixture::client().suspend("/s3");
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().get_news(),"Expected no change since suite s3 is not in our handle");

      // make a change to a suite *not* in our handle, that *does* cause state propagation.
      TestFixture::client().force("/s3/t0","aborted");
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().get_news(),"Expected change via state propagation to defs, even though s3 not in our handle");

      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental change, not a full update");


      // make a change to a suite in our handle
      TestFixture::client().force("/s0","complete");
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().get_news(),"Expected news since state changed");

      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental change, not a full update");

      TestFixture::client().ch_drop(client_handle);
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   {
      // register suites s0,s1,s2
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);
      unsigned int client_handle = TestFixture::client().server_reply().client_handle();
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().get_news(),"Expected news after registering");
      TestFixture::client().sync_local(); // sync for any changes to get full update before test starts
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");


      // make a small change to a suite *IN* our handle
      TestFixture::client().force("/s1","complete");
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental change, not a full update");

      // Change the order
      TestFixture::client().order("/s0","alpha");
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental update");

      TestFixture::client().ch_drop(client_handle);
      TestFixture::client().suites();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   {
      // register suites s0,s1,s2
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);
      unsigned int client_handle = TestFixture::client().server_reply().client_handle();
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().get_news(),"Expected news after registering");
      TestFixture::client().sync_local(); // sync for any changes to get full update **before** test starts
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");


      // make a small change to a suite *IN* our handle
      TestFixture::client().force("/s1","unknown");
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().full_sync(),"Expected incremental change, not a full update");


      // DELETE suite s2, i.e make a change that should force a *FULL* update
      TestFixture::client().delete_node("/s2",true/*force*/);
      TestFixture::client().news_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().get_news(),"Expected news after deleting node");
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected full update");


      // Check that suite s2 is STILL in our handle.
      // delete suites STAY registered until they are explicitly deleted
      TestFixture::client().ch_suites();
      ch_suites = TestFixture::client().server_reply().get_client_handle_suites();
      BOOST_CHECK_MESSAGE(ch_suites[0].second == suites_s0_s1_s2,"Expected suites s0,s1,s2, in handle");

      TestFixture::client().ch_drop(client_handle);
      TestFixture::client().ch_suites();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}


BOOST_AUTO_TEST_CASE( test_handle_add_remove_add )
{
   DurationTimer timer;
   cout << "Test:: ...test_handle_add_remove_add " << flush;
   TestClean clean_at_start_and_end;

   defs_ptr theDefs  =  Defs::create(); {
      for(int s = 0; s < 3; s++) {
         suite_ptr suite = theDefs->add_suite("s" + boost::lexical_cast<std::string>(s));
         suite->addDefStatus(DState::SUSPENDED); // NO NEED to run jobs for this test:
         for(int t = 0; t < 2; t++) { suite->add_task("t" + boost::lexical_cast<std::string>(t)); }
      }
   }

   // The test harness will create corresponding directory structure & default ecf file
   // ADD
   ServerTestHarness serverTestHarness;
   serverTestHarness.run(*theDefs,
                         ServerTestHarness::testDataDefsLocation("test_handle_sync.def"),
                         1 /*timeout*/,
                         false/* don't wait for test to finish */);

   TestFixture::client().set_throw_on_error( true );
   std::vector<std::string> suites_s0_s1_s2; suites_s0_s1_s2.push_back("s0"); suites_s0_s1_s2.push_back("s1"); suites_s0_s1_s2.push_back("s2");

   {
      // register suites s0
      TestFixture::client().ch_register(false/*add new suites to handle*/,suites_s0_s1_s2);
      unsigned int client_handle = TestFixture::client().server_reply().client_handle();
      BOOST_CHECK_MESSAGE(client_handle == 1 ,"Expected handle of value 1 but found " << client_handle );

      // Get the registered suites
      TestFixture::client().ch_suites();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have registered suites");


      // Check the sync_local() does a full sync for our handle
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().in_sync(),"Expected to be in sync after syn_local()");
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after registering");
      BOOST_CHECK_MESSAGE(TestFixture::client().defs()->suiteVec().size() == 3,"Expected 3 suites back from sync " << *TestFixture::client().defs());


      // DELETE suites. They should stay *registered*
      TestFixture::client().delete_node("/s0");
      TestFixture::client().delete_node("/s1");
      TestFixture::client().delete_node("/s2");
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() after deleting suite");
      BOOST_CHECK_MESSAGE(TestFixture::client().defs()->suiteVec().size() == 0,"Expected 0 suites back from sync " << *TestFixture::client().defs());

      // Check suites are still registered. Only explicit drop can remove registered suites
      TestFixture::client().ch_suites();
      BOOST_CHECK_MESSAGE(!TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have registered suites");


      // RELOAD the defs, which includes s0,s1,s2:: THIS SHOULD GET RE_ADDED TO OUR EXISTING HANDLE's
      // The handle references to the suites should get *refreshed*
      TestFixture::client().load(theDefs);
      TestFixture::client().sync_local();
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().full_sync(),"Expected a full_sync() since client handle should be refreshed with new suite_pts");
      BOOST_CHECK_MESSAGE(TestFixture::client().defs()->suiteVec().size() == 3,"Expected 3 suites back from sync " << *TestFixture::client().defs());

      TestFixture::client().ch_drop(client_handle);
      BOOST_CHECK_MESSAGE(TestFixture::client().server_reply().get_client_handle_suites().empty(),"Expected to have no registered suites");
   }

   cout << timer.duration() << " update-calendar-count(" << serverTestHarness.serverUpdateCalendarCount() << ")\n";
}

BOOST_AUTO_TEST_SUITE_END()
