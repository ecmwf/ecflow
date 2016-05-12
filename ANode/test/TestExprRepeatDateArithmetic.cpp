/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_repeat_data_arithmetic )
{
   cout << "ANode:: ...test_repeat_data_arithmetic\n" ;

   Defs theDefs;
   task_ptr t2,t1;
   {
      suite_ptr s1 = theDefs.add_suite("s1");
      t1 = s1->add_task("t1"); t1->addRepeat( RepeatDate("YMD",20090101,20091231,1));
      t2 = s1->add_task("t2"); t2->add_trigger("t1:YMD ge 20080601");
   }
   theDefs.beginAll();

   // Check trigger expressions
   std::string errorMsg, warningMsg;
   BOOST_REQUIRE_MESSAGE(theDefs.check(errorMsg,warningMsg),"Expected triggers expressions to parse " << errorMsg);

   // Get the trigger AST
   Ast* trigger = t2->triggerAst();
   BOOST_REQUIRE_MESSAGE(trigger,"Expected trigger");

   // check evaluation
   BOOST_CHECK_MESSAGE(trigger->evaluate(),"Expected trigger to evaluate i.e 20090101 >= 20080601");

   // Check date arithmetic. Basics, end of months
   t2->changeTrigger("t1:YMD - 1 eq 20081231");   // 20090101 - 1  == 20081231
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic for evaluation");

   t2->changeTrigger("t1:YMD + 1 eq 20090102");   // 20090101 + 1  == 20090102
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic for evaluation");
}

BOOST_AUTO_TEST_CASE( test_repeat_data_arithmetic_add_to_end_of_month )
{
   cout << "ANode:: ...test_repeat_data_arithmetic_add_to_end_of_month\n" ;

   Defs theDefs;
   task_ptr t2,t1;
   {
      suite_ptr s1 = theDefs.add_suite("s1");
      t1 = s1->add_task("t1"); t1->addRepeat( RepeatDate("YMD",20090101,20091231,1));
      t2 = s1->add_task("t2"); t2->add_trigger("t1:YMD ge 20080601");
   }
   theDefs.beginAll();

   // Check trigger expressions
   std::string errorMsg, warningMsg;
   BOOST_REQUIRE_MESSAGE(theDefs.check(errorMsg,warningMsg),"Expected triggers expressions to parse " << errorMsg);

   // Check the end of each month + 1
   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090131,20101231,1));   // jan
   t2->changeTrigger("t1:YMD + 1 eq 20090201");   // 20090131 + 1  == 20090201
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090228,20101231,1));  // feb
   t2->changeTrigger("t1:YMD + 1 eq 20090301");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090331,20101231,1)); // mar
   t2->changeTrigger("t1:YMD + 1 eq 20090401");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090430,20101231,1)); // apr
   t2->changeTrigger("t1:YMD + 1 eq 20090501");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090531,20101231,1)); // may
   t2->changeTrigger("t1:YMD + 1 eq 20090601");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090630,20101231,1)); // June
   t2->changeTrigger("t1:YMD + 1 eq 20090701");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090731,20101231,1)); // July
   t2->changeTrigger("t1:YMD + 1 eq 20090801");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090831,20101231,1)); // Aug
   t2->changeTrigger("t1:YMD + 1 eq 20090901");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090930,20101231,1)); // Sept
   t2->changeTrigger("t1:YMD + 1 eq 20091001");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20091031,20101231,1)); // Oct
   t2->changeTrigger("t1:YMD + 1 eq 20091101");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20091130,20101231,1)); // Nov
   t2->changeTrigger("t1:YMD + 1 eq 20091201");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20091231,20101231,1)); // Dec
   t2->changeTrigger("t1:YMD + 1 eq 20100101");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");
}


BOOST_AUTO_TEST_CASE( test_repeat_data_arithmetic_take_from_end_of_month )
{
   cout << "ANode:: ...test_repeat_data_arithmetic_take_from_end_of_month\n" ;

   Defs theDefs;
   task_ptr t2,t1;
   {
      suite_ptr s1 = theDefs.add_suite("s1");
      t1 = s1->add_task("t1"); t1->addRepeat( RepeatDate("YMD",20090101,20091231,1));
      t2 = s1->add_task("t2"); t2->add_trigger("t1:YMD ge 20080601");
   }
   theDefs.beginAll();

   // Check trigger expressions
   std::string errorMsg, warningMsg;
   BOOST_REQUIRE_MESSAGE(theDefs.check(errorMsg,warningMsg),"Expected triggers expressions to parse " << errorMsg);

   // Check the end of each month - 1
   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090201,20101231,1));   // jan
   t2->changeTrigger("t1:YMD - 1 eq 20090131");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090301,20101231,1));  // feb
   t2->changeTrigger("t1:YMD - 1 eq 20090228");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090401,20101231,1)); // mar
   t2->changeTrigger("t1:YMD - 1 eq 20090331");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090501,20101231,1)); // apr
   t2->changeTrigger("t1:YMD - 1 eq 20090430");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090601,20101231,1)); // may
   t2->changeTrigger("t1:YMD - 1 eq 20090531");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090701,20101231,1)); // June
   t2->changeTrigger("t1:YMD - 1 eq 20090630");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090801,20101231,1)); // July
   t2->changeTrigger("t1:YMD - 1 eq 20090731");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20090901,20101231,1)); // Aug
   t2->changeTrigger("t1:YMD - 1 eq 20090831");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20091001 ,20101231,1)); // Sept
   t2->changeTrigger("t1:YMD - 1 eq 20090930");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20091101 ,20101231,1)); // Oct
   t2->changeTrigger("t1:YMD - 1 eq 20091031");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20091201 ,20101231,1)); // Nov
   t2->changeTrigger("t1:YMD - 1 eq 20091130");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");

   t1->deleteRepeat();
   t1->addRepeat( RepeatDate("YMD",20100101 ,20101231,1)); // Dec
   t2->changeTrigger("t1:YMD - 1 eq 20091231");
   BOOST_CHECK_MESSAGE(t2->triggerAst()->evaluate(),"Expected trigger to use date arithmetic");
}

BOOST_AUTO_TEST_SUITE_END()
