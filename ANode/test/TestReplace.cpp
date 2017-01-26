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
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Ecf.hpp"

using namespace std;
using namespace ecf;

class ExpectStateChange  {
public:
   ExpectStateChange() : state_change_no_(Ecf::state_change_no()) {Ecf::set_server(true); }
   ~ExpectStateChange() { BOOST_CHECK_MESSAGE(state_change_no_ != Ecf::state_change_no() ,"Expected state change" );Ecf::set_server(false); }
private:
   unsigned int state_change_no_;
};

class ExpectModifyChange  {
public:
   ExpectModifyChange() : modify_change_no_(Ecf::modify_change_no()) {Ecf::set_server(true); }
   ~ExpectModifyChange() { BOOST_CHECK_MESSAGE(modify_change_no_ != Ecf::modify_change_no() ,"Expected Modify change" );Ecf::set_server(false);}
private:
   unsigned int modify_change_no_;
};

class ExpectNoChange {
public:
   ExpectNoChange() : state_change_no_(Ecf::state_change_no()), modify_change_no_(Ecf::modify_change_no()) { Ecf::set_server(true);}
   ~ExpectNoChange() {
      BOOST_CHECK_MESSAGE(state_change_no_ == Ecf::state_change_no() && modify_change_no_ == Ecf::modify_change_no(),"Expected no change" );
      Ecf::set_server(false);
   }
private:
   unsigned int state_change_no_;
   unsigned int modify_change_no_;
};

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_replace_add_task )
{
   cout << "ANode:: ...test_replace_add_task\n" ;
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = Suite::create( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task( "t2"  );
      clientDef->addSuite( suite );
   }

   // add Child t2 to the server defs
   Defs serverDefs; {
      suite_ptr suite = Suite::create( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task( "t1"  );
      serverDefs.addSuite( suite );
   }

   Defs expectedDefs;  {
      suite_ptr suite = Suite::create( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task(  "t2"   );   // notice we preserve client position, and not server position
      fam->add_task(  "t1"   );
      expectedDefs.addSuite( suite );
   }

   ExpectStateChange expect_state_change;
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/family/t2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
   BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
}

BOOST_AUTO_TEST_CASE( test_replace_add_suite )
{
   cout << "ANode:: ...test_replace_add_suite\n";
   // In this test the server defs is *EMPTY* hence we should copy/move over the whole suite
   // provided the a/ Path to node exists in the client def, b/ create nodes as needed is TRUE
   Defs expectedDefs;  {
      suite_ptr suite = expectedDefs.add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task(  "t1"   );
      fam->add_task(  "t2"   );
   }

   {
      // The whole suite should get *MOVED* from clientDef to the *EMPTY* server def. i.e add
      defs_ptr clientDef = Defs::create(); {
         suite_ptr suite = clientDef->add_suite( "suite1" ) ;
         family_ptr fam = suite->add_family("family" ) ;
         fam->add_task( "t1"  );
         fam->add_task( "t2"  );
      }

      ExpectModifyChange expect_state_change;
      Defs serverDefs;  // Server defs is empty

      std::string errorMsg;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/family/t2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }

   {
      // The whole suite should get *MOVED* from clientDef to the *EMPTY* server def. i.e add
       defs_ptr clientDef = Defs::create(); {
          suite_ptr suite = clientDef->add_suite( "suite1" ) ;
          family_ptr fam = suite->add_family("family" ) ;
          fam->add_task( "t1"  );
          fam->add_task( "t2"  );
       }

       ExpectModifyChange expect_state_change;
       Defs serverDefs; // Server defs is empty

       std::string errorMsg;
       BOOST_REQUIRE_MESSAGE(serverDefs.replaceChild("/suite1/family",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
       BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }

   {
       // The whole suite should get *MOVED* from clientDef to the *EMPTY* server def. i.e add
       defs_ptr clientDef = Defs::create(); {
          suite_ptr suite = clientDef->add_suite( "suite1" ) ;
          family_ptr fam = suite->add_family("family" ) ;
          fam->add_task( "t1"  );
          fam->add_task( "t2"  );
       }

       ExpectModifyChange expect_state_change;
       Defs serverDefs;  // Server defs is empty

       std::string errorMsg;
       BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
       BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
}

BOOST_AUTO_TEST_CASE( test_replace_child )
{
	cout << "ANode:: ...test_replace_child\n" ;
	defs_ptr clientDef = Defs::create(); {
 		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
 		fam->addTask(   Task::create( "t1" )  );
 		suite->addFamily( fam );
 		clientDef->addSuite( suite );
 	}
	Defs comparisonDef;  {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
 		fam->addTask( Task::create( "t1" )  );
 		suite->addFamily( fam );
 		comparisonDef.addSuite( suite );
 	}
	BOOST_CHECK_MESSAGE(comparisonDef == *clientDef,"client and comparisonDef should be the same");

   ExpectModifyChange expect_state_change;
 	std::string errorMsg;
 	Defs serverDefs;
 	BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
	BOOST_CHECK_MESSAGE(comparisonDef == serverDefs,"comparisonDef and servers defs should be the same");
}

BOOST_AUTO_TEST_CASE( test_replace_add_preserves_states )
{
   cout << "ANode:: ...test_replace_add_preserves_states\n" ;
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = clientDef->add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task( "t1"  );
      fam->add_task( "t2"  );
      fam->add_task( "t3"  );
      fam->add_task( "t4"  );
   }

   // add Child t4 to the server defs, the states on t1->t3 should be preserved
   // The abort should be progagated up the node tree
   family_ptr fam;
   suite_ptr suite;
   Defs serverDefs; {
      suite = serverDefs.add_suite( "suite1" ) ;
      fam = suite->add_family("family" ) ;
      task_ptr t1 = fam->add_task( "t1"  );
      task_ptr t2 = fam->add_task( "t2"  );
      task_ptr t3 = fam->add_task( "t3"  );
      serverDefs.beginAll();
      t1->set_state(NState::COMPLETE);
      t2->set_state(NState::ABORTED);
      t3->set_state(NState::ACTIVE);
   }

   //cout << serverDefs;
   ExpectStateChange expect_state_change;
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/family/t4",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );

   /// The Nodes t1,t2,t3 may have been replaced hence we must get Nodes again
   node_ptr st1 = serverDefs.findAbsNode("/suite1/family/t1");
   node_ptr st2 = serverDefs.findAbsNode("/suite1/family/t2");
   node_ptr st3 = serverDefs.findAbsNode("/suite1/family/t3");
   node_ptr st4 = serverDefs.findAbsNode("/suite1/family/t4");
   BOOST_REQUIRE_MESSAGE(st1,"Expected to find task t1");
   BOOST_REQUIRE_MESSAGE(st2,"Expected to find task t2");
   BOOST_REQUIRE_MESSAGE(st3,"Expected to find task t3");
   BOOST_REQUIRE_MESSAGE(st4,"Expected to find task t4");
   BOOST_REQUIRE_MESSAGE(st1->state() == NState::COMPLETE," state on task t1 not preserved after replace");
   BOOST_REQUIRE_MESSAGE(st2->state() == NState::ABORTED," state on task t2 not preserved after replace");
   BOOST_REQUIRE_MESSAGE(st3->state() == NState::ACTIVE," state on task t3 not preserved after replace");
   BOOST_REQUIRE_MESSAGE(st4->state() == NState::QUEUED," state on task t4 to be queued");
   BOOST_REQUIRE_MESSAGE(fam->state() == NState::ABORTED,"Aborted should have propagated to family");
   BOOST_REQUIRE_MESSAGE(suite->state() == NState::ABORTED,"Aborted should have propagated to suite");
   BOOST_REQUIRE_MESSAGE(serverDefs.state() == NState::ABORTED,"Aborted should have propagated to Defs");
}

BOOST_AUTO_TEST_CASE( test_replace_preserves_sibling_states )
{
   cout << "ANode:: ...test_replace_preserves_sibling_states\n" ;
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = Suite::create( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task( "t4"  );
      clientDef->addSuite( suite );
   }

   // add Child t4 to the server defs, the states on t1->t4 should be preserved
   task_ptr t1,t2,t3;
   Defs serverDefs; {
      suite_ptr suite = serverDefs.add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      t1 = fam->add_task( "t1"  ); t1->set_state(NState::COMPLETE);
      t2 = fam->add_task( "t2"  ); t2->set_state(NState::ABORTED);
      t3 = fam->add_task( "t3"  ); t3->set_state(NState::ACTIVE);
      fam->add_task( "t4"  );
   }

   ExpectStateChange expect_state_change;
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/family/t4",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
   BOOST_REQUIRE_MESSAGE(t1->state() == NState::COMPLETE," state on task t1 not preserved after replace");
   BOOST_REQUIRE_MESSAGE(t2->state() == NState::ABORTED," state on task t2 not preserved after replace");
   BOOST_REQUIRE_MESSAGE(t3->state() == NState::ACTIVE," state on task t3 not preserved after replace");
}

BOOST_AUTO_TEST_CASE( test_replace_preserves_begun_status )
{
   cout << "ANode:: ...test_replace_preserves_begun_status\n" ;
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = clientDef->add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family( "family" ) ;
      fam->addTask(   Task::create( "t1" )  );
   }
   Defs comparisonDef;  {
      suite_ptr suite = comparisonDef.add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family( "family" ) ;
      fam->addTask( Task::create( "t1" )  );
   }
   BOOST_CHECK_MESSAGE(comparisonDef == *clientDef,"client and comparisonDef should be the same");
   comparisonDef.beginAll();

   defs_ptr serverDefs = Defs::create(); {
      suite_ptr suite = serverDefs->add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family( "family" ) ;
      fam->addTask(   Task::create( "t1" )  );
   }
   serverDefs->beginAll();

   ExpectStateChange expect_state_change;
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE( serverDefs->replaceChild("/suite1",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
   BOOST_REQUIRE_MESSAGE( serverDefs->findSuite("suite1"),"Can't find suite1");
   BOOST_REQUIRE_MESSAGE( serverDefs->findSuite("suite1")->begun(),"Expected replaced suite to preserve begun status");
   Ecf::set_debug_equality(true);  // only has affect in DEBUG build
   BOOST_CHECK_MESSAGE(comparisonDef == *serverDefs,"comparisonDef and servers defs should be the same");
   Ecf::set_debug_equality(false);
}

BOOST_AUTO_TEST_CASE( test_replace_add_node )
{
	cout << "ANode:: ...test_replace_add_node\n" ;
	defs_ptr clientDef = Defs::create(); {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" )  );
 		suite->addFamily( fam );
  		suite->addTask(  Task::create( "t2" ) );
 		clientDef->addSuite( suite );
 	}
	Defs comparisonDef; {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" ) );
 		suite->addFamily( fam );
  		suite->addTask( Task::create( "t2" )  );
 		comparisonDef.addSuite( suite );
 	}
	BOOST_CHECK_MESSAGE(comparisonDef == *clientDef,"client and comparisonDef should be the same");


	// Here /suite1/t2 does not exist in the server. Moved from client defs to server
 	Defs serverDefs; {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" ) );
  		suite->addFamily( fam );
 		serverDefs.addSuite( suite );
 	}

   ExpectStateChange expect_state_change;
   std::string errorMsg;
 	BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/t2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
	BOOST_CHECK_MESSAGE(comparisonDef == serverDefs,"comparisonDef and servers defs should be the same");
}

BOOST_AUTO_TEST_CASE( test_replace_add_hierarchy )
{
	cout << "ANode:: ...test_replace_add_hierarchy\n" ;
	defs_ptr clientDef = Defs::create(); {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fa = Family::create( "fa" ) ;
 		family_ptr fb = Family::create( "fb" ) ;
 		family_ptr fc = Family::create( "fc" ) ;
 		family_ptr fd = Family::create( "fd" ) ;
 		fa->addFamily( fb );
 		fb->addFamily( fc );
 		fc->addFamily( fd );
    	fd->addTask( Task::create( "t1" )  );
 		suite->addFamily( fa );
  		clientDef->addSuite( suite );
 	}
	Defs comparisonDef; {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fa = Family::create( "fa" ) ;
 		family_ptr fb = Family::create( "fb" ) ;
 		family_ptr fc = Family::create( "fc" ) ;
 		family_ptr fd = Family::create( "fd" ) ;
 		fa->addFamily( fb );
 		fb->addFamily( fc );
 		fc->addFamily( fd );
    	fd->addTask( Task::create( "t1" )  );
 		suite->addFamily( fa );
 		comparisonDef.addSuite( suite );
 	}
	BOOST_CHECK_MESSAGE(comparisonDef == *clientDef,"client and comparisonDef should be the same");


	// Here /suite1/fa/fb/fc/fd/t1 does not exist in the server.  These should be created.
	// by adding family "fa" as a child of suite1
 	Defs serverDefs; {
		suite_ptr suite = Suite::create( "suite1" ) ;
  		serverDefs.addSuite( suite );
 	}

   ExpectStateChange expect_state_change;
   std::string errorMsg;
 	BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/fa/fb/fc/fd/t1",clientDef,true/*create nodes as needed*/, false/*force*/,errorMsg), errorMsg  );
	BOOST_CHECK_MESSAGE(comparisonDef == serverDefs,"comparisonDef and servers defs should be the same");
}


BOOST_AUTO_TEST_CASE( test_replace_order_preserved_for_suite )
{
   cout << "ANode:: ...test_replace_order_preserved_for_suite\n" ;
   // Test that when we replace a suite, its order is preserved,
   // See  ECFLOW-23 - When replacing a node the order is changed.
   defs_ptr clientDef = Defs::create(); {
      clientDef->add_suite( "s1" ) ;
      clientDef->add_suite( "s2" ) ;
      clientDef->add_suite( "s3" ) ;
   }

   // Replace suite s1 with another suite s1 check order is preserved
   Defs serverDefs; {
      serverDefs.add_suite( "s1" ) ;
      serverDefs.add_suite( "s2" ) ;
      serverDefs.add_suite( "s3" ) ;
   }

   Defs expectedDefs;  {
      expectedDefs.add_suite( "s1" ) ;
      expectedDefs.add_suite( "s2" ) ;
      expectedDefs.add_suite( "s3" ) ;
   }

   std::string errorMsg;
   {
      ExpectModifyChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/s1",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   {
      ExpectModifyChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/s2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   {
      ExpectModifyChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/s3",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
}


BOOST_AUTO_TEST_CASE( test_replace_order_preserved_for_family )
{
   cout << "ANode:: ...test_replace_order_preserved_for_family\n" ;
   // Test that when we replace a family, its order is preserved,
   // See  ECFLOW-23 - When replacing a node the order is changed.
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = clientDef->add_suite( "suite1" ) ;
      suite->add_family("f1" ) ;
      suite->add_family("f2" ) ;
      suite->add_family("f3" ) ;
    }

   // Replace family f1 with another family f1 check order is preserved
   Defs serverDefs; {
      suite_ptr suite = serverDefs.add_suite( "suite1" ) ;
      suite->add_family("f1" ) ;
      suite->add_family("f2" ) ;
      suite->add_family("f3" ) ;
   }

   Defs expectedDefs;  {
      suite_ptr suite = expectedDefs.add_suite( "suite1" ) ;
      suite->add_family("f1" ) ;
      suite->add_family("f2" ) ;
      suite->add_family("f3" ) ;
   }

   std::string errorMsg;
   Ecf::set_debug_equality(true);  // only has affect in DEBUG build
   {
      ExpectStateChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/f1",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   {
      ExpectStateChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/f2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   {
      ExpectStateChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/f3",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   Ecf::set_debug_equality(false);
}

BOOST_AUTO_TEST_CASE( test_replace_order_preserved_for_task )
{
   cout << "ANode:: ...test_replace_order_preserved_for_task\n" ;
   // Test that when we replace a family, its order is preserved,
   // See  ECFLOW-23 - When replacing a node the order is changed.
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = clientDef->add_suite( "suite1" ) ;
      family_ptr f1 = suite->add_family("f1" ) ;
      f1->add_task("t1");
      f1->add_task("t2");
      f1->add_task("t3");
    }

   // Replace task t1 with another task t1, check order is preserved
   Defs serverDefs; {
      suite_ptr suite = serverDefs.add_suite( "suite1" ) ;
      family_ptr f1 = suite->add_family("f1" ) ;
      f1->add_task("t1");
      f1->add_task("t2");
      f1->add_task("t3");
   }

   Defs expectedDefs;  {
      suite_ptr suite = expectedDefs.add_suite( "suite1" ) ;
      family_ptr f1 = suite->add_family("f1" ) ;
      f1->add_task("t1");
      f1->add_task("t2");
      f1->add_task("t3");
   }

   std::string errorMsg;
   Ecf::set_debug_equality(true);  // only has affect in DEBUG build
   {
      ExpectStateChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/f1/t1",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   {
      ExpectStateChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/f1/t2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   {
      ExpectStateChange expect_state_change;
      BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/f1/t3",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), errorMsg  );
      BOOST_CHECK_MESSAGE(expectedDefs == serverDefs,"expectedDefs and servers defs should be the same");
   }
   Ecf::set_debug_equality(false);
}

BOOST_AUTO_TEST_CASE( test_replace_child_errors )
{
	cout << "ANode:: ...test_replace_child_errors\n" ;
	defs_ptr clientDef = Defs::create(); {
 		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" ) );
 		suite->addFamily( fam );
  		suite->addTask(  Task::create( "t2" ) );
 		clientDef->addSuite( suite );
 	}

	ExpectNoChange expect_no_change;
 	Defs serverDefs;
 	std::string errorMsg;
 	BOOST_REQUIRE_MESSAGE(!serverDefs.replaceChild("/suite1/i/dont/exist", clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), "Expected failure"  );
}

BOOST_AUTO_TEST_CASE( test_replace_child_errors_2 )
{
	cout << "ANode:: ...test_replace_child_errors_2\n" ;
	defs_ptr clientDef = Defs::create(); {
 		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" )  );
 		suite->addFamily( fam );
  		suite->addTask(  Task::create( "t2" ) );
 		clientDef->addSuite( suite );
 	}
  	Defs serverDefs; {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" ) );
  		suite->addFamily( fam );
 		serverDefs.addSuite( suite );
 	}

  	std::string errorMsg;
  	{
  	   ExpectNoChange expect_no_change;
  	   // because createNodesAsNeeded is false, child adoption should fail since /suite1/t2
  	   // does not exist on the server
  	   BOOST_REQUIRE_MESSAGE(!serverDefs.replaceChild("/suite1/t2", clientDef , false/*create nodes as needed*/, false, errorMsg), "Expected failure");
  	}
  	{
  	   // With flag now set, we will create any missing nodes even if they dont exist in the server
      ExpectStateChange expect_state_change;
  	   errorMsg.clear();
  	   BOOST_REQUIRE_MESSAGE(serverDefs.replaceChild("/suite1/t2", clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg), "Expected success " << errorMsg );
  	}
}

BOOST_AUTO_TEST_CASE( test_replace_child_errors_3 )
{
	// test force option
	cout << "ANode:: ...test_replace_child_errors_3\n" ;
	defs_ptr clientDef = Defs::create(); {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" )  );
 		suite->addFamily( fam );
  		suite->addTask(  Task::create( "t2" ) );
 		clientDef->addSuite( suite );
 	}

  	Defs serverDefs; {
		suite_ptr suite = Suite::create( "suite1" ) ;
 		family_ptr fam = Family::create( "family" ) ;
  		fam->addTask( Task::create( "t1" )  );
  		suite->addFamily( fam );
 		task_ptr t2 = Task::create( "t2" );
  		suite->addTask(  t2 );
  		serverDefs.addSuite( suite );

 		t2->set_state( NState::ACTIVE ); // Must be done after parent has been setup
 	}

 	std::string errorMsg;
 	{
      ExpectNoChange expect_no_change;
 	   BOOST_REQUIRE_MESSAGE( !serverDefs.replaceChild("/suite1/t2", clientDef, true/*create nodes as needed*/, false/*force*/, errorMsg),
 	      "Expected failure since server task t2 is active, and force not used");
 	}
 	{
 	   errorMsg.clear();
      ExpectStateChange expect_state_change;
 	   BOOST_REQUIRE_MESSAGE( serverDefs.replaceChild("/suite1/t2",  clientDef, true/*create nodes as needed*/, true/*force*/, errorMsg),  errorMsg);
 	}
}

BOOST_AUTO_TEST_CASE( test_replace_add_task_with_bad_trigger )
{
   cout << "ANode:: ...test_replace_add_task_with_bad_trigger\n" ;
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = clientDef->add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      task_ptr task = fam->add_task( "t2"  );
      task->add_trigger("txx eq complete");
   }

   // add Child t2 to the server defs
   Defs serverDefs; {
      suite_ptr suite = serverDefs.add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task( "t1"  );
   }

   // Expect to pass, since the replace part is ok.
   std::string errorMsg;
   node_ptr replaced_node;
   {
      ExpectStateChange expect_state_change;
      replaced_node = serverDefs.replaceChild("/suite1/family/t2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg);
      BOOST_REQUIRE_MESSAGE( replaced_node, "Expected replaced to succeed, even though triggers are dodgy"  );
   }

   // Although we have change the data model, Check if the trigger expressions are still valid. Should fail
   std::string warning_msg;
   BOOST_REQUIRE_MESSAGE(!replaced_node->suite()->check(errorMsg,warning_msg),"Expected failure " << errorMsg);
}

BOOST_AUTO_TEST_CASE( test_replace_add_suite_with_bad_triggers )
{
   cout << "ANode:: ...test_replace_add_suite_with_bad_triggers\n";

   // The whole suite should get *MOVED* from clientDef to the *EMPTY* server def. i.e add
   defs_ptr clientDef = Defs::create(); {
      suite_ptr suite = clientDef->add_suite( "suite1" ) ;
      family_ptr fam = suite->add_family("family" ) ;
      fam->add_task( "t1"  );
      fam->add_task( "t2"  );
      task_ptr t3 = fam->add_task("t3");
      t3->add_trigger("txxxxx eq complete");
   }

   // Server defs is empty
   Defs serverDefs;

   // Expect to pass, since the replace part is ok.
   std::string errorMsg;
   node_ptr replaced_node;
   {
      ExpectModifyChange expect_state_change;
      replaced_node = serverDefs.replaceChild("/suite1/family/t2",clientDef,true/*create nodes as needed*/, false/*force*/, errorMsg);
      BOOST_REQUIRE_MESSAGE( replaced_node, "Expected replaced to succeed, even though triggers are dodgy"  );
   }

   // Although we have changed the data model, Check if the trigger expressions are still valid. Should fail.
   std::string warning_msg;
   BOOST_REQUIRE_MESSAGE(!replaced_node->suite()->check(errorMsg,warning_msg),"Expected failure " << errorMsg);

   // reset, to avoid effecting downstream tests
   Ecf::set_state_change_no(0);
   Ecf::set_modify_change_no(0);
}

BOOST_AUTO_TEST_SUITE_END()
