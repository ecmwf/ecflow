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
#include "AutoRestoreAttr.hpp"
#include "AutoArchiveAttr.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace fs = boost::filesystem;

/// Simulate definition files that are created on then fly. This allows us to validate
/// Defs file, to check for correctness

BOOST_AUTO_TEST_SUITE( SimulatorTestSuite )

BOOST_AUTO_TEST_CASE( test_autorestore_suite )
{
   cout << "Simulator:: ...test_autorestore_suite\n";
   // ****: Since we have no time dependencies the simulator calendar increment
   // ****: is in hours. Hence autoarchive at hour resolution
   Defs theDefs;
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),File::test_data("CSim/test","CSim")); // required for archive
   string s1_path;
   {
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr s1 = theDefs.add_suite("test_autorestore_suite");
      s1->addClock( clockAttr );
      s1->add_autoarchive( ecf::AutoArchiveAttr(0));
      s1->add_family("family")->add_task("t");
      s1_path = s1->absNodePath();
   }
   {
      std::vector<std::string> nodes_to_restore; nodes_to_restore.push_back( s1_path );
      ClockAttr clockAttr(true);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr s3 = theDefs.add_suite("test_autorestore_suite_2");
      s3->addClock( clockAttr );
      task_ptr t1 = s3->add_family("family")->add_task("t");
      t1->add_autorestore(ecf::AutoRestoreAttr(nodes_to_restore));
      t1->add_trigger("/test_autorestore_suite == complete");
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs, TestUtil::testDataLocation("test_autorestore_suite.def"), errorMsg),errorMsg);

//   PrintStyle::setStyle(PrintStyle::MIGRATE);
//   cout << theDefs;

   // suite test_autorestore_suite, should have been archived then restored
   suite_ptr s1 = theDefs.findSuite("test_autorestore_suite");
   BOOST_REQUIRE_MESSAGE(s1,"Expected to find suite s1");
   BOOST_CHECK_MESSAGE(s1->get_flag().is_set(ecf::Flag::RESTORED),"Expected suite " << s1->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(!s1->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected suite " << s1->absNodePath() << " to be restored");
   BOOST_CHECK_MESSAGE(!fs::exists(s1->archive_path()),"Expected file " << s1->archive_path() << " to be removed");
   BOOST_CHECK_MESSAGE(!s1->nodeVec().empty(),"Expected suite " << s1->absNodePath() << " to be restored");

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_autorestore_suite.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_CASE( test_autorestore_family )
{
   cout << "Simulator:: ...test_autorestore_family\n";

   // *** Autoarchiving takes place in Defs::updatecalendar(..) which happens every 60/seconds
   // *** Autorestore   takes place immediately after state change, hence if could occur before updateCalendar has run
   // ***               hence in this test we use time attributes, to ensure the updateCalendar/autoarchive is run before
   // ***               autorestore
   Defs theDefs;
   theDefs.set_server().add_or_update_user_variables(Str::ECF_HOME(),File::test_data("CSim/test","CSim")); // required for archive

   std::vector<std::string> vec;
   {
      ClockAttr clockAttr(false);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("test_autoarchive_family");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      fam->add_autoarchive( 0 );
      task_ptr task = fam->add_task("t");
      task->addTime(ecf::TimeAttr(1,0,true));
      vec.push_back(fam->absNodePath());
   }
   {
      ClockAttr clockAttr(false);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      suite_ptr suite = theDefs.add_suite("test_autoarchive_family_1");
      suite->addClock( clockAttr );

      family_ptr fam = suite->add_family("family");
      fam->add_autoarchive( ecf::AutoArchiveAttr( ecf::TimeSlot(2,0), false));
      task_ptr task = fam->add_task("t");
      task->addTime(ecf::TimeAttr(1,0,true));
      vec.push_back(fam->absNodePath());
   }
   {
      ClockAttr clockAttr(false);
      clockAttr.date(12,10,2009); // 12 October 2009 was a Monday
      ClockAttr end_clock(false);
      end_clock.date(14,10,2009); // 14 October 2009 was a Wednesday

      suite_ptr suite = theDefs.add_suite("test_autoarchive_family_2");
      suite->addClock( clockAttr );
      suite->add_end_clock( end_clock );

      family_ptr fam = suite->add_family("do_autorestore");
      task_ptr t = fam->add_task("t");
      t->add_autorestore( ecf::AutoRestoreAttr(vec) );
      t->add_trigger(vec[0] + " == complete and " + vec[1] + " == complete");
      t->addTime(ecf::TimeAttr(2,0,true)); // make sure we finish 1 hour after, since restore is immediate
   }

   Simulator simulator;
   std::string errorMsg;
   BOOST_CHECK_MESSAGE(simulator.run(theDefs,TestUtil::testDataLocation("test_autorestore_family.def"), errorMsg),errorMsg);
   //PrintStyle::setStyle(PrintStyle::MIGRATE);
   //cout << theDefs;

   // make sure all familes has been archived
   std::vector<Family*> famVec;
   theDefs.getAllFamilies(famVec);
   for(auto f : famVec) {
      if (f->name() == "do_autorestore") continue;
      BOOST_CHECK_MESSAGE(f->get_flag().is_set(ecf::Flag::RESTORED),"Expected family " << f->absNodePath() << " to be restored");
      BOOST_CHECK_MESSAGE(!f->get_flag().is_set(ecf::Flag::ARCHIVED),"Expected family " << f->absNodePath() << " to be restored");
      BOOST_CHECK_MESSAGE(!fs::exists(f->archive_path()),"Expected file " << f->absNodePath() << " to be removed");
      BOOST_CHECK_MESSAGE(!f->nodeVec().empty(),"Expected family " << f->absNodePath() << " to be restored");
   }

   // remove generated log file. Comment out to debug
   std::string logFileName = TestUtil::testDataLocation("test_autorestore_family.def") + ".log";
   fs::remove(logFileName);
}

BOOST_AUTO_TEST_SUITE_END()
