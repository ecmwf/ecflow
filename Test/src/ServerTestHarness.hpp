#ifndef SERVERTESTHARNESS_HPP_
#define SERVERTESTHARNESS_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #46 $ 
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

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include "NodeFwd.hpp"
#include "ZombieUtil.hpp"
class ClientInvoker;

// This class provides a test harness for running defs file in a client server environment
// To avoid Address in use errors, we can have client/server use a different port number
// This is more important when doing instrumentation in HP-UX, as that can take a long time.
//
// The Server is started/stopped by use of the Global text Fixture
// See class TestFixture.hpp
//
class ServerTestHarness : private boost::noncopyable {
public:
	// if standardVerification is true, we test task goes through normal life cycle
	// changes. else we compare the log file states with golden log file
	// Comparing log files across platforms is not reliable, and ignored
  	ServerTestHarness();

  	/// generate man files for Suite and family nodes
  	void generateManFileForNodeContainers() { generateManFileForNodeContainers_ = true; }

  	/// Enable/disable checking for server poll being to small.
  	/// This test creates dummy .ecf files. Since these tests are run with the server poll
  	/// which is less than 60, we need the task duration between submit->active->complete duration to be less
  	/// the server poll. Tests with time dependencies of one minute separation will be missed.\n"
   /// Make this configurable since for large defs, when we have 1000's of tasks the
  	/// task duration times are not deterministic
  	void check_task_duration_less_than_server_poll(bool f) { check_task_duration_less_than_server_poll_ =  f;}

  	/// Sometimes on slow systems we want to disable sleep time.
  	/// since the time duration between submit->active->complete is greater than job submission interval
  	/// and hence time slots get missed.
  	/// The Default is to add the sleep time always for events and meters and when there is NO events or meters
  	/// This flags is used for the *specific* case when there is *no* events/meters.
  	void add_default_sleep_time(bool f) { add_default_sleep_time_ = f ;}

	// Run the following defs file in the test harness.
	// If no map is provided then a default sms file is fabricated, otherwise
	// the map allows a sms file string to be associated with a task path
	// map.first = absolute task path
	// map.second =  sms file string
#if defined(_AIX)
   void run(Defs&, const std::string& defs_file, const std::map<std::string,std::string>&, int timeout = 120, bool waitForTestCompletion = true);
   void run(Defs&, const std::string& defs_file, int timeout = 120,bool waitForTestCompletion = true);
#elif defined(HPUX)
   void run(Defs&, const std::string& defs_file, const std::map<std::string,std::string>&, int timeout = 100, bool waitForTestCompletion = true);
   void run(Defs&, const std::string& defs_file, int timeout = 100,bool waitForTestCompletion = true);
#else
 	void run(Defs&, const std::string& defs_file, const std::map<std::string,std::string>&, int timeout = 40, bool waitForTestCompletion = true);
	void run(Defs&, const std::string& defs_file, int timeout = 40,bool waitForTestCompletion = true);
#endif

	/// Returns the location of the defs file, such thats it in the test data area
	static std::string testDataDefsLocation( const std::string& defsFile);

	/// The test data location
	static std::string testDataLocation( const std::string& defsFile);


	/// This function is used for waiting for test to finish
	/// returns the defs from the server at test completeion
	defs_ptr testWaiter( const Defs& theClientDefs,// The defs on the client side
 	                      int timeout,              // How long should we wait for test to finish
 	                      bool verifyAttr);         // Test verification use verify attributes on defs

	/// for debug return the number of times the calendar was updated in the server
	unsigned int serverUpdateCalendarCount() const { return serverUpdateCalendarCount_;}
private:

	defs_ptr doRun(Defs&, const std::map<std::string,std::string>&, int sleepTime,bool waitForTestCompletion);

	void createDirAndEcfFiles(
	   NodeContainer* nc,
	   const std::string& smshome,
	   const std::map<std::string,std::string>& taskSmsMap,
	   int& customSmsCnt) const;

	/// default ecf file will cater for events and meters
  	std::string getDefaultTemplateEcfFile(Task* t) const;

private:
	bool generateManFileForNodeContainers_;
	bool check_task_duration_less_than_server_poll_;
	bool add_default_sleep_time_;
	int serverUpdateCalendarCount_;
	std::string defs_filename_;
};
#endif
