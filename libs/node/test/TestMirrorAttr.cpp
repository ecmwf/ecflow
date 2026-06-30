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

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/InLimit.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_MirrorAttr)

BOOST_AUTO_TEST_CASE(mirror_propagate_updates_parent_state) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    //
    // This test does not exercise Mirror attribute itself.
    //
    // Instead it ensures that the `propagate` option on MirrorAttr correctly updates the
    // parent node state when the mirrored task transitions to COMPLETE.
    //
    // Since the mirrored task does not actually run locally,
    // this essentially exercises the following calling the sequence
    // to ensure that limits are not consumed/released.
    //
    //    task->setStateOnly(...)
    //    task->set_most_significant_state_up_node_tree(...)
    //

    std::string definition = R"(
        suite s1
          limit slot 3
          family f1
            task t1
              inlimit slot 1
          endfamily
        endsuite
    )";

    Defs defs;
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_CHECK_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);

    auto s = defs.findAbsNode("/s1");
    auto f = defs.findAbsNode("/s1/f1");
    auto t = defs.findAbsNode("/s1/f1/t1");

    defs.beginAll(); // all nodes → QUEUED

    auto l = s->find_limit("slot");
    BOOST_REQUIRE(l);
    BOOST_CHECK_EQUAL(l->value(), 0); // limit starts "empty"

    // --- Ensure setStateOnly() must NOT call update_limits()
    //
    t->setStateOnly(NState::SUBMITTED, true);
    BOOST_CHECK_EQUAL(l->value(), 0);
    t->setStateOnly(NState::COMPLETE, true);
    BOOST_CHECK_EQUAL(l->value(), 0);

    // --- Ensure set_most_significant_state_up_node_tree() propagates to parent
    //
    BOOST_CHECK_NE(f->state(), NState::COMPLETE);
    t->set_most_significant_state_up_node_tree();
    BOOST_CHECK_EQUAL(f->state(), NState::COMPLETE);

    // --- Ensure set_state() DOES call update_limits()
    //
    // This *negative* test proves WHY `set_state(...)` cannot be used (i.e. incorrectly updates limits),
    // and calling `setStateOnly(...)`+`set_most_significant_state_up_node_tree(...)` actually works
    f->setStateOnly(NState::QUEUED);
    t->setStateOnly(NState::QUEUED, true);
    l->reset();
    BOOST_CHECK_EQUAL(l->value(), 0);

    t->set_state(NState::SUBMITTED);
    BOOST_CHECK_EQUAL(l->value(), 1); // This represents the incorrect behaviour!
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
