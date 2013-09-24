//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #42 $ 
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
#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "TestVerification.hpp"
#include "ServerTestHarness.hpp"
#include "TestFixture.hpp"
#include "ClientInvoker.hpp"
#include "ClientToServerCmd.hpp"

#include "Defs.hpp"
#include "Task.hpp"
#include "File.hpp"
#include "Str.hpp"
#include "LogVerification.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v) {
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, " "));
    return os;
}


void createNewLogFileFromServer(const std::string& newLog)
{
#ifdef DEBUG_TEST_HARNESS
	 cout << "Get the log file from the server\n";
#endif
	 // remove exist log file first
 	 if ( fs::exists( newLog ) ) {
	 	BOOST_CHECK_MESSAGE(fs::remove(fs::path(newLog)),"Failed to delete " << newLog);
	 }

	 // write log file to disk, for analysing and comparison
	 ClientInvoker theClient ;
	 BOOST_REQUIRE_MESSAGE( theClient.getLog() == 0,  "get log failed should return 0 ");
	 ofstream log(newLog.c_str(),ios::out);
	 BOOST_CHECK_MESSAGE(log," Failed to create log file " << newLog)	;
	 log << theClient.get_string();
	 log.close();
}

void createNewLogFileByCopy(const std::string& newLog)
{
 	std::string smsLog = TestFixture::pathToLogFile();

	try {
		// remove old log file if it exists, otherwise copy_file, wont work
		if ( fs::exists( newLog ) ) {
 			BOOST_CHECK_MESSAGE(fs::remove(fs::path(newLog)),"Failed to delete " << newLog);
		}

		// copy ecf.log -> <defsfilename>.log
 		BOOST_CHECK_MESSAGE(fs::exists(smsLog),"File " << smsLog << " does not exist, can't copy to " << newLog);

		// make sure the log file is NOT empty.
		BOOST_CHECK_MESSAGE(fs::file_size( smsLog ) != 0,"Log file " << smsLog << " is empty.");

		fs::copy_file( smsLog, newLog );

  		BOOST_CHECK_MESSAGE(fs::file_size(smsLog) == fs::file_size(newLog),
 		                   smsLog << " file size:" << fs::file_size( smsLog )
 		                   << " is not same as " << newLog << " file_size:" << fs::file_size(newLog));
	}
	catch ( ... ) {
		BOOST_CHECK_MESSAGE(false,"Unknown exception could not copy log file " << TestFixture::pathToLogFile() << " ---> " << newLog);
	}
}


std::string TestVerification::locate_log_file(const std::string& defs_filename )
{
	std::string newLog =  defs_filename + "_log";

 	if (!TestFixture::serverOnLocalMachine()) {
 		createNewLogFileFromServer(newLog);
 	}

	// For very first test run, create the golden log file if required
	createGoldenLogFileIfRequired( newLog );

	return newLog;
}


void TestVerification::createGoldenLogFileIfRequired(const std::string& logFile)
{
   	std::string goldenLogFile = ServerTestHarness::goldenTestDataLocation(logFile);

	//cout << "logfile = " << logFile << " goldenLogFile=" << goldenLogFile << "\n";
	if (!fs::exists(goldenLogFile))  fs::copy_file(logFile,goldenLogFile);
	else {
		// If the file exists. Check if its writable, if it is, overwrite log file
		ofstream my_file(goldenLogFile.c_str());
		if (!my_file.good()) {
			// read able file. Do Nothing
			// cout << "file " << goldenLogFile << " is readable \n";
 		}
		else {
			// writable file, overwrite
			cout << "Golden log file " << goldenLogFile << " is write-able, overwriting.\n";
			fs::remove(goldenLogFile);
			fs::copy_file(logFile,goldenLogFile);
 		}
	}
}

void verifyNormalLifecycle(const std::string& logfile, const std::vector<Task*>& theTasks)
{
	// Each *task* must have gone through the following life cycle changes
 	// unknown->queued->submitted->active->complete
	// Record the state life cycle for each task, and verify
	// The extracted pairs of <nodepath,state> are in the order they were read in
	// from the log file:
	std::string errorMsg;
	std::vector< std::pair< std::string, std::string > > lines;
	BOOST_REQUIRE_MESSAGE(LogVerification::extractNodePathAndState(logfile,lines,errorMsg),errorMsg);

	typedef std::map<std::string, std::vector<std::string> > TaskLifeMap_t;
	TaskLifeMap_t taskLifeMap;
 	for(size_t i = 0; i < lines.size(); i++) {

 		std::string theNodePath = lines[i].first;
 		std::string theState = lines[i].second;

	    // find task in map, and add to state change vector, ignore suites/family's
	    bool fndTask = false;
 	    for( std::vector<Task*>::const_iterator t = theTasks.begin(); t!=theTasks.end(); ++t) {
	    	if ( (*t)->absNodePath() == theNodePath ) {
	    		fndTask = true;
	    		break;
	    	}
	    }
 		if (!fndTask) continue;

		TaskLifeMap_t::iterator iter = taskLifeMap.find( theNodePath );
	    if ( iter == taskLifeMap.end()) {
	    	std::vector<  std::string > vec; vec.reserve(5);
	    	vec.push_back( theState );
	    	taskLifeMap.insert( std::make_pair(theNodePath, vec ) );
	    }
	    else {
	    	std::vector< std::string > vec = (*iter).second;
	    	vec.push_back( theState );
	    	taskLifeMap[ theNodePath ] = vec;
  	    }
	}

	// make sure we found all tasks
	BOOST_CHECK_MESSAGE( taskLifeMap.size() == theTasks.size(), "Expected to find " << theTasks.size() << " tasks in log file(" << logfile <<") but found " << taskLifeMap.size());

	// iterate over map and compare with normal life cycle map.
	std::vector<  std::string > normalLifeCycle; normalLifeCycle.reserve(5);
 	normalLifeCycle.push_back(  string("queued") );
	normalLifeCycle.push_back(  string("submitted") );
	normalLifeCycle.push_back(  string("active") );
	normalLifeCycle.push_back(  string("complete") );

 	BOOST_FOREACH( TaskLifeMap_t::value_type &m, taskLifeMap ) {
 		BOOST_CHECK_MESSAGE( m.second == normalLifeCycle, "In logfile " << logfile << " Task " << m.first << " did not follow normal lifecycle\n");
 		if (m.second != normalLifeCycle) {
 			cout << "Expected:\n";
 			std::vector<  std::string >::iterator i;
 			for(i = normalLifeCycle.begin(); i != normalLifeCycle.end(); ++i) {
 				cout << "         " << (*i) << "\n";
 			}
 			cout << "Found:\n";
 			for(i = m.second.begin(); i != m.second.end(); ++i) {
 				cout << "         " << (*i) << "\n";
  			}
   		}
  	}
}

void TestVerification::standardVerification(const std::string& logFile, const Defs& theDefs)
{
 	// Verify life cycle of each task, by checking the log file.
 	// Each task must have gone through the following life cycle changes
 	// unknown->queued->submitted->active->complete
 	std::vector<Task*> theTasks;
 	theDefs.getAllTasks(theTasks);
 	verifyNormalLifecycle(logFile,theTasks);

 	// verify that state changes are in the right order. This is done
 	// by comparing the input log file, with the golden reference log file.
 	//
 	// We ignore those state changes that are dependent on scheduling order
 	std::string errorMsg;
 	BOOST_CHECK_MESSAGE(LogVerification::compareNodeStates(logFile,ServerTestHarness::goldenTestDataLocation(logFile),errorMsg),errorMsg);
}
