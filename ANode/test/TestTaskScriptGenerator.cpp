/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <stdlib.h>

#include "boost/make_shared.hpp"
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

BOOST_AUTO_TEST_CASE( test_task_script_generator )
{
   cout << "ANode:: ...test_task_script_generator\n";

   // SET ECF_HOME
   std::string ecf_home = File::test_data("ANode/test/data/TaskScriptGenerator","ANode");

   std::string head = ecf_home + "/head.h";
   std::string tail = ecf_home + "/tail.h";
   fs::remove_all(ecf_home);
   BOOST_REQUIRE_MESSAGE( !fs::exists(head), "Remove of head file failed");
   BOOST_REQUIRE_MESSAGE( !fs::exists(tail), "Remove of tail file failed");


   // Create a defs file corresponding to:
   //suite suite
   // edit ECF_INCLUDE $ECF_HOME/includes
   // edit SLEEP 10
   // task t1
   //    family f1
   //       task t1
   //       task t2
   //    endfamily
   //endsuite
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("suite");
      suite->add_variable( Str::ECF_INCLUDE(),ecf_home );
      suite->add_variable( Str::ECF_HOME(), ecf_home );
      suite->add_variable(  "SLEEP","10" );
      task_ptr t1 = suite->add_task("t1");
      t1->addEvent( Event("event1") );
      t1->addEvent( Event("event2") );
      t1->addEvent( Event("event4") );
      t1->addMeter( Meter("meter1",1,100,90));
      t1->addMeter( Meter("meter2",1,100,90));
      t1->addLabel( Label("label","label value"));
      family_ptr fam = suite->add_family("f1");
      fam->add_task("t1");
      fam->add_task("t2");
//    cerr << theDefs << "\n";
   }

   /// generate the scripts and head.h and tail.h
   theDefs.generate_scripts();

   /// Test the ecf file were created, by doing job creation
   /// JobCreationCtrl is used control what node we generate the jobs for:
   /// Since we have not set the node on it, we force job generation for all tasks
   job_creation_ctrl_ptr jobCtrl = boost::make_shared<JobCreationCtrl>();
   theDefs.check_job_creation(jobCtrl);
   BOOST_REQUIRE_MESSAGE(jobCtrl->get_error_msg().empty(), jobCtrl->get_error_msg());
   BOOST_REQUIRE_MESSAGE(jobCtrl->fail_submittables().empty(),"Expected no failing tasks");

   /// Additional sanity tests #######################################################

   /// test that header and tail file were created
   BOOST_REQUIRE_MESSAGE( fs::exists(head), "Head file " << head << " not created");
   BOOST_REQUIRE_MESSAGE( fs::exists(tail), "Tail file " << tail << " not created");

   // get all the task, assume non hierarchical families
   std::vector<Task*> theTasks;
   theDefs.getAllTasks(theTasks);

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

   // Remove the directories that were generated
   fs::remove_all(ecf_home);
}


BOOST_AUTO_TEST_CASE( test_task_script_generator_with_dummy_tasks )
{
   cout << "ANode:: ...test_task_script_generator_with_dummy_tasks\n";

   // SET ECF_HOME
   std::string ecf_home = File::test_data("ANode/test/data/TaskScriptGenerator","ANode");

   std::string head = ecf_home + "/head.h";
   std::string tail = ecf_home + "/tail.h";
   fs::remove_all(ecf_home);
   BOOST_REQUIRE_MESSAGE( !fs::exists(head), "Remove of head file failed");
   BOOST_REQUIRE_MESSAGE( !fs::exists(tail), "Remove of tail file failed");


   // Create a defs file corresponding to:
   // suite suite
   //   edit ECF_INCLUDE $ECF_HOME/includes
   //   edit ECF_HOME $ECF_HOME/includes
   //   edit SLEEP 10
   //   family f1
   //       task t1
   //       task t2
   //   endfamily
   //   family f2
   //       edit ECF_DUMMY_TASK ''
   //       task t1
   //       task t2
   //   endfamily
   // endsuite
   std::vector<task_ptr> tasks_with_scripts,tasks_without_scripts;
   Defs theDefs;
   {
      suite_ptr suite = theDefs.add_suite("suite");
      suite->add_variable( Str::ECF_INCLUDE(),ecf_home );
      suite->add_variable( Str::ECF_HOME(), ecf_home );
      suite->add_variable(  "SLEEP","10" );
      family_ptr f1 = suite->add_family("f1");
      tasks_with_scripts.push_back(f1->add_task("t1"));
      family_ptr f2 = suite->add_family("f2");
      f2->add_variable( "ECF_DUMMY_TASK", "" );
      tasks_without_scripts.push_back(f2->add_task("t1"));
      tasks_without_scripts.push_back(f2->add_task("t2"));
//      cout << theDefs << "\n";
   }

   /// generate the scripts and head.h and tail.h
   theDefs.generate_scripts();

   /// Test the ecf file were created, by doing job creation
   /// JobCreationCtrl is used control what node we generate the jobs for:
   /// Since we have *NOT* set the node on it, we force job generation for all tasks
   job_creation_ctrl_ptr jobCtrl = boost::make_shared<JobCreationCtrl>();
   theDefs.check_job_creation(jobCtrl);
   BOOST_REQUIRE_MESSAGE(jobCtrl->get_error_msg().empty(), jobCtrl->get_error_msg());
   BOOST_REQUIRE_MESSAGE(jobCtrl->fail_submittables().empty(),"Expected no failing tasks");

   /// Additional sanity tests #######################################################

   /// test that header and tail file were created
   BOOST_REQUIRE_MESSAGE( fs::exists(head), "Head file " << head << " not created");
   BOOST_REQUIRE_MESSAGE( fs::exists(tail), "Tail file " << tail << " not created");

   /// begin , will cause creation of generated variables. The generated variables
   /// are use in client scripts and used to locate the ecf files
   theDefs.beginAll();

   // Test for script generation
   BOOST_FOREACH(task_ptr t, tasks_with_scripts) {
      try {
         EcfFile ecf_file = t->locatedEcfFile();
         BOOST_REQUIRE_MESSAGE( ecf_file.valid(), "Could not locate ecf file for task ");
      }
      catch (std::exception& e) {
         BOOST_REQUIRE_MESSAGE(false,"Could not locate ecf file for task " << e.what());
      }
   }

   // Test  that no scripts are generated when ECF_DUMMY_TASK is used
   BOOST_FOREACH(task_ptr t, tasks_without_scripts) {
      BOOST_REQUIRE_THROW(t->locatedEcfFile(),std::runtime_error);
   }

   // Remove the directories that were generated
   fs::remove_all(ecf_home);
}

BOOST_AUTO_TEST_SUITE_END()
