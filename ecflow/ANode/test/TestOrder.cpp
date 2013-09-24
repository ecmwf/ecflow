/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2012 ECMWF.
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

template <typename T>
static std::vector<std::string> toStrVec(const std::vector<T>& vec)
{
	std::vector<std::string> retVec; retVec.reserve(vec.size());
	BOOST_FOREACH(T s, vec) { retVec.push_back(s->name()); }
	return retVec;
}

std::string toString(const std::vector<std::string>& c)
{
	std::stringstream ss;
	std::copy (c.begin(), c.end(), std::ostream_iterator <std::string> (ss, ", "));
	return ss.str();
}

static void test_invariants(Defs& the_defs) {
   std::string errorMsg;
   BOOST_REQUIRE_MESSAGE( the_defs.checkInvariants(errorMsg),"Invariants failed " << errorMsg);
}


BOOST_AUTO_TEST_CASE( test_order )
{
	cout << "ANode:: ...test_order\n" ;
	std::vector<std::string> vec; vec.reserve(5);
	vec.push_back("a");
	vec.push_back("b");
	vec.push_back("c");
	vec.push_back("d");
	vec.push_back("e");
	Defs theDefs; {
		for(size_t s = 0; s < vec.size(); s++) {
 			suite_ptr suite = Suite::create( vec[s] ) ;
 			for(size_t f = 0; f < vec.size(); f++) {
 				family_ptr fam = Family::create( vec[f] ) ;
 				for(size_t t = 0; t < vec.size(); t++) {
 			 		fam->addTask(   Task::create(  vec[t])  );
 				}
 		 		suite->addFamily( fam );
 			}
 			theDefs.addSuite( suite );
		}
	}

	// Test suite ordering ==========================================================================
	// In init state all suite should be in alpha order
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::ALPHA);
	test_invariants(theDefs);
	std::vector<std::string> expected;
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::ALPHA Init state not as expected" );

	// sort in reverse order
	std::sort(expected.begin(),expected.end(),std::greater<std::string>());
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::ORDER);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::ORDER order not as expected" );

	// Change back to alpha, then move suite 'se' to the top
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::ALPHA);
   test_invariants(theDefs);
	expected.clear();
	expected.push_back("e");
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	theDefs.order(theDefs.findAbsNode("/e").get(), NOrder::TOP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::TOP order not as expected" );

	//  move suite 'se' back to the bottom
 	expected.clear();
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	theDefs.order(theDefs.findAbsNode("/e").get(), NOrder::BOTTOM);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::BOTTOM order not as expected" );

	// move suite 'a' up one place. Should be no change, since its already at the top
 	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::UP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::UP order not as expected" );

	// move suite 'e' down one place. Should be no change, since its already at the bottom
 	theDefs.order(theDefs.findAbsNode("/e").get(), NOrder::DOWN);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::DOWN order not as expected" );

	// Move suite 'a' down by one place
	expected.clear();
	expected.push_back("b");
	expected.push_back("a");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::DOWN);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::DOWN order not as expected" );

	// Move suite 'b' up by one place
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::ALPHA);
   test_invariants(theDefs);
	expected.clear();
	expected.push_back("b");
	expected.push_back("a");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	theDefs.order(theDefs.findAbsNode("/b").get(), NOrder::UP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::UP order not as expected" );


	// Test family ordering ==========================================================================
	// In init state all suite should be in alpha order
	suite_ptr suite = theDefs.findSuite("a");
	BOOST_REQUIRE_MESSAGE( suite.get() ,"Expected suite /a to exist " );

	theDefs.order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA);
   test_invariants(theDefs);
	expected.clear();
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::ALPHA Init order " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// sort in reverse order
	std::sort(expected.begin(),expected.end(),std::greater<std::string>());
 	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ORDER);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::ORDER order " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// Change back to alpha, then move family 'e' to the top
	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA);
   test_invariants(theDefs);
	expected.clear();
	expected.push_back("e");
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	suite->order(theDefs.findAbsNode("/a/e").get(), NOrder::TOP);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::TOP order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	//  move family 'e' back to the bottom
 	expected.clear();
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	suite->order(theDefs.findAbsNode("/a/e").get(), NOrder::BOTTOM);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::BOTTOM order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// move family 'a' up one place. Should be no change, since its already at the top
 	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::UP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::UP order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// move family 'e' down one place. Should be no change, since its already at the bottom
 	suite->order(theDefs.findAbsNode("/a/e").get(), NOrder::DOWN);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::DOWN order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// Move family 'a' down by one place
	expected.clear();
	expected.push_back("b");
	expected.push_back("a");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::DOWN);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::DOWN order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// Move family 'b' up by one place
	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA); // reset
   test_invariants(theDefs);
	expected.clear();
	expected.push_back("b");
	expected.push_back("a");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	suite->order(theDefs.findAbsNode("/a/b").get(), NOrder::UP);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::UP order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));


	// Test Task ordering ==========================================================================
	// In init state all tasks should be in alpha order
	Family* family = theDefs.findAbsNode("/a/a")->isFamily();
	BOOST_REQUIRE_MESSAGE( family ,"Expected family /a/a to exist " );

	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA);
   test_invariants(theDefs);
	expected.clear();
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::ALPHA Init state " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// sort in reverse order
	std::sort(expected.begin(),expected.end(),std::greater<std::string>());
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ORDER);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::ORDER  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// Change back to alpha, then move task 'e' to the top
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA); // reset
	expected.clear();
	expected.push_back("e");
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	family->order(theDefs.findAbsNode("/a/a/e").get(), NOrder::TOP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::TOP order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	//  move family 'e' back to the bottom
 	expected.clear();
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	family->order(theDefs.findAbsNode("/a/a/e").get(), NOrder::BOTTOM);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::BOTTOM order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// move task 'a' up one place. Should be no change, since its already at the top
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::UP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::UP order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// move task 'e' down one place. Should be no change, since its already at the bottom
	family->order(theDefs.findAbsNode("/a/a/e").get(), NOrder::DOWN);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::DOWN order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// Move task 'a' down by one place
	expected.clear();
	expected.push_back("b");
	expected.push_back("a");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::DOWN);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::DOWN order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// Move task 'b' up by one place
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA); // reset
	expected.clear();
	expected.push_back("b");
	expected.push_back("a");
	expected.push_back("c");
	expected.push_back("d");
	expected.push_back("e");
	family->order(theDefs.findAbsNode("/a/a/b").get(), NOrder::UP);
   test_invariants(theDefs);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::UP order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));
}

BOOST_AUTO_TEST_CASE( test_alias_order )
{
   cout << "ANode:: ...test_alias_order\n" ;
   task_ptr task;
   Defs theDefs; {
      suite_ptr s = theDefs.add_suite("s");
      task = s->add_task("t");
      task->add_alias_only(); // alias0
      task->add_alias_only(); // alias1
      task->add_alias_only(); // alias2
      task->add_alias_only(); // alias3
   }

   // Test alias ordering ==========================================================================
   // In init state all suite should be in alpha order
   alias_ptr alias0 = task->find_alias("alias0");
   BOOST_REQUIRE_MESSAGE( alias0,"expected to find alias0");

   std::vector<std::string> expected;
   expected.push_back("alias1");
   expected.push_back("alias0");
   expected.push_back("alias2");
   expected.push_back("alias3");
   task->order(alias0.get(), NOrder::DOWN);
   test_invariants(theDefs);
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::DOWN expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );


   task->order(alias0.get(), NOrder::ALPHA);
   test_invariants(theDefs);
   expected.clear();
   expected.push_back("alias0");
   expected.push_back("alias1");
   expected.push_back("alias2");
   expected.push_back("alias3");
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::ALPHA expectex " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );


   task->order(task->find_alias("alias3").get(), NOrder::UP);
   test_invariants(theDefs);
   expected.clear();
   expected.push_back("alias0");
   expected.push_back("alias1");
   expected.push_back("alias3");
   expected.push_back("alias2");
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::UP expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );

   // sort in reverse order
   std::sort(expected.begin(),expected.end(),std::greater<std::string>());
   task->order(alias0.get(), NOrder::ORDER);
   test_invariants(theDefs);
   expected.clear();
   expected.push_back("alias3");
   expected.push_back("alias2");
   expected.push_back("alias1");
   expected.push_back("alias0");
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::ORDER expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );
}

BOOST_AUTO_TEST_SUITE_END()
