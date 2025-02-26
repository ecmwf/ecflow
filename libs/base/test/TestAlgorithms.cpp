/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#define BOOST_TEST_MODULE Test_Base
#include <boost/test/included/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/base/Algorithms.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/server/BaseServer.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_Path)

BOOST_AUTO_TEST_CASE(test_cannot_create_path_from_empty_string) {
    ECF_NAME_THIS_TEST();

    auto result = ecf::Path::make("");
    BOOST_CHECK_MESSAGE(!result.ok(), "expected !ok");
}

BOOST_AUTO_TEST_CASE(test_can_create_path_from_root_only) {
    ECF_NAME_THIS_TEST();

    auto result = ecf::Path::make("/");
    BOOST_CHECK(result.ok());
    auto path = result.value();
    BOOST_CHECK(path.empty());
    BOOST_CHECK_EQUAL(path.size(), 0ul);
    BOOST_CHECK_EQUAL(path.to_string(), "/");
}

BOOST_AUTO_TEST_CASE(test_can_create_path_with_single_token) {
    ECF_NAME_THIS_TEST();

    auto result = ecf::Path::make("/suite");
    BOOST_CHECK(result.ok());
    auto path = result.value();
    BOOST_CHECK(!path.empty());
    BOOST_CHECK_EQUAL(path.size(), 1ul);
    BOOST_CHECK_EQUAL(path[0], "suite");
    BOOST_CHECK_EQUAL(path.to_string(), "/suite");
}

BOOST_AUTO_TEST_CASE(test_can_create_path_with_multiple_tokens) {
    ECF_NAME_THIS_TEST();

    auto result = ecf::Path::make("/suite/family/task");
    BOOST_CHECK(result.ok());
    auto path = result.value();
    BOOST_CHECK(!path.empty());
    BOOST_CHECK_EQUAL(path.size(), 3ul);
    BOOST_CHECK_EQUAL(path[0], "suite");
    BOOST_CHECK_EQUAL(path[1], "family");
    BOOST_CHECK_EQUAL(path[2], "task");
    BOOST_CHECK_EQUAL(path.to_string(), "/suite/family/task");
}

BOOST_AUTO_TEST_CASE(test_can_create_path_with_empty_tokens) {
    ECF_NAME_THIS_TEST();

    auto result = ecf::Path::make("///suite///family///task");
    BOOST_CHECK(result.ok());
    auto path = result.value();
    BOOST_CHECK(!path.empty());
    BOOST_CHECK_EQUAL(path.size(), 3ul);
    BOOST_CHECK_EQUAL(path[0], "suite");
    BOOST_CHECK_EQUAL(path[1], "family");
    BOOST_CHECK_EQUAL(path[2], "task");
    BOOST_CHECK_EQUAL(path.to_string(), "/suite/family/task");
}

BOOST_AUTO_TEST_SUITE_END() // T_Path

BOOST_AUTO_TEST_SUITE(T_Algorithms)

BOOST_AUTO_TEST_CASE(test_can_visit_defs) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s  = defs.add_suite("suite");
    family_ptr f = s->add_family("family");
    task_ptr t   = f->add_task("task");

    struct Visitor
    {
        void operator()(const Defs& defs) { collected.push_back("defs"); }
        void operator()(const Node& s) { collected.push_back("node: " + s.name()); }

        std::vector<std::string> collected;
    };

    auto path = ecf::Path::make("/suite/family/task").value();
    Visitor visitor;
    ecf::visit(defs, path, visitor);

    BOOST_CHECK_EQUAL(visitor.collected.size(), 4ul);
    BOOST_CHECK_EQUAL(visitor.collected[0], "defs");
    BOOST_CHECK_EQUAL(visitor.collected[1], "node: suite");
    BOOST_CHECK_EQUAL(visitor.collected[2], "node: family");
    BOOST_CHECK_EQUAL(visitor.collected[3], "node: task");
}

BOOST_AUTO_TEST_SUITE_END() // T_Algorithms

BOOST_AUTO_TEST_SUITE_END()
