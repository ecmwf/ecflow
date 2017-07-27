//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
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
#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h> // for getenv()

#include "DurationTimer.hpp"
#include "File.hpp"
#include "NodePath.hpp"

using namespace boost;
using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

//#define FILE_PERF_CHECK_IMPLEMENTATIONS 1;

BOOST_AUTO_TEST_CASE( test_splitFileIntoLines )
{
   // This is sanity test for splitFileIntoLines used extensively
   cout << "ACore:: ...test_splitFileIntoLines\n";

   std::string path = File::test_data("ACore/test/data/test_splitFileIntoLines.txt","ACore");

   std::string theText = "This is a test string";
   {
      {
         std::ofstream file_with_one_line( path.c_str() );
         file_with_one_line << theText;
      }

      std::vector<std::string> lines;
      BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path << " (" << strerror(errno) << ")");
      BOOST_CHECK_MESSAGE( lines.size() == 1," Expected 1 line but found " << lines.size());

      fs::remove(path); // Remove the file. Comment out for debugging
   }

   {
      {
         std::ofstream file_with_one_line( path.c_str() );
         file_with_one_line << theText << "\n";  // addition of '/n' , should still be one line
      }

      std::vector<std::string> lines;
      BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path << " (" << strerror(errno) << ")");
      BOOST_CHECK_MESSAGE( lines.size() == 1," Expected 1 line but found " << lines.size());

      fs::remove(path); // Remove the file. Comment out for debugging
   }

   {
      {
         std::ofstream file_with_two_line( path.c_str() );
         file_with_two_line << theText << "\n";
         file_with_two_line << theText;
      }

      std::vector<std::string> lines;
      BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path << " (" << strerror(errno) << ")");
      BOOST_CHECK_MESSAGE( lines.size() == 2," Expected 2 line but found " << lines.size());

      fs::remove(path); // Remove the file. Comment out for debugging
   }

   {
      {
         std::ofstream file_with_three_line( path.c_str() );
         file_with_three_line << theText << "\n";
         file_with_three_line << theText << "\n";
         file_with_three_line << theText;
      }

      std::vector<std::string> lines;
      BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path<< " (" << strerror(errno) << ")");
      BOOST_CHECK_MESSAGE( lines.size() == 3," Expected 3 line but found " << lines.size());

      fs::remove(path); // Remove the file. Comment out for debugging
   }

   {
      {
         std::ofstream file_with_three_line( path.c_str() );
         file_with_three_line << theText << "\n";
         file_with_three_line << theText << "\n";
         file_with_three_line << theText << "\n";
      }

      std::vector<std::string> lines;
      BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path<< " (" << strerror(errno) << ")");
      BOOST_CHECK_MESSAGE( lines.size() == 3," Expected 3 line but found " << lines.size());

      fs::remove(path); // Remove the file. Comment out for debugging
   }
}

BOOST_AUTO_TEST_CASE( test_file_tokenizer )
{
	cout << "ACore:: ...test_file_tokenizer\n";

   std::string path = File::test_data("ACore/test/data/test_file_tokenizer.txt","ACore");

	size_t linesWithText = 100;
	std::string theText = "This is a test string";
	{
		std::ofstream file( path.c_str() );
		for(size_t i =0; i < linesWithText; i++) {
			if (i % 2 == 0) file << "\n";       // 51 empty lines
			file << theText << "\n";            // 100  text lines
		}
	}
	{
		std::vector<std::string> lines;
		BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines,true/*ignore empty lines*/)," Failed to open file " << path<< " (" << strerror(errno) << ")");
		BOOST_CHECK_MESSAGE( lines.size() == linesWithText,"Expected " << linesWithText << " but found " <<  lines.size());
		BOOST_CHECK_MESSAGE( lines[0] == theText,"Expected '" << theText << "' but found " <<  lines[0]);
		BOOST_CHECK_MESSAGE( lines[linesWithText-1] == theText,"Expected '" << theText << "' but found " <<  lines[linesWithText-1]);

		lines.clear();
		size_t totalLines = 151;
		BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path<< " (" << strerror(errno) << ")");
		BOOST_CHECK_MESSAGE( lines.size() == totalLines-1,"Expected " << totalLines-1 << " but found " <<  lines.size());
		BOOST_CHECK_MESSAGE( lines[0] == "","Expected empty string  but found " <<  lines[0]);
		BOOST_CHECK_MESSAGE( lines[1] == theText,"Expected '" << theText << "' but found " <<  lines[1]);
		BOOST_CHECK_MESSAGE( lines[2] == theText,"Expected '" << theText << "' but found " <<  lines[2]);
		BOOST_CHECK_MESSAGE( lines[3] == "","Expected empty string  but found " <<  lines[3]);
   }

#ifdef FILE_PERF_CHECK_IMPLEMENTATIONS
	{
		size_t openFileNTimes = 100000;
	 	boost::timer timer; // measures CPU, replace with cpu_timer with boost > 1.51, measures cpu & elapsed
 		for (size_t i = 0; i < openFileNTimes; i++) {
			std::vector<std::string> lines;
			BOOST_CHECK_MESSAGE( File::splitFileIntoLines(path,lines)," Failed to open file " << path<< " (" << strerror(errno) << ")");
  		}
		cout << "Time for opening file " << openFileNTimes << " times = " << timer.elapsed() << "\n";
	}
#endif

	// Remove the file. Comment out for debugging
	fs::remove(path);
}

BOOST_AUTO_TEST_CASE( test_file_backwardSearch )
{
	cout << "ACore:: ...test_file_backwardSearch\n";

	std::string nodePath = "dir0/dir1/dir2/dir3/dir4/dir5";
   std::string rootPath = File::test_data("ACore/test/data","ACore");
   std::string expected = File::test_data("ACore/test/data/","ACore") + nodePath;

	std::string path = rootPath;
	std::string dir = "dir";
 	for(int i = 0; i < 6; i++) {
		path += "/" + dir + boost::lexical_cast<std::string>(i);
  	}
	// Should have test/data/dir0/dir1/dir3/dir3/dir4/dir5
	//         or  ACore/test/data/dir0/dir1/dir3/dir3/dir4/dir5
 	BOOST_REQUIRE_MESSAGE(path == expected," Error expected " << expected << " but found " << path);

	// Create the missing directories
 	BOOST_REQUIRE_MESSAGE(File::createDirectories(path),"Failed to create dirs");

	// Create a file in each of the directories. See Page 21 SMS User Guide.
	std::vector<std::string> fileContents; fileContents.push_back("something");
	vector<std::string> nodePathTokens;
 	NodePath::split(nodePath,nodePathTokens);
 	while ( nodePathTokens.size() > 0 ) {

		// Reconstitute the path
	 	std::string path = NodePath::createPath(nodePathTokens);
 		std::string combinedPath = rootPath + path;

		BOOST_REQUIRE_MESSAGE(File::createDirectories(combinedPath),"Failed to create dirs " << combinedPath);

		combinedPath += File::ECF_EXTN(); // .ecf, .man , etc

		//std::cout << "Creating file " << combinedPath << "\n";
		std::string errorMsg;
		BOOST_REQUIRE_MESSAGE(File::create(combinedPath,fileContents,errorMsg),"Failed to create " << combinedPath << " because " << errorMsg);

		nodePathTokens.erase(nodePathTokens.begin()); // consume first path token
	}

	// Now do a backward search for them
	int filesFound = 0;
	for(int i = 0; i < 6; i++) {
		std::string theFile = File::backwardSearch(rootPath,nodePath,File::ECF_EXTN() );
		BOOST_CHECK_MESSAGE( !theFile.empty(), i << ": Failed to find dir5.ecf with rootPath " << rootPath << " and node path " << nodePath);
		if (!theFile.empty()) {
			filesFound++;
//			std::cout << "About to remove file " << theFile << "\n";
			fs::remove(theFile); // remove it so we don't find it again.
		}
	}
	// Expect the following files to be found:
	//	test/data/dir0/dir1/dir2/dir3/dir4/dir5.ecf
	//	test/data/dir1/dir2/dir3/dir4/dir5.ecf
	//	test/data/dir2/dir3/dir4/dir5.ecf
	//	test/data/dir3/dir4/dir5.ecf
	//	test/data/dir4/dir5.ecf
	//	test/data/dir5.ecf
	BOOST_CHECK_MESSAGE( filesFound == 6 ," expect to find 6 files but found " << filesFound );

	// Remove the test dir. Comment out for debugging
 	for(int i = 0; i < 6; i++) {
 		path = rootPath + "/" + dir + boost::lexical_cast<std::string>(i);
  		BOOST_CHECK_MESSAGE(File::removeDir( path ),"Failed to remove dir " << path);
   	}
}


BOOST_AUTO_TEST_CASE( test_create_missing_directories )
{
   cout << "ACore:: ...test_create_missing_directories";

   // This test FAIL's randomly on the cray in BATCH mode, but passes in interactive mode.
   if (getenv("ECFLOW_CRAY_BATCH")) {
      cout << " **** SKIPPING test, until HPC team can  fix File::createMissingDirectories.(like mkdir -p)  *****\n";
      return;
   }
   cout << "\n";

   std::string nodePath = "dir0/dir1/dir2/dir3/dir4/dir5";
   std::string rootPath = File::test_data("ACore/test/data","ACore");
   std::string expected = File::test_data("ACore/test/data/","ACore") + nodePath;

   std::string dir_remove = rootPath + "/dir0";
   {
      // Test basics first, expect "ACore/test/data/dir0/dir1/dir2/dir3/dir4/dir5" to be created
      BOOST_CHECK_MESSAGE(File::createMissingDirectories(expected),expected << " expected directories to be created");
      BOOST_CHECK_MESSAGE(fs::exists(expected),expected << " directory not created");

      // remove the directory
      BOOST_CHECK_MESSAGE(File::removeDir( dir_remove ),"Failed to remove dir " << dir_remove);

//      fs::path fs_path(expected);
//      std::cout << "parent path " << fs_path.parent_path() << "\n";
//      std::cout << "root path " << fs_path.root_path()  << "\n";
//      std::cout << "root name " << fs_path.root_name()  << "\n";
//      std::cout << "root directory " << fs_path.root_directory()  << "\n";
//      std::cout << "relative_path " << fs_path.relative_path()  << "\n";
//      std::cout << "filename " << fs_path.filename()  << "\n";
//      std::cout << "stem " << fs_path.stem()  << "\n";
//      std::cout << "extension " << fs_path.extension()  << "\n";
   }
   {
      // Test "ACore/test/data/dir0/dir1/dir2/dir3/dir4/dir5/fred.ecf" to be created
      std::string dir_with_file = expected + "/fred.ecf";
      BOOST_CHECK_MESSAGE(File::createMissingDirectories(dir_with_file),"Expected '" <<  dir_with_file << "' to be created");
      BOOST_CHECK_MESSAGE(fs::exists(expected),expected << " directory not created");

      // remove the directory
      BOOST_CHECK_MESSAGE(File::removeDir( dir_remove ),"Failed to remove dir " << dir_remove);

//      fs::path fs_path(dir_with_file);
//      std::cout << "parent path " << fs_path.parent_path() << "\n";
//      std::cout << "root path " << fs_path.root_path()  << "\n";
//      std::cout << "root name " << fs_path.root_name()  << "\n";
//      std::cout << "root directory " << fs_path.root_directory()  << "\n";
//      std::cout << "relative_path " << fs_path.relative_path()  << "\n";
//      std::cout << "filename " << fs_path.filename()  << "\n";
//      std::cout << "stem " << fs_path.stem()  << "\n";
//      std::cout << "extension " << fs_path.extension()  << "\n";
   }

   {
      // Create directories twice. Need to minimise call to fstat
      BOOST_CHECK_MESSAGE(File::createMissingDirectories(expected),"expected file to be created");
      BOOST_CHECK_MESSAGE(File::createMissingDirectories(expected),"expected file to be created");
      BOOST_CHECK_MESSAGE(fs::exists(expected),expected << " directory not created");

      // remove the directory
      BOOST_CHECK_MESSAGE(File::removeDir( dir_remove ),"Failed to remove dir " << dir_remove);
   }

   {
      // Create directories twice. Need to minimise call to fstat
      std::string dir_with_file = expected + "/fred.ecf";
      BOOST_CHECK_MESSAGE(File::createMissingDirectories(dir_with_file),"expected file to be created");
      BOOST_CHECK_MESSAGE(File::createMissingDirectories(dir_with_file),"expected file to be created");
      BOOST_CHECK_MESSAGE(fs::exists(expected),expected << " directory not created");

      // remove the directory
      BOOST_CHECK_MESSAGE(File::removeDir( dir_remove ),"Failed to remove dir " << dir_remove);
   }
}

BOOST_AUTO_TEST_CASE( test_get_last_lines_of_a_file )
{
   cout << "ACore:: ...test_get_last_lines_of_a_file\n";

   std::string path = File::test_data("ACore/test/data/test_get_last_lines_of_a_file.txt","ACore");
   std::string last_100_lines;
   size_t no_of_lines = 100;
   { // create file with 100 lines 0-99
      std::stringstream ss;
      std::ofstream file( path.c_str() );
      for(size_t i=0; i < no_of_lines; i++) {
         file << i << ": the line\n";
         ss << i << ": the line\n";
      }
      last_100_lines = ss.str();
   }
   { // get negative lines
       std::string error_msg;
       std::string last_lines = File::get_last_n_lines(path,-1,error_msg);
       BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
       BOOST_REQUIRE_MESSAGE(last_lines == "" ,"Expected '' but found " <<  last_lines );
    }
   { // get no lines
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,0,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "" ,"Expected '' but found " <<  last_lines );
   }
   { // get the last line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,1,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "99: the line\n" ,"Expected '99: the line\n' but found " <<  last_lines );
   }
   { // get the last 2 line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,2,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "98: the line\n99: the line\n" ,"Expected last 2 lines but got " <<  last_lines );
   }
   { // get the last 100 line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,no_of_lines,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == last_100_lines,"Expected last " << no_of_lines << " lines but got " <<  last_lines );
   }
   { // get the last 1000 line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,1000,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == last_100_lines,"Expected last " << no_of_lines << " lines but got " <<  last_lines );
   }

   fs::remove(path); // Remove the file. Comment out for debugging


   // ===================================================================================
   // Now check empty file
   // ===================================================================================
   {
      std::ofstream file( path.c_str() ); // create empty file
   }

   { // get no lines ?
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,0,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "" ,"Expected '' but found " <<  last_lines );
   }
   { // get the last line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,1,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "" ,"Expected '' but found " <<  last_lines );
   }
   { // get the last 2 line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,2,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "" ,"Expected '' but found " <<  last_lines );
   }
   { // get the last 100 line only
      std::string error_msg;
      std::string last_lines = File::get_last_n_lines(path,no_of_lines,error_msg);
      BOOST_REQUIRE_MESSAGE(error_msg.empty(),"Expected no failure but got " << error_msg );
      BOOST_REQUIRE_MESSAGE(last_lines == "" ,"Expected '' but found " <<  last_lines );
   }

   fs::remove(path); // Remove the file. Comment out for debugging
}

BOOST_AUTO_TEST_SUITE_END()
