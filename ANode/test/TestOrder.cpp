/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/core/CalendarUpdateParams.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Order)

static void test_invariants(Defs& the_defs, int line) {
    std::string errorMsg;
    bool passed = the_defs.checkInvariants(errorMsg);
    BOOST_REQUIRE_MESSAGE(passed, "Invariants failed " << errorMsg << " at line " << line);
}

BOOST_AUTO_TEST_CASE(test_order) {
    cout << "ANode:: ...test_order\n";
    std::vector<std::string> vec;
    vec.reserve(5);
    vec.emplace_back("a");
    vec.emplace_back("A");
    vec.emplace_back("b");
    vec.emplace_back("B");
    vec.emplace_back("c");
    Defs theDefs;
    {
        for (size_t s = 0; s < vec.size(); s++) {
            suite_ptr suite = theDefs.add_suite(vec[s]);
            for (size_t f = 0; f < vec.size(); f++) {
                family_ptr fam = suite->add_family(vec[f]);
                for (const auto& t : vec) {
                    fam->add_task(t);
                }
            }
        }
    }

    std::vector<std::string> alpha;
    alpha.emplace_back("a");
    alpha.emplace_back("A");
    alpha.emplace_back("b");
    alpha.emplace_back("B");
    alpha.emplace_back("c");

    std::vector<std::string> order;
    order.emplace_back("c");
    order.emplace_back("B");
    order.emplace_back("b");
    order.emplace_back("A");
    order.emplace_back("a");

    // Test suite ordering ==========================================================================
    // In init state all suite should be in alpha order
    theDefs.order(theDefs.findAbsNode("/A").get(), NOrder::ALPHA);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == alpha,
                          "NOrder::ALPHA expected "
                              << ecf::algorithm::join(alpha) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec())));

    // sort in reverse order
    theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::ORDER);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == order,
                          "NOrder::ORDER expected "
                              << ecf::algorithm::join(order) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec())));

    // Change back to alpha, then move suite 'c' to the top
    theDefs.order(theDefs.findAbsNode("/A").get(), NOrder::ALPHA);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == alpha,
                          "NOrder::ALPHA expected "
                              << ecf::algorithm::join(alpha) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec())));
    test_invariants(theDefs, __LINE__);

    std::vector<std::string> expected;
    expected.emplace_back("c");
    expected.emplace_back("a");
    expected.emplace_back("A");
    expected.emplace_back("b");
    expected.emplace_back("B");
    theDefs.order(theDefs.findAbsNode("/c").get(), NOrder::TOP);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == expected,
                          "NOrder::TOP expected "
                              << ecf::algorithm::join(expected) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec())));

    // move suite 'c' back to the bottom
    theDefs.order(theDefs.findAbsNode("/c").get(), NOrder::BOTTOM);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == alpha,
                          "NOrder::BOTTOM order not as expected");

    // move suite 'a' up one place. Should be no change, since its already at the top
    theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::UP);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == alpha,
                          "NOrder::UP order not as expected");

    // move suite 'c' down one place. Should be no change, since its already at the bottom
    theDefs.order(theDefs.findAbsNode("/c").get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == alpha,
                          "NOrder::DOWN order not as expected");

    // Move suite 'a' down by one place
    expected.clear();
    expected.emplace_back("A");
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("B");
    expected.emplace_back("c");
    theDefs.order(theDefs.findAbsNode("/a").get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == expected,
                          "NOrder::DOWN order not as expected");

    // Move suite 'b' up by one place
    expected.clear();
    expected.emplace_back("A");
    expected.emplace_back("b");
    expected.emplace_back("a");
    expected.emplace_back("B");
    expected.emplace_back("c");
    theDefs.order(theDefs.findAbsNode("/b").get(), NOrder::UP);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(theDefs.suiteVec()) == expected,
                          "NOrder::UP order not as expected");

    // Test family ordering ==========================================================================
    // In init state all suite should be in alpha order
    suite_ptr suite = theDefs.findSuite("a");
    BOOST_REQUIRE_MESSAGE(suite.get(), "Expected suite /a to exist ");

    theDefs.order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == alpha,
                          "NOrder::ALPHA Init order "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // sort in reverse order
    std::sort(expected.begin(), expected.end(), std::greater<std::string>());
    suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ORDER);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == order,
                          "NOrder::ORDER order "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(order));

    // Change back to alpha, then move family 'e' to the top
    suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == alpha,
                          "NOrder::ALPHA expected "
                              << ecf::algorithm::join(alpha) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec())));
    test_invariants(theDefs, __LINE__);
    expected.clear();
    expected.emplace_back("c");
    expected.emplace_back("a");
    expected.emplace_back("A");
    expected.emplace_back("b");
    expected.emplace_back("B");
    suite->order(theDefs.findAbsNode("/a/c").get(), NOrder::TOP);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == expected,
                          "NOrder::TOP order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(expected));

    //  move family 'c' back to the bottom
    suite->order(theDefs.findAbsNode("/a/c").get(), NOrder::BOTTOM);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == alpha,
                          "NOrder::BOTTOM order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // move family 'a' up one place. Should be no change, since its already at the top
    suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::UP);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == alpha,
                          "NOrder::UP order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // move family 'c' down one place. Should be no change, since its already at the bottom
    suite->order(theDefs.findAbsNode("/a/c").get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == alpha,
                          "NOrder::DOWN order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // Move family 'a' down by one place
    expected.clear();
    expected.emplace_back("A");
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("B");
    expected.emplace_back("c");
    suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == expected,
                          "NOrder::DOWN order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(expected));

    // Move family 'b' up by one place
    suite->order(theDefs.findAbsNode("/a/a").get(), NOrder::ALPHA); // reset
    test_invariants(theDefs, __LINE__);
    expected.clear();
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("A");
    expected.emplace_back("B");
    expected.emplace_back("c");
    suite->order(theDefs.findAbsNode("/a/b").get(), NOrder::UP);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(suite->nodeVec()) == expected,
                          "NOrder::UP order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(suite->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(expected));

    // Test Task ordering ==========================================================================
    // In init state all tasks should be in alpha order
    Family* family = theDefs.findAbsNode("/a/a")->isFamily();
    BOOST_REQUIRE_MESSAGE(family, "Expected family /a/a to exist ");

    family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == alpha,
                          "NOrder::ALPHA Init state "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // sort in reverse order
    family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ORDER);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == order,
                          "NOrder::ORDER  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(order));

    // Change back to alpha, then move task 'c' to the top
    family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::ALPHA); // reset
    expected.clear();
    expected.emplace_back("c");
    expected.emplace_back("a");
    expected.emplace_back("A");
    expected.emplace_back("b");
    expected.emplace_back("B");
    family->order(theDefs.findAbsNode("/a/a/c").get(), NOrder::TOP);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == expected,
                          "NOrder::TOP order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(expected));

    //  move task 'c' back to the bottom
    family->order(theDefs.findAbsNode("/a/a/c").get(), NOrder::BOTTOM);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == alpha,
                          "NOrder::BOTTOM order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // move task 'a' up one place. Should be no change, since its already at the top
    family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::UP);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == alpha,
                          "NOrder::UP order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // move task 'e' down one place. Should be no change, since its already at the bottom
    family->order(theDefs.findAbsNode("/a/a/c").get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == alpha,
                          "NOrder::DOWN order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(alpha));

    // Move task 'a' down by one place
    expected.clear();
    expected.emplace_back("A");
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("B");
    expected.emplace_back("c");
    family->order(theDefs.findAbsNode("/a/a/a").get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == expected,
                          "NOrder::DOWN order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(expected));

    // Move task 'b' up by one place
    family->order(theDefs.findAbsNode("/a/a/b").get(), NOrder::DOWN);
    expected.clear();
    expected.emplace_back("A");
    expected.emplace_back("a");
    expected.emplace_back("B");
    expected.emplace_back("b");
    expected.emplace_back("c");
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(family->nodeVec()) == expected,
                          "NOrder::UP order  "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(family->nodeVec()))
                              << " not as expected " << ecf::algorithm::join(expected));
}

BOOST_AUTO_TEST_CASE(test_alias_order) {
    cout << "ANode:: ...test_alias_order\n";
    task_ptr task;
    Defs theDefs;
    {
        suite_ptr s = theDefs.add_suite("s");
        task        = s->add_task("t");
        task->add_alias_only(); // alias0
        task->add_alias_only(); // alias1
        task->add_alias_only(); // alias2
        task->add_alias_only(); // alias3
    }

    // Test alias ordering ==========================================================================
    // In init state all suite should be in alpha order
    alias_ptr alias0 = task->find_alias("alias0");
    BOOST_REQUIRE_MESSAGE(alias0, "expected to find alias0");

    std::vector<std::string> expected;
    expected.emplace_back("alias1");
    expected.emplace_back("alias0");
    expected.emplace_back("alias2");
    expected.emplace_back("alias3");
    task->order(alias0.get(), NOrder::DOWN);
    test_invariants(theDefs, __LINE__);
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                          "NOrder::DOWN expected "
                              << ecf::algorithm::join(expected) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));

    task->order(alias0.get(), NOrder::ALPHA);
    test_invariants(theDefs, __LINE__);
    expected.clear();
    expected.emplace_back("alias0");
    expected.emplace_back("alias1");
    expected.emplace_back("alias2");
    expected.emplace_back("alias3");
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                          "NOrder::ALPHA expectex "
                              << ecf::algorithm::join(expected) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));

    task->order(task->find_alias("alias3").get(), NOrder::UP);
    test_invariants(theDefs, __LINE__);
    expected.clear();
    expected.emplace_back("alias0");
    expected.emplace_back("alias1");
    expected.emplace_back("alias3");
    expected.emplace_back("alias2");
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                          "NOrder::UP expected "
                              << ecf::algorithm::join(expected) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));

    // sort in reverse order
    std::sort(expected.begin(), expected.end(), std::greater<std::string>());
    task->order(alias0.get(), NOrder::ORDER);
    test_invariants(theDefs, __LINE__);
    expected.clear();
    expected.emplace_back("alias3");
    expected.emplace_back("alias2");
    expected.emplace_back("alias1");
    expected.emplace_back("alias0");
    BOOST_REQUIRE_MESSAGE(ecf::algorithm::transform_to_name_vector(task->aliases()) == expected,
                          "NOrder::ORDER expected "
                              << ecf::algorithm::join(expected) << " but found "
                              << ecf::algorithm::join(ecf::algorithm::transform_to_name_vector(task->aliases())));
}

BOOST_AUTO_TEST_CASE(test_order_by_runtime) {
    cout << "ANode:: ...test_order_by_runtime\n";
    Defs defs;
    {
        std::vector<string> vec{"3", "2", "1"};
        for (const auto& str0 : vec) {
            suite_ptr s = defs.add_suite("s" + str0);
            for (const auto& str : vec) {
                family_ptr f1 = s->add_family("f" + str);
                for (const auto& str2 : vec) {
                    f1->add_task("t" + str2);
                }
            }
        }
    }
    Defs expectedDefs;
    {
        std::vector<string> vec{"1", "2", "3"};
        for (const auto& str0 : vec) {
            suite_ptr s = expectedDefs.add_suite("s" + str0);
            for (const auto& str : vec) {
                family_ptr f1 = s->add_family("f" + str);
                for (const auto& str2 : vec) {
                    f1->add_task("t" + str2);
                }
            }
        }
    }
    // std::cout << defs;

    defs.beginAll();
    expectedDefs.beginAll();

    CalendarUpdateParams calUpdateParams(hours(1));

    Jobs jobs(&defs);
    JobsParam jobsParam;
    jobs.generate(jobsParam);

    for (Submittable* s : jobsParam.submitted()) {
        defs.updateCalendar(calUpdateParams);
        s->set_state(NState::ACTIVE);

        defs.updateCalendar(calUpdateParams);
        if (s->name() == "t2" || s->name() == "t1")
            defs.updateCalendar(calUpdateParams);
        if (s->name() == "t1")
            defs.updateCalendar(calUpdateParams);
        s->set_state(NState::COMPLETE);
    }

    // PrintStyle style(PrintStyle::MIGRATE);
    // std::cout << defs;

    defs.order(nullptr, NOrder::RUNTIME); // moot when you only have one suite
    for (auto suite : defs.suiteVec()) {
        suite->order(nullptr, NOrder::RUNTIME);
        for (auto family : suite->familyVec()) {
            family->order(nullptr, NOrder::RUNTIME);
        }
    }
    // std::cout << defs;

    defs.requeue();
    expectedDefs.requeue();

    BOOST_CHECK_MESSAGE(defs == expectedDefs, "Defs are not the same");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
