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
#include <cstdlib>

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_ECFLOW_195 )
{
   cout << "ANode:: ...test_ECFLOW_195 re-queue on task should retain label value\n";

   defs_ptr defs = Defs::create();
   suite_ptr s1 = defs->add_suite("s1");
   family_ptr f1 = s1->add_family("f1");
   task_ptr t1 = f1->add_task("t1");

   s1->addLabel( Label("s1","s1"));
   f1->addLabel( Label("f1","f1"));
   t1->addLabel( Label("t1","t1"));

   defs->beginAll();

   // Set the labels, with new values
   s1->changeLabel("s1","xx");
   f1->changeLabel("f1","xx");
   t1->changeLabel("t1","xx");

   {  // Check new values have not changed
      Label s1_label = s1->find_label("s1");
      BOOST_CHECK_MESSAGE(!s1_label.empty(),"expected to find label 's1'");
      BOOST_CHECK_MESSAGE(s1_label.new_value() == "xx","expected xx but found " << s1_label.new_value());

      Label f1_label = f1->find_label("f1");
      BOOST_CHECK_MESSAGE(!f1_label.empty(),"expected to find label 'f1'");
      BOOST_CHECK_MESSAGE(f1_label.new_value() == "xx","expected xx but found " << f1_label.new_value());

      Label t1_label = t1->find_label("t1");
      BOOST_CHECK_MESSAGE(!t1_label.empty(),"expected to find label 't1'");
      BOOST_CHECK_MESSAGE(t1_label.new_value() == "xx","expected xx but found " << t1_label.new_value());
   }

   // Now requee. the suite and family should be cleared and task label should remain.
   defs->requeue();

   {  // Suite and Family labels should be reset, and task labels should retain their values
      Label s1_label = s1->find_label("s1");
      BOOST_CHECK_MESSAGE(!s1_label.empty(),"expected to find label 's1'");
      BOOST_CHECK_MESSAGE(s1_label.new_value().empty(),"expected empty string for suite label value after re-queue, but found " << s1_label.new_value());

      Label f1_label = f1->find_label("f1");
      BOOST_CHECK_MESSAGE(!f1_label.empty(),"expected to find label 'f1'");
      BOOST_CHECK_MESSAGE(f1_label.new_value().empty(),"expected empty string for family label value after re-queue, but found " << f1_label.new_value());

      Label t1_label = t1->find_label("t1");
      BOOST_CHECK_MESSAGE(!t1_label.empty(),"expected to find label 't1'");
      BOOST_CHECK_MESSAGE(t1_label.new_value() == "xx","Expected task label to remain unchanged after re-queue but found " << t1_label.new_value());
   }

   // After explicit re-queue expect new labels to be empty
   s1->requeue_labels();
   f1->requeue_labels();
   t1->requeue_labels();
   {
      Label s1_label = s1->find_label("s1");
      BOOST_CHECK_MESSAGE(!s1_label.empty(),"expected to find label 's1'");
      BOOST_CHECK_MESSAGE(s1_label.new_value().empty(),"expected empty string for suite label value after explicit re-queue, but found " << s1_label.new_value());

      Label f1_label = f1->find_label("f1");
      BOOST_CHECK_MESSAGE(!f1_label.empty(),"expected to find label 'f1'");
      BOOST_CHECK_MESSAGE(f1_label.new_value().empty(),"expected empty string for family label value after explicit re-queue, but found " << f1_label.new_value());

      Label t1_label = t1->find_label("t1");
      BOOST_CHECK_MESSAGE(!t1_label.empty(),"expected to find label 't1'");
      BOOST_CHECK_MESSAGE(t1_label.new_value().empty(),"expected empty string for task label value after explicit re-queue, but found " << t1_label.new_value());
   }
}

BOOST_AUTO_TEST_SUITE_END()
