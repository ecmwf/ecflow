//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $
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
#include "ServerToClientCmd.hpp"
#include "MyDefsFixture.hpp"
#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "System.hpp"
#include "PrintStyle.hpp"
#include "Defs.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( BaseTestSuite )

static defs_ptr create_defs()
{
   // suite s1
   //   family f1
   //     trigger f2 == complete
   //     task t1
   //     task t2
   //   endfamily
   //   family f2
   //     task t1
   //     task t2
   //   endfamily
   // endsuite
   defs_ptr theDefs = Defs::create();
   suite_ptr suite = theDefs->add_suite( "s1" ) ;
   family_ptr f1  = suite->add_family( "f1" ) ;
   family_ptr f2  = suite->add_family( "f2" ) ;
   f1->add_trigger("f2 == complete");
   f1->add_task("t1");
   f1->add_task("t2");
   f2->add_task("t1");
   f2->add_task("t2");

   return theDefs;
}

BOOST_AUTO_TEST_CASE( test_ECFLOW_189 )
{
   cout << "Base:: ...test_ECFLOW_189\n";
   defs_ptr the_defs = create_defs();
   the_defs->beginAll();
   node_ptr s1 = the_defs->findAbsNode("/s1");
   node_ptr f1 = the_defs->findAbsNode("/s1/f1");
   node_ptr t1 = the_defs->findAbsNode("/s1/f1/t1");
   node_ptr t2 = the_defs->findAbsNode("/s1/f1/t2");

   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND, t1->absNodePath())));
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::SUSPEND, t2->absNodePath())));

   TestHelper::test_state(t1,DState::SUSPENDED);
   TestHelper::test_state(t2,DState::SUSPENDED);

   // Now resume /s1/f1/t1 and /s1/f1/t2
   TestHelper::invokeRequest(the_defs.get(),Cmd_ptr( new PathsCmd(PathsCmd::RESUME, t1->absNodePath())));

   // We expect state to be queued since the trigger on /s1/f1 should prevent jobs from running
   // If we find submitted or aborted(i.e it was free to run, but could not generate the jobs), then its an error
   TestHelper::test_state(t1,NState::QUEUED);
   TestHelper::test_state(t2,NState::QUEUED);

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
