//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #21 $ 
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
#include <stdlib.h> // for getenv()
#include <string>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Log.hpp"
#include "File.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

std::string getLogPath() {

   return File::test_data("ACore/test/logfile.txt","ACore");
}

BOOST_AUTO_TEST_CASE( test_log )
{
	cout << "ACore:: ...test_log\n";

	std::string path = getLogPath();

	// delete the log file if it exists.
	fs::remove(path);
	BOOST_CHECK_MESSAGE(!fs::exists( path ), "log file not deleted " << path << " not created \n");

	Log::create(path);
	LOG(Log::MSG,"First Message");
	LOG(Log::LOG,"LOG");
	LOG(Log::ERR,"ERROR");
	LOG(Log::WAR,"WARNING");
	LOG(Log::DBG,"DEBUG");
	LOG(Log::OTH,"OTHER");
	log(Log::OTH,"test: void log(Log::LogType,const std::string& message)");

	LOG(Log::OTH,"test: LOG(level,path << path) " << path << " " << path);

 	Log::instance()->log(Log::OTH,"OTHER2");

	BOOST_CHECK_MESSAGE(fs::exists( path ), "log file " << path << " not created \n");
}

BOOST_AUTO_TEST_CASE( test_log_append )
{
	cout << "ACore:: ...test_log_append\n";

	std::string path = getLogPath();

 	BOOST_CHECK_MESSAGE(fs::exists( path ), "log file " << path << " not created by previous test\n");

 	LOG(Log::MSG,"Last Message");

	// Load the log file into a vector, of strings, and test content
    std::vector<std::string> lines;
 	BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(path,lines,true/*IGNORE EMPTY LINE AT THE END*/),"Failed to open log file");
 	BOOST_REQUIRE(lines.size() != 0);
	BOOST_CHECK_MESSAGE(lines.size() == 10," Expected 10 lines in log, but found " << lines.size() << "\n");
	BOOST_CHECK_MESSAGE(lines[0].find("First Message") != string::npos,"Expected first line to contain 'First Message' but found " << lines[0] << "\n");
	BOOST_CHECK_MESSAGE(lines.back().find("Last Message") != string::npos,"Expected last line to contain 'Last Message' but found " << lines.back() << "\n");

	// Clear the log file. Comment out for debugging
	Log::instance()->clear();
	BOOST_CHECK_MESSAGE(fs::file_size( path ) == 0, "Clear of log file failed\n");

	// Remove the log file. Comment out for debugging
	fs::remove(path);

	// Explicitly destroy log. To keep valgrind happy
	Log::destroy();
}


BOOST_AUTO_TEST_CASE( test_log_path )
{
   cout << "ACore:: ...test_log_path\n";

   Log::create("test_log_path.log");

   // make sure path returned is absolute
   std::string path = Log::instance()->path();
   BOOST_REQUIRE_MESSAGE(path[0] == '/',"Expected absolute paths for log file but found " << path);

   // Remove the log file. Comment out for debugging
   fs::remove(path);

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();
}

BOOST_AUTO_TEST_CASE( test_log_new_path_errors )
{
   cout << "ACore:: ...test_log_new_path_errors\n";

   // delete the log file if it exists.
   fs::remove(getLogPath());

   // create a now log file.
   Log::create(getLogPath());
   LOG(Log::MSG,"First Message");
   LOG(Log::LOG,"LOG");

   // Specify bad paths for new log file
   // First test empty path throws
   BOOST_REQUIRE_THROW(Log::instance()->new_path(""),std::runtime_error);

   // If a path is specified make sure parent directory exists
   fs::path current_path = fs::current_path();
   std::string path2 = current_path.string();
   path2 +=  "/a/made/up/path/fred.log";
   //cout << path2<< "\n";
   BOOST_REQUIRE_THROW(Log::instance()->new_path(path2),std::runtime_error);

   // Make sure path does not correspond to a directory
   // cout << "parent directory: " << current_path.parent_path() << "\n";
   BOOST_REQUIRE_THROW(Log::instance()->new_path( current_path.parent_path().string() ),std::runtime_error);

//   {
//      fs::path valid_path = getLogPath();
//      std::cout << "valid_path = " << valid_path << "\n";
//      std::cout << "valid_path.root_path(): " << valid_path.root_path() << "\n";
//      std::cout << "valid_path.root_name() : " << valid_path.root_name()  << "\n";
//      std::cout << "valid_path.root_directory()  : " << valid_path.root_directory()   << "\n";
//      std::cout << "valid_path.relative_path()  : " << valid_path.relative_path()   << "\n";
//      std::cout << "valid_path.parent_path()  : " << valid_path.parent_path()   << "\n";
//      std::cout << "valid_path.filename()  : " << valid_path.filename()   << "\n";
//      std::cout << "valid_path.stem()  : " << valid_path.stem()   << "\n";
//      std::cout << "valid_path.extension()  : " << valid_path.extension()   << "\n";
//   }

   // Remove the log file. Comment out for debugging
   fs::remove(Log::instance()->path());

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();
}


BOOST_AUTO_TEST_CASE( test_log_new_path )
{
   cout << "ACore:: ...test_log_new_path\n";

   // delete the log file if it exists.
   fs::remove(getLogPath());

   // create a new log file.
   Log::create(getLogPath());
   BOOST_CHECK_MESSAGE(fs::exists( Log::instance()->path() ), "Log file should be created after explicit call to Log::create()\n");
   LOG(Log::LOG,"LOG");
   fs::remove(Log::instance()->path());


   // Specify a new log path. Path could be a relative path like "test/logfile.log"
   std::string relative_path = File::test_data("ACore/test/logfile.log","ACore");

   BOOST_REQUIRE_NO_THROW(Log::instance()->new_path( relative_path ));
   BOOST_CHECK_MESSAGE(!fs::exists( Log::instance()->path() ), "Log file should *NOT* be created until first message is logged\n");
   LOG(Log::LOG,"LOG");
   BOOST_CHECK_MESSAGE(fs::exists( Log::instance()->path() ), "Log file should be created after first message is logged\n");
   fs::remove(Log::instance()->path());


   // Specify a new log path. This time we just specify a file name, without a path.
   BOOST_REQUIRE_NO_THROW(Log::instance()->new_path( "testlog.log" ));
   BOOST_CHECK_MESSAGE(!fs::exists( Log::instance()->path() ), "Log file should not be created until first message is logged\n");
   // File not created until a message is logged
   LOG(Log::LOG,"LOG");
   BOOST_CHECK_MESSAGE(fs::exists( Log::instance()->path() ), "Log file should be created after first message is logged\n");
   fs::remove(Log::instance()->path());

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
