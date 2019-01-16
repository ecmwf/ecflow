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
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Str.hpp"
#include "Cal.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

static void findParentVariableValue(task_ptr t, const std::string& name, const std::string& expected)
{
   std::string value;
   BOOST_CHECK_MESSAGE(t->findParentVariableValue(name,value), "Task " << t->debugNodePath() << " could not find variable of name " << name );
//   if (expected.empty()) std::cout << name  << " = " << value << "\n";
   if (!expected.empty())
      BOOST_CHECK_MESSAGE( value == expected , "From task " << t->debugNodePath() << " for variable " << name  << " expected value " << expected << " but found " <<  value );
}

BOOST_AUTO_TEST_CASE( test_generated_variables )
{
   std::cout <<  "ANode:: ...test_generated_variables\n";

   task_ptr t;

   Defs defs; {

      suite_ptr suite = defs.add_suite("suite");
      suite->addRepeat( RepeatInteger("RepeatInteger",10,20,1));

      family_ptr fam = suite->add_family("f" );
      fam->addRepeat( RepeatDate("YMD",20090101,20091231,1));

      t = fam->add_family("f2")->add_task("t");
      std::vector<std::string> stringList; stringList.reserve(3);
      stringList.emplace_back("AA"); stringList.emplace_back("BB"); stringList.emplace_back("CC");
      t->addRepeat( RepeatEnumerated("AEnum",stringList));
   }

   // Generate variables, needed since,findParentVariableValue also serach's the generated variables
   defs.beginAll();

   // Check Submittable generated variables
   findParentVariableValue(t,"TASK","t");
   findParentVariableValue(t,Str::ECF_RID(),"");
   findParentVariableValue(t,Str::ECF_TRYNO(),"0");
   findParentVariableValue(t,Str::ECF_NAME(),"/suite/f/f2/t");
   findParentVariableValue(t,Str::ECF_PASS(),"");
   findParentVariableValue(t,Str::ECF_JOB(),"./suite/f/f2/t.job0");
   findParentVariableValue(t,Str::ECF_JOBOUT(),"./suite/f/f2/t.0");
   findParentVariableValue(t,Str::ECF_SCRIPT(),"./suite/f/f2/t.ecf");

   // Check Family generated variables
   findParentVariableValue(t,"FAMILY","f/f2");
   findParentVariableValue(t,"FAMILY1","f2");

   // Check Suite generated variables
   findParentVariableValue(t,"SUITE","suite");
   findParentVariableValue(t,"YYYY","");
   findParentVariableValue(t,"DOW","");
   findParentVariableValue(t,"DOY","");
   findParentVariableValue(t,"DATE","");
   findParentVariableValue(t,"DAY","");
   findParentVariableValue(t,"DD","");
   findParentVariableValue(t,"MM","");
   findParentVariableValue(t,"MONTH","");
   findParentVariableValue(t,"ECF_DATE","");
   findParentVariableValue(t,"ECF_CLOCK","");
   findParentVariableValue(t,"ECF_TIME","");
   findParentVariableValue(t,"ECF_JULIAN","");

   // Test repeat generated variables
   findParentVariableValue(t,"AEnum","AA");
   findParentVariableValue(t,"YMD","20090101");
   findParentVariableValue(t,"YMD_YYYY","2009");
   findParentVariableValue(t,"YMD_MM","1");
   findParentVariableValue(t,"YMD_DD","1");
   findParentVariableValue(t,"YMD_DOW","4");
   findParentVariableValue(t,"YMD_JULIAN",boost::lexical_cast<std::string>(Cal::date_to_julian(20090101)));
   findParentVariableValue(t,"RepeatInteger","10");
}

BOOST_AUTO_TEST_SUITE_END()
