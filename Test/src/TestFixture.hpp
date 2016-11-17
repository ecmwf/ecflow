#ifndef TESTFIXTURE_HPP_
#define TESTFIXTURE_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : This Fixture facilitates the test of client/server on different platforms
//
// In order to carry out the test, we must have a common file system.
// We will use $SCRATCH as this is accessible by both client and server.
// This means copying over the test data
//
// When TextFixture is GLOBAL, then we can't seem to call any of the
// BOOST_REQUIRE_MESSAGE() macro in constructor/descructor as this causes a crash
// i.e order of initialisation issues
//============================================================================

#include "ClientInvoker.hpp"
#include <string>
class Defs;

//
// Need to use static data, since with boost global fixture, its not possible to access
// the global test fixture in each of the test cases
//
struct TestFixture {

	// Constructor will invoke the server, destructor will kill the server
	// Since this class is static, the constructor/destructor can not call
	// any of BOOST MACRO, since the unit test will not be there.
	// When running across platforms will will assume server is already running
   TestFixture(const std::string& project_test_dir /* Test or view */);
   TestFixture();
	~TestFixture();

	// Configure the server with the job submission interval.
	// i.e for each 'n' seconds of job submission interval the calendar
	// is typically incremented by 1 minute. Hence speeding up the
	// time and thus the testing. See Calendar for further details
	static int job_submission_interval();

	/// The location of ECF home will vary. If client/server on same machines we
	/// return test data location. Otherwise we need return a common file system location
	/// that was created in the constructor
	static std::string smshome();

	/// Will end up checking to see if ECF_HOST is specified. This specifies the name
	/// of the machine that is running the server. Otherwise return true
	static bool serverOnLocalMachine();

	/// If running locally returns  location of client exe, if a server is on a remote
	/// machine, we need to determine its location.
 	// Several options:
	//    a/ Search for hard code path
	//    b/ Ask server about test data, i.e. client exe path, log file location , etc
	//       More flexible
	static std::string theClientExePath();

	/// When multiple tests are run , we need to clear the log file
	static void clearLog();

	/// When local just returns ecf.log, when remote return path to log file
	static std::string pathToLogFile();

	/// Given a task name like "a" find the find the first task matching that name
	/// and returns is abs node path
	static std::string taskAbsNodePath(const Defs& theDefs, const std::string& taskName);

	/// Location of the includes used in the ecf file
	static std::string includes();

	/// returns the server version as an integer.
	/// This allows as to ignore some tests, when testing old servers.(with new clients).
	static int server_version();

	// Use for all comms with server
	static ClientInvoker& client();
	static std::string port() { return port_;}

private:

	static std::string local_ecf_home();

   void init(const std::string& project_test_dir);

private:

	static std::string scratchSmsHome_;
	static std::string host_;
	static std::string port_;
   static std::string test_dir_;          // used when we have an external server, different platform
   static std::string project_test_dir_;  // "Test" or "view"
};

#endif
