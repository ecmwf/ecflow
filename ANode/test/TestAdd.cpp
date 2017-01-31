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
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_add )
{
   cout << "ANode:: ...test_add\n";

   defs_ptr defs = Defs::create();
   task_ptr t1 = Task::create("t1");
   family_ptr f1 = Family::create("f1");
   suite_ptr s1 = Suite::create("s1");

   defs->addSuite(s1);
   s1->addFamily(f1);
   f1->addTask(t1);

   defs_ptr defs2 = Defs::create();
   suite_ptr s2 = Suite::create("s2");
   family_ptr f2 = Family::create("f2");

   // This should all fail since, they are already owned.
   // Otherwise we end up with two different container owning the same object
   BOOST_CHECK_THROW( s2->addFamily(f1),std::runtime_error);
   BOOST_CHECK_THROW( f2->addTask(t1),std::runtime_error);
   BOOST_CHECK_THROW( defs2->addSuite(s1),std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
