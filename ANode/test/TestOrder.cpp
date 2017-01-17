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

static void test_invariants(Defs& the_defs, int line) {
   std::string errorMsg;
   bool passed = the_defs.checkInvariants(errorMsg);
   BOOST_REQUIRE_MESSAGE( passed,"Invariants failed " << errorMsg << " at line " << line);
}


BOOST_AUTO_TEST_CASE( test_order )
{
	cout << "ANode:: ...test_order\n" ;
	std::vector<std::string> vec; vec.reserve(5);
	vec.push_back("a");
	vec.push_back("A");
	vec.push_back("b");
	vec.push_back("B");
	vec.push_back("c");
	Defs theDefs; {
		for(size_t s = 0; s < vec.size(); s++) {
 			suite_ptr suite = theDefs.add_suite( vec[s] ) ;
 			for(size_t f = 0; f < vec.size(); f++) {
 				family_ptr fam = suite->add_family( vec[f] ) ;
 				for(size_t t = 0; t < vec.size(); t++) {
 			 		fam->add_task( vec[t] );
 				}
 			}
		}
	}

   std::vector<std::string> alpha;
   alpha.push_back("a");
   alpha.push_back("A");
   alpha.push_back("b");
   alpha.push_back("B");
   alpha.push_back("c");

   std::vector<std::string> order;
   order.push_back("c");
   order.push_back("B");
   order.push_back("b");
   order.push_back("A");
   order.push_back("a");

	// Test suite ordering ==========================================================================
	// In init state all suite should be in alpha order
	theDefs.order(theDefs.findAbsNode("/A").get(), NOrder::ALPHA);
	test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == alpha,"NOrder::ALPHA expected " << toString(alpha) << " but found " << toString(toStrVec(theDefs.suiteVec())) );

	// sort in reverse order
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::ORDER);
   test_invariants(theDefs,__LINE__);
   BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == order,"NOrder::ORDER expected " << toString(order) << " but found " << toString(toStrVec(theDefs.suiteVec())) );

	// Change back to alpha, then move suite 'c' to the top
	theDefs.order(theDefs.findAbsNode("/A").get(), NOrder::ALPHA);
   BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == alpha,"NOrder::ALPHA expected " << toString(alpha) << " but found " << toString(toStrVec(theDefs.suiteVec())) );
   test_invariants(theDefs,__LINE__);

   std::vector<std::string> expected;
   expected.push_back("c");
   expected.push_back("a");
   expected.push_back("A");
   expected.push_back("b");
   expected.push_back("B");
	theDefs.order(theDefs.findAbsNode("/c").get(), NOrder::TOP);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::TOP expected " << toString(expected) << " but found " << toString(toStrVec(theDefs.suiteVec())) );

	// move suite 'c' back to the bottom
	theDefs.order(theDefs.findAbsNode("/c").get(), NOrder::BOTTOM);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == alpha,"NOrder::BOTTOM order not as expected" );

	// move suite 'a' up one place. Should be no change, since its already at the top
 	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::UP);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == alpha,"NOrder::UP order not as expected" );

	// move suite 'c' down one place. Should be no change, since its already at the bottom
 	theDefs.order(theDefs.findAbsNode("/c").get(), NOrder::DOWN);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == alpha,"NOrder::DOWN order not as expected" );

	// Move suite 'a' down by one place
	expected.clear();
   expected.push_back("A");
   expected.push_back("a");
   expected.push_back("b");
   expected.push_back("B");
   expected.push_back("c");
	theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::DOWN);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::DOWN order not as expected" );

	// Move suite 'b' up by one place
 	expected.clear();
   expected.push_back("A");
   expected.push_back("b");
   expected.push_back("a");
   expected.push_back("B");
   expected.push_back("c");
	theDefs.order(theDefs.findAbsNode("/b").get(), NOrder::UP);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(theDefs.suiteVec()) == expected,"NOrder::UP order not as expected" );


	// Test family ordering ==========================================================================
	// In init state all suite should be in alpha order
	suite_ptr suite = theDefs.findSuite("a");
	BOOST_REQUIRE_MESSAGE( suite.get() ,"Expected suite /a to exist " );

	theDefs.order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == alpha,"NOrder::ALPHA Init order " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(alpha));

	// sort in reverse order
	std::sort(expected.begin(),expected.end(),std::greater<std::string>());
 	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ORDER);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == order,"NOrder::ORDER order " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(order));

	// Change back to alpha, then move family 'e' to the top
	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA);
   BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == alpha,"NOrder::ALPHA expected " << toString(alpha) << " but found " << toString(toStrVec(suite->nodeVec())) );
   test_invariants(theDefs,__LINE__);
	expected.clear();
	expected.push_back("c");
	expected.push_back("a");
	expected.push_back("A");
	expected.push_back("b");
	expected.push_back("B");
	suite->order(theDefs.findAbsNode("/a/c").get(), NOrder::TOP);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::TOP order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	//  move family 'c' back to the bottom
	suite->order(theDefs.findAbsNode("/a/c").get(), NOrder::BOTTOM);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == alpha,"NOrder::BOTTOM order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(alpha));

	// move family 'a' up one place. Should be no change, since its already at the top
 	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::UP);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == alpha,"NOrder::UP order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(alpha));

	// move family 'c' down one place. Should be no change, since its already at the bottom
 	suite->order(theDefs.findAbsNode("/a/c").get(), NOrder::DOWN);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == alpha,"NOrder::DOWN order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(alpha));

	// Move family 'a' down by one place
	expected.clear();
	expected.push_back("A");
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("B");
	expected.push_back("c");
	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::DOWN);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::DOWN order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));

	// Move family 'b' up by one place
	suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA); // reset
   test_invariants(theDefs,__LINE__);
	expected.clear();
   expected.push_back("a");
   expected.push_back("b");
   expected.push_back("A");
   expected.push_back("B");
   expected.push_back("c");
	suite->order(theDefs.findAbsNode("/a/b").get(), NOrder::UP);
	BOOST_REQUIRE_MESSAGE( toStrVec(suite->nodeVec()) == expected,"NOrder::UP order  " << toString(toStrVec(suite->nodeVec())) << " not as expected " << toString(expected));


	// Test Task ordering ==========================================================================
	// In init state all tasks should be in alpha order
	Family* family = theDefs.findAbsNode("/a/a")->isFamily();
	BOOST_REQUIRE_MESSAGE( family ,"Expected family /a/a to exist " );

	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == alpha,"NOrder::ALPHA Init state " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(alpha));

	// sort in reverse order
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ORDER);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == order,"NOrder::ORDER  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(order));

	// Change back to alpha, then move task 'c' to the top
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA); // reset
	expected.clear();
	expected.push_back("c");
	expected.push_back("a");
	expected.push_back("A");
	expected.push_back("b");
	expected.push_back("B");
	family->order(theDefs.findAbsNode("/a/a/c").get(), NOrder::TOP);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::TOP order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	//  move task 'c' back to the bottom
	family->order(theDefs.findAbsNode("/a/a/c").get(), NOrder::BOTTOM);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == alpha,"NOrder::BOTTOM order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(alpha));

	// move task 'a' up one place. Should be no change, since its already at the top
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::UP);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == alpha,"NOrder::UP order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(alpha));

	// move task 'e' down one place. Should be no change, since its already at the bottom
	family->order(theDefs.findAbsNode("/a/a/c").get(), NOrder::DOWN);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == alpha,"NOrder::DOWN order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(alpha));

	// Move task 'a' down by one place
	expected.clear();
	expected.push_back("A");
	expected.push_back("a");
	expected.push_back("b");
	expected.push_back("B");
	expected.push_back("c");
	family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::DOWN);
   test_invariants(theDefs,__LINE__);
	BOOST_REQUIRE_MESSAGE( toStrVec(family->nodeVec()) == expected,"NOrder::DOWN order  " << toString(toStrVec(family->nodeVec())) << " not as expected " << toString(expected));

	// Move task 'b' up by one place
	family->order(theDefs.findAbsNode("/a/a/b").get(), NOrder::DOWN);
	expected.clear();
   expected.push_back("A");
   expected.push_back("a");
   expected.push_back("B");
   expected.push_back("b");
   expected.push_back("c");
   test_invariants(theDefs,__LINE__);
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
   test_invariants(theDefs,__LINE__);
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::DOWN expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );


   task->order(alias0.get(), NOrder::ALPHA);
   test_invariants(theDefs,__LINE__);
   expected.clear();
   expected.push_back("alias0");
   expected.push_back("alias1");
   expected.push_back("alias2");
   expected.push_back("alias3");
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::ALPHA expectex " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );


   task->order(task->find_alias("alias3").get(), NOrder::UP);
   test_invariants(theDefs,__LINE__);
   expected.clear();
   expected.push_back("alias0");
   expected.push_back("alias1");
   expected.push_back("alias3");
   expected.push_back("alias2");
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::UP expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );

   // sort in reverse order
   std::sort(expected.begin(),expected.end(),std::greater<std::string>());
   task->order(alias0.get(), NOrder::ORDER);
   test_invariants(theDefs,__LINE__);
   expected.clear();
   expected.push_back("alias3");
   expected.push_back("alias2");
   expected.push_back("alias1");
   expected.push_back("alias0");
   BOOST_REQUIRE_MESSAGE( toStrVec(task->aliases()) == expected,"NOrder::ORDER expected " << toString(expected) << " but found " << toString(toStrVec(task->aliases())) );
}

BOOST_AUTO_TEST_SUITE_END()
