#ifndef TESTVERIFICATION_HPP_
#define TESTVERIFICATION_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #15 $ 
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

#include <boost/noncopyable.hpp>
#include <string>
class Defs;

//
// Test verification class:
// This class verifies that state changes reported in the log files are as expected.
// Please note that we can't do an exact compare for state changes
// 	 	submitted -->active
//  	active    -->complete
// since the order in the log file, is dependent on the process scheduler
//
class TestVerification : private boost::noncopyable {
public:
	/// If testing on remote server, Copy the log file, i.e copied ecf.log  --> <defs file name>.def.log
	/// Required for verification. For cross platform testing this relies on the ecf.log to be sync'ed to disk
	/// Otherwise the copy will NOT be correct and lead to false errors
	/// Create golden log file, if it does not exist
 	static std::string locate_log_file(const std::string& defs_filename);

 	/// The standard test verification will check that each task goes through the
 	/// normal life cycle changes, and then compares the state changes in the log file
 	/// with the golden reference
 	static void standardVerification(const std::string& logFile, const Defs&);

private:
	/// When the test are run for the very first time, the golden log file does not exist
	/// this function will create the golden log file if it does not exist
	static void createGoldenLogFileIfRequired(const std::string& logfile);


 	TestVerification();
};

#endif
