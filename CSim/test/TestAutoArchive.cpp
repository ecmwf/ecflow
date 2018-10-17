//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
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
#include <cstdlib>

#include <boost/test/unit_test.hpp>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "Simulator.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "TestUtil.hpp"
#include "PrintStyle.hpp"
#include "AutoArchiveAttr.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace fs = boost::filesystem;

/// Simulate definition files that are created on then fly. This allows us to validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

BOOST_AUTO_TEST_CASE( test_autoarchive_suite )
{
   cout << "Simulator:: ...test_autoarchive_suite\n";
   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autoarchive at hour resolution
   Defs theDefs;
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),File::test_data("CSim/test","CSim")); // required for archive
   suite_ptr s1,s2,s3;
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      s1 = theDefs.add_suite("test_autoarchive_10_hours_relative");
      s1->addClock( clockAttr );
      s1->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(10,0), true));
      s1->add_family("family")->add_task("t");
   }
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      s2 = theDefs.add_suite("test_autoarchive_1_hours_real");
      s2->addClock( clockAttr );
      s2->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(1,0), false));
      s2->add_family("family")->add_task("t");
   }
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      s3 = theDefs.add_suite("test_autoarchive_1_day_relative");
      s3->addClock( clockAttr );
      s3->add_autoarchive( ecf::AutoArchiveAttr(1) );
      s3->add_family("family")->add_task("t");
      //       cout << theDefs << "\n";
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_autoarchive_suite.def"), errorMsg),errorMsg);

   // make sure autoarchive archives the suite.
   BOOST_CHECK_MESSAGE(s1->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s1->absNodePath() << " to be archived");
   BOOST_CHECK_MESSAGE(s2->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s2->absNodePath() << " to be archived");
   BOOST_CHECK_MESSAGE(s3->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s3->absNodePath() << " to be archived");

   BOOST_CHECK_MESSAGE(fs::exists(s1->archive_path()),"Expected suite " << s1->absNodePath() << " to be archived");
   BOOST_CHECK_MESSAGE(fs::exists(s2->archive_path()),"Expected suite " << s2->absNodePath() << " to be archived");
   BOOST_CHECK_MESSAGE(fs::exists(s3->archive_path()),"Expected suite " << s3->absNodePath() << " to be archived");

   BOOST_CHECK_MESSAGE(s1->nodeVec().empty(),"Expected suite " << s1->absNodePath() << " to be empty");
   BOOST_CHECK_MESSAGE(s1->nodeVec().empty(),"Expected suite " << s2->absNodePath() << " to be empty");
   BOOST_CHECK_MESSAGE(s1->nodeVec().empty(),"Expected suite " << s3->absNodePath() << " to be empty");

   s1->restore();
   s2->restore();
   s3->restore();

   BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s1->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(!s2->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s2->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(!s3->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s3->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(s1->get_flag().is_set(ecf::Flag::RESTORED),"Expected suite " << s1->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(s2->get_flag().is_set(ecf::Flag::RESTORED),"Expected suite " << s2->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(s3->get_flag().is_set(ecf::Flag::RESTORED),"Expected suite " << s3->absNodePath() << " to be restored");

   BOOST_CHECK_MESSAGE(!fs::exists(s1->archive_path()),"Expected file " << s1->archive_path() << " to be removed");
   BOOST_CHECK_MESSAGE(!fs::exists(s2->archive_path()),"Expected file " << s2->archive_path() << " to be removed");
   BOOST_CHECK_MESSAGE(!fs::exists(s3->archive_path()),"Expected file " << s3->archive_path() << " to be removed");

   BOOST_CHECK_MESSAGE(!s1->nodeVec().empty(),"Expected suite " << s1->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(!s1->nodeVec().empty(),"Expected suite " << s2->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(!s1->nodeVec().empty(),"Expected suite " << s3->absNodePath() << " to be restored");

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_autoarchive_suite.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE( test_autoarchive_ast_node_reset )
{
   cout << "Simulator:: ...test_autoarchive_ast_node_reset\n";

   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autoarchive at hour resolution
   Defs theDefs;
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),File::test_data("CSim/test","CSim")); // required for archive

   suite_ptr suite_s2;
   suite_ptr suite_s3;
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("s1");
      suite->addClock( clockAttr );
      suite->addDefStatus(DState::COMPLETE);

      family_ptr fam = suite->add_family("family");
      fam->add_task("t")->add_trigger( "/s2/family/t == complete" );

      task_ptr task2 = fam->add_task("t2");
      task2->add_trigger( "/s3/family/t == complete" );
      //    cout << theDefs << "\n";
   }
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_s2= theDefs.add_suite("s2");
      suite_s2->addClock( clockAttr );
      suite_s2->add_autoarchive( ecf::AutoArchiveAttr( 0 )); // archive on next calendar update
      suite_s2->add_family("family")->add_task("t");
   }
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_s3 = theDefs.add_suite("s3");
      suite_s3->addClock( clockAttr );
      suite_s3->add_autoarchive( ecf::AutoArchiveAttr(0)); // archive on next calendar update
      suite_s3->add_family("family")->add_task("t");
   }

   // Check number of AST nodes. The AST should be created on the fly
   std::set<Node*> theSet;
   theDefs.getAllAstNodes(theSet);
   BOOST_CHECK_MESSAGE(theSet.size() == 2,"Expected to have 2 AST nodes in trigger/complete expressions but found " << theSet.size());

   // Run the simulator
   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_autoarchive_ast_node_reset.def"), errorMsg),errorMsg);

   // Auto archive should archive suite s2 and s3, leaving one suite i.e s1
   const std::vector<suite_ptr>& suites = theDefs.suiteVec();
   for(const auto & suite : suites) {
      if (suite->name() == "s2" || suite->name() == "s3") {
         BOOST_CHECK_MESSAGE(suite->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << suite->absNodePath() << " to be archived");
      }
   }

   // The references to nodes in suites s2, s3 should have been cleared in suite s1
   {
      std::set<Node*> theSet;
      theDefs.getAllAstNodes(theSet);
      BOOST_CHECK_MESSAGE(theSet.empty(),"Expected to have 0 AST nodes in trigger/complete expressions but found " << theSet.size());
   }

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_autocancel_ast_node_reset.def") + ".log";
   fs::remove(logFileName);
   fs::remove(suite_s2->archive_path()); // remove generated archive files
   fs::remove(suite_s3->archive_path()); // remove generated archive files
}

BOOST_AUTO_TEST_CASE( test_autoarchive_family )
{
   cout << "Simulator:: ...test_autoarchive_family\n";
   Defs theDefs;
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),File::test_data("CSim/test","CSim")); // required for archive

   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("test_autoarchive_family");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      fam->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(10,0), true));
      task_ptr task = fam->add_task("t");
   }
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("test_autoarchive_1_2_hours_real");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      fam->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(2,0), false));
      task_ptr task = fam->add_task("t");
   }
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      ClockAttr end_clock(true);
      end_clock.date(14,10,2009); // 14 October 2009 was a Wednesday

      suite_ptr suite = theDefs.add_suite("test_autoarchive_1_2_day_relative");
      suite->addClock( clockAttr );
      suite->add_end_clock( end_clock );

      family_ptr fam = suite->add_family("family");
      fam->add_autoarchive( ecf::AutoArchiveAttr(2) );  // 2 days
      fam->add_task("t");
   }
   //cout << theDefs << "\n";

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_autoarchive_family.def"), errorMsg),errorMsg);

   // make sure all familes has been archived
   std::vector<Family*> famVec;
   theDefs.getAllFamilies(famVec);
   for(auto f : famVec) {
      BOOST_CHECK_MESSAGE(f->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected family " << f->absNodePath() << " to be archived");
      BOOST_CHECK_MESSAGE(fs::exists(f->archive_path()),"Expected family " << f->absNodePath() << " to be archived");
      BOOST_CHECK_MESSAGE(f->nodeVec().empty(),"Expected family " << f->absNodePath() << " to be empty");

      f->restore();
      BOOST_CHECK_MESSAGE(f->get_flag().is_set(ecf::Flag::RESTORED),"Expected family " << f->absNodePath() << " to be restored");
      BOOST_CHECK_MESSAGE(!f->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected family " << f->absNodePath() << " to be restored");
      BOOST_CHECK_MESSAGE(!fs::exists(f->archive_path()),"Expected file " << f->absNodePath() << " to be removed");
      BOOST_CHECK_MESSAGE(!f->nodeVec().empty(),"Expected family " << f->absNodePath() << " to be restored");
   }

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_autoarchive_family_and_task.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE( test_two_autoarchive_in_hierarchy )
{
   cout << "Simulator:: ...test_two_autoarchive_in_hierarchy\n";
   Defs theDefs;
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),File::test_data("CSim/test","CSim")); // required for archive

   suite_ptr suite;
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite = theDefs.add_suite("test_two_autoarchive_in_hierarchy");
      suite->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(1,0), true));
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      fam->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(1,0), true));
      task_ptr task = fam->add_task("t");
   }
//   PrintStyle::setStyle(PrintStyle::MIGRATE);
//   cout << theDefs;

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_two_autoarchive_in_hierarchy.def"), errorMsg),errorMsg);

   // Check ONLY the suite got archived an not family
   BOOST_CHECK_MESSAGE(suite->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << suite->absNodePath() << " to be archived");
   BOOST_CHECK_MESSAGE(fs::exists(suite->archive_path()),"Expected family " << suite->absNodePath() << " to be archived");
   BOOST_CHECK_MESSAGE(suite->nodeVec().empty(),"Expected family " << suite->absNodePath() << " to be empty");

   // The family should have been deleted
   node_ptr fam = theDefs.findAbsNode("/test_two_autoarchive_in_hierarchy/family");
   BOOST_CHECK_MESSAGE(!fam,"Expected family to be deleted");

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_two_autoarchive_in_hierarchy.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_SUITE_END()
