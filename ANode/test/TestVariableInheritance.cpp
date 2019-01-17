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

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

#include <boost/test/unit_test.hpp>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

static void findParentVariableValue(task_ptr t, const std::string& name, const std::string& expected)
{
   std::string value;
   BOOST_CHECK_MESSAGE(t->findParentVariableValue(name,value), "Task " << t->debugNodePath() << " could not find variable of name " << name );
   BOOST_CHECK_MESSAGE( value == expected , "From task " << t->debugNodePath() << " for variable " << name  << " expected value " << expected << " but found " <<  value );
}

BOOST_AUTO_TEST_CASE( test_variable_inheritance )
{
   std::cout <<  "ANode:: ...test_variable_inheritance\n";

   // See page 31, section 5.1 variable inheritance, of SMS users guide
   task_ptr t;
   task_ptr t2 ;
   task_ptr z;

   Defs defs; {
      suite_ptr suite = defs.add_suite("suite");
      suite->addVariable(Variable("TOPLEVEL","10"));
      suite->addVariable(Variable("MIDDLE","10"));
      suite->addVariable(Variable("LOWER","10"));

      family_ptr fam = suite->add_family("f" );
      fam->addVariable( Variable("MIDDLE","20") );
      t = fam->add_task("t");
      t->addVariable( Variable("LOWER","abc") );
      t2 = fam->add_task("t2");

      family_ptr fam2 = suite->add_family("f2" );
      fam2->addVariable( Variable("TOPLEVEL","40") );
      z = fam2->add_task("z");
   }

   // Generate variables, needed since,findParentVariableValue also serach's the generated variables
   defs.beginAll();

   // See page 31, section 5.1 variable inheritance, of SMS users guide
   findParentVariableValue(t,"TOPLEVEL","10");
   findParentVariableValue(t2, "TOPLEVEL","10");
   findParentVariableValue(z,"TOPLEVEL","40");

   findParentVariableValue(t,"MIDDLE","20");
   findParentVariableValue(t2, "MIDDLE","20");
   findParentVariableValue(z,"MIDDLE","10");

   findParentVariableValue(t, "LOWER","abc");
   findParentVariableValue(t2, "LOWER","10");
   findParentVariableValue(z, "LOWER","10");
}

BOOST_AUTO_TEST_SUITE_END()

