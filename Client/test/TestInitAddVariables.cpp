//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #50 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "TestHelper.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite)

BOOST_AUTO_TEST_CASE( test_init_add_variables )
{
   cout << "Client:: ...test_init_add_variables " << endl;
	TestLog test_log("test_init_add_variables.log"); // will create log file, and destroy log and remove file at end of scope

   Defs defs;
   suite_ptr suite = defs.add_suite("test_init_add_variables");
   family_ptr f = suite->add_family("f");
   task_ptr t1 = f->add_task("t1");
   t1->add_trigger(":name == 1 and :name2 == 2");
   {
      std::string errorMsg; BOOST_CHECK_MESSAGE( defs.checkInvariants(errorMsg),errorMsg);
   }
   {
      std::string errorMsg, warningMsg;
      BOOST_CHECK_MESSAGE(!defs.check(errorMsg,warningMsg),"Expected error since variable name and name2 are not defined" );
   }

   BOOST_CHECK_MESSAGE( suite->state() == NState::UNKNOWN," Initial suite state should be NState::UNKNOWN");
   TestHelper::invokeRequest(&defs,Cmd_ptr( new BeginCmd("test_init_add_variables")));

   {
      // Test that init command with --add, will add variables
      std::vector<Variable> vec { Variable("name","1"), Variable("name2","2") };
      TestHelper::invokeRequest(&defs,Cmd_ptr( new InitCmd(t1->absNodePath(),Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,vec)));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new CtsCmd( CtsCmd::FORCE_DEP_EVAL)),false);
      const Variable& v = t1->findVariable("name");
      const Variable& v2 = t1->findVariable("name2");
      BOOST_CHECK_MESSAGE( !v.empty(),  "Expected --init to add variable");
      BOOST_CHECK_MESSAGE( !v2.empty(), "Expected --init to add variable");

      std::string errorMsg, warningMsg;
      BOOST_CHECK_MESSAGE(defs.check(errorMsg,warningMsg),"Expected check to pass since variable name and name2 are defined " << errorMsg );
   }

   {
      // Test that comeplete command with --remove, will delete variables
      std::vector<std::string> vec{ "name", "name2"};
      TestHelper::invokeRequest(&defs,Cmd_ptr( new CompleteCmd(t1->absNodePath(),Submittable::DUMMY_JOBS_PASSWORD(),Submittable::DUMMY_PROCESS_OR_REMOTE_ID(),1,vec)));
      const Variable& v = t1->findVariable("name");
      const Variable& v2 = t1->findVariable("name2");
      BOOST_CHECK_MESSAGE( v.empty(),  "Expected --complete to delete variable");
      BOOST_CHECK_MESSAGE( v2.empty(), "Expected --complete to delete variable");
   }
}

BOOST_AUTO_TEST_SUITE_END()
