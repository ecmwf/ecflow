//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #22 $
//
// Copyright 2009-2012 ECMWF.
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
#include "TestHelper.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

BOOST_AUTO_TEST_CASE( test_requeue_with_suspend )
{
   cout << "Base:: ...test_requeue_with_suspend\n";

   defs_ptr the_defs = Defs::create();
   suite_ptr s1 = the_defs->add_suite( "s1" ) ;
   family_ptr f1  = s1->add_family( "f1" ) ;
   task_ptr t1 = f1->add_task("t1");
   task_ptr t2 = f1->add_task("t2");
   task_ptr t3 = f1->add_task("t3");
   t3->addDefStatus(DState::SUSPENDED);

   the_defs->beginAll();

   // After begin/requeue we must honour defs status
   BOOST_CHECK_MESSAGE(t3->isSuspended(),"Expected node to be suspended");

   // Suspend all nodes
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,s1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,f1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,t1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND,t2->absNodePath())));
   BOOST_CHECK_MESSAGE(s1->isSuspended(),"Expected node to be suspended");
   BOOST_CHECK_MESSAGE(f1->isSuspended(),"Expected node to be suspended");
   BOOST_CHECK_MESSAGE(t1->isSuspended(),"Expected node to be suspended");
   BOOST_CHECK_MESSAGE(t2->isSuspended(),"Expected node to be suspended");

   // Re-queue of the nodes, that are suspended, they should *stay* suspended
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(t1->absNodePath())));
   BOOST_CHECK_MESSAGE(t1->isSuspended(),"Expected node to stay suspended");

   // Now re-queue the top level suite, this is suspended.
   // This should stay suspended *BUT* child nodes which are suspend should be cleared
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new RequeueNodeCmd(s1->absNodePath())));
   BOOST_CHECK_MESSAGE(s1->isSuspended(),"Suite should stay suspend");
   BOOST_CHECK_MESSAGE(!f1->isSuspended(),"Expected child nodes to be un-suspended");
   BOOST_CHECK_MESSAGE(!t1->isSuspended(),"Expected child nodes to be un-suspended");
   BOOST_CHECK_MESSAGE(!t2->isSuspended(),"Expected child nodes to be un-suspended");
   BOOST_CHECK_MESSAGE(t3->isSuspended(), "Requeue must honour def status");
}

BOOST_AUTO_TEST_SUITE_END()
