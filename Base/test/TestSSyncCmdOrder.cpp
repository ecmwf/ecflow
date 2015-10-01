//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
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
#include <boost/test/unit_test.hpp>
#include <boost/function.hpp>

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "MyDefsFixture.hpp"
#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "SSyncCmd.hpp"
#include "Ecf.hpp"
#include "NodeFwd.hpp"
#include "SuiteChanged.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

// The client handle commands do not change state & modify change number, hence need to bypass these checks
static bool bypass_state_modify_change_check = false;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

/// define a function which returns nothing, and takes a defs_ptr parameter
typedef boost::function<void (defs_ptr)> defs_change_cmd;

template <typename T>
static std::vector<std::string> toStrVec(const std::vector<T>& vec)
{
   std::vector<std::string> retVec; retVec.reserve(vec.size());
   BOOST_FOREACH(T s, vec) { retVec.push_back(s->name()); }
   return retVec;
}

static std::string toString(const std::vector<std::string>& c)
{
   std::stringstream ss;
   std::copy (c.begin(), c.end(), std::ostream_iterator <std::string> (ss, ", "));
   return ss.str();
}

static std::vector<std::string> vector_abcd() {
    std::vector<std::string> names; names.reserve(4);
    names.push_back("a"); names.push_back("b"); names.push_back("c"); names.push_back("d");
    return names;
}
static std::vector<std::string> vector_dcba() {
    std::vector<std::string> names; names.reserve(4);
    names.push_back("d"); names.push_back("c"); names.push_back("b"); names.push_back("a");
    return names;
}

static defs_ptr create_defs()
{
   std::vector<std::string> names = vector_dcba();
   defs_ptr defs =  Defs::create();
   for(size_t j = 0; j <names.size(); j++)  {
      suite_ptr s = defs->add_suite( names[j] );
      if ( 0 == j) { // only add family to first suite
         for(size_t k = 0; k <names.size(); k++)  {
            family_ptr f = s->add_family(names[k]);
            for(size_t l = 0; l <names.size(); l++)  {
               task_ptr t = f->add_task(names[l]);
//               t->add_alias_only(); // alias0
//               t->add_alias_only(); // alias1
//               t->add_alias_only(); // alias2
            }
         }
      }
   }
   return defs;
}


/// Re-use the same test scaffold to modify and then resync, by passing in a function that will modify the defs
static void test_sync_scaffold( defs_change_cmd the_defs_change_command,
                               unsigned int test_equality,  /*0 means test equality, any other number test size*/
                               bool full_sync, unsigned int client_handle)
{
   defs_ptr server_defs = create_defs();
   ServerReply server_reply;
   server_reply.set_client_defs( create_defs() );


   Ecf::set_debug_equality(true); // only has affect in DEBUG build
   BOOST_CHECK_MESSAGE( *server_defs == *server_reply.client_defs(), "Starting point client and server defs should be the same");
   Ecf::set_debug_equality(false);

   // Get change number before any changes
   Ecf::set_state_change_no(0);
   Ecf::set_modify_change_no(0);
   unsigned int client_state_change_no = Ecf::state_change_no();
   unsigned int client_modify_change_no = Ecf::modify_change_no();

   // make some change to the server
   {
      Ecf::set_server(true);

      the_defs_change_command(server_defs);
      std::string errorMsg;
      BOOST_REQUIRE_MESSAGE( server_defs->checkInvariants(errorMsg),"Invariants failed " << errorMsg);
      BOOST_REQUIRE_MESSAGE( !(*server_reply.client_defs() == *server_defs), "Expected client and server defs to differ " << *server_reply.client_defs() << "\n" << "server defs   = " << *server_defs);
      Ecf::set_server(false);
   }

//   cout << "test_sync_scaffold AFTER Command before SYNC: Server:\n" << *server_defs << "\n";
//   cout << "test_sync_scaffold AFTER Command before SYNC: Client:\n" << *server_reply.client_defs() << "\n";

   MockServer mock_server(server_defs);
   SSyncCmd cmd(client_handle, client_state_change_no,  client_modify_change_no, &mock_server );
   string error_msg;BOOST_REQUIRE_MESSAGE(mock_server.defs()->checkInvariants(error_msg), error_msg);
   BOOST_CHECK_MESSAGE( cmd.do_sync( server_reply ), "Expected server to change");
   BOOST_CHECK_MESSAGE( server_reply.in_sync(),      "Expected to be in sync");
   BOOST_CHECK_MESSAGE( server_reply.full_sync() == full_sync, "Expected full sync");

   if (0 == test_equality) {
      Ecf::set_debug_equality(true);
      BOOST_CHECK_MESSAGE( *server_defs == *server_reply.client_defs(), "Server and client should be same after sync" );
      Ecf::set_debug_equality(false);
   }
   else {
      BOOST_CHECK_MESSAGE( server_reply.client_defs()->suiteVec().size() == test_equality, "Expected suite of size " << test_equality << " but found " <<  server_reply.client_defs()->suiteVec().size());
   }
}

static void reorder_suites(defs_ptr theDefs) {

   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new OrderNodeCmd("/a",NOrder::ALPHA)));
   BOOST_REQUIRE_MESSAGE( toStrVec(theDefs->suiteVec()) == vector_abcd(),"NOrder::ALPHA expected " << toString(vector_abcd())<< " but found: " << toString(toStrVec(theDefs->suiteVec())));
}

static void reorder_family(defs_ptr theDefs) {
//   std::cout << "reorder_family\n" << *theDefs << "\n";

   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new OrderNodeCmd("/d/d",NOrder::ALPHA)));

   std::vector<Family*> families;
   theDefs->findSuite("d")->getAllFamilies(families);
   BOOST_REQUIRE_MESSAGE( toStrVec(families) == vector_abcd(),"NOrder::ALPHA  expected " << toString(vector_abcd())<< " but found: " << toString(toStrVec(families)));
}

static void reorder_task(defs_ptr theDefs) {
   //std::cout << "reorder_task\n" << *theDefs << "\n";

   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new OrderNodeCmd("/d/d/d",NOrder::ALPHA)));

   std::vector<Task*> tasks;
   theDefs->findAbsNode("/d/d")->getAllTasks(tasks);
   BOOST_REQUIRE_MESSAGE( toStrVec(tasks) == vector_abcd(),"NOrder::ALPHA  expected " << toString(vector_abcd())<< " but found: " << toString(toStrVec(tasks)));
}

//static void reorder_alias(defs_ptr theDefs) {
//   //std::cout << "reorder_task\n" << *theDefs << "\n";
//
//   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new OrderNodeCmd("/d/d/d/alias0",NOrder::ALPHA)));
//
//   std::vector<alias_ptr> aliases;
//   theDefs->findAbsNode("/d/d/d")->get_all_aliases(aliases);
//   BOOST_REQUIRE_MESSAGE( toStrVec(aliases) == vector_abcd(),"NOrder::ALPHA  expected " << toString(vector_abcd())<< " but found: " << toString(toStrVec(tasks)));
//}


static void reorder_suites_using_handles(defs_ptr theDefs) {

   // *** NOTE ****: Whenever we register a handle, we get a *FULL* sync

   // create client handle which references all the suites
   // It should be noted that invokeRequest, uses a MockServer, which set/unsets
   // Hence after this call Ecf::server_ is false. Hence we need to ensure that following
   // commands/ DM function set Ecf::server_ to true.
   std::vector<std::string> suite_names = vector_abcd();
   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new ClientHandleCmd(suite_names,false)),bypass_state_modify_change_check);
   BOOST_CHECK_MESSAGE(theDefs->client_suite_mgr().clientSuites().size() == 1,"Expected 1 Client suites but found " << theDefs->client_suite_mgr().clientSuites().size());

   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new OrderNodeCmd("/a",NOrder::ALPHA)));
   BOOST_REQUIRE_MESSAGE( toStrVec(theDefs->suiteVec()) == vector_abcd(),"NOrder::ALPHA  expected " << toString(vector_abcd())<< " but found: " << toString(toStrVec(theDefs->suiteVec())));
}

static void reorder_family_using_handles(defs_ptr theDefs) {

   // *** NOTE ****: Whenever we register a handle, we get a *FULL* sync

   // create client handle which references all the suites
   // It should be noted that invokeRequest, uses a MockServer, which set/unsets
   // Hence after this call Ecf::server_ is false. Hence we need to ensure that following
   // commands/ DM function set Ecf::server_ to true.
   std::vector<std::string> suite_names ; suite_names.push_back("d"); // clinet handle for suite 'd' ONLY
   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new ClientHandleCmd(suite_names,false)),bypass_state_modify_change_check);
   BOOST_CHECK_MESSAGE(theDefs->client_suite_mgr().clientSuites().size() == 1,"Expected 1 Client suites but found " << theDefs->client_suite_mgr().clientSuites().size());

   /// Don't call, data model function directly, since Ecf::server_ is false. *here*
   /// The suite should stay the same, only suite d's family should change
   TestHelper::invokeRequest(theDefs.get(),Cmd_ptr( new OrderNodeCmd("/d/d",NOrder::ALPHA)));
   BOOST_REQUIRE_MESSAGE( toStrVec(theDefs->suiteVec()) == vector_dcba(),"expected " << toString(vector_dcba())<< " but found: " << toString(toStrVec(theDefs->suiteVec())));

   suite_ptr suite_a = theDefs->findSuite("d");
   std::vector<Family*> families;  suite_a->getAllFamilies(families);
   BOOST_REQUIRE_MESSAGE( toStrVec(families) == vector_abcd(),"NOrder::ALPHA  expected " << toString(vector_abcd())<< " but found: " << toString(toStrVec(families)));
}


BOOST_AUTO_TEST_CASE( test_ssync_cmd_test_order  )
{
   cout << "Base:: ...test_ssync_cmd_test_order\n";
   test_sync_scaffold(reorder_suites, 0 /* test equality */, false /* expect full_sync */, 0);
   test_sync_scaffold(reorder_family, 0 /* test equality */, false /* expect full_sync */, 0);
   test_sync_scaffold(reorder_task,   0 /* test equality */, false /* expect full_sync */, 0);
//   test_sync_scaffold(reorder_alias,  0 /* test equality */, false /* expect full_sync */, 0);

   test_sync_scaffold(reorder_suites_using_handles,  0 /* test equality */, true /* expect full_sync */, 1 /* client handle */);
   test_sync_scaffold(reorder_family_using_handles,  1 /* test size     */, true /* expect full_sync */, 1 /* client handle */);

   /// Keep valgrind happy
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()

