#define BOOST_TEST_MODULE TestClient
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

#include "ClientEnvironment.hpp"
#include "Str.hpp"
#include "File.hpp"

namespace fs = boost::filesystem;
using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ClientTestSuite )

// **************************************************************************************
// test the client environment:
// In particular test host file parsing
// **************************************************************************************
BOOST_AUTO_TEST_CASE( test_client_environment_host_file_parsing )
{
	std::cout << "Client:: ...test_client_environment_host_file_parsing" << endl;

   std::string good_host_file = File::test_data("Client/test/data/good_hostfile","Client");

	// local host should be implicitly added to internal host list
   std::string the_host = ClientEnvironment::hostSpecified();
   if (the_host.empty()) the_host = Str::LOCALHOST();

	std::vector<std::string> expectedHost;
	expectedHost.push_back(the_host);
	expectedHost.push_back("host1");
	expectedHost.push_back("host2");
	expectedHost.push_back("host3");
	expectedHost.push_back("host4");
   expectedHost.push_back("host5");
   expectedHost.push_back("host6");


	ClientEnvironment client_env( good_host_file );
//	cout << "client_env " << client_env.toString() << "\n";
	std::string home_host = client_env.host();
	std::string host;
//	cout << "client_env home host " << client_env.host() << " job supplied port " << client_env.port() << "\n";
	size_t count = 0;
	BOOST_CHECK_MESSAGE( home_host == expectedHost[count], "Expected home host " << expectedHost[count] << " but found " << home_host);

	bool home_host_fnd = false;
	while (home_host != host) {
	   // Cycle through the host until we reach the home host
	   std::string errorMsg;
	   BOOST_CHECK_MESSAGE( client_env.get_next_host(errorMsg), errorMsg);
//	   cout << "client_env host " << client_env.host() << "  port " << client_env.port() << "\n";
	   host = client_env.host();
	   count++;
	   if (host == home_host) {
	      home_host_fnd = true ;
	      count = 0;
	   }
	   BOOST_REQUIRE_MESSAGE( count < expectedHost.size(), "Test file out of date");
	   BOOST_CHECK_MESSAGE( host == expectedHost[count], "Expected host " << expectedHost[count] << " but found " << host);
	}
	BOOST_CHECK_MESSAGE( home_host_fnd, "Cycling through host file, should lead to home host");
   cout << "Client:: ...-END\n";
}

BOOST_AUTO_TEST_CASE( test_client_environment_host_file_defaults )
{
   std::cout << "Client:: ...test_client_environment_host_file_defaults" << endl;

   // When the HOST file does *NOT* indicate the port, it should be taken
   // from the config/environment.
   // In file good_hostfile, host3 and host5 do not specify a port, hence
   // this port is assumed to be the job supplied port, that was read from
   // config or environment. To test this correctly we need to specify a port
   // other than the default

   std::string good_host_file = File::test_data("Client/test/data/good_hostfile","Client");


   // local host should be implicitly added to internal host list
   std::vector<std::pair<std::string,std::string> > expectedHost;
   expectedHost.push_back( make_pair(Str::LOCALHOST(), string("5111")) ); // here 5111 is job supplied port
   expectedHost.push_back( make_pair(string("host1"), string("3142")) );
   expectedHost.push_back( make_pair(string("host2"), string("3141")) );
   expectedHost.push_back( make_pair(string("host3"), string("5111")) ); // not specified in host file, hence expect 5111
   expectedHost.push_back( make_pair(string("host4"), string("4001")) );
   expectedHost.push_back( make_pair(string("host5"), string("5111")) ); // not specified in host file, hence expect 5111
   expectedHost.push_back( make_pair(string("host6"), string("4081")) );

   // Create the ClientEnvironment overriding the config & environment. To specify host and port
   ClientEnvironment client_env( good_host_file , Str::LOCALHOST(),"5111");
//   cout << "client_env " << client_env.toString() << "\n";
   std::string home_host = client_env.host();
   std::string home_port = client_env.port();
   BOOST_CHECK_MESSAGE( Str::LOCALHOST() == home_host && "5111" == home_port,"host host & port not as expected");

   std::string host;
//   cout << "client_env home host " << client_env.host() << "  job supplied port " << client_env.port() << "\n";
   size_t count = 0;
   bool home_host_fnd = false;
   while (home_host != host) {
      // Cycle through the host until we reach the home host
      std::string errorMsg;
      BOOST_CHECK_MESSAGE( client_env.get_next_host(errorMsg), errorMsg);
//      cout << "client_env host " << client_env.host() << "  port " << client_env.port() << "\n";
      host = client_env.host();
      std::string port = client_env.port();
      count++;
      if (host == home_host) {
         BOOST_CHECK_MESSAGE( count-1 == 6, "Expected 6 hosts in host file"); // ignore last increment of count, hence -1
         home_host_fnd = true ;
         count = 0;
      }
      BOOST_REQUIRE_MESSAGE( count < expectedHost.size(), "Test file out of date");
      BOOST_CHECK_MESSAGE( host == expectedHost[count].first, "Expected host " << expectedHost[count].first << " but found " << host);
      BOOST_CHECK_MESSAGE( port == expectedHost[count].second, "Expected port " << expectedHost[count].second << " but found " << port);
   }
   BOOST_CHECK_MESSAGE( home_host_fnd, "Cycling through host file, should lead to home host");
   cout << "Client:: ...-END\n";
}


BOOST_AUTO_TEST_CASE( test_client_environment_empty_host_file )
{
   std::cout << "Client:: ...test_client_environment_empty_host_file" << endl;

   std::string empty_host_file = File::test_data("Client/test/data/empty_hostfile","Client");

   std::string errormsg;
   BOOST_CHECK_MESSAGE(File::create(empty_host_file,"",errormsg), "Failed to create empty host file " << errormsg);

   ClientEnvironment client_env( empty_host_file );
   std::string errorMsg;
   BOOST_CHECK_MESSAGE( client_env.get_next_host(errorMsg),errorMsg);
   BOOST_CHECK_MESSAGE( client_env.get_next_host(errorMsg),errorMsg);

   fs::remove(empty_host_file);
   cout << "Client:: ...-END\n";
}


BOOST_AUTO_TEST_CASE( test_client_environment_errors )
{
   if (getenv("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")) {
      cout << "Client:: ...test_client_environment_errors-ECF_ALLOW_NEW_CLIENT_OLD_SERVER: ignoring test when ECF_ALLOW_NEW_CLIENT_OLD_SERVER specified\n";
      return;
   }
   if (getenv("ECF_PORT")) {
      cout << "Client:: ...test_client_environment_errors-ECF_ALLOW_NEW_CLIENT_OLD_SERVER: ignoring test when ECF_PORT specified\n";
      return;
   }

   std::cout << "Client:: ...test_client_environment_errors-ECF_ALLOW_NEW_CLIENT_OLD_SERVER" << endl;
   {
      char* put = const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER=xx");
      BOOST_CHECK_MESSAGE(putenv(put) == 0,"putenv failed for " << put);
      BOOST_CHECK_THROW(ClientEnvironment client_env, std::runtime_error );
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }

   std::string the_host = ClientEnvironment::hostSpecified();
   if (the_host.empty()) the_host = Str::LOCALHOST();

   {
      // ONLY run this test if enviroment variable ECF_PORT not defined
      std::string env = "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=";
      env += the_host; env += ":"; env += Str::DEFAULT_PORT_NUMBER(); env += ":xx";
      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
      BOOST_CHECK_THROW(ClientEnvironment client_env, std::runtime_error );
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }
   {
      std::string env = "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=";
      env += the_host; env += Str::DEFAULT_PORT_NUMBER(); env += "12";
      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
      BOOST_CHECK_THROW(ClientEnvironment client_env, std::runtime_error );
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }
   {
      std::stringstream ss;
      ss << "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=fred:2222:0,bill:333:2222," << the_host << ":" << Str::DEFAULT_PORT_NUMBER() << ":xx";
      std::string env = ss.str();
      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
      BOOST_CHECK_THROW(ClientEnvironment client_env, std::runtime_error );
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }

   cout << "Client:: ...-END\n";
}

BOOST_AUTO_TEST_CASE( test_client_environment )
{
   if (getenv("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")) {
      cout << "Client:: ...test_client_environment-ECF_ALLOW_NEW_CLIENT_OLD_SERVER: ignoring test when ECF_ALLOW_NEW_CLIENT_OLD_SERVER specified\n";
      return;
   }
   if (getenv("ECF_PORT")) {
      cout << "Client:: ...test_client_environment-ECF_ALLOW_NEW_CLIENT_OLD_SERVER: ignoring test when ECF_PORT specified\n";
      return;
   }

   std::cout << "Client:: ...test_client_environment-ECF_ALLOW_NEW_CLIENT_OLD_SERVER" << endl;

   std::string the_host = ClientEnvironment::hostSpecified();
   if (the_host.empty()) the_host = Str::LOCALHOST();

   {
      std::string env = "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=";
      env += the_host; env += ":"; env += Str::DEFAULT_PORT_NUMBER(); env += ":11";
      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
      ClientEnvironment client_env;
      BOOST_CHECK_MESSAGE(client_env.allow_new_client_old_server()==11,"Expected 11 but found " << client_env.allow_new_client_old_server() << " for env " << env);
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }
   {
      std::stringstream ss;
      ss << "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=fred:2222:0,bill:333:2222," << the_host << ":" << Str::DEFAULT_PORT_NUMBER() << ":" << 33;
      std::string env = ss.str();
      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
      ClientEnvironment client_env;
      BOOST_CHECK_MESSAGE(client_env.allow_new_client_old_server()==33,"Expected 33 but found " << client_env.allow_new_client_old_server() << " for env " << env);
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }
   {
      // Create a valid ECF_ALLOW_NEW_CLIENT_OLD_SERVER list where there is no match with our host/port.
      // hence allow_new_client_old_server should remain zero
      std::stringstream ss;
      ss << "ECF_ALLOW_NEW_CLIENT_OLD_SERVER=fred:2222:0,bill:333:2222,bill:333:2222,bill:333:2222,bill:333:2222,bill:333:2222";
      std::string env = ss.str();
      BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(env.c_str())) == 0,"putenv failed for " << env);
      ClientEnvironment client_env;
      BOOST_CHECK_MESSAGE(client_env.allow_new_client_old_server()==0,"Should remain unchanged but found " << client_env.allow_new_client_old_server());
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }
   {
      char* put = const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER=10");
      BOOST_CHECK_MESSAGE(putenv(put) == 0,"putenv failed for " << put);
      ClientEnvironment client_env;
      BOOST_CHECK_MESSAGE(client_env.allow_new_client_old_server()==10,"expcted 10 but found " << client_env.allow_new_client_old_server());
      putenv(const_cast<char*>("ECF_ALLOW_NEW_CLIENT_OLD_SERVER")); // remove from env, otherwise valgrind complains
   }
   cout << "Client:: ...-END\n";
}

BOOST_AUTO_TEST_SUITE_END()
