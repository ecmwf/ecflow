//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #4 $
//
// Copyright 2009-2016 ECMWF.
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

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

#ifdef ECF_SECURE_USER
BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// ************************************************************************************
// Note: If you make edits to node tree, they will have no effect until the server is rebuilt
//
// Note: To test HPUX->Linux, invoke serve on (Linux/HPUX) and the client cmds on other system
//       On the client side set ECF_NODE to machine name. To allow further testing if ECF_NODE
//       is specified then *don't* shutdown the server
// ************************************************************************************

class Add_ECF_PASSWD_env {
public:
   Add_ECF_PASSWD_env(const std::string& passwd_file) : env_("ECF_PASSWD=") {
      env_ += passwd_file;
      char* put = const_cast<char*>(env_.c_str());
      BOOST_CHECK_MESSAGE(putenv(put) == 0,"putenv failed for " << put);
   }
   ~Add_ECF_PASSWD_env() {
      putenv(const_cast<char*>("ECF_PASSWD")); // remove from env, otherwise valgrind complains
   }
private:
   std::string env_;
};


BOOST_AUTO_TEST_CASE( test_loading_of_passwd )
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
      std::cout << "...Client:: ...test_loading_of_passwd ignoring when HOST specified\n";
      return;
   }

   // cout << "passwd_file " << passwd_file << "\n";

   // Set ECF_PASSWD environment variable. Use same file for client and server
   // add on construction, and remove at destruction.
   Add_ECF_PASSWD_env ecf_passwd(passwd_file);

   // Create a valid passwd file; Before server start
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE(PasswdFile::createWithAccess(passwd_file,the_host.name(),port,passwd,errorMsg),errorMsg);

   // This will remove check pt and backup file before server start,
   // to avoid the server from loading previous test data
   InvokeServer invokeServer("Client:: ...test_loading_of_passwd",port);

   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   theClient.set_throw_on_error(false);

   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());

   // reloading the same passwd file succeed.
   BOOST_REQUIRE_MESSAGE( theClient.reloadpasswdfile() == 0, CtsApi::reloadpasswdfile() << " should return 1\n" );

   // Invoking a client request that requires authorisation
   BOOST_CHECK_MESSAGE( theClient.shutdownServer() == 0,"should return 0\n" << theClient.errorMsg());
   BOOST_CHECK_MESSAGE( theClient.getDefs() == 0,"should return 0\n" << theClient.errorMsg());
   BOOST_CHECK_MESSAGE( theClient.sync_local() == 0,"should return 0\n" << theClient.errorMsg());
   BOOST_CHECK_MESSAGE( theClient.news_local() == 0,"should return 0\n" << theClient.errorMsg());

   // clear the password file.
   errorMsg.clear();
   BOOST_REQUIRE_MESSAGE(PasswdFile::clear(passwd_file,errorMsg),errorMsg);

   // reload the password file, which is now empty
   BOOST_REQUIRE_MESSAGE( theClient.reloadpasswdfile() == 0, CtsApi::reloadpasswdfile() << " should return 1\n" );

   // all client command should now *FAIL*.
   BOOST_CHECK_MESSAGE( theClient.shutdownServer() == 1,"should return 1\n");
   BOOST_REQUIRE_MESSAGE( theClient.reloadpasswdfile() == 1, CtsApi::reloadpasswdfile() << " should return 1\n" );

   // Remove the white list file. Comment out for debug
   fs::remove(passwd_file);
}

BOOST_AUTO_TEST_CASE( test_loading_of_passwd_fail )
{
   // TEST user *MUST* be in ECF_PASSWD file, into order to *ALLOW* reloadpasswdfile
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
      std::cout << "...Client:: ...test_loading_of_passwd_fail ignoring when HOST specified\n";
      return;
   }

   // Set ECF_PASSWD environment variable. Use same file for client and server
   // add on construction, and remove at destruction.
   Add_ECF_PASSWD_env ecf_passwd(passwd_file);

   // This will remove check pt and backup file before server start,
   // to avoid the server from loading previous test data
   InvokeServer invokeServer("Client:: ...test_loading_of_passwd_fail",port);

   /// Passwd file is specified but does not exist. Command to succeed
   ClientInvoker theClient(invokeServer.host(),invokeServer.port());
   theClient.set_throw_on_error(false);

   BOOST_REQUIRE_MESSAGE( theClient.delete_all() == 0,CtsApi::to_string(CtsApi::delete_node()) << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.shutdownServer() == 0,CtsApi::shutdownServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.haltServer() == 0,CtsApi::haltServer() << " should return 0\n" << theClient.errorMsg());
   BOOST_REQUIRE_MESSAGE( theClient.restartServer() == 0,CtsApi::restartServer() << " should return 0\n" << theClient.errorMsg());

   // OK now test passwd file functionality
   // The passwd file should not exist, hence reload SHOULD fail. i.e because we deleted it earlier
   BOOST_REQUIRE_MESSAGE( theClient.reloadpasswdfile() == 1, CtsApi::reloadpasswdfile() << " should return 1\n" );

   // Create a valid passwd file; For the FIRST time:
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE(PasswdFile::createWithAccess(passwd_file,the_host.name(),port,passwd,errorMsg),errorMsg);

   //  Reload should STILL fail. Since reloadpasswdfile *ITSELF* requires authentication.
   //  Hence user *MUST* be in ECF_PASSWD file, into order to *ALLOW* reloadpasswdfile
   BOOST_REQUIRE_MESSAGE( theClient.reloadpasswdfile() == 1, CtsApi::reloadpasswdfile() << " should return 1\n" << theClient.errorMsg());

   // Remove the white list file. Comment out for debug
   fs::remove(passwd_file);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
