/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "ClientSuiteMgr.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_client_suite_mgr_remove_user )
{
   cout << "ANode:: ...test_client_suite_mgr_remove_user\n";
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_client_suite_mgr_remove_user");
      family_ptr fam = suite->add_family("family");
      fam->add_task("t1");
   }

   std::vector<std::string> suites = {"s1","s2","s3"};
   std::vector<std::string> suites1 = {"s1","s2","s3","s4"};
   std::vector<std::string> names;

   ClientSuiteMgr mgr(&theDefs);

   unsigned int handle = mgr.create_client_suite(true,suites,"avi");
   BOOST_CHECK_MESSAGE( mgr.valid_handle(handle),"Expected valid handle");
   BOOST_CHECK_MESSAGE( handle == 1,"Expected handle 1 but found " << handle);
   mgr.suites(handle,names);
   BOOST_CHECK_MESSAGE( names == suites,"suite not as expected");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 1,"Expected client suite of size 1 but found " << mgr.clientSuites().size());

   names.clear();
   handle = mgr.create_client_suite(true,suites1,"avi");
   BOOST_CHECK_MESSAGE( mgr.valid_handle(handle),"Expected valid handle");
   BOOST_CHECK_MESSAGE( handle == 2,"Expected handle 2 but found " << handle);
   mgr.suites(handle,names);
   BOOST_CHECK_MESSAGE( names == suites1,"suite not as expected for handle_fred");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 2,"Expected client suite of size 2 but found " << mgr.clientSuites().size());

   names.clear();
   handle = mgr.create_client_suite(true,suites,"fred");
   BOOST_CHECK_MESSAGE( mgr.valid_handle(handle),"Expected valid handle");
   BOOST_CHECK_MESSAGE( handle == 3,"Expected handle 3 but found " << handle);
   mgr.suites(handle,names);
   BOOST_CHECK_MESSAGE( names == suites,"suite not as expected for handle_fred");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 3,"Expected client suite of size 3 but found " << mgr.clientSuites().size());

   mgr.remove_client_suites("avi"); // should remove two handles
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 1,"Expected client suite of size 1 but found " << mgr.clientSuites().size());
   BOOST_CHECK_THROW( mgr.remove_client_suites("avi"), std::runtime_error);

   mgr.remove_client_suites("fred");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 0,"Expected client suite of size 0 but found " << mgr.clientSuites().size());
   BOOST_CHECK_THROW( mgr.remove_client_suites("fred"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( test_client_suite_mgr_remove_handle )
{
   cout << "ANode:: ...test_client_suite_mgr_remove_handle\n";
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("test_client_suite_mgr_remove_handle");
      family_ptr fam = suite->add_family("family");
      fam->add_task("t1");
   }

   std::vector<std::string> suites = {"s1","s2","s3"};
   std::vector<std::string> suites1 = {"s1","s2","s3","s4"};
   std::vector<std::string> names;

   ClientSuiteMgr mgr(&theDefs);

   unsigned int handle1 = mgr.create_client_suite(true,suites,"avi");
   BOOST_CHECK_MESSAGE( mgr.valid_handle(handle1),"Expected valid handle");
   BOOST_CHECK_MESSAGE( handle1 == 1,"Expected handle 1 but found " << handle1);
   mgr.suites(handle1,names);
   BOOST_CHECK_MESSAGE( names == suites,"suite not as expected");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 1,"Expected client suite of size 1 but found " << mgr.clientSuites().size());

   names.clear();
   unsigned int handle2 = mgr.create_client_suite(true,suites1,"avi");
   BOOST_CHECK_MESSAGE( mgr.valid_handle(handle2),"Expected valid handle");
   BOOST_CHECK_MESSAGE( handle2 == 2,"Expected handle 2 but found " << handle2);
   mgr.suites(handle2,names);
   BOOST_CHECK_MESSAGE( names == suites1,"suite not as expected for handle_fred");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 2,"Expected client suite of size 2 but found " << mgr.clientSuites().size());

   names.clear();
   unsigned int handle3 = mgr.create_client_suite(true,suites,"fred");
   BOOST_CHECK_MESSAGE( mgr.valid_handle(handle3),"Expected valid handle");
   BOOST_CHECK_MESSAGE( handle3 == 3,"Expected handle 3 but found " << handle3);
   mgr.suites(handle3,names);
   BOOST_CHECK_MESSAGE( names == suites,"suite not as expected for handle_fred");
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 3,"Expected client suite of size 3 but found " << mgr.clientSuites().size());

   mgr.remove_client_suite(handle1);
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 2,"Expected client suite of size 2 but found " << mgr.clientSuites().size());
   BOOST_CHECK_THROW( mgr.remove_client_suite(handle1), std::runtime_error);

   mgr.remove_client_suite(handle2);
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 1,"Expected client suite of size 1 but found " << mgr.clientSuites().size());
   BOOST_CHECK_THROW( mgr.remove_client_suite(handle2), std::runtime_error);

   mgr.remove_client_suite(handle3);
   BOOST_CHECK_MESSAGE( mgr.clientSuites().size() == 0,"Expected client suite of size 0 but found " << mgr.clientSuites().size());
   BOOST_CHECK_THROW( mgr.remove_client_suite(handle3), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
