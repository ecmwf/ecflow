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

BOOST_AUTO_TEST_CASE( test_state_hierarchy )
{
   cout << "ANode:: ...test_state_hierarchy\n";

   // Create a defs file corresponding to:
   //suite s1
   // family f1
   //    task t1
   //    task t2
   //    task t3
   //    task t4
   //    task t5
   //  endfamily
   //endsuite
   Defs theDefs;
   suite_ptr s1 = theDefs.add_suite( "suite1" );
   family_ptr f1 = s1->add_family( "family" );
   task_ptr t1 = f1->add_task( "t1" );
   task_ptr t2 = f1->add_task( "t2" );
   task_ptr t3 = f1->add_task( "t3" );
   task_ptr t4 = f1->add_task( "t4" );

   NState::State result = s1->computedState(Node::IMMEDIATE_CHILDREN);
   BOOST_REQUIRE_MESSAGE(result == NState::UNKNOWN,"Expected NState::UNKNOWN for suite but found " << NState::toString(result));

   result = f1->computedState(Node::IMMEDIATE_CHILDREN);
   BOOST_REQUIRE_MESSAGE(result == NState::UNKNOWN,"Expected NState::UNKNOWN for family but found " << NState::toString(result));

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::QUEUED,"Expected NState::QUEUED for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t1->set_state( NState::ABORTED);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::ABORTED,"Expected NState::ABORTED for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t2->set_state( NState::SUBMITTED);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::SUBMITTED,"Expected NState::SUBMITTED for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t3->set_state( NState::ACTIVE);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::ACTIVE,"Expected NState::ACTIVE for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t3->set_state( NState::ACTIVE);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::ACTIVE,"Expected NState::ACTIVE for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t1->set_state(NState::COMPLETE);
      t2->set_state( NState::COMPLETE);
      t3->set_state( NState::COMPLETE);
      t4->set_state( NState::COMPLETE);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::COMPLETE,"Expected NState::COMPLETE for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t1->set_state( NState::COMPLETE);
      t2->set_state( NState::ACTIVE);
      t3->set_state( NState::ACTIVE);
      t4->set_state( NState::QUEUED);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::ACTIVE,"Expected NState::ACTIVE for family but found " << NState::toString(result));
   }

   {
      f1->setStateOnlyHierarchically(NState::QUEUED);
      t1->set_state( NState::COMPLETE);
      t2->set_state( NState::ACTIVE);
      t3->set_state( NState::ABORTED);
      t4->set_state( NState::QUEUED);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::ABORTED,"Expected NState::ABORTED for family but found " << NState::toString(result));
   }

   { // ECFLOW-347
      f1->setStateOnlyHierarchically(NState::UNKNOWN);
      t1->set_state( NState::COMPLETE);
      result = f1->computedState(Node::IMMEDIATE_CHILDREN);
      BOOST_REQUIRE_MESSAGE(result == NState::COMPLETE,"Expected NState::COMPLETE for family but found " << NState::toString(result));
   }
}

BOOST_AUTO_TEST_SUITE_END()
