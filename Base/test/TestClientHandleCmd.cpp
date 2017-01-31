//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
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

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "TestHelper.hpp"
#include "Suite.hpp"

using namespace std;
using namespace ecf;

// The client handle commands do not change state & modify change number, hence need to bypass these checks
static bool bypass_state_modify_change_check = false;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_client_handle_cmd_empty_server )
{
   cout << "Base:: ...test_client_handle_cmd_empty_server\n";

   std::vector<std::string> suite_names; suite_names.reserve(5);
   for(int i=0; i < 5; i++)  suite_names.push_back( "s" + boost::lexical_cast<std::string>(i) );

   defs_ptr new_defs = Defs::create();

   // Register new handle on an EMPTY server. register the suite
   for(size_t j = 0; j < suite_names.size(); j++)  {
      std::vector<std::string> names; names.push_back(suite_names[j]);
      TestHelper::invokeRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(names,false)),bypass_state_modify_change_check);
   }

   // We should have 5 handle, 1,2,3,4,5

   //std::cout << "   expect failure when dropping(DROP_USER) on handle that does not exist, handle 6" << endl;
   TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6)));

   //std::cout << "   expect failure for auto add,on handle that does not exist, handle 6" << endl;
   TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6,true /* auto add */)));

   //std::cout << "   expect failure for ADD,on handle that does not exist, handle 6" << endl;
   TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6,suite_names,ClientHandleCmd::ADD)));

   //std::cout << "   expect failure for REMOVE, on handle that does not exist, handle 6" << endl;
   TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6,suite_names,ClientHandleCmd::REMOVE)));

   for(int handle=1; handle < 6; handle++) {

      //std::cout << "   expect success for dropping handle " << handle << endl;
      TestHelper::invokeRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(handle)),bypass_state_modify_change_check);

      //std::cout << "   expect failure for auto add,on handle that does not exist, handle " << handle << endl;
      TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6,true /* auto add */)));

      //std::cout << "   expect failure for ADD,on handle that does not exist, handle " << handle <<endl;
      TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6,suite_names,ClientHandleCmd::ADD)));

      //std::cout << "   expect failure for REMOVE, on handle that does not exist, handle " << handle << "\n" << endl;
      TestHelper::invokeFailureRequest(new_defs.get(),Cmd_ptr( new ClientHandleCmd(6,suite_names,ClientHandleCmd::REMOVE)));
   }
}


BOOST_AUTO_TEST_CASE( test_client_handle_cmd_register_and_drop )
{
	cout << "Base:: ...test_client_handle_cmd_register_and_drop\n";

	std::vector<std::string> suite_names; suite_names.reserve(6);
	for(int i=0; i < 5; i++)  suite_names.push_back( "s" + boost::lexical_cast<std::string>(i) );

	Defs defs;
	for(size_t j = 0; j < suite_names.size(); j++)  defs.addSuite( Suite::create(suite_names[j]) );

	// Register new handle
	for(size_t j = 0; j < suite_names.size(); j++)  {
		std::vector<std::string> names; names.push_back(suite_names[j]);
		TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd(names,false)),bypass_state_modify_change_check);
		BOOST_CHECK_MESSAGE(defs.client_suite_mgr().clientSuites().size() == j+1,"Expected " << j+1 << " Client suites but found " << defs.client_suite_mgr().clientSuites().size());
	}

	// Drop the handles. take a copy, since we about to delete clientSuites
	std::vector<ecf::ClientSuites> clientSuites = defs.client_suite_mgr().clientSuites();
	for(size_t k =0; k< clientSuites.size(); k++) {
		TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd( clientSuites[k].handle() )),bypass_state_modify_change_check);
	}
	BOOST_CHECK_MESSAGE(defs.client_suite_mgr().clientSuites().empty(),"Expected  no client handles, but found " << defs.client_suite_mgr().clientSuites().size());
}


BOOST_AUTO_TEST_CASE( test_client_handle_cmd_auto_add )
{
	cout << "Base:: ...test_client_handle_cmd_auto_add\n";

	std::vector<std::string> suite_names; suite_names.reserve(6);
	for(int i=0; i < 5; i++)  suite_names.push_back( "s" + boost::lexical_cast<std::string>(i) );

	Defs defs;
	for(size_t j = 0; j < suite_names.size(); j++)  defs.addSuite( Suite::create(suite_names[j]) );

	// Register new handle, with no suites, but with auto add new suites
	std::vector<std::string> names;
	TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd(names,false/* auto_add */)),bypass_state_modify_change_check);
	BOOST_CHECK_MESSAGE(defs.client_suite_mgr().clientSuites().size() == 1,"Expected 1 Client suites but found " << defs.client_suite_mgr().clientSuites().size());
	BOOST_CHECK_MESSAGE(defs.client_suite_mgr().clientSuites().front().auto_add_new_suites() == false,"Expected auto add to be disabled");

	// Now enable auto add
	TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd(defs.client_suite_mgr().clientSuites().front().handle(),true /* auto add */)),bypass_state_modify_change_check);
	BOOST_CHECK_MESSAGE(defs.client_suite_mgr().clientSuites().front().auto_add_new_suites(),"Expected auto add to be enabled");

	// now add new suite, they should get added to the new client handles
	names.clear();
	for(int i=5; i < 10; i++)  names.push_back( "s" + boost::lexical_cast<std::string>(i) );
	for(size_t j = 0; j < names.size(); j++)  defs.addSuite( Suite::create(names[j]) );

	std::vector<ecf::ClientSuites> clientSuites = defs.client_suite_mgr().clientSuites();
	std::vector<std::string> handle_suites;
	clientSuites[0].suites(handle_suites);
	BOOST_CHECK_MESSAGE(handle_suites == names ,"Expected suites to be automatically added to our handle");

	// Delete the suite s5,s6,s7,s8,s9
	for(int i=5; i < 10; i++)  {
		suite_ptr suite = defs.findSuite("s" + boost::lexical_cast<std::string>(i));
		BOOST_CHECK_MESSAGE(suite.get(),"Failed to find suite s" << i);
		suite->remove();
	}

	{  // Even when suites have been removed, we still keep registered suite
	   // These have to be explicitly deleted by the user
		std::vector<ecf::ClientSuites> clientSuites = defs.client_suite_mgr().clientSuites();
		std::vector<std::string> handle_suites;
		clientSuites[0].suites(handle_suites);
		BOOST_CHECK_MESSAGE(handle_suites.size() == 5,"Expected handle to have 5 suites but found" << handle_suites.size());
	}
}

BOOST_AUTO_TEST_CASE( test_client_handle_cmd_add_remove )
{
	cout << "Base:: ...test_client_handle_cmd_add_remove\n";

	std::vector<std::string> suite_names; suite_names.reserve(6);
	for(int i=0; i < 5; i++)  suite_names.push_back( "s" + boost::lexical_cast<std::string>(i) );

	defs_ptr defs =  Defs::create();
	for(size_t j = 0; j < suite_names.size(); j++)  defs->addSuite( Suite::create(suite_names[j]) );
	std::vector<std::string> empty_vec;

	// Register new handle, with no suites,
	{
	   TestHelper::invokeRequest(defs.get(),Cmd_ptr( new ClientHandleCmd(empty_vec,false)),bypass_state_modify_change_check);
	   BOOST_CHECK_MESSAGE(defs->client_suite_mgr().clientSuites().size() == 1,"Expected 1 Client suites but found " << defs->client_suite_mgr().clientSuites().size());
	   BOOST_CHECK_MESSAGE(!defs->client_suite_mgr().handle_changed(1),"Expected No handle change, when no real suites added");

	   defs_ptr created_defs = defs->client_suite_mgr().create_defs(1,defs); // clear the handle change
	   BOOST_CHECK_MESSAGE(created_defs,"Expected defs to be created");
	   BOOST_CHECK_MESSAGE(created_defs->suiteVec().empty(),"Expected no suites");
	   BOOST_CHECK_MESSAGE(!defs->client_suite_mgr().handle_changed(1),"Expected handle changed to be cleared after create_defs()");
	}

   // Now add suites to the existing handle, then check they match.
	std::vector<std::string> handle_suites;
   {
      TestHelper::invokeRequest(defs.get(),Cmd_ptr( new ClientHandleCmd(defs->client_suite_mgr().clientSuites().front().handle(),suite_names,ClientHandleCmd::ADD)),bypass_state_modify_change_check);
      defs->client_suite_mgr().clientSuites().front().suites(handle_suites);
      BOOST_CHECK_MESSAGE(handle_suites == suite_names ,"Expected suites to be added to our handle");
      BOOST_CHECK_MESSAGE(defs->client_suite_mgr().handle_changed(1),"Expected handle changed when adding new suites to our handle");

      defs_ptr created_defs = defs->client_suite_mgr().create_defs(1,defs); // clear the handle change
      BOOST_CHECK_MESSAGE(created_defs,"Expected defs to be created");
      BOOST_CHECK_MESSAGE(created_defs.get() == defs.get(),"When *ALL* suites registered, the returned defs should be the same");
      BOOST_CHECK_MESSAGE(created_defs->suiteVec().size() == suite_names.size(),"Not all suites created");
   }

   // Now remove the suites from the handle
   {
      TestHelper::invokeRequest(defs.get(),Cmd_ptr( new ClientHandleCmd(defs->client_suite_mgr().clientSuites().front().handle(),suite_names,ClientHandleCmd::REMOVE)),bypass_state_modify_change_check);
      handle_suites.clear();
      defs->client_suite_mgr().clientSuites().front().suites(handle_suites);
      BOOST_CHECK_MESSAGE(handle_suites == empty_vec ,"Expected no suites in our handle but found " << handle_suites.size());
      BOOST_CHECK_MESSAGE(defs->client_suite_mgr().handle_changed(1),"Expected handle changed when adding new removing suites");

      defs_ptr created_defs = defs->client_suite_mgr().create_defs(1,defs); // clear the handle change
      BOOST_CHECK_MESSAGE(created_defs,"Expected defs to be created");
      BOOST_CHECK_MESSAGE(created_defs->suiteVec().empty(),"Expected no suites");
      BOOST_CHECK_MESSAGE(!defs->client_suite_mgr().handle_changed(1),"Expected handle changed to be cleared after create_defs()");
   }
}

static bool check_ordering(Defs& defs)
{
   // make sure order of suites in handles is the same as server order
   const std::vector<suite_ptr>& suite_vec = defs.suiteVec();
   std::vector<std::string> suite_names;
   for(size_t i = 0; i <  suite_vec.size(); i++) suite_names.push_back(suite_vec[i]->name());

   // Drop the handles. take a copy, since we about to delete clientSuites
   std::vector<ecf::ClientSuites> clientSuites = defs.client_suite_mgr().clientSuites();
   for(size_t k =0; k< clientSuites.size(); k++) {
      std::vector<std::string> names; names.reserve(6);
      clientSuites[k].suites(names);
      if (names != suite_names) return false;
   }
   return true;
}

BOOST_AUTO_TEST_CASE( test_client_handle_suite_ordering )
{
   cout << "Base:: ...test_client_handle_suite_ordering\n";
   // ensure order of suites in a handle is the same as server suites

   std::vector<std::string> suite_names; suite_names.reserve(6);
   for(int i=0; i < 5; i++)  suite_names.push_back( "s" + boost::lexical_cast<std::string>(i) );

   Defs defs;
   for(size_t j = 0; j < suite_names.size(); j++)  defs.addSuite( Suite::create(suite_names[j]) );

   // Register 3 new handle
   TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd(suite_names,true)),bypass_state_modify_change_check);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd(suite_names,true)),bypass_state_modify_change_check);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new ClientHandleCmd(suite_names,true)),bypass_state_modify_change_check);
   BOOST_CHECK_MESSAGE(defs.client_suite_mgr().clientSuites().size() == 3,"Expected 3 Client suites but found " << defs.client_suite_mgr().clientSuites().size());

   // After registration make sure ordering is the same
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after registration of handles");


   // Check ordering after OrderNodeCmd
   TestHelper::invokeRequest(&defs,Cmd_ptr( new OrderNodeCmd("/s0",NOrder::DOWN)));
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after NOrder::DOWN");

   TestHelper::invokeRequest(&defs,Cmd_ptr( new OrderNodeCmd("/s0",NOrder::UP)));
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after NOrder::UP");

   TestHelper::invokeRequest(&defs,Cmd_ptr( new OrderNodeCmd("/s0",NOrder::BOTTOM)));
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after NOrder::BOTTOM");

   TestHelper::invokeRequest(&defs,Cmd_ptr( new OrderNodeCmd("/s0",NOrder::TOP)));
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after NOrder::TOP");

   TestHelper::invokeRequest(&defs,Cmd_ptr( new OrderNodeCmd("/s0",NOrder::ALPHA)));
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after NOrder::ALPHA");


   // check ordering after adding new suites, notice we auto add new suites to all our handles
   defs.add_suite("sxx");
   BOOST_CHECK_MESSAGE(check_ordering(defs),"Ordering not preserved after adding a new suite");
}

BOOST_AUTO_TEST_SUITE_END()
