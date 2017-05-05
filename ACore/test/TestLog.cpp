//============================================================================
// Name        : Request
// Author      : Avi
// Revision    : $Revision: #21 $ 
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
#include <fstream>
#include <stdlib.h> // for getenv()
#include <string>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "Log.hpp"
#include "File.hpp"
#include "DurationTimer.hpp"
#include "Pid.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

static std::string getLogPath() {

   // ECFLOW-712, generate unique name for log file, To allow parallel test
   std::string log_file = "ACore/test/logfile";
   log_file += Pid::getpid(); // can throw
   log_file += ".txt";
   return File::test_data(log_file,"ACore");
}

BOOST_AUTO_TEST_CASE( test_log )
{
	cout << "ACore:: ...test_log\n";

	std::string path = getLogPath();

	// delete the log file if it exists.
	fs::remove(path);
	BOOST_REQUIRE_MESSAGE(!fs::exists( path ), "log file not deleted " << path);

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

	BOOST_REQUIRE_MESSAGE(fs::exists( path ), "log file " << path << " not created by previous test\n");

 	LOG(Log::MSG,"Last Message");

	// Load the log file into a vector, of strings, and test content
    std::vector<std::string> lines;
 	BOOST_REQUIRE_MESSAGE(File::splitFileIntoLines(path,lines,true/*IGNORE EMPTY LINE AT THE END*/),"Failed to open log file"<< " (" << strerror(errno) << ")");
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
   std::string path = getLogPath();
   fs::remove(path);

   // create a now log file.
   Log::create(path);
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
   std::string path = getLogPath();
   fs::remove(path);

   // create a new log file.
   Log::create(path);
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

BOOST_AUTO_TEST_CASE( test_get_last_n_lines_from_log )
{
   cout << "ACore:: ...test_get_last_n_lines_from_log\n";

   // delete the log file if it exists.
   std::string path = getLogPath();
   fs::remove(path);
   BOOST_REQUIRE_MESSAGE(!fs::exists( path ), "log file not deleted " << path);

   // Create the log file;
   Log::create(path);
   BOOST_CHECK_MESSAGE(fs::exists( path ), "log file " << path << " not created \n");

   // Log file should be empty
   const int NO_OF_LINES_IN_LOG_FILE = 200;
   {
      for(int i =0; i < NO_OF_LINES_IN_LOG_FILE; i++) {
         std::string line = Log::instance()->contents(i);
         BOOST_CHECK_MESSAGE(line.empty(), "Expected empty string but found\n" << line);
      }
   }

   // Populate the log file
   std::string msg = "This is message ";
   for(int i = 0; i < NO_OF_LINES_IN_LOG_FILE; ++i)  LOG(Log::MSG,msg << i);

   // Now check, getting the lines
   {
      std::string line = Log::instance()->contents(0);
      BOOST_CHECK_MESSAGE(line.empty(), "Expected empty string but found\n" << line);
   }
   {
      // Check we get back the number of line requested
      for(int i = 0; i< NO_OF_LINES_IN_LOG_FILE; i++) {
         std::string lines = Log::instance()->contents(i);
         int newlineCount = std::count( lines.begin(), lines.end(), '\n');
         BOOST_CHECK_MESSAGE(i == newlineCount, "expected to  " << i << " newlines but found "  <<  newlineCount);
      }
   }
   {
      // Check we get back *ALL* lines requested
      std::string lines = Log::instance()->contents(NO_OF_LINES_IN_LOG_FILE);
      for(int i = 0; i< NO_OF_LINES_IN_LOG_FILE; i++) {
         std::stringstream ss; ss << msg << i;
         std::string str_to_find = ss.str();
         BOOST_CHECK_MESSAGE(lines.find(str_to_find) != std::string::npos, "expected to find " << str_to_find << " in the log file");
      }
   }

   {
      // Request more than is available, should only get back whats there
      std::string lines = Log::instance()->contents(NO_OF_LINES_IN_LOG_FILE*2);
      int newlineCount = std::count( lines.begin(), lines.end(), '\n');
      BOOST_CHECK_MESSAGE(NO_OF_LINES_IN_LOG_FILE == newlineCount, "expected " << NO_OF_LINES_IN_LOG_FILE << " newlines but found "  <<  newlineCount);
   }

   fs::remove(Log::instance()->path());

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();
}

BOOST_AUTO_TEST_CASE( test_get_first_n_lines_from_log )
{
   cout << "ACore:: ...test_get_first_n_lines_from_log\n";

   // delete the log file if it exists.
   std::string path = getLogPath();
   fs::remove(path);
   BOOST_REQUIRE_MESSAGE(!fs::exists( path ), "log file not deleted " << path << " not created \n");

   // Create the log file;
   Log::create(path);
   BOOST_CHECK_MESSAGE(fs::exists( path ), "log file " << path << " not created \n");

   // Populate the log file
   const int NO_OF_LINES_IN_LOG_FILE = 200;
   std::string msg = "This is message ";
   for(int i = 0; i < NO_OF_LINES_IN_LOG_FILE; ++i)  LOG(Log::MSG,msg << i);

   // Now check, getting the lines
   {
      // Get the first line
      std::string line = Log::instance()->contents(-1);
      std::string expected = msg + "0";
      BOOST_CHECK_MESSAGE(line.find(expected) != std::string::npos, "Expected '" << expected << "' but found\n" << line);
   }
   {
      // Get the first & second line
      std::string line = Log::instance()->contents(-2);
      std::string expected0 = msg + "0";
      std::string expected1 = msg + "1";
      BOOST_CHECK_MESSAGE(line.find(expected0) != std::string::npos, "Expected '" << expected0 << "' but found\n" << line);
      BOOST_CHECK_MESSAGE(line.find(expected1) != std::string::npos, "Expected '" << expected1 << "' but found\n" << line);
   }
   {
      // Check we get back the number of line requested
      for(int i = 0; i< NO_OF_LINES_IN_LOG_FILE; i++) {
         std::string lines = Log::instance()->contents(-i);
         int newlineCount = std::count( lines.begin(), lines.end(), '\n');
         BOOST_CHECK_MESSAGE(i == newlineCount, "expected to  " << i << " newlines but found "  <<  newlineCount);
      }
   }
   {
      std::string lines = Log::instance()->contents(-NO_OF_LINES_IN_LOG_FILE);
      for(int i = 0; i < NO_OF_LINES_IN_LOG_FILE; i++){
         std::stringstream ss; ss << msg << i;
         std::string expected = ss.str();
         BOOST_CHECK_MESSAGE(lines.find(expected) != std::string::npos, "Expected '" << expected << "' but found for i " << i);
      }
   }

   {
      // Request more than is available, should only get back whats there
      std::string lines = Log::instance()->contents(-NO_OF_LINES_IN_LOG_FILE*2);
      int newlineCount = std::count( lines.begin(), lines.end(), '\n');
      BOOST_CHECK_MESSAGE(NO_OF_LINES_IN_LOG_FILE == newlineCount, "expected " << NO_OF_LINES_IN_LOG_FILE << " newlines but found "  <<  newlineCount);
   }

   fs::remove(Log::instance()->path());

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();
}


BOOST_AUTO_TEST_CASE( test_get_log_timing )
{
   cout << "ACore:: ...test_get_log_timing: " << flush;

   // *************************************************************************************
   // This test was used with *DIFFERENT* implementations for Log::instance()->contents(1)
   // What is shows, is that for optimal performance we should *NOT* load the entire log file
   // This can be several giga bytes.
   // **************************************************************************************

   // delete the log file if it exists.
   std::string path = getLogPath();
   fs::remove(getLogPath());
   BOOST_REQUIRE_MESSAGE(!fs::exists( path ), "log file not deleted " << path << " not created \n");

   // Create the log file;
   Log::create(path);
   BOOST_CHECK_MESSAGE(fs::exists( path ), "log file " << path << " not created \n");

   // Populate the log file
   const int NO_OF_LINES_IN_LOG_FILE = 20000;
   std::string msg = "This is message ";
   for(int i = 0; i < NO_OF_LINES_IN_LOG_FILE; ++i)  LOG(Log::MSG,msg << i);

   DurationTimer timer;

   {
      const int LOOP = 100;
      for(int i = 0; i< LOOP; i++) {
         std::string lines = Log::instance()->contents(1);
         BOOST_CHECK_MESSAGE(!lines.empty(), "expected entry");
      }
   }

   fs::remove(Log::instance()->path());

   // Explicitly destroy log. To keep valgrind happy
   Log::destroy();

   cout << timer.duration() << "s\n" << flush;
}

BOOST_AUTO_TEST_SUITE_END()
