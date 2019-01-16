//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #48 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/test/unit_test.hpp>

#include "ClientToServerCmd.hpp"
#include "ServerToClientCmd.hpp"
#include "TestHelper.hpp"
#include "Str.hpp"
#include "System.hpp"
#include "Ecf.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "Suite.hpp"
#include "Defs.hpp"
#include "Limit.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

class TestStateChanged {
public:
   explicit TestStateChanged(suite_ptr s)
   : suite_(s),
     initial_suite_state_change_no_(s->state_change_no()),
     initial_suite_modify_change_no_(s->modify_change_no()) { Ecf::set_server(true);}

   ~TestStateChanged() {
      Ecf::set_server(false);
      BOOST_CHECK_MESSAGE(suite_->state_change_no() != initial_suite_state_change_no_ ||
                          suite_->modify_change_no()	!= initial_suite_modify_change_no_,
                          "Suite " << suite_->name() << " has no change in attributes. Forget to increment change no?"
                          << " suite_->state_change_no(" << suite_->state_change_no()<< ")-"
                          << "-initial_suite_state_change_no_(" << initial_suite_state_change_no_ << ") "
                          << " suite_->modify_change_no(" << suite_->modify_change_no() << ")-"
                          << "-initial_suite_modify_change_no_(" << initial_suite_modify_change_no_ << ")"
      );
   }
private:
   suite_ptr suite_;
   unsigned int initial_suite_state_change_no_;
   unsigned int initial_suite_modify_change_no_;
};


class TestDefsStateChanged {
public:
   explicit TestDefsStateChanged(Defs* s)
   : defs_(s),
     initial_state_change_no_(s->defs_only_max_state_change_no()) {}

   ~TestDefsStateChanged() {
      Ecf::set_server(false);
      BOOST_CHECK_MESSAGE(defs_->defs_only_max_state_change_no() != initial_state_change_no_,
                          "Defs has no change in attributes. Forget to increment change no?"
      );
   }
private:
   Defs* defs_;
   unsigned int initial_state_change_no_;
};


BOOST_AUTO_TEST_CASE( test_alter_cmd_for_clock_type_hybrid )
{
   cout << "Base:: ...test_alter_cmd_for_clock_type_hybrid\n";

   // In this test the suite has NO Clock attribute. It should get added automatically
   // when a new clock is added, we should sync with the computer clock
   Defs defs;
   suite_ptr s = Suite::create("suite");
   defs.addSuite( s );

   BOOST_CHECK_MESSAGE( !s->clockAttr(), "Expected no clock");
   std::string error_msg;
   BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

   {  // Change clock type =====================================================================================================
      defs.beginAll();
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_TYPE,"hybrid","")));
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( v && v->hybrid() , "expected clock to be added and be hybrid");

      // Altering the suite clock attributes, should force suite calendar to  align to todays date and time
      boost::posix_time::ptime date_now = Calendar::second_clock_time();
      int day_of_month = date_now.date().day();
      int month        = date_now.date().month();
      int year         = date_now.date().year();
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after re-queue/begin. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
      BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after re-queue/begin. Expected " << month << " but found " << s->calendar().month());
      BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after re-queued/begin");
      BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);
   }
}

BOOST_AUTO_TEST_CASE( test_alter_cmd_for_clock_type_real )
{
   cout << "Base:: ...test_alter_cmd_for_clock_type_real\n";

   // In this test the suite has NO Clock attribute. It should get added automatically
   // when a new clock is added, we should sync with the computer clock
   Defs defs;
   suite_ptr s = defs.add_suite("suite");

   BOOST_CHECK_MESSAGE( !s->clockAttr(), "Expected no clock");
   std::string error_msg; BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

   {  // Change clock type =====================================================================================================
      defs.beginAll();
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_TYPE,"real","")));
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( v && !v->hybrid() , "expected clock to be added and be real");

      // Altering the suite clock attributes, should force suite calendar to  align to todays date and time
      boost::posix_time::ptime date_now = Calendar::second_clock_time();
      int day_of_month = date_now.date().day();
      int month        = date_now.date().month();
      int year         = date_now.date().year();
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after re-queue/begin. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
      BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after re-queue/begin. Expected " << month << " but found " << s->calendar().month());
      BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after re-queued/begin");
      BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);
   }
}

BOOST_AUTO_TEST_CASE( test_alter_cmd_for_clock_sync )
{
   cout << "Base:: ...test_alter_cmd_for_clock_sync\n";

   // Add a suite with a hybrid clock set to the past, on switch to real time, should have todays date
   // Since the clock exists on the suite, with another date, we must explicitly sync with computer
   // alternatively user can change clock date and clock gain to align with computer
   Defs defs;
   suite_ptr s = defs.add_suite("suite");
   ClockAttr clockAttr(true); // add hybrid clock
   clockAttr.date(1,1,2009);
   s->addClock( clockAttr );

   BOOST_REQUIRE_MESSAGE( s->clockAttr(), "Expected clock");
   BOOST_REQUIRE_MESSAGE( s->clockAttr()->hybrid(), "Expected hybrid clock");
   std::string error_msg; BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

   {  // Change clock type =====================================================================================================
      defs.beginAll();
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_TYPE,"real","")));
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( v && !v->hybrid() , "expected clock to be added and be real");

      // When we reque, the date should be unchanged
      TestHelper::invokeRequest(&defs,Cmd_ptr( new RequeueNodeCmd("/" + s->name())));
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == 1, "Calendar should be updated after re-queue/begin. Expected day of month " << 1 << " but found " << s->calendar().day_of_month());
      BOOST_CHECK_MESSAGE( s->calendar().month() == 1, "Calendar should be updated after re-queue/begin. Expected month " << 1 << " but found " << s->calendar().month());
      BOOST_CHECK_MESSAGE( s->calendar().year() == 2009, "Calendar should be updated after re-queued/begin, Expected year " << 2009 << " but found " << s->calendar().year());
      BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

      // After a clk sync, data should match computer
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_SYNC,"","")));

      boost::posix_time::ptime date_now = Calendar::second_clock_time();
      int day_of_month = date_now.date().day();
      int month        = date_now.date().month();
      int year         = date_now.date().year();
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after re-queue/begin. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
      BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after re-queue/begin. Expected " << month << " but found " << s->calendar().month());
      BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after re-queued/begin");
      BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);
   }
}

BOOST_AUTO_TEST_CASE( test_alter_cmd_for_clock_date )
{
   cout << "Base:: ...test_alter_cmd_for_clock_date\n";

   // In this test the suite has NO Clock attribute. It should get added automatically
   Defs defs;
   suite_ptr s = defs.add_suite("suite");
   BOOST_CHECK_MESSAGE( !s->clockAttr(), "Expected no clock");
   std::string error_msg; BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

   // Change clock date =====================================================================================================
   TestStateChanged changed(s);
   TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_DATE,"12.12.2012","")));
   clock_ptr v = s->clockAttr();
   BOOST_CHECK_MESSAGE( v.get(), "expected clock to be added");

   // Check that calendar is updated  by the Alter
   {
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == 12, "Calendar should be updated");
      BOOST_CHECK_MESSAGE( s->calendar().month() == 12, "Calendar should be updated");
      BOOST_CHECK_MESSAGE( s->calendar().year() == 2012, "Calendar should be updated");

      defs.beginAll();
      TestHelper::invokeRequest(&defs,Cmd_ptr( new RequeueNodeCmd(s->absNodePath())));
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == 12, "Calendar should be updated after re-queued");
      BOOST_CHECK_MESSAGE( s->calendar().month() == 12, "Calendar should be updated after re-queued");
      BOOST_CHECK_MESSAGE( s->calendar().year() == 2012, "Calendar should be updated after re-queued");

      // Now re sync with the computers clock, the suite calendar should align to todays date and time
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_SYNC,"","")));

      boost::posix_time::ptime date_now = Calendar::second_clock_time();
      int day_of_month = date_now.date().day();
      int month        = date_now.date().month();
      int year         = date_now.date().year();
      BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after re-queue/begin. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
      BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after re-queue/begin. Expected " << month << " but found " << s->calendar().month());
      BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after re-queued/begin");
      BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);
   }
}

BOOST_AUTO_TEST_CASE( test_alter_cmd_for_clock_gain )
{
   cout << "Base:: ...test_alter_cmd_for_clock_gain\n";

   // In this test the suite has NO Clock attribute. It should get added automatically
   Defs defs;
   std::string suitename = "suite";
   suite_ptr s = defs.add_suite(suitename);
   BOOST_CHECK_MESSAGE( !s->clockAttr(), "Expected no clock");
   std::string error_msg; BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

   {  // Change clock gain  =====================================================================================================
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(suitename,AlterCmd::CLOCK_GAIN,"86400",""))); // add 24 hours in seconds
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( v.get(), "expected clock to be added");
      BOOST_CHECK_MESSAGE( v && v->gain() == 86400 , "expected clock gain to be 86400 but found " << v->gain());

      // Check that calendar is updated by the alter
      {
         // add one day, to current date, to simulate a gain of 24 hours
         boost::posix_time::ptime date_now = Calendar::second_clock_time();
         boost::gregorian::date newDate = date_now.date();
         boost::gregorian::date_duration one_day(1);
         newDate += one_day;
         int day_of_month = newDate.day();
         int month        = newDate.month();
         int year         = newDate.year();
         BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after alter. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
         BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after alter. Expected " << month << " but found " << s->calendar().month());
         BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after alter");

         // This should init calendar with todays date + 86400 seconds. i.e +1 day.
         defs.beginAll();
         TestHelper::invokeRequest(&defs,Cmd_ptr( new RequeueNodeCmd("/" + suitename)));
         BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after re-queue/begin. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
         BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after re-queue/begin. Expected " << month << " but found " << s->calendar().month());
         BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after re-queued/begin");

         // Now re sync with the computers clock,When we reque, the suite calendar should align to todays date and time
         TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_SYNC,"","")));
         day_of_month = date_now.date().day();
         month        = date_now.date().month();
         year         = date_now.date().year();
         BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == day_of_month, "Calendar should be updated after re-queue/begin. Expected " << day_of_month << " but found " << s->calendar().day_of_month());
         BOOST_CHECK_MESSAGE( s->calendar().month() == month, "Calendar should be updated after re-queue/begin. Expected " << month << " but found " << s->calendar().month());
         BOOST_CHECK_MESSAGE( s->calendar().year() == year, "Calendar should be updated after re-queued/begin");
         BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);
      }
   }
}

BOOST_AUTO_TEST_CASE( test_alter_cmd )
{
   cout << "Base:: ...test_alter_cmd\n";

   Defs defs;
   suite_ptr s = defs.add_suite("suite");
   task_ptr task = s->add_task("t1");
   task_ptr t2 = s->add_task("t2");
   {
      ClockAttr clockAttr(false); // real clock
      clockAttr.date(1,1,2009);
      clockAttr.set_gain_in_seconds(3600);
      clockAttr.startStopWithServer(true);
      s->addClock( clockAttr );
      s->addDefStatus(DState::SUSPENDED); // avoid AlterCmd from job submission
   }

   std::string error_msg; BOOST_CHECK_MESSAGE(defs.checkInvariants(error_msg),"checkInvariants failed " << error_msg);

   { // Change server state. This will cause Flag::MESSAGE to be set on the defs
      TestDefsStateChanged chenged(&defs);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"_fred_","value")));
      BOOST_CHECK_MESSAGE( defs.server().find_variable("_fred_") == "value" , "expected to find value");

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::VARIABLE,"_fred_","_value_")));
      BOOST_CHECK_MESSAGE( defs.server().find_variable("_fred_") == "_value_" , "expected to find _value_");

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::DEL_VARIABLE,"_fred_")));
      BOOST_CHECK_MESSAGE( defs.server().find_variable("_fred_") == "" , "expected to find empty string");

      BOOST_CHECK_MESSAGE( defs.get_edit_history("/").size() == 3,
                           "expected edit_history of 3 to be added but found " <<  defs.get_edit_history("/").size());

      {
         // test set and clear flags of the definition
         // Note: we can not really test Flag::Message clear, since that act of clearing, sets the message
         std::vector<Flag::Type> flag_list = Flag::list();
         for(auto & i : flag_list) {
            // When any user command(including setting flags) invoked, we set Flag::MESSAGE on the defs.
            // Hence setting flag Flag::MESSAGE has no effect. Likewise clearing has no affect since it get set
            if ( i == Flag::MESSAGE)  continue;

            TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/",i,true)));
            BOOST_CHECK_MESSAGE( defs.flag().is_set(i), "Expected flag " << i << " to be set ");

            TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/",i,false)));
            BOOST_CHECK_MESSAGE( ! defs.flag().is_set(i), "Expected flag " << i << " to be clear ");
         }
      }
   }

   {  // Change clock type =====================================================================================================
      defs.beginAll();
      BOOST_CHECK_MESSAGE( s->calendar().hybrid() == false, "expected calendar to be real after begin");
      BOOST_CHECK_MESSAGE( s->calendar().hybrid() == s->clockAttr()->hybrid(), "Calendar and Clock attribute must be in sync after begin");

      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_TYPE,"hybrid","")));
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( v && v->hybrid() , "expected clock to be hybrid");
      BOOST_CHECK_MESSAGE( defs.get_edit_history(s->absNodePath()).size() == 1,
                           "expected edit_history of 1 to be added but found " <<  defs.get_edit_history(s->absNodePath()).size());

      // Check that calendar is in sync with clock attribute after the alter
      {
         BOOST_CHECK_MESSAGE( s->calendar().hybrid() == s->clockAttr()->hybrid(), "Calendar and Clock attribute must be in sync after alter");
         BOOST_CHECK_MESSAGE( s->calendar().hybrid() == true, "expected calendar to be re-initialised to be hybrid after alter");
      }

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_TYPE,"real","")));
      BOOST_CHECK_MESSAGE( v && !v->hybrid() , "expected clock to be real");
      BOOST_CHECK_MESSAGE( defs.get_edit_history(s->absNodePath()).size() == 2,
                           "expected edit_history of 2 to be added but found " <<  defs.get_edit_history(s->absNodePath()).size());

      // Check that calendar is in sync with clock attribute after the alter
      {
         BOOST_CHECK_MESSAGE( s->calendar().hybrid() == s->clockAttr()->hybrid(), "Calendar and Clock attribute must be in sync after alter");
         BOOST_CHECK_MESSAGE( s->calendar().hybrid() == false, "expected calendar to be re-initialised to be real after alter");
      }
   }

   {  // Change clock date =====================================================================================================
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_DATE,"12.12.2012","")));
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( (v->day() == 12 && v->month() == 12 && v->year() == 2012) , "expected clock date to be 12.12.2012 but found " << v->toString());
      BOOST_CHECK_MESSAGE( defs.get_edit_history(s->absNodePath()).size() == 3,
                           "expected edit_history of 3 to be added but found " <<  defs.get_edit_history(s->absNodePath()).size());

      // Check that calendar is updated after alter
      {
         BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == 12, "Calendar should be updated after re-queued");
         BOOST_CHECK_MESSAGE( s->calendar().month() == 12, "Calendar should be updated after re-queued");
         BOOST_CHECK_MESSAGE( s->calendar().year() == 2012, "Calendar should be updated after re-queued");
      }
   }

   {  // Change clock gain  =====================================================================================================
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::CLOCK_GAIN,"86400",""))); // add 24 hours in seconds
      clock_ptr v = s->clockAttr();
      BOOST_CHECK_MESSAGE( v && v->gain() == 86400 , "expected clock gain to be 3333 but found " << v->gain());
      BOOST_CHECK_MESSAGE( defs.get_edit_history(s->absNodePath()).size() == 4,
                           "expected edit_history of 4 to be added but found " <<  defs.get_edit_history(s->absNodePath()).size());

      // Check that calendar is updated after the alter
      {
         BOOST_CHECK_MESSAGE( s->calendar().day_of_month() == 13, "Calendar should be updated after re-queued");
         BOOST_CHECK_MESSAGE( s->calendar().month() == 12, "Calendar should be updated after re-queued");
         BOOST_CHECK_MESSAGE( s->calendar().year() == 2012, "Calendar should be updated after re-queued");
      }
   }

   // test add, the deleting of a specific attribute
   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DATE,"12.12.2010")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DAY,"sunday")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TIME,"23:00")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TODAY,"23:00")));
      BOOST_CHECK_MESSAGE( task->dates().size() == 1, "expected 1 date to be added but found " <<  s->dates().size());
      BOOST_CHECK_MESSAGE( task->days().size() == 1, "expected 1 day to be added but found " <<  s->days().size());
      BOOST_CHECK_MESSAGE( task->timeVec().size() == 1, "expected 1 time to be added but found " <<  s->timeVec().size());
      BOOST_CHECK_MESSAGE( task->todayVec().size() == 1, "expected 1 today attr, to be added but found " <<  s->todayVec().size());
      BOOST_CHECK_MESSAGE( defs.get_edit_history(task->absNodePath()).size() == 4, "expected edit_history of 4 to be added but found " <<  defs.get_edit_history(task->absNodePath()).size());
   }
   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_DATE,"12.12.2010")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_DAY,"sunday")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_TIME,"23:00")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_TODAY,"23:00")));
      BOOST_CHECK_MESSAGE( task->dates().size() == 0, "expected 0 dates,   but found " <<  s->dates().size());
      BOOST_CHECK_MESSAGE( task->days().size() == 0, "expected 0 day       but found " <<  s->days().size());
      BOOST_CHECK_MESSAGE( task->timeVec().size() == 0, "expected 0 time   but found " <<  s->timeVec().size());
      BOOST_CHECK_MESSAGE( task->todayVec().size() == 0, "expected 0 today but found " <<  s->todayVec().size());
      BOOST_CHECK_MESSAGE( defs.get_edit_history(task->absNodePath()).size() == 8, "expected edit_history of 8 to be added but found " <<  defs.get_edit_history(task->absNodePath()).size());
   }

   {   // test delete all
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DATE,"12.12.2010")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DATE,"12.*.2010")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DATE,"8.*.2010")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DAY,"sunday")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DAY,"monday")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_DAY,"tuesday")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TIME,"09:00")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TIME,"22:00")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TIME,"23:00")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TODAY,"02:00")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_TODAY,"03:00")));
      BOOST_CHECK_MESSAGE( defs.get_edit_history(task->absNodePath()).size() == 19, "expected edit_history of 19 to be added but found " <<  defs.get_edit_history(task->absNodePath()).size());

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_DATE)));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_DAY)));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_TIME)));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_TODAY)));
      BOOST_CHECK_MESSAGE( task->dates().size() == 0, "expected 0 dates,   but found " <<  s->dates().size());
      BOOST_CHECK_MESSAGE( task->days().size() == 0, "expected 0 day       but found " <<  s->days().size());
      BOOST_CHECK_MESSAGE( task->timeVec().size() == 0, "expected 0 time   but found " <<  s->timeVec().size());
      BOOST_CHECK_MESSAGE( task->todayVec().size() == 0, "expected 0 today but found " <<  s->todayVec().size());

      /// Edit history should be truncated to max of 20
      BOOST_CHECK_MESSAGE( defs.get_edit_history(task->absNodePath()).size() == 20, "expected edit_history to be truncated to 20, but found " <<  defs.get_edit_history(task->absNodePath()).size());
   }

   {   // test add variables
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_VARIABLE,"FRED1","_val_")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_VARIABLE,"FRED2","_val_")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_VARIABLE,"FRED3","_val_")));
      BOOST_CHECK_MESSAGE( s->variables().size() == 3, "expected 3 variable but found " <<  s->variables().size());

      // test delete variables
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_VARIABLE,"FRED1")));
      BOOST_CHECK_MESSAGE( s->variables().size() == 2, "expected 2 variable but found " <<  s->variables().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_VARIABLE)));
      BOOST_CHECK_MESSAGE( s->variables().size() == 0, "expected 0 variable but found " <<  s->variables().size());

      // test change variable
      s->add_variable("FRED1","BILL");
      {
         TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::VARIABLE,"FRED1","BILL1")));
         const Variable& v = s->findVariable("FRED1");
         BOOST_CHECK_MESSAGE( !v.empty() && v.theValue() == "BILL1", "expected to find variable FRED1, with value BILL1");
      }
      {
         TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::VARIABLE,"FRED1")));
         const Variable& v = s->findVariable("FRED1");
         BOOST_CHECK_MESSAGE( !v.empty() && v.theValue() == "", "expected to find variable FRED1, with empty value");
      }
   }

   {   // test add event
      TestStateChanged changed(s);
      s->addEvent( Event(1,"event1") );
      s->addEvent( Event(2,"event2") );
      s->addEvent( Event(3,"event3") );
      BOOST_CHECK_MESSAGE( s->events().size() == 3, "expected 3  but found " <<  s->events().size());

      // test delete event
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_EVENT,"event1")));
      BOOST_CHECK_MESSAGE( s->events().size() == 2, "expected 2  but found " <<  s->events().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_EVENT)));
      BOOST_CHECK_MESSAGE( s->events().size() == 0, "expected 0  but found " <<  s->events().size());

      // test change event
      s->addEvent( Event(1,"event1") );
      s->addEvent( Event(2,"event2") );
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::EVENT,"event1",Event::SET())));
      {const Event& v = s->findEventByNameOrNumber("event1");
      BOOST_CHECK_MESSAGE( v.value() == 1, "expected to find  event with value set");}

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::EVENT,"event1",Event::CLEAR())));
      {const Event& v = s->findEventByNameOrNumber("event1");
      BOOST_CHECK_MESSAGE( !v.empty() && v.value() == 0, "expected to find  event with value cleared");}

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::EVENT,"event1","")));
      {const Event& v = s->findEventByNameOrNumber("event1");
      BOOST_CHECK_MESSAGE( v.value() == 1, "expected to find  event with value set");}
   }

   {   // test add meter
      //TestStateChanged changed(s);
      s->addMeter( Meter("meter",0,100,100) );
      s->addMeter( Meter("meter1",0,100,100) );
      s->addMeter( Meter("meter2",0,100,100) );
      BOOST_CHECK_MESSAGE( s->meters().size() == 3, "expected 3  but found " <<  s->meters().size());

      // test delete meter
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_METER,"meter")));
      BOOST_CHECK_MESSAGE( s->meters().size() == 2, "expected 2  but found " <<  s->meters().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_METER)));
      BOOST_CHECK_MESSAGE( s->meters().size() == 0, "expected 0 meters but found " <<  s->meters().size());

      // test change meter value
      s->addMeter( Meter("meter",0,100,100) );
      BOOST_CHECK_MESSAGE( s->meters().size() == 1, "expected 1 meter but found " << s->meters().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::METER,"meter","10")));
      const Meter& v = s->findMeter("meter");
      BOOST_CHECK_MESSAGE( v.value() == 10, "expected to find meter with value 10");
   }

   {   // test add label
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LABEL,"label","labelValue")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LABEL,"label1","labelValue")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LABEL,"label2","labelValue")));
      BOOST_CHECK_MESSAGE( s->labels().size() == 3, "expected 3  but found " <<  s->labels().size());

      // test delete label
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LABEL,"label")));
      BOOST_CHECK_MESSAGE( s->labels().size() == 2, "expected 2  but found " <<  s->labels().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LABEL)));
      BOOST_CHECK_MESSAGE( s->labels().size() == 0, "expected 0  but found " <<  s->labels().size());

      // test change label value
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LABEL,"label","labelValue")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LABEL,"label","--fred--")));
      std::string label_value;
      BOOST_CHECK_MESSAGE(s->getLabelNewValue("label",label_value ),"Expected to find label");
      BOOST_CHECK_MESSAGE( label_value == "--fred--", "expected to find label with value --fred--");

      // change label to be empty, ECFLOW-648
      label_value.clear();
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LABEL,"label","")));
      BOOST_CHECK_MESSAGE(s->getLabelNewValue("label",label_value ),"Expected to find label");
      BOOST_CHECK_MESSAGE( label_value.empty(), "expected to find label with empty value");
   }

   {   // test add Trigger
      TestStateChanged changed(s);
      task->add_trigger( "t1 == complete");
      BOOST_CHECK_MESSAGE( task->get_trigger(), "expected  trigger to be added");

      // test delete trigger
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_TRIGGER)));
      BOOST_CHECK_MESSAGE( !task->get_trigger(), "expected  trigger to be deleted");

      // test change trigger expression
      task->add_trigger( "t1 == complete" );
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::TRIGGER,"x == complete","")));

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::TRIGGER,"t2 == complete","")));
      BOOST_CHECK_MESSAGE( task->triggerExpression() == "trigger t2 == complete", "expected trigger to be changed found " << task->triggerExpression());
   }

   {   // test add complete expression
      TestStateChanged changed(s);
      task->add_complete( "t1 == complete" );
      BOOST_CHECK_MESSAGE( task->get_complete(), "expected complete to be added");

      // test delete complete
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_COMPLETE)));
      BOOST_CHECK_MESSAGE( !task->get_complete(), "expected complete to be deleted");

      // test change complete expression
      task->add_complete(  "t1 == complete" );
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::COMPLETE,"x == complete","")));

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::COMPLETE,"t2 == complete","")));
      BOOST_CHECK_MESSAGE( task->completeExpression() == "complete t2 == complete", "expected complete expression to be changed found " << task->completeExpression() );
   }

   {   // test add limit
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LIMIT,"limit","10")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LIMIT,"limit1","10")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LIMIT,"limit2","10")));
      BOOST_CHECK_MESSAGE( s->limits().size() == 3, "expected 3  but found " <<  s->limits().size());

      // test delete limit
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LIMIT,"limit2")));
      BOOST_CHECK_MESSAGE( s->limits().size() == 2, "expected 2  but found " <<  s->limits().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LIMIT)));
      BOOST_CHECK_MESSAGE( s->limits().size() == 0, "expected 0  but found " <<  s->limits().size());

      // test change limit max value
      s->addLimit( Limit("limit",10) );
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LIMIT_MAX,"limit","90")));
      limit_ptr v = s->find_limit("limit");
      BOOST_CHECK_MESSAGE( v.get() && v->theLimit() == 90, "expected to find limit with max value of 90");

      // test change limit value
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LIMIT_VAL,"limit","33")));
      BOOST_CHECK_MESSAGE( v.get() && v->value() == 33, "expected to find limit with value of 33");

      // Test delete limit path
      std::set<std::string> paths; paths.insert("made_up_path");
      Limit limit_path("limit_name",10);
      limit_path.set_paths(paths);

      s->addLimit( limit_path );
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LIMIT_PATH,"limit_name","made_up_path")));
      limit_ptr lm = s->find_limit("limit_name");
      BOOST_CHECK_MESSAGE( lm.get() && lm->paths().empty(), "Expected no paths but found " << lm->paths().size());
   }

   {   // test add in-limit
       TestStateChanged changed(s);
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"limit_name")));
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"limit_name1","11")));
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/to/limit:limit_name2")));
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/to/limit:limit_name3","10")));
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/to/limit:limitA","10")));
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/to/limit/a:limitA","10")));
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/to/limit/aa:limitA","10")));
       BOOST_CHECK_MESSAGE( s->inlimits().size() == 7, "expected 7  but found " <<  s->inlimits().size());

       // test delete in-limit
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"limit_name")));
       BOOST_CHECK_MESSAGE( s->inlimits().size() == 6, "expected 6 but found " <<  s->inlimits().size());
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"/path/to/limit:limitA")));
       BOOST_CHECK_MESSAGE( s->inlimits().size() == 5, "expected 5 but found " <<  s->inlimits().size());
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"/path/to/limit/a:limitA")));
       BOOST_CHECK_MESSAGE( s->inlimits().size() == 4, "expected 4 but found " <<  s->inlimits().size());
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"/path/to/limit/aa:limitA")));
       BOOST_CHECK_MESSAGE( s->inlimits().size() == 3, "expected 3 but found " <<  s->inlimits().size());
       TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT)));
       BOOST_CHECK_MESSAGE( s->inlimits().size() == 0, "expected 0  but found " <<  s->inlimits().size());
   }

   {   // test add repeat
      TestStateChanged changed(s);
      s->addRepeat( RepeatDate("YMD",20090916,20090930,1));
      BOOST_CHECK_MESSAGE( !s->repeat().empty(), "expected repeat to be added");

      // test delete repeat
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT)));
      BOOST_CHECK_MESSAGE( s->repeat().empty(), "expected repeat to be deleted");

      // test change repeat value
      s->addRepeat( RepeatDate("YMD",20090916,20090930,1));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::REPEAT,"20090917","")));
      BOOST_CHECK_MESSAGE( s->repeat().valueAsString() == "20090917", "expected 20090917 but found " << s->repeat().valueAsString());
   }
   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT))); // delete previous repeat

      std::vector<std::string> stringList; stringList.reserve(3);
      stringList.emplace_back("a");
      stringList.emplace_back("b");
      stringList.emplace_back("c");

      s->addRepeat( RepeatEnumerated("AEnum",stringList));
      BOOST_CHECK_MESSAGE( !s->repeat().empty(), "expected repeat to be added");

      // test delete repeat
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT)));
      BOOST_CHECK_MESSAGE( s->repeat().empty(), "expected repeat to be deleted");

      // test change repeat value
      s->addRepeat( RepeatEnumerated("AEnum",stringList));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::REPEAT,"1","")));
      BOOST_CHECK_MESSAGE( s->repeat().valueAsString() == "b", "expected 'b' but found " << s->repeat().valueAsString());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::REPEAT,"c","")));
      BOOST_CHECK_MESSAGE( s->repeat().valueAsString() == "c", "expected 'c' but found " << s->repeat().valueAsString());
   }
   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT))); // delete previous repeat

      std::vector<std::string> stringList; stringList.reserve(3);
      stringList.emplace_back("a");
      stringList.emplace_back("b");
      stringList.emplace_back("c");

      s->addRepeat( RepeatString("RepeatString",stringList));
      BOOST_CHECK_MESSAGE( !s->repeat().empty(), "expected repeat to be added");

      // test delete repeat
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT)));
      BOOST_CHECK_MESSAGE( s->repeat().empty(), "expected repeat to be deleted");

      // test change repeat value
      s->addRepeat( RepeatString("RepeatString",stringList));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::REPEAT,"1","")));
      BOOST_CHECK_MESSAGE( s->repeat().valueAsString() == "b", "expected 'b' but found " << s->repeat().valueAsString());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::REPEAT,"c","")));
      BOOST_CHECK_MESSAGE( s->repeat().valueAsString() == "c", "expected 'c' but found " << s->repeat().valueAsString());
   }

   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT))); // delete previous repeat

      std::vector<std::string> stringList; stringList.reserve(3);
      stringList.emplace_back("a");
      stringList.emplace_back("b");
      stringList.emplace_back("c");

      s->addRepeat( RepeatInteger("rep",0,100,1));
      BOOST_CHECK_MESSAGE( !s->repeat().empty(), "expected repeat to be added");

      // test delete repeat
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_REPEAT)));
      BOOST_CHECK_MESSAGE( s->repeat().empty(), "expected repeat to be deleted");

      // test change repeat value
      s->addRepeat( RepeatInteger("rep",0,100,1));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::REPEAT,"22","")));
      BOOST_CHECK_MESSAGE( s->repeat().valueAsString() == "22", "expected '22' but found " << s->repeat().valueAsString());
   }

   {    // add cron
      TestStateChanged changed(s);
      ecf::CronAttr cronAttr;
      ecf::TimeSlot start( 0, 0 );
      ecf::TimeSlot finish( 10, 0 );
      ecf::TimeSlot incr( 0, 5 );
      std::vector<int> weekdays;   for(int i=0;i<7;++i) weekdays.push_back(i);
      std::vector<int> daysOfMonth;for(int i=1;i<32;++i) daysOfMonth.push_back(i);
      std::vector<int> months;     for(int i=1;i<13;++i) months.push_back(i);
      cronAttr.addTimeSeries(start,finish,incr);
      cronAttr.addWeekDays( weekdays  );
      cronAttr.addDaysOfMonth(daysOfMonth);
      cronAttr.addMonths(  months );
      task->addCron( cronAttr  );

      BOOST_CHECK_MESSAGE( task->crons().size() == 1, "expected cron to be added");

      // test delete cron
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::DEL_CRON)));
      BOOST_CHECK_MESSAGE( task->crons().size() == 0, "expected cron to be delete");
   }

   {   // test suite changed
      TestStateChanged changed(s);

      // test add zombie
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_ZOMBIE,"user:fob:init:300")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_ZOMBIE,"path:fob:init:300")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_ZOMBIE,"ecf:fob:init:300")));
      BOOST_CHECK_MESSAGE( s->zombies().size() == 3, "expected 3  but found " <<  s->zombies().size());

      // test delete zombie
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_ZOMBIE,"ecf")));
      BOOST_CHECK_MESSAGE( s->zombies().size() == 2, "expected 2  but found " <<  s->zombies().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_ZOMBIE,"path")));
      BOOST_CHECK_MESSAGE( s->zombies().size() == 1, "expected 1  but found " <<  s->zombies().size());
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_ZOMBIE,"user")));
      BOOST_CHECK_MESSAGE( s->zombies().size() == 0, "expected 0  but found " <<  s->zombies().size());
   }

   {    // free password
      TestStateChanged changed(s);
      std::string returnedValue;
      BOOST_CHECK_MESSAGE( !task->findVariableValue(Str::ECF_PASS(),returnedValue), "Expected no variable of name ECF_PASS");

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(task->absNodePath(),AlterCmd::ADD_VARIABLE,Str::ECF_PASS(),Submittable::FREE_JOBS_PASSWORD())));

      BOOST_CHECK_MESSAGE( task->findVariableValue(Str::ECF_PASS(),returnedValue), "Expected to find variable ECF_PASS on the task");
      BOOST_CHECK_MESSAGE( returnedValue == Submittable::FREE_JOBS_PASSWORD(), "Expected variable value of name " << Submittable::FREE_JOBS_PASSWORD() << " but found " << returnedValue);
   }

   {
      // test set and clear flags
      // Note: we can not really test Flag::Message clear, since that act of clearing, sets the message
      std::vector<Flag::Type> flag_list = Flag::list();
      for(auto & i : flag_list) {
         // When any user command(including setting flags) invoked, we set Flag::MESSAGE on the defs.
         // Hence setting flag Flag::MESSAGE has no effect. Likewise clearing has no affect since it get set
         if ( i == Flag::MESSAGE)  continue;

         TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),i,true)));
         BOOST_CHECK_MESSAGE( s->flag().is_set(i), "Expected flag " << i << " to be set ");

         TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),i,false)));
         BOOST_CHECK_MESSAGE( ! s->flag().is_set(i), "Expected flag " << i << " to be clear ");
      }
   }

   BOOST_CHECK_MESSAGE( defs.get_edit_history(s->absNodePath()).size() == 20, "expected edit_history to be truncated to 20, but found " <<  defs.get_edit_history(s->absNodePath()).size());

   {  // Change suite def status =====================================================================================================
      TestStateChanged changed(s);
      std::vector<std::string> dstates = DState::allStates();
      for(const auto & dstate : dstates) {
         TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEFSTATUS,dstate)));
      }
      // reset back to suspended
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEFSTATUS,"suspended")));
   }

   // ================================ LATE ==================================================================
   {   // test add late
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LATE,"late -s 10:10 -a 23:10 -c +23:10")));
      BOOST_CHECK_MESSAGE( s->get_late(), "expected late to be added");

      // test change late
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LATE,"late -s 10:10")));
      BOOST_CHECK_MESSAGE( s->get_late() && s->get_late()->toString() == "late -s +10:10", "expected 'late -s 10:10' but found " <<  s->get_late()->toString());

      // test delete variable
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LATE)));
      BOOST_CHECK_MESSAGE( !s->get_late(), "expected late to be deleted");
   }
   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LATE,"-s 10:10 -a 23:10 -c +23:10")));
      BOOST_CHECK_MESSAGE( s->get_late(), "expected late to be added");

      // test change late
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::LATE,"-s 10:10 -a 12:00")));
      BOOST_CHECK_MESSAGE( s->get_late() && s->get_late()->toString() == "late -s +10:10 -a 12:00", "expected 'late -s +10:10 -a 12:00' but found " <<  s->get_late()->toString());

      // test delete variable
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_LATE)));
      BOOST_CHECK_MESSAGE( !s->get_late(), "expected late to be deleted");
   }


   // ================================ Generic ==================================================================
   // Can not add generic via Alter
   {
      s->add_generic(GenericAttr("a"));
      s->add_generic(GenericAttr("b"));
      s->add_generic(GenericAttr("3"));
      BOOST_CHECK_MESSAGE( s->generics().size() == 3, "Expected 3 generics but found " << s->generics().size() );

      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_GENERIC,"a")));
      BOOST_CHECK_MESSAGE( s->generics().size() == 2, "Expected 2 generics but found " << s->generics().size() );

      // test delete variable
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_GENERIC)));
      BOOST_CHECK_MESSAGE( s->generics().size() == 0, "Expected no generics but found " << s->generics().size() );
   }
}


void add_sortable_attributes(Node* node) {
   node->add_variable("varz","-");
   node->add_variable("vary","-");
   node->add_variable("vara","-");
   node->addEvent(Event("zevent"));
   node->addEvent(Event("aevent"));
   node->addEvent(Event(1));
   node->addEvent(Event(2));
   node->addMeter(Meter("zmeter",1,100));
   node->addMeter(Meter("ameter",1,100));
   node->addLabel(Label("zlabel","-"));
   node->addLabel(Label("alabel","-"));
   node->addLimit(Limit("zlimit",10));
   node->addLimit(Limit("ylimit",10));
   node->addLimit(Limit("xlimit",10));
}
void add_sorted_attributes(Node* node) {
   node->add_variable("vara","-");
   node->add_variable("vary","-");
   node->add_variable("varz","-");
   node->addEvent(Event(1));
   node->addEvent(Event(2));
   node->addEvent(Event("aevent"));
   node->addEvent(Event("zevent"));
   node->addMeter(Meter("ameter",1,100));
   node->addMeter(Meter("zmeter",1,100));
   node->addLabel(Label("alabel","-"));
   node->addLabel(Label("zlabel","-"));
   node->addLimit(Limit("xlimit",10));
   node->addLimit(Limit("ylimit",10));
   node->addLimit(Limit("zlimit",10));
}

BOOST_AUTO_TEST_CASE( test_alter_sort_attributes )
{
   cout << "Base:: ...test_alter_sort_attributes\n";

   Defs defs;
   defs.set_server().add_or_update_user_variables("z","z");
   defs.set_server().add_or_update_user_variables("y","y");
   defs.set_server().add_or_update_user_variables("x","x");
   suite_ptr s = defs.add_suite("suite"); add_sortable_attributes(s.get());
   family_ptr f1 = s->add_family("f1");   add_sortable_attributes(f1.get());
   task_ptr t1 = f1->add_task("t1");      add_sortable_attributes(t1.get());

   Defs sorted_defs;sorted_defs.flag().set(ecf::Flag::MESSAGE); // take into account alter
   sorted_defs.set_server().add_or_update_user_variables("x","x");
   sorted_defs.set_server().add_or_update_user_variables("y","y");
   sorted_defs.set_server().add_or_update_user_variables("z","z");
   suite_ptr ss = sorted_defs.add_suite("suite"); add_sorted_attributes(ss.get());
   family_ptr sf1 = ss->add_family("f1");         add_sorted_attributes(sf1.get());
   task_ptr st1 = sf1->add_task("t1");            add_sorted_attributes(st1.get());
   sorted_defs.sort_attributes(ecf::Attr::VARIABLE,false/*recursive*/); // just sort the server variables

   {
      TestDefsStateChanged chenged(&defs);
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/","event","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/","meter","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/","label","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/","limit","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd("/","variable","recursive")));
      DebugEquality debug_equality; // only as affect in DEBUG build
      BOOST_CHECK_MESSAGE(defs == sorted_defs,"Sort failed expected\n" << sorted_defs << "\nbut found\n" << defs);
    }
}

BOOST_AUTO_TEST_CASE( test_alter_sort_attributes_for_task )
{
   cout << "Base:: ...test_alter_sort_attributes_for_task\n";

   Defs defs;
   suite_ptr s = defs.add_suite("suite"); add_sortable_attributes(s.get());
   family_ptr f1 = s->add_family("f1");   add_sortable_attributes(f1.get());
   task_ptr t1 = f1->add_task("t1");      add_sortable_attributes(t1.get());

   Defs sorted_defs;//sorted_defs.flag().set(ecf::Flag::MESSAGE); // take into account alter
   suite_ptr ss = sorted_defs.add_suite("suite");  add_sortable_attributes(ss.get());
   family_ptr sf1 = ss->add_family("f1");          add_sortable_attributes(sf1.get());
   task_ptr st1 = sf1->add_task("t1");             add_sorted_attributes(st1.get());
   st1->flag().set(ecf::Flag::MESSAGE);
   {
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(t1->absNodePath(),"event","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(t1->absNodePath(),"meter","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(t1->absNodePath(),"label","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(t1->absNodePath(),"limit","recursive")));
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(t1->absNodePath(),"variable","recursive")));
      DebugEquality debug_equality; // only as affect in DEBUG build
      BOOST_CHECK_MESSAGE(defs == sorted_defs,"Sort failed expected\n" << sorted_defs << "\nbut found\n" << defs);
    }
}

BOOST_AUTO_TEST_CASE( test_alter_cmd_errors )
{
   cout << "Base:: ...test_alter_cmd_errors\n";

   Defs defs;
   suite_ptr s = defs.add_suite("suite");
   s->add_task("t1");

   {   // test add variables
      TestStateChanged changed(s);
      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_VARIABLE,"FRED1","_val_")));
      BOOST_CHECK_MESSAGE( s->variables().size() == 1, "expected 1 variable but found " <<  s->variables().size());

      // test delete variables, with a non existent path
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd("/idont/exist",AlterCmd::DEL_VARIABLE,"FRED1")));

      // ECFLOW-380 test change read only server variables
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"ECF_HOST","a")));
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"ECF_PORT","a")));
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"ECF_PID","a")));
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"ECF_VERSION","a")));
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd("/",AlterCmd::ADD_VARIABLE,"ECF_LISTS","a")));

      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LIMIT,"limit")));      // no limit value
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LIMIT,"limit","xx"))); // value not convertible to integer
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LIMIT,".1","10")));    // not a valid name

      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"")));           // no inlimit value
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/limit")));     // limit path, but no name
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/tolimit:limit","xx"))); // tokens must be convertible to an integer

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_INLIMIT,"/path/tolimit:limit","1")));
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"/path/tolimit")));       // no limit name
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"/path/tolimit:")));      // no limit name
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::DEL_INLIMIT,"/path/tolimit:12 34"))); // invalid limit name

      TestHelper::invokeRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LABEL,"label","1")));
      TestHelper::invokeFailureRequest(&defs,Cmd_ptr( new AlterCmd(s->absNodePath(),AlterCmd::ADD_LABEL,"label","value")));   // duplicate label name
   }

   /// Destroy singleton's to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
