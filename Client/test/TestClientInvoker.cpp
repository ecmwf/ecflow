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
#include <iostream>
#include <vector>
#include <string>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientInvoker.hpp"
#include "Str.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

BOOST_AUTO_TEST_CASE( test_client_invoker )
{
//   if (getenv("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")) {
//      cout << "Client:: ...test_client_invoker: ignoring test when ECF_ALLOW_NEW_CLIENT_OLD_SERVER specified\n";
//      return;
//   }
//
//   std::cout << "Client:: ...test_client_invoker" << endl;
//   {
//      char* put = const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER=10");
//      BOOST_CHECK_MESSAGE(putenv(put) == 0,"putenv failed for " << put);
//      ClientInvoker invoker;
//      BOOST_CHECK_MESSAGE(invoker.allow_new_client_old_server()==10,"Expected 10 but found " << invoker.allow_new_client_old_server() << " for env " << put);
//      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
//   }
//   {
//      std::stringstream ss;
//      ss << "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=fred:2222:12,bill:3333:2222,bill4:6666:1313";
//      std::string env = ss.str();
//      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
//      ClientInvoker invoker("bill","3333");
//      BOOST_CHECK_MESSAGE(invoker.allow_new_client_old_server()==2222,"Expected 2222 but found " << invoker.allow_new_client_old_server() << " for env " << env);
//
//      invoker.set_host_port("bill4","6666");
//      BOOST_CHECK_MESSAGE(invoker.allow_new_client_old_server()==1313,"Expected 1313 but found " << invoker.allow_new_client_old_server() << " for env " << env);
//      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains, + *COULD* affect other tests
//   }
}

BOOST_AUTO_TEST_SUITE_END()

