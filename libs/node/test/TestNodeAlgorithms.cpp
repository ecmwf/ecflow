/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

void populate(Defs& defs) {

    // Create a Suite Definition corresponding to
    //
    // suite s
    //   family f
    //     task t1
    //       trigger 0 != 0
    //       complete 0 == 0
    //     task t2
    //       trigger 1 == 1
    //       alias a
    //   endfamily
    // endsuite

    auto s  = defs.add_suite("s");
    auto f  = s->add_family("f");
    auto t1 = f->add_task("t1");
    t1->add_trigger("/s/f/t2 == complete");
    t1->add_complete("0 == 0");
    auto t2 = f->add_task("t2");
    t2->add_trigger("1 == 1");
    auto a = t2->add_alias("a");
}

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_NodeAlgorithms)

BOOST_AUTO_TEST_CASE(can_get_all_nodes_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_nodes(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{5});
    }

    {
        const Defs& cdefs = defs;
        auto found        = ecf::get_all_nodes(cdefs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{5});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_nodes_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{5});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{4});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_families_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_families(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        const Defs& cdefs = defs;
        auto found        = ecf::get_all_families(cdefs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_families_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_families(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_families(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_families(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_families(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_families(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_tasks_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_tasks(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        const Defs& cdefs = defs;
        auto found        = ecf::get_all_tasks(cdefs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_tasks_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_tasks(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node         = defs.findAbsNode("/s");
        const Node& cnode = *node;

        auto found = ecf::get_all_tasks(cnode);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_tasks(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_tasks(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_tasks(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_tasks(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_aliases_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_aliases(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_aliases_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_aliases(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node         = defs.findAbsNode("/s");
        const Node& cnode = *node;

        auto found = ecf::get_all_aliases(cnode);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_aliases(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_aliases(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_aliases(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_aliases(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_nodes_referenced_in_ast_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_ast_nodes(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        const Defs& cdefs = defs;
        auto found        = ecf::get_all_ast_nodes(cdefs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_nodes_referenced_in_ast_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_ast_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node         = defs.findAbsNode("/s");
        const Node& cnode = *node;

        auto found = ecf::get_all_ast_nodes(cnode);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_ast_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_ast_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_ast_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_ast_nodes(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{0});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_active_submittable_nodes_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        // Set task t1 to active
        auto t1 = defs.findAbsNode("/s/f/t1");
        t1->set_state(NState::ACTIVE);

        // Set alias a to submitted
        auto a = defs.findAbsNode("/s/f/t2/a");
        a->set_state(NState::SUBMITTED);
    }

    {
        auto found = ecf::get_all_active_submittables(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_active_submittable_nodes_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        // Set task t1 to active
        auto t1 = defs.findAbsNode("/s/f/t1");
        t1->set_state(NState::ACTIVE);

        // Set alias a to submitted
        auto a = defs.findAbsNode("/s/f/t2/a");
        a->set_state(NState::SUBMITTED);
    }

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_active_submittables(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_active_submittables(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_active_submittables(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_active_submittables(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_active_submittables(*node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_nodes_ptr_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_nodes_ptr(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{5});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_nodes_ptr_from_a_node) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto node = defs.findAbsNode("/s");

        auto found = ecf::get_all_nodes_ptr(node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{5});
    }

    {
        auto node = defs.findAbsNode("/s/f");

        auto found = ecf::get_all_nodes_ptr(node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{4});
    }

    {
        auto node = defs.findAbsNode("/s/f/t1");

        auto found = ecf::get_all_nodes_ptr(node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2");

        auto found = ecf::get_all_nodes_ptr(node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }

    {
        auto node = defs.findAbsNode("/s/f/t2/a");

        auto found = ecf::get_all_nodes_ptr(node);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{1});
    }
}

BOOST_AUTO_TEST_CASE(can_get_all_tasks_ptr_from_a_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    populate(defs);

    {
        auto found = ecf::get_all_tasks_ptr(defs);
        BOOST_CHECK_EQUAL(found.size(), std::size_t{2});
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
