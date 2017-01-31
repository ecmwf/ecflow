/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <stdlib.h>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "EcfFile.hpp"
#include "Str.hpp"
#include "File.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_ecf_file_locator )
{
	cout << "ANode:: ...test_ecf_file_locator\n";

	// SET ECF_HOME
   std::string smshome = File::test_data("ANode/test/data/SMSHOME","ANode");

	// Create a defs file corresponding to:
 	//# Test the sms file can be found via ECF_SCRIPT
	//#
	//suite suite
	//	edit ECF_INCLUDE $ECF_HOME/includes
	//	edit SLEEPTIME 10
	//	family family
	//   		task t1
	//   		task t2
	//    		task t3
	//  	endfamily
	//endsuite
	//
	//#
	//# This test suite should force a backwards search since the ecf files
	//# are located in ECF_HOME
	//suite suite1
	//  	family family
	//   		task suite1_task1
	//   		task suite1_task2
	//    		task suite1_task3
	//    endfamily
	//endsuite
	//
	//#
	//# This suite is used to test command substition with ECF_FETCH command
	//#
	//suite suite2
	//	edit ECF_INCLUDE $ECF_HOME/includes
	//	edit ECF_FILES   $ECF_HOME
	//	family family
	//		edit SMSFETCH "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"
	//		task t2
	//	endfamily
	//endsuite
 	Defs theDefs;
 	{
 	   suite_ptr suite = theDefs.add_suite("suite");
		suite->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
 		suite->addVariable( Variable( "SLEEPTIME", "10" ) );
 		family_ptr fam = suite->add_family( "family" );
		fam->add_task( "t1" );
		fam->add_task( "t2" );
		fam->add_task( "t3" );
 	}
 	{
      suite_ptr suite = theDefs.add_suite("suite1");
      family_ptr fam = suite->add_family( "family" );
      fam->add_task( "suite1_task1" );
      fam->add_task( "suite1_task2" );
      fam->add_task( "suite1_task3" );
 	}
 	{
      suite_ptr suite2 = theDefs.add_suite("suite2");
		suite2->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
 		suite2->addVariable( Variable( Str::ECF_FILES(), "$ECF_HOME" ) );
      family_ptr fam = suite2->add_family( "family" );
		fam->addVariable( Variable( Str::ECF_FETCH(), "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%" ) );
 		fam->add_task( "t2" );
 	}
// 	cerr << theDefs << "\n";

	// get all the task, assume non hierarchical families
	std::vector<Task*> theTasks;
 	theDefs.getAllTasks(theTasks);
	BOOST_REQUIRE_MESSAGE(theTasks.size() == 7, "Expected 7 tasks but found, " << theTasks.size());

	// Override ECF_HOME.   ECF_HOME is need to locate to the ecf files
	theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),smshome);


	/// begin , will cause creation of generated variables. The generated variables
	/// are use in client scripts and used to locate the sms files
	theDefs.beginAll();

	// Test for ECF_ file location
  	BOOST_FOREACH(Task* t, theTasks) {
  		try {
  			EcfFile ecf_file = t->locatedEcfFile();
  			BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task ");
  		}
  		catch (std::exception& e) {
  			BOOST_REQUIRE_MESSAGE(false,"Could not locate ecf file for task " << e.what());
  		}
	}
}


BOOST_AUTO_TEST_CASE( test_ecf_file_locator_using_ECF_FILES )
{
   cout << "ANode:: ...test_ecf_file_locator_using_ECF_FILES\n";

   // This test will check we can locate the ecf files in ECF_FILES directory

   // SET ECF_HOME. *** TO A DIRECTORY WHERE THERE ARE NO .ECF FILES **
   std::string smshome = File::test_data("ANode/test/data","ANode");

   // set ECF_FILES where we do have some .ecf files
   std::string ecf_files = File::test_data("ANode/test/data/SMSHOME/suite/family","ANode");

   // Create a defs file corresponding to:
   //# Test the sms file can be found via ECF_SCRIPT
   //#
   //edit ECF_HOME    ANode/test/data
   //edit ECF_FILES   ANode/test/data/SMSHOME/suite/family
   //suite suite
   // edit ECF_INCLUDE $ECF_HOME/includes
   // edit SLEEPTIME 10
   // family family
   //       task t1
   //       task t2
   //       task t3
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("suite");
      suite->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
      suite->addVariable( Variable( "SLEEPTIME", "10" ) );
      family_ptr fam = suite->add_family( "family" );
      fam->add_task( "t1" );
      fam->add_task( "t2" );
      fam->add_task( "t3" );
   }

   // get all the task, assume non hierarchical families
   std::vector<Task*> theTasks;
   theDefs.getAllTasks(theTasks);
   BOOST_REQUIRE_MESSAGE(theTasks.size() == 3, "Expected 3 tasks but found, " << theTasks.size());

   // ECF_HOME, a directory with no .ecf files
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),smshome);
   theDefs.set_server().add_or_update_user_variables(Str::ECF_FILES(),ecf_files);

//    cerr << theDefs << "\n";

   /// begin , will cause creation of generated variables. The generated variables
   /// are use in client scripts and used to locate the sms files
   theDefs.beginAll();

   // Test for ECF_ file location
   BOOST_FOREACH(Task* t, theTasks) {
      try {
         EcfFile ecf_file = t->locatedEcfFile();
         BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task\n");
      }
      catch (std::exception& e) {
         BOOST_REQUIRE_MESSAGE(false,"Could not locate ecf file for task\n" << e.what());
      }
   }
}

BOOST_AUTO_TEST_CASE( test_ecf_file_locator_using_ECF_FILES_variable_substitution )
{
   cout << "ANode:: ...test_ecf_file_locator_using_ECF_FILES_variable_substitution\n";

   // ECFLOW-788
   // This test will check we can locate the ecf files in ECF_FILES directory
   // Will attempt variable substitution, on ECF_FILES is directory does not exist

   // SET ECF_HOME. ***TO A DIRECTORY WHERE THERE ARE NO .ECF FILE **
   std::string smshome = File::test_data("ANode/test/data","ANode");

   // set ECF_FILES where we do have some .ecf files
   std::string ecf_files = File::test_data("ANode/test/data/SMSHOME/suite/%FAMILY%","ANode");

   // Create a defs file corresponding to:
   //# Test the sms file can be found via ECF_SCRIPT
   //#
   //edit ECF_HOME    ANode/test/data
   //edit ECF_FILES   ANode/test/data/SMSHOME/suite/%FAMILY%   # test we variable substitute
   //suite suite
   // edit ECF_INCLUDE $ECF_HOME/includes
   // edit SLEEPTIME 10
   // family family
   //       task t1
   //       task t2
   //       task t3
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("suite");
      suite->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
      suite->addVariable( Variable( "SLEEPTIME", "10" ) );
      family_ptr fam = suite->add_family( "family" );
      fam->addVariable( Variable( "FAMILY", "family" ) );
      fam->add_task( "t1" );
      fam->add_task( "t2" );
      fam->add_task( "t3" );
   }

   // get all the task, assume non hierarchical families
   std::vector<Task*> theTasks;
   theDefs.getAllTasks(theTasks);
   BOOST_REQUIRE_MESSAGE(theTasks.size() == 3, "Expected 3 tasks but found, " << theTasks.size());

   // ECF_HOME, a directory with no .ecf files
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),smshome);
   theDefs.set_server().add_or_update_user_variables(Str::ECF_FILES(),ecf_files);

//    cerr << theDefs << "\n";

   /// begin , will cause creation of generated variables. The generated variables
   /// are use in client scripts and used to locate the sms files
   theDefs.beginAll();

   // Test for ECF_ file location
   BOOST_FOREACH(Task* t, theTasks) {
      try {
         EcfFile ecf_file = t->locatedEcfFile();
         BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task\n");
      }
      catch (std::exception& e) {
         BOOST_REQUIRE_MESSAGE(false,"Could not locate ecf file for task\n" << e.what());
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
