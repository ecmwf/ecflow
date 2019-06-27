//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $
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
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "ClientInvoker.hpp"
#include "ClientEnvironment.hpp"
#include "PasswdFile.hpp"
#include "InvokeServer.hpp"
#include "SCPort.hpp"
#include "Str.hpp"
#include "User.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
//
// Note: To test HPUX->Linux, invoke serve on (Linux/HPUX) and the client cmds on other system
//       On the client side set ECF_HOST to machine name. To allow further testing if ECF_HOST
//       is specified then *don't* shutdown the server
// ************************************************************************************

class Add_ECF_PASSWD_and_ECF_USER_env {
public:
   Add_ECF_PASSWD_and_ECF_USER_env(const std::string& passwd_file) : ecf_passwd_("ECF_PASSWD="),ecf_user_("ECF_USER=") {
      ecf_passwd_ += passwd_file;
      auto* put = const_cast<char*>(ecf_passwd_.c_str());
      BOOST_CHECK_MESSAGE(putenv(put) == 0,"putenv failed for " << put);

      ecf_user_ += User::login_name();
      auto* put2 = const_cast<char*>(ecf_user_.c_str());
      BOOST_CHECK_MESSAGE(putenv(put2) == 0,"putenv failed for " << put2);
   }
   ~Add_ECF_PASSWD_and_ECF_USER_env() {
      putenv(const_cast<char*>("ECF_PASSWD")); // remove from env, otherwise valgrind complains
      putenv(const_cast<char*>("ECF_USER"));   // remove from env, otherwise valgrind complains
   }
private:
   std::string ecf_passwd_;
   std::string ecf_user_;
};

BOOST_AUTO_TEST_CASE( test_custom_user )
{
   Host the_host;
   std::string host = ClientEnvironment::hostSpecified();
   std::string port = SCPort::next();
   std::string passwd_file = the_host.ecf_passwd_file(port);
   std::string passwd = "xxxx";
   if (host.empty()) {
      // make sure NO passed file is present before the server is started.
      // Only do this locally, as passwd file on remote machine may not be accessible
      fs::remove(passwd_file);
   }
   else {
      // The test only works if server is started locally.
      std::cout << "...Client:: ...test_custom_user ignoring when HOST specified\n";
      return;
   }

   //cout << "passwd_file " << passwd_file << "\n";

   {  // *** Need a separate scope so that server shuts down whilst password file is *STILL* present
      // *** Otherwise terminate of server will fail. i.e ECF_PASSWORD is still valid

      // Set ECF_USER and ECF_PASSWD environment variable. Use same file for client and server
      // add on construction, and remove at destruction.
      Add_ECF_PASSWD_and_ECF_USER_env custom_user(passwd_file);

      // Create a valid passwd file; Before server start; make sure server closes before password file is deleted:
      std::string errorMsg;
      BOOST_REQUIRE_MESSAGE(PasswdFile::createWithAccess(passwd_file,the_host.name(),port,passwd,errorMsg),errorMsg);

      // This will remove check pt and backup file before server start,
      // to avoid the server from loading previous test data
      InvokeServer invokeServer("Client:: ...test_custom_user",port);
      BOOST_REQUIRE_MESSAGE( invokeServer.server_started(), "Server failed to start on " <<  invokeServer.host() << ":" << invokeServer.port() );

      ClientInvoker theClient(invokeServer.host(),invokeServer.port());
      theClient.set_throw_on_error(false);

      // Invoking a client request that requires authorisation
      BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());

      // Change to a user not in password file
      theClient.set_user_name("dodgy_geezer"); // this should clear password, so that its reloaded when *next* cmd runs
      errorMsg.clear();

      // all client command should now *FAIL*.
      BOOST_CHECK_MESSAGE( theClient.reloadpasswdfile() == 1, CtsApi::reloadpasswdfile() << " should return 1\n" );
      BOOST_CHECK_MESSAGE( theClient.delete_all() == 1,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.shutdownServer() == 1,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.haltServer() == 1,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.restartServer() == 1,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());

      // reset to a valid user again:
      theClient.set_user_name(User::login_name()); // this should clear password, so that its reloaded when *next* cmd runs

      // all client command should now pass. Invoking a client request that requires authorisation
      BOOST_CHECK_MESSAGE( theClient.shutdownServer() == 0,"should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.getDefs() == 0,"should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.sync_local() == 0,"should return 0\n" << theClient.errorMsg());
      BOOST_CHECK_MESSAGE( theClient.news_local() == 0,"should return 0\n" << theClient.errorMsg());

      // terminate server with a valid valid user since ECF_PASSWORD/ECF_USER is still in effect.
   }

   // Remove the white list file. Comment out for debug
   fs::remove(passwd_file);
}

BOOST_AUTO_TEST_SUITE_END()
