// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <string>
#include <variant>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/CTree.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::CTree;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgCTree)

BOOST_AUTO_TEST_CASE(test_tree_holds_path_and_node) {
    ECF_NAME_THIS_TEST();

    const CTree tree("/variables/ECF_PORT", std::int64_t{3141});
    BOOST_CHECK_EQUAL(tree.path(), "/variables/ECF_PORT");
    BOOST_REQUIRE(std::holds_alternative<std::int64_t>(tree.node()));
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(tree.node()), 3141);

    const CTree root("", CTree::Null{});
    BOOST_CHECK_EQUAL(root.path(), "");
    BOOST_CHECK(std::holds_alternative<CTree::Null>(root.node()));
}

BOOST_AUTO_TEST_CASE(test_find_in_mapping) {
    ECF_NAME_THIS_TEST();

    const CTree tree("",
                     CTree::Mapping{
                         {"b", CTree("/b", true)},
                         {"a", CTree("/a", std::string{"text"})},
                     });

    const auto* b = tree.find("b");
    BOOST_REQUIRE(b);
    BOOST_CHECK_EQUAL(b->path(), "/b");
    BOOST_CHECK(std::get<bool>(b->node()));

    const auto* a = tree.find("a");
    BOOST_REQUIRE(a);
    BOOST_CHECK_EQUAL(std::get<std::string>(a->node()), "text");

    BOOST_CHECK(tree.find("c") == nullptr);
    BOOST_CHECK(tree.find("") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_find_outside_mapping) {
    ECF_NAME_THIS_TEST();

    // Only a mapping has keys
    const CTree sequence("", CTree::Sequence{CTree("/0", true)});
    BOOST_CHECK(sequence.find("0") == nullptr);

    const CTree text("", std::string{"a"});
    BOOST_CHECK(text.find("a") == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
