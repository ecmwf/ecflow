/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2019 ECMWF.
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
#include "Pid.hpp"

using namespace std;
using namespace ecf;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )


void located_ecf_file(task_ptr task, EcfFile::Origin expected_origin, EcfFile::EcfFileSearchAlgorithm expected_search, int line) {
   try {
      task->update_generated_variables();
      EcfFile ecf_file = task->locatedEcfFile();
      //cout << "located_ecf_file task: " << task->absNodePath() << " " << ecf_file.ecf_file_origin_dump() << "\n";
      BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "\nline:" << line << " Expected search to pass with " << EcfFile::search_algorithm_str(expected_search));
      BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == expected_origin, "\nline:" << line  << " Expected ecf_file to be obtained from(" << EcfFile::origin_str(expected_origin) << ") but found " << ecf_file.ecf_file_origin_dump());
      BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_search_algorithm() == expected_search, "\nline:" << line << " Expected search " << EcfFile::search_algorithm_str(expected_search) << " but found " << EcfFile::search_algorithm_str(ecf_file.ecf_file_search_algorithm()));
   }
   catch (std::exception& e) {
      BOOST_REQUIRE_MESSAGE(true,"Expected search to succeed " << e.what());
   }
}

void located_ecf_file_fail(task_ptr task, int line) {
   task->update_generated_variables();
   try {
      EcfFile ecf_file = task->locatedEcfFile();
      BOOST_REQUIRE_MESSAGE( !ecf_file.valid(), "\nline:" << line << " Expected search to fail with standard look up(i.e PRUNE_ROOT) "<< ecf_file.ecf_file_origin_dump() );
   }
   catch (std::exception& e) {
      BOOST_REQUIRE_MESSAGE(true,"\nline:" << line << "Expected search to fail " << e.what());
   }
}

void create_ecf_file(const std::string& ecf_file_location) {
   //cout << "create file  = " << ecf_file_location << "\n";
   // generate the ecf file;
   string header = "%include <simple_head.h>\n";
   string body = "#body\n";
   string tail = "%include <simple_tail.h>\n";
   string ecf_file = header;
   ecf_file += body;
   ecf_file += tail;

   BOOST_CHECK_MESSAGE(File::createMissingDirectories(ecf_file_location),"Could not create missing dir\n");
   string errormsg;
   BOOST_CHECK_MESSAGE(File::create(ecf_file_location, ecf_file , errormsg), errormsg);
   BOOST_CHECK_MESSAGE(fs::exists(ecf_file_location), "Expected File " << ecf_file_location << " to exist");
}


BOOST_AUTO_TEST_CASE(  test_ecf_file_search )
{
   cout << "ANode:: ...test_ecf_file_search\n";
   // Do test which fails first, i.e does not find the ecf file, then add variable ECF_FILES_LOOKUP(PRUNE_LEAF)
   // The test should then succeed
   // Note: Setting ECF_FILES_LOOKUP(PRUNE_LEAF) or ECF_FILES_LOOKUP(PRUNE_ROOT)/default affects the lookup for
   //       ecf files in BOTH ECF_HOME and ECF_LISTS
   // PRUNE_ROOT/defaults
   //    <root>/suite/f1/f2/f3/task.ecf
   //    <root>/f1/f2/f3/task.ecf
   //    <root>/f2/f3/task.ecf
   //    <root>/f3/task.ecf
   //    <root>/task.ecf
   // PRUNE_LEAF( new from ecflow 4.12.0)
   //    <root>/suite/f1/f2/f3/task.ecf
   //    <root>/suite/f1/f2/task.ecf
   //    <root>/suite/f1/task.ecf
   //    <root>/suite/task.ecf
   //    <root>/task.ecf
   // IMPORTANT: The first and last lookups are the *SAME* for both search lookups.

   // Create a defs file corresponding to:
   //suite test_ecf_file_search
   // edit ECF_INCLUDE $ECF_HOME/includes
   // family f1
   //    family f2
   //       family f3
   //          task task
   Defs theDefs;
   suite_ptr suite = theDefs.add_suite(Pid::unique_name("test_ecf_file_search"));
   suite->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
   family_ptr f1 = suite->add_family( "f1" ) ;
   family_ptr f2 = f1->add_family( "f2" ) ;
   family_ptr f3 = f2->add_family( "f3" ) ;
   task_ptr task = f3->add_task( "task" ) ;

   // Override ECF_HOME. ECF_HOME is as default location for .ecf files, when ECF_INCLUDE not specified
   // or when file does not exist in ECF_INCLUDE
   std::string ecf_home = File::test_data("ANode/test/data","ANode");
   std::string ecf_lists = File::test_data("ANode/test/data","ANode");
   suite->add_variable(Str::ECF_HOME(),ecf_home);
   //cerr << theDefs << "\n";

   /// begin , will cause creation of generated variables. The generated variables
   /// are use in client scripts and used to locate the ecf files
   theDefs.beginAll();

   {
      //cout << "Test ECF_HOME/ECF_LISTS at leaf location this should be using ECF_SCRIPT\n";
      string ecf_file_location  = ecf_home + task->absNodePath() +  File::ECF_EXTN();
      create_ecf_file(ecf_file_location);

      located_ecf_file(task,EcfFile::ECF_SCRIPT,EcfFile::PRUNE_ROOT,__LINE__);
      suite->add_variable( "ECF_FILES_LOOKUP", "prune_leaf" );                 // change look up method
      located_ecf_file(task,EcfFile::ECF_SCRIPT,EcfFile::PRUNE_ROOT,__LINE__); // ECF_FILES_LOOKUP make *NO* difference for lookup with EcfFile::ECF_SCRIPT

      suite->deleteVariable("ECF_FILES_LOOKUP"); // cleanup

      suite->add_variable(Str::ECF_FILES(),ecf_lists);
      located_ecf_file(task,EcfFile::ECF_SCRIPT ,EcfFile::PRUNE_ROOT,__LINE__);// ECF_FILES make no difference for lookup with EcfFile::ECF_SCRIPT
      suite->add_variable( "ECF_FILES_LOOKUP", "prune_leaf" );                 // change look up method
      located_ecf_file(task,EcfFile::ECF_SCRIPT,EcfFile::PRUNE_ROOT,__LINE__); // ECF_FILES_LOOKUP make *NO* difference for lookup with EcfFile::ECF_SCRIPT

      suite->deleteVariable("ECF_FILES_LOOKUP");   // cleanup
      suite->deleteVariable("ECF_FILES");          // cleanup
      boost::filesystem::remove(ecf_file_location);// cleanup
   }
   {
      //cout << "Test ECF_HOME/ECF_LISTS at root location\n";
      string ecf_file_location  = ecf_home + "/" + task->name() +  File::ECF_EXTN();
      create_ecf_file(ecf_file_location);

      located_ecf_file(task,EcfFile::ECF_HOME,EcfFile::PRUNE_ROOT,__LINE__);
      suite->add_variable( "ECF_FILES_LOOKUP", "prune_leaf" );               // change look up method
      located_ecf_file(task,EcfFile::ECF_HOME,EcfFile::PRUNE_LEAF,__LINE__);

      suite->deleteVariable("ECF_FILES_LOOKUP"); // cleanup

      suite->add_variable(Str::ECF_FILES(),ecf_lists);
      located_ecf_file(task,EcfFile::ECF_FILES,EcfFile::PRUNE_ROOT,__LINE__);
      suite->add_variable( "ECF_FILES_LOOKUP", "prune_leaf" );                 // change look up method
      located_ecf_file(task,EcfFile::ECF_FILES,EcfFile::PRUNE_LEAF,__LINE__);

      suite->deleteVariable("ECF_FILES_LOOKUP");   // cleanup
      suite->deleteVariable("ECF_FILES");          // cleanup
      boost::filesystem::remove(ecf_file_location);// cleanup
   }
   {
      //cout << "Test ECF_HOME at intermediate location prune_leaf\n";
      Node* node = task.get();
      node = node->parent();
      node = node->parent();
      while(node) {

         string ecf_file_location = ecf_home + node->absNodePath() + "/" + task->name() + File::ECF_EXTN();
         create_ecf_file(ecf_file_location);

         // cout << "Node: " << node->absNodePath() << "\n";
         located_ecf_file_fail(task,__LINE__);

         // change look up method
         suite->add_variable( "ECF_FILES_LOOKUP", "prune_leaf" );
         located_ecf_file(task,EcfFile::ECF_HOME,EcfFile::PRUNE_LEAF,__LINE__);

         // Remove this file, required for the following test
         boost::filesystem::remove(ecf_file_location);

         // Remove ECF_FILES_LOOKUP variable
         suite->deleteVariable("ECF_FILES_LOOKUP");

         node = node->parent();
      }
      /// Remove all the generated files
      boost::filesystem::remove_all( ecf_home + suite->absNodePath() );
   }
   {
      //cout << "Test ECF_FILES at intermediate location, by prune_leaf\n";
      suite->add_variable(Str::ECF_FILES(),ecf_lists);
      Node* node = task.get();
      node = node->parent();
      node = node->parent();
      while(node) {

         string ecf_file_location =  ecf_lists + node->absNodePath() + "/" + task->name() + File::ECF_EXTN();
         create_ecf_file(ecf_file_location);

         //cout << "Node: " << node->absNodePath() << "\n";
         located_ecf_file_fail(task,__LINE__);

         // change look up method
         suite->add_variable( "ECF_FILES_LOOKUP", "prune_leaf" );
         located_ecf_file(task,EcfFile::ECF_FILES,EcfFile::PRUNE_LEAF,__LINE__);

         // Remove this file, required for the following test
         boost::filesystem::remove(ecf_file_location);

         // Remove ECF_FILES_LOOKUP variable
         suite->deleteVariable("ECF_FILES_LOOKUP");

         node = node->parent();
      }

      /// Remove all the generated files
      boost::filesystem::remove_all( ecf_home + suite->absNodePath() );
   }
}


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
   {
      suite_ptr suite = theDefs.add_suite("suite3");
      suite->addVariable( Variable( Str::ECF_INCLUDE(), "$ECF_HOME/includes" ) );
      suite->addVariable( Variable( Str::ECF_FILES(), "$ECF_HOME" ) );
      family_ptr fam = suite->add_family( "family" );
      fam->addVariable( Variable( "ECF_SCRIPT_CMD", "script_cmd -F %ECF_FILES% -I %ECF_INCLUDE%" ) );
      fam->add_task( "t2" );
   }
// 	cerr << theDefs << "\n";

	// get all the task, assume non hierarchical families
	std::vector<Task*> theTasks;
 	theDefs.getAllTasks(theTasks);
	BOOST_REQUIRE_MESSAGE(theTasks.size() == 8, "Expected 8 tasks but found, " << theTasks.size());

	// Override ECF_HOME.   ECF_HOME is need to locate to the ecf files
	theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),smshome);


	/// begin , will cause creation of generated variables. The generated variables
	/// are use in client scripts and used to locate the sms files
	theDefs.beginAll();

	// Test for ECF_ file location
  	BOOST_FOREACH(Task* t, theTasks) {
  		try {
  			EcfFile ecf_file = t->locatedEcfFile();
  		   //cout << "Task: " << t->absNodePath() << " " << ecf_file.ecf_file_origin_dump() << "\n";
         BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task ");
         Suite* suite = t->suite();
         if (suite->name() == "suite")   BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == EcfFile::ECF_SCRIPT, "Expected ecf_file to be obtained from ECF_SCRIPT " << ecf_file.ecf_file_origin_dump());
         if (suite->name() == "suite1")  BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == EcfFile::ECF_HOME, "Expected ecf_file to be obtained from ECF_HOME "<< ecf_file.ecf_file_origin_dump());
         if (suite->name() == "suite2")  BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == EcfFile::ECF_FETCH_CMD, "Expected ecf_file to be obtained from ECF_FETCH_CMD "<< ecf_file.ecf_file_origin_dump());
         if (suite->name() == "suite3")  BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == EcfFile::ECF_SCRIPT_CMD, "Expected ecf_file to be obtained from ECF_SCRIPT_CMD "<< ecf_file.ecf_file_origin_dump());
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
         //cout << "Task: " << t->absNodePath() << " " << ecf_file.ecf_file_origin_dump() << "\n";
         BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task " << t->absNodePath());
         BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == EcfFile::ECF_FILES, "Expected ecf_file to be obtained from ECF_FILES "<< ecf_file.ecf_file_origin_dump());
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
         //cout << "Task: " << t->absNodePath() << " " << ecf_file.ecf_file_origin_dump() << "\n";
         BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task " << t->absNodePath());
         BOOST_REQUIRE_MESSAGE( ecf_file.ecf_file_origin() == EcfFile::ECF_FILES, "Expected ecf_file to be obtained from ECF_FILES "<< ecf_file.ecf_file_origin_dump());
      }
      catch (std::exception& e) {
         BOOST_REQUIRE_MESSAGE(false,"Could not locate ecf file for task\n" << e.what());
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
