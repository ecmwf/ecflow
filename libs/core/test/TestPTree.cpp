/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/PTree.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace {

std::string test_data_file(const std::string& name) {
    std::string path = ecf::File::test_data("libs/core/test/data/PTree", "libs/core");
    return path + "/" + name;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_PTree)

BOOST_AUTO_TEST_CASE(ctor_default_constructor_produces_null_empty_node) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    BOOST_CHECK(t.empty());
    BOOST_CHECK(!t.is_leaf());
    BOOST_CHECK(!t.is_object());
    BOOST_CHECK(!t.is_array());
}

BOOST_AUTO_TEST_CASE(ctor_string_constructor_produces_leaf_node) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t("hello");
    BOOST_CHECK(t.is_leaf());
    BOOST_CHECK(t.empty());
    BOOST_CHECK(!t.is_object());
    BOOST_CHECK(!t.is_array());
    BOOST_CHECK(t.get_value<std::string>() == "hello");
}

BOOST_AUTO_TEST_CASE(ctor_int_constructor_produces_leaf_node) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t(42);
    BOOST_CHECK(t.is_leaf());
    BOOST_CHECK(t.empty());
    BOOST_CHECK(t.get_value<int>() == 42);
    // Also accessible as string
    BOOST_CHECK(t.get_value<std::string>() == "42");
}

BOOST_AUTO_TEST_CASE(ctor_cstring_constructor_produces_leaf_node) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t("literal");
    BOOST_CHECK(t.get_value<std::string>() == "literal");
}

BOOST_AUTO_TEST_CASE(ctor_copy_constructor_does_not_share_content) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree a;
    a.put("x", std::string("10"));
    PTree b = a; // copy
    BOOST_CHECK(b.get<std::string>("x") == "10");

    // Mutating b does not affect a
    b.put("x", std::string("99"));
    BOOST_CHECK(a.get<std::string>("x") == "10");
    BOOST_CHECK(b.get<std::string>("x") == "99");
}

BOOST_AUTO_TEST_CASE(ctor_move_constructor_transfers_content) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree a;
    a.put("k", std::string("v"));
    PTree b = std::move(a);
    BOOST_CHECK(b.get<std::string>("k") == "v");
}

BOOST_AUTO_TEST_CASE(clear_resets_to_null) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("key", std::string("value"));
    BOOST_CHECK(!t.empty());
    t.clear();
    BOOST_CHECK(t.empty());
    BOOST_CHECK(!t.contains("key"));
}

BOOST_AUTO_TEST_CASE(object_node_is_not_a_leaf_and_not_empty_when_it_has_children) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("a", std::string("1"));
    BOOST_CHECK(!t.empty());
    BOOST_CHECK(t.is_object());
    BOOST_CHECK(!t.is_leaf());
}

BOOST_AUTO_TEST_CASE(put_and_get_string_round_trip) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("name", std::string("Alice"));
    BOOST_CHECK(t.get<std::string>("name") == "Alice");
}

BOOST_AUTO_TEST_CASE(put_and_get_int_round_trip) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("count", 7);
    BOOST_CHECK(t.get<int>("count") == 7);
}

BOOST_AUTO_TEST_CASE(put_cstring_and_get_string) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("key", "value");
    BOOST_CHECK(t.get<std::string>("key") == "value");
}

BOOST_AUTO_TEST_CASE(put_overwrites_existing_value) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("x", std::string("first"));
    t.put("x", std::string("second"));
    BOOST_CHECK(t.get<std::string>("x") == "second");
}

BOOST_AUTO_TEST_CASE(put_and_get_with_2_level_dotted_path) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("server.host", std::string("localhost"));
    t.put("server.port", 3141);
    BOOST_CHECK(t.get<std::string>("server.host") == "localhost");
    BOOST_CHECK(t.get<int>("server.port") == 3141);
}

BOOST_AUTO_TEST_CASE(put_and_get_with_4_level_dotted_path) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("server.notification.aborted.enabled", std::string("true"));
    BOOST_CHECK(t.get<std::string>("server.notification.aborted.enabled") == "true");
}

BOOST_AUTO_TEST_CASE(intermediate_nodes_are_created_automatically) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("a.b.c.d", std::string("deep"));
    BOOST_CHECK(t.contains("a"));
    BOOST_CHECK(t.contains("a.b"));
    BOOST_CHECK(t.contains("a.b.c"));
    BOOST_CHECK(t.contains("a.b.c.d"));
}

BOOST_AUTO_TEST_CASE(get_with_default_returns_default_when_path_is_absent) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    BOOST_CHECK(t.get<std::string>("missing", "default") == "default");
    BOOST_CHECK(t.get<int>("missing", -1) == -1);
    BOOST_CHECK(t.get("missing", "fallback") == "fallback");
    BOOST_CHECK(t.get("missing", std::string("fallback")) == "fallback");
}

BOOST_AUTO_TEST_CASE(get_with_default_returns_default_when_intermediate_path_is_absent) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("a.b", std::string("x"));
    // "a.c" never set
    BOOST_CHECK(t.get<std::string>("a.c", "nope") == "nope");
}

BOOST_AUTO_TEST_CASE(get_without_default_throws_when_path_is_absent) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    BOOST_CHECK_THROW(t.get<std::string>("no.such.path"), ecf::PTreePathError);
}

BOOST_AUTO_TEST_CASE(get_without_default_throws_via_JsonPathError_catchable_as_runtime_error) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    bool caught = false;
    try {
        t.get<std::string>("missing");
    }
    catch (const std::runtime_error&) {
        caught = true;
    }
    BOOST_CHECK(caught);
}

BOOST_AUTO_TEST_CASE(get_works_with_single_segment_path_without_dot) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("key", std::string("val"));
    BOOST_CHECK(t.get<std::string>("key", "") == "val");
}

BOOST_AUTO_TEST_CASE(get_int_coerces_string_encoded_integers) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("x", std::string("100")); // stored as JSON string
    BOOST_CHECK(t.get<int>("x", 0) == 100);
}

BOOST_AUTO_TEST_CASE(get_string_coerces_int_typed_json_values) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("n", 42);
    BOOST_CHECK(t.get<std::string>("n", "") == "42");
}

BOOST_AUTO_TEST_CASE(value_string_on_int_node_returns_string_representation) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree leaf(99);
    BOOST_CHECK(leaf.get_value<std::string>() == "99");
}

BOOST_AUTO_TEST_CASE(value_int_on_string_with_int_node_works) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree leaf(std::string("42"));
    BOOST_CHECK(leaf.get_value<int>() == 42);
}

BOOST_AUTO_TEST_CASE(get_with_default_does_not_throw_on_type_mismatch_returns_default) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("x", std::string("not_a_number"));
    // Conversion string→int fails; should return default, not throw
    BOOST_CHECK(t.get<int>("x", -99) == -99);
}

BOOST_AUTO_TEST_CASE(contains_returns_true_for_existing_paths) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("a.b.c", std::string("v"));
    BOOST_CHECK(t.contains("a"));
    BOOST_CHECK(t.contains("a.b"));
    BOOST_CHECK(t.contains("a.b.c"));
}

BOOST_AUTO_TEST_CASE(contains_returns_false_for_absent_paths) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("a.b", std::string("v"));
    BOOST_CHECK(!t.contains("a.c"));
    BOOST_CHECK(!t.contains("x"));
    BOOST_CHECK(!t.contains("a.b.c.d"));
}

BOOST_AUTO_TEST_CASE(get_child_optional_returns_value_for_existing_subtree) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("server.host", std::string("h1"));
    t.put("server.port", 1234);

    auto opt = t.get_child_optional("server");
    BOOST_REQUIRE(opt.has_value());
    BOOST_CHECK(opt->get<std::string>("host") == "h1");
    BOOST_CHECK(opt->get<int>("port") == 1234);
}

BOOST_AUTO_TEST_CASE(get_child_optional_returns_nullopt_for_absent_path) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    auto opt = t.get_child_optional("nonexistent");
    BOOST_CHECK(!opt.has_value());
}

BOOST_AUTO_TEST_CASE(get_child_optional_can_iterate_the_returned_subtree) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("section.a", std::string("1"));
    t.put("section.b", std::string("2"));

    auto opt = t.get_child_optional("section");
    BOOST_REQUIRE(opt.has_value());

    std::vector<std::string> keys;
    for (const auto& [k, v] : *opt) {
        keys.push_back(k);
    }

    BOOST_CHECK(keys.size() == 2);
    BOOST_CHECK(keys[0] == "a");
    BOOST_CHECK(keys[1] == "b");
}

BOOST_AUTO_TEST_CASE(array_node_is_not_empty_after_push_back) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree arr;
    arr.push_back_array_element(PTree("x"));
    BOOST_CHECK(!arr.empty());
    BOOST_CHECK(arr.is_array());
    BOOST_CHECK(!arr.is_leaf());
}

BOOST_AUTO_TEST_CASE(push_back_array_element_converts_null_node_to_array) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree arr;
    arr.push_back_array_element(PTree("a"));
    BOOST_CHECK(arr.is_array());
}

BOOST_AUTO_TEST_CASE(push_back_array_element_appends_elements_in_order) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree arr;
    arr.push_back_array_element(PTree("first"));
    arr.push_back_array_element(PTree("second"));
    arr.push_back_array_element(PTree("third"));

    std::vector<std::string> result;
    for (const auto& [k, v] : arr) {
        BOOST_CHECK(k.empty()); // anonymous key
        result.push_back(v.get_value<std::string>());
    }
    BOOST_REQUIRE(result.size() == 3);
    BOOST_CHECK(result[0] == "first");
    BOOST_CHECK(result[1] == "second");
    BOOST_CHECK(result[2] == "third");
}

BOOST_AUTO_TEST_CASE(push_back_array_element_with_int_leaves) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree arr;
    for (int i : {10, 20, 30}) {
        arr.push_back_array_element(PTree(i));
    }

    std::vector<int> result;
    for (const auto& [k, v] : arr) {
        result.push_back(v.get_value<int>());
    }

    BOOST_REQUIRE(result.size() == 3);
    BOOST_CHECK(result[0] == 10);
    BOOST_CHECK(result[1] == 20);
    BOOST_CHECK(result[2] == 30);
}

BOOST_AUTO_TEST_CASE(vsettings_style_build_vector_string_as_array_and_retrieve_it) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    std::vector<std::string> original = {"host1:3141", "host2:3141", "host3:3141"};

    // Build
    PTree root;
    PTree arr;
    for (const auto& s : original) {
        arr.push_back_array_element(PTree(s));
    }
    root.put_child("favourites", arr);

    // Retrieve
    auto opt = root.get_child_optional("favourites");
    BOOST_REQUIRE(opt.has_value());
    std::vector<std::string> retrieved;
    for (const auto& [k, v] : *opt) {
        retrieved.push_back(v.get_value<std::string>());
    }

    BOOST_CHECK(retrieved == original);
}

BOOST_AUTO_TEST_CASE(vsettings_style_build_vector_int_as_array_and_retrieve_it) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    std::vector<int> original = {1, 2, 3, 5, 8, 13};

    PTree root;
    PTree arr;
    for (int n : original) {
        arr.push_back_array_element(PTree(n));
    }
    root.put_child("counts", arr);

    auto opt = root.get_child_optional("counts");
    BOOST_REQUIRE(opt.has_value());
    std::vector<int> retrieved;
    for (const auto& [k, v] : *opt) {
        retrieved.push_back(v.get_value<int>());
    }

    BOOST_CHECK(retrieved == original);
}

BOOST_AUTO_TEST_CASE(vsettings_style_build_array_of_object_subtrees) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    // Simulate: for each tab, build a sub-JsonTree and push it into an array.
    struct TabInfo
    {
        std::string id, name;
    };
    std::vector<TabInfo> tabs = {{"0", "Ops"}, {"1", "Research"}, {"2", "Backup"}};

    PTree root;
    PTree arr;
    for (const auto& tab : tabs) {
        PTree sub;
        sub.put("id", tab.id);
        sub.put("name", tab.name);
        arr.push_back_array_element(sub);
    }
    root.put_child("tabs", arr);

    // Retrieve
    auto opt = root.get_child_optional("tabs");
    BOOST_REQUIRE(opt.has_value());

    std::vector<TabInfo> retrieved;
    for (const auto& [k, v] : *opt) {
        BOOST_CHECK(k.empty());
        retrieved.push_back({v.get<std::string>("id"), v.get<std::string>("name")});
    }

    BOOST_REQUIRE(retrieved.size() == 3);
    BOOST_CHECK(retrieved[0].id == "0");
    BOOST_CHECK(retrieved[0].name == "Ops");
    BOOST_CHECK(retrieved[1].id == "1");
    BOOST_CHECK(retrieved[1].name == "Research");
    BOOST_CHECK(retrieved[2].id == "2");
    BOOST_CHECK(retrieved[2].name == "Backup");
}

BOOST_AUTO_TEST_CASE(empty_array_node_reports_empty_eq_true) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree arr;
    arr.push_back_array_element(PTree("x"));
    arr.push_back_array_element(PTree("y"));
    BOOST_CHECK(!arr.empty());

    PTree empty_arr;
    BOOST_CHECK(empty_arr.empty());
}

BOOST_AUTO_TEST_CASE(put_child_with_empty_array_node_round_trips_correctly) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    PTree arr; // null, empty
    arr.push_back_array_element(PTree("a"));
    arr.push_back_array_element(PTree("b"));
    root.put_child("list", arr);

    auto list_opt = root.get_child_optional("list");
    BOOST_REQUIRE(list_opt.has_value());
    int count = 0;
    for ([[maybe_unused]] const auto& item : *list_opt) {
        ++count;
    }
    BOOST_CHECK(count == 2);
}

BOOST_AUTO_TEST_CASE(iterate_array_of_string_encoded_ints_array_coercion) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree arr;
    arr.push_back_array_element(PTree(std::string("1")));
    arr.push_back_array_element(PTree(std::string("2")));
    arr.push_back_array_element(PTree(std::string("3")));

    PTree root;
    root.put_child("ids", arr);

    std::vector<int> result;
    auto ids_opt = root.get_child_optional("ids");
    BOOST_REQUIRE(ids_opt.has_value());
    for (const auto& [k, v] : *ids_opt) {
        result.push_back(v.get_value<int>());
    }

    BOOST_CHECK(result == std::vector({1, 2, 3}));
}

BOOST_AUTO_TEST_CASE(push_back_array_element_throws_if_node_already_has_named_children) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put("key", std::string("val")); // now t is an object
    BOOST_CHECK_THROW(t.push_back_array_element(PTree("oops")), ecf::PTreeInvalidStateError);
}

BOOST_AUTO_TEST_CASE(object_iteration_yields_key_value_pairs) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    t.put("alpha", std::string("a"));
    t.put("beta", std::string("b"));
    t.put("gamma", std::string("c"));

    std::vector<std::string> keys, values;
    for (const auto& [k, v] : t) {
        keys.push_back(k);
        values.push_back(v.get_value<std::string>());
    }

    BOOST_REQUIRE(keys.size() == 3);
    BOOST_CHECK(keys[0] == "alpha");
    BOOST_CHECK(values[0] == "a");
    BOOST_CHECK(keys[1] == "beta");
    BOOST_CHECK(values[1] == "b");
    BOOST_CHECK(keys[2] == "gamma");
    BOOST_CHECK(values[2] == "c");
}

BOOST_AUTO_TEST_CASE(insertion_order_is_preserved_for_object_nodes) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    std::vector<std::string> inserted = {"zebra", "apple", "mango", "kiwi", "banana"};
    for (const auto& k : inserted) {
        t.put(k, std::string("v"));
    }

    std::vector<std::string> observed;
    for (const auto& [k, v] : t) {
        observed.push_back(k);
    }

    BOOST_CHECK(observed == inserted);
}

BOOST_AUTO_TEST_CASE(nested_object_iteration_VConfig_MenuHandler_pattern) {
    ECF_NAME_THIS_TEST();

    ecf::PTree root;
    root.put("a.x", std::string("1"));
    root.put("a.y", std::string("2"));
    root.put("b.x", std::string("3"));

    std::vector<std::pair<std::string, std::string>> leaves;
    for (const auto& [outer_key, outer_val] : root) {
        for (const auto& [inner_key, inner_val] : outer_val) {
            leaves.emplace_back(outer_key + "." + inner_key, inner_val.get_value<std::string>());
        }
    }

    BOOST_REQUIRE(leaves.size() == 3);
    BOOST_CHECK(leaves[0].first == "a.x");
    BOOST_CHECK(leaves[0].second == "1");
    BOOST_CHECK(leaves[1].first == "a.y");
    BOOST_CHECK(leaves[1].second == "2");
    BOOST_CHECK(leaves[2].first == "b.x");
    BOOST_CHECK(leaves[2].second == "3");
}

BOOST_AUTO_TEST_CASE(two_level_iteration_Palette_pattern_outer_key_inner_key_leaf_value) {
    ECF_NAME_THIS_TEST();

    ecf::PTree palette;
    palette.put("active.Window", std::string("#f0f0f0"));
    palette.put("active.Highlight", std::string("#0078d4"));
    palette.put("inactive.Window", std::string("#d8d8d8"));
    palette.put("inactive.Highlight", std::string("#4a9fcf"));

    std::vector<std::tuple<std::string, std::string, std::string>> entries;
    for (const auto& [group, group_node] : palette) {
        for (const auto& [role, role_node] : group_node) {
            entries.emplace_back(group, role, role_node.get_value<std::string>());
        }
    }

    BOOST_REQUIRE(entries.size() == 4);
    BOOST_CHECK(std::get<0>(entries[0]) == "active");
    BOOST_CHECK(std::get<1>(entries[0]) == "Window");
    BOOST_CHECK(std::get<2>(entries[0]) == "#f0f0f0");
    BOOST_CHECK(std::get<0>(entries[2]) == "inactive");
    BOOST_CHECK(std::get<1>(entries[2]) == "Window");
}

BOOST_AUTO_TEST_CASE(iterating_a_null_node_yields_nothing) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    int count = 0;
    for ([[maybe_unused]] const auto& item : t) {
        ++count;
    }
    BOOST_CHECK(count == 0);
    BOOST_CHECK(t.begin() == t.end());
}

BOOST_AUTO_TEST_CASE(iterating_an_empty_object_yields_nothing) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree t;
    t.put_child("sub", PTree());
    auto opt = t.get_child_optional("sub");
    BOOST_REQUIRE(opt.has_value());
    int count = 0;
    for ([[maybe_unused]] const auto& item : *opt) {
        ++count;
    }
    BOOST_CHECK(count == 0);
}

BOOST_AUTO_TEST_CASE(find_returns_iterator_to_child_when_key_exists) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    t.put("rules.color", std::string("red"));
    t.put("rules.bold", std::string("true"));

    auto it = t.find("rules");
    BOOST_CHECK(it != t.end());
    BOOST_CHECK(it->first == "rules");
    // The second is a subtree — iterate it
    std::vector<std::string> child_keys;
    for (const auto& [k, v] : it->second) {
        child_keys.push_back(k);
    }
    BOOST_CHECK(child_keys == std::vector<std::string>({"color", "bold"}));
}

BOOST_AUTO_TEST_CASE(find_returns_end_when_key_is_absent) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    t.put("a", std::string("1"));
    auto it = t.find("nonexistent");
    BOOST_CHECK(it == t.end());
}

BOOST_AUTO_TEST_CASE(highlighter_style_pattern_find_nested_find_get_value) {
    ECF_NAME_THIS_TEST();

    ecf::PTree root;
    root.put("ecfscript.0.pattern", std::string("^%[A-Z]+%"));
    root.put("ecfscript.0.color", std::string("#0000cc"));
    root.put("ecfscript.1.pattern", std::string("#.*$"));
    root.put("ecfscript.1.color", std::string("#808080"));

    auto itTop = root.find("ecfscript");
    BOOST_REQUIRE(itTop != root.end());

    std::vector<std::string> patterns;
    for (const auto& [rule_key, rule_node] : itTop->second) {
        auto itPar = rule_node.find("pattern");
        if (itPar != rule_node.end()) {
            patterns.push_back(itPar->second.get_value<std::string>());
        }
    }
    BOOST_REQUIRE(patterns.size() == 2);
    BOOST_CHECK(patterns[0] == "^%[A-Z]+%");
    BOOST_CHECK(patterns[1] == "#.*$");
}

BOOST_AUTO_TEST_CASE(put_child_replaces_existing_subtree) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    root.put("sub.a", std::string("old"));

    PTree replacement;
    replacement.put("b", std::string("new"));
    root.put_child("sub", replacement);

    BOOST_CHECK(!root.contains("sub.a"));
    BOOST_CHECK(root.get<std::string>("sub.b") == "new");
}

BOOST_AUTO_TEST_CASE(add_child_appends_overwrites_a_direct_child) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    root.put("x", std::string("1"));

    PTree extra;
    extra.put("y", std::string("2"));
    root.add_child("extra", extra);

    BOOST_CHECK(root.get<std::string>("x") == "1");
    BOOST_CHECK(root.get<std::string>("extra.y") == "2");
}

BOOST_AUTO_TEST_CASE(vconfig_savesettings_pattern_splice_subtree_children_into_parent) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree server_settings;
    server_settings.put("host", std::string("prod.ecflow.int"));
    server_settings.put("port", 3141);
    server_settings.put("timeout", 30);

    PTree global;
    global.put("version", std::string("7.0"));
    for (const auto& [k, v] : server_settings) {
        global.add_child(k, v);
    }

    BOOST_CHECK(global.get<std::string>("version") == "7.0");
    BOOST_CHECK(global.get<std::string>("host") == "prod.ecflow.int");
    BOOST_CHECK(global.get<int>("port") == 3141);
    BOOST_CHECK(global.get<int>("timeout") == 30);
}

BOOST_AUTO_TEST_CASE(post_increment_iterator_works_correctly) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    t.put("a", std::string("1"));
    t.put("b", std::string("2"));

    auto it    = t.begin();
    auto first = it++; // post-increment: returns copy of old position
    BOOST_CHECK(first->first == "a");
    BOOST_CHECK(it->first == "b");
}

BOOST_AUTO_TEST_CASE(iterator_with_view_has_correct_first_and_second) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    root.put("alpha", std::string("A"));
    root.put("beta", std::string("B"));
    root.put("gamma", std::string("C"));

    auto it = root.begin();
    BOOST_CHECK(it->first == "alpha");
    BOOST_CHECK(it->second.get_value<std::string>() == "A");
    ++it;
    BOOST_CHECK(it->first == "beta");
    BOOST_CHECK(it->second.get_value<std::string>() == "B");
    ++it;
    BOOST_CHECK(it->first == "gamma");
    BOOST_CHECK(it->second.get_value<std::string>() == "C");
    ++it;
    BOOST_CHECK(it == root.end());
}

BOOST_AUTO_TEST_CASE(iterator_dereference_returns_stable_reference_not_copy) {
    ECF_NAME_THIS_TEST();

    // Verification:
    //   (a) Correct key and value are accessible on a large subtree.
    //   (b) operator* on the same position returns the same address on every call
    //       (stable reference into children_, not a new allocation each time).
    //   (c) operator-> and operator* give consistent addresses.

    using ecf::PTree;

    // Build a child with many items so any copy would be clearly non-trivial.
    PTree large_child;
    for (int i = 0; i < 50; ++i) {
        large_child.put("key" + std::to_string(i), std::to_string(i));
    }

    PTree root;
    root.add_child("child", large_child);

    auto it = root.begin();

    // (a) Values are correct
    BOOST_CHECK(it->first == "child");
    BOOST_CHECK(it->second.contains("key0"));
    BOOST_CHECK(it->second.get<std::string>("key0") == "0");
    BOOST_CHECK(it->second.get<std::string>("key49") == "49");

    // (b) Same address on repeated dereference at the same position:
    //     a new deep copy would produce a different heap address each time.
    const PTree* addr1 = &(*it).second;
    const PTree* addr2 = &(*it).second;
    BOOST_CHECK_EQUAL(addr1, addr2);

    // (c) operator-> and operator* are consistent
    BOOST_CHECK_EQUAL(&it->second, &(*it).second);
}

BOOST_AUTO_TEST_CASE(iterator_cache_is_reset_and_reflects_new_position_after_increment) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    root.put("x", std::string("1"));
    root.put("y", std::string("2"));

    auto it          = root.begin();
    const PTree* p_x = &it->second; // address of first child
    ++it;
    const PTree* p_y = &it->second; // address of second child

    // Different positions must give different addresses (different children).
    BOOST_CHECK_NE(p_x, p_y);
    BOOST_CHECK(it->first == "y");
    BOOST_CHECK(it->second.get_value<std::string>() == "2");
}

BOOST_AUTO_TEST_CASE(iterator_structured_bindings_work_over_large_nested_tree) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    for (int group = 0; group < 5; ++group) {
        PTree sub;
        for (int item = 0; item < 20; ++item) {
            sub.put("item" + std::to_string(item), std::string("val") + std::to_string(group * 20 + item));
        }
        root.add_child("group" + std::to_string(group), sub);
    }

    int groups_seen = 0;
    int total_items = 0;
    for (const auto& [gk, gv] : root) {
        ++groups_seen;
        BOOST_CHECK(gk.substr(0, 5) == "group");
        for (const auto& [ik, iv] : gv) {
            ++total_items;
            BOOST_CHECK(!iv.get_value<std::string>().empty());
        }
    }

    BOOST_CHECK_EQUAL(groups_seen, 5);
    BOOST_CHECK_EQUAL(total_items, 100);
}

BOOST_AUTO_TEST_CASE(iterator_post_increment_copy_holds_correct_view_of_old_position) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;
    root.add_child("first_key", PTree(std::string("first_val")));
    root.add_child("second_key", PTree(std::string("second_val")));

    auto it   = root.begin();
    auto prev = it++; // prev = copy before advance, it = second child

    BOOST_CHECK(prev->first == "first_key");
    BOOST_CHECK(prev->second.get_value<std::string>() == "first_val");
    BOOST_CHECK(it->first == "second_key");
    BOOST_CHECK(it->second.get_value<std::string>() == "second_val");
}

BOOST_AUTO_TEST_CASE(vsettings_put_int_get_int_round_trip_at_nested_path) {
    ECF_NAME_THIS_TEST();

    ecf::PTree pt;
    pt.put("geom.x", 100);
    pt.put("geom.y", 200);
    BOOST_CHECK(pt.get<int>("geom.x", 0) == 100);
    BOOST_CHECK(pt.get<int>("geom.y", 0) == 200);
}

BOOST_AUTO_TEST_CASE(vsettings_booleans_stored_as_true_or_false_strings) {
    ECF_NAME_THIS_TEST();

    ecf::PTree pt;
    pt.put("notification.aborted.enabled", std::string("true"));
    pt.put("notification.submitted.enabled", std::string("false"));

    BOOST_CHECK(pt.get<std::string>("notification.aborted.enabled", "false") == "true");
    BOOST_CHECK(pt.get<std::string>("notification.submitted.enabled", "true") == "false");
}

BOOST_AUTO_TEST_CASE(vsettings_string_get_with_cstring_default) {
    ECF_NAME_THIS_TEST();

    ecf::PTree pt;
    pt.put("theme", std::string("dark"));
    BOOST_CHECK(pt.get("theme", "light") == "dark");
    BOOST_CHECK(pt.get("missing", "light") == "light");
}

BOOST_AUTO_TEST_CASE(vsettings_get_child_optional_iterate_array) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    std::vector<std::string> expected = {"a.def", "b.def", "c.def"};
    PTree pt;
    PTree arr;
    for (const auto& s : expected) {
        arr.push_back_array_element(PTree(s));
    }
    pt.put_child("recentFiles", arr);

    auto opt = pt.get_child_optional("recentFiles");
    BOOST_REQUIRE(opt.has_value());

    std::vector<std::string> result;
    for (const auto& [k, v] : *opt) {
        result.push_back(v.get_value<std::string>());
    }
    BOOST_CHECK(result == expected);
}

BOOST_AUTO_TEST_CASE(vsettings_get_child_optional_is_nullopt_for_absent_key) {
    ECF_NAME_THIS_TEST();

    ecf::PTree pt;
    auto opt = pt.get_child_optional("nonexistent");
    BOOST_CHECK(!opt.has_value());
}

BOOST_AUTO_TEST_CASE(vsettings_array_of_subtrees_with_vector_vsettings_pattern) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    struct Session
    {
        std::string id, server;
        int width;
    };
    std::vector<Session> sessions = {{"0", "oper", 1280}, {"1", "rd", 800}, {"2", "backup", 640}};

    PTree pt;
    PTree arr;
    for (const auto& s : sessions) {
        PTree sub;
        sub.put("id", s.id);
        sub.put("server", s.server);
        sub.put("width", s.width);
        arr.push_back_array_element(sub);
    }
    pt.put_child("sessions", arr);

    // Retrieve and verify
    auto opt = pt.get_child_optional("sessions");
    BOOST_REQUIRE(opt.has_value());

    std::vector<Session> retrieved;
    for (const auto& [k, v] : *opt) {
        retrieved.push_back({v.get<std::string>("id"), v.get<std::string>("server"), v.get<int>("width")});
    }

    BOOST_REQUIRE(retrieved.size() == 3);
    BOOST_CHECK(retrieved[0].id == "0");
    BOOST_CHECK(retrieved[0].server == "oper");
    BOOST_CHECK(retrieved[0].width == 1280);
    BOOST_CHECK(retrieved[1].id == "1");
    BOOST_CHECK(retrieved[1].server == "rd");
    BOOST_CHECK(retrieved[1].width == 800);
    BOOST_CHECK(retrieved[2].id == "2");
    BOOST_CHECK(retrieved[2].server == "backup");
    BOOST_CHECK(retrieved[2].width == 640);
}

BOOST_AUTO_TEST_CASE(vsettings_clear_empties_the_whole_tree) {
    ECF_NAME_THIS_TEST();

    ecf::PTree pt;
    pt.put("a.b.c", std::string("deep"));
    pt.put("x", 99);
    BOOST_CHECK(!pt.empty());

    pt.clear();
    BOOST_CHECK(pt.empty());
    BOOST_CHECK(!pt.contains("a"));
    BOOST_CHECK(!pt.contains("x"));
}

BOOST_AUTO_TEST_CASE(vconfig_leaf_vs_subtree_empty_distinguishes_them) {
    ECF_NAME_THIS_TEST();

    ecf::PTree root;
    root.put("gui.default", std::string("10"));  // leaf
    root.put("gui.section.x", std::string("y")); // section has children

    // Leaf node (string)
    auto opt_leaf = root.get_child_optional("gui.default");
    BOOST_REQUIRE(opt_leaf.has_value());
    BOOST_CHECK(opt_leaf->empty()); // leaf → empty = true (no children)
    BOOST_CHECK(opt_leaf->is_leaf());

    // Sub-section (object with children)
    auto opt_sec = root.get_child_optional("gui.section");
    BOOST_REQUIRE(opt_sec.has_value());
    BOOST_CHECK(!opt_sec->empty()); // has children → empty = false
    BOOST_CHECK(!opt_sec->is_leaf());
}

BOOST_AUTO_TEST_CASE(vconfig_loadimportedsettings_pattern) {
    ECF_NAME_THIS_TEST();

    ecf::PTree imported;
    imported.put("server.host", std::string("prod.ecflow.int"));
    imported.put("server.timeout", std::string("60"));

    if (auto opt = imported.get_child_optional("server")) {
        BOOST_CHECK(opt->get<std::string>("host", "localhost") == "prod.ecflow.int");
        BOOST_CHECK(opt->get<std::string>("timeout", "30") == "60");
        BOOST_CHECK(opt->get<std::string>("missing", "none") == "none");
    }
    else {
        BOOST_FAIL("expected server subtree to exist");
    }
}

BOOST_AUTO_TEST_CASE(vconfig_savesettings_splice_top_level_children_via_add_child) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree server_settings;
    server_settings.put("host", std::string("oper"));
    server_settings.put("port", 3141);
    server_settings.put("timeout", 30);

    // "global" config tree starts with some data
    PTree global;
    global.put("version", std::string("7.0.0"));

    // Splice
    for (const auto& [k, v] : server_settings) {
        global.add_child(k, v);
    }

    BOOST_CHECK(global.get<std::string>("version") == "7.0.0");
    BOOST_CHECK(global.get<std::string>("host") == "oper");
    BOOST_CHECK(global.get<int>("port") == 3141);
    BOOST_CHECK(global.get<int>("timeout") == 30);
}

BOOST_AUTO_TEST_CASE(vconfig_readrcfile_pattern_build_tree_from_key_value_pairs_suite_list_array) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree pt;
    pt.put("server.name", std::string("oper_server"));
    pt.put("server.host", std::string("ecflow-oper.ecmwf.int"));
    pt.put("server.port", std::string("3141")); // rc files use strings

    std::vector<std::string> suite_names = {"oper", "rd", "wave"};
    PTree suites;
    for (const auto& s : suite_names) {
        suites.push_back_array_element(PTree(s));
    }
    pt.put_child("suites", suites);

    BOOST_CHECK(pt.get<std::string>("server.name") == "oper_server");
    BOOST_CHECK(pt.get<std::string>("server.host") == "ecflow-oper.ecmwf.int");
    BOOST_CHECK(pt.get<int>("server.port", 0) == 3141);

    auto opt = pt.get_child_optional("suites");
    BOOST_REQUIRE(opt.has_value());
    std::vector<std::string> result;
    for (const auto& [k, v] : *opt) {
        result.push_back(v.get_value<std::string>());
    }
    BOOST_CHECK(result == suite_names);
}

BOOST_AUTO_TEST_CASE(menuhandler_get_with_string_literal_default_on_each_field) {
    ECF_NAME_THIS_TEST();

    ecf::PTree item;
    item.put("command", std::string("execute"));
    item.put("handler", std::string("NodeHandler"));
    item.put("icon", std::string("run.png"));

    BOOST_CHECK(item.get("command", "NoCommand") == "execute");
    BOOST_CHECK(item.get("handler", "") == "NodeHandler");
    BOOST_CHECK(item.get("icon", "") == "run.png");
    BOOST_CHECK(item.get("tooltip", "") == "");           // uses default
    BOOST_CHECK(item.get("enabled", "false") == "false"); // uses default
}

BOOST_AUTO_TEST_CASE(menuhandler_top_level_plus_nested_iteration_order_preserved) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    PTree root;

    // menus array
    PTree menus;
    for (const auto& name : {"Node", "Server", "NodeContext"}) {
        PTree m;
        m.put("name", std::string(name));
        menus.push_back_array_element(m);
    }
    root.put_child("menus", menus);

    // menu_items array
    PTree items;
    for (const auto& cmd : {"execute", "suspend", "requeue"}) {
        PTree item;
        item.put("command", std::string(cmd));
        item.put("handler", std::string("NodeHandler"));
        items.push_back_array_element(item);
    }
    root.put_child("menu_items", items);

    // Verify order
    std::vector<std::string> menu_names, item_cmds;
    auto menus_opt = root.get_child_optional("menus");
    auto items_opt = root.get_child_optional("menu_items");
    BOOST_REQUIRE(menus_opt.has_value());
    BOOST_REQUIRE(items_opt.has_value());
    for (const auto& [k, v] : *menus_opt) {
        menu_names.push_back(v.get("name", ""));
    }
    for (const auto& [k, v] : *items_opt) {
        item_cmds.push_back(v.get("command", ""));
    }

    BOOST_CHECK(menu_names == std::vector<std::string>({"Node", "Server", "NodeContext"}));
    BOOST_CHECK(item_cmds == std::vector<std::string>({"execute", "suspend", "requeue"}));
}

BOOST_AUTO_TEST_CASE(read_json_parses_simple_json) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    BOOST_REQUIRE_NO_THROW(read_json(test_data_file("simple.json"), t));

    BOOST_CHECK(t.get<std::string>("name") == "Alice");
    BOOST_CHECK(t.get<int>("age") == 30);
    BOOST_CHECK(t.get<std::string>("address.city") == "London");
    BOOST_CHECK(t.get<std::string>("address.zip") == "EC1A 1BB");
    BOOST_CHECK(t.contains("empty_obj"));
}

BOOST_AUTO_TEST_CASE(read_json_parses_settings_json_with_string_encoded_scalars) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    BOOST_REQUIRE_NO_THROW(read_json(test_data_file("settings.json"), t));

    // Top-level scalars stored as strings (boost convention)
    BOOST_CHECK(t.get<std::string>("tabCount") == "3");
    BOOST_CHECK(t.get<std::string>("currentTabId") == "0");
    BOOST_CHECK(t.get<std::string>("theme") == "dark");

    // Coerce string-encoded int
    BOOST_CHECK(t.get<int>("tabCount", 0) == 3);

    // Nested path
    BOOST_CHECK(t.get<std::string>("server.notification.aborted.enabled") == "true");
    BOOST_CHECK(t.get<std::string>("server.notification.aborted.maxItems") == "100");
    BOOST_CHECK(t.get<std::string>("geom.x") == "100");
}

BOOST_AUTO_TEST_CASE(read_json_parses_settings_json_with_favourites_array_in_order) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    read_json(test_data_file("settings.json"), t);

    auto opt = t.get_child_optional("favourites");
    BOOST_REQUIRE(opt.has_value());

    std::vector<std::string> hosts;
    for (const auto& [k, v] : *opt) {
        BOOST_CHECK(k.empty()); // anonymous key == array element
        hosts.push_back(v.get_value<std::string>());
    }
    BOOST_REQUIRE(hosts.size() == 3);
    BOOST_CHECK(hosts[0] == "host1:3141");
    BOOST_CHECK(hosts[1] == "host2:3141");
    BOOST_CHECK(hosts[2] == "host3:3141");
}

BOOST_AUTO_TEST_CASE(read_json_parses_settings_json_tabs_array_of_objects) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    read_json(test_data_file("settings.json"), t);

    auto opt = t.get_child_optional("tabs");
    BOOST_REQUIRE(opt.has_value());

    std::vector<std::string> ids, names;
    for (const auto& [k, v] : *opt) {
        BOOST_CHECK(k.empty());
        ids.push_back(v.get<std::string>("id"));
        names.push_back(v.get<std::string>("name"));
    }
    BOOST_REQUIRE(ids.size() == 3);
    BOOST_CHECK(ids[0] == "0");
    BOOST_CHECK(names[0] == "Operations");
    BOOST_CHECK(ids[1] == "1");
    BOOST_CHECK(names[1] == "Research");
    BOOST_CHECK(ids[2] == "2");
    BOOST_CHECK(names[2] == "Backup");
}

BOOST_AUTO_TEST_CASE(read_json_parses_menu_json_following_menuhandler_infopanelhandler_pattern) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    BOOST_REQUIRE_NO_THROW(read_json(test_data_file("menu.json"), t));

    // Top-level keys: menus and menu_items
    auto menus_opt = t.get_child_optional("menus");
    BOOST_REQUIRE(menus_opt.has_value());
    int menu_count = 0;
    for ([[maybe_unused]] const auto& m : *menus_opt) {
        ++menu_count;
    }
    BOOST_CHECK(menu_count == 3);

    auto items_opt = t.get_child_optional("menu_items");
    BOOST_REQUIRE(items_opt.has_value());

    // First item
    auto it = items_opt->begin();
    BOOST_REQUIRE(it != items_opt->end());
    BOOST_CHECK(it->second.get("command", std::string("NoCommand")) == "execute");
    BOOST_CHECK(it->second.get("handler", std::string("")) == "NodeHandler");
    BOOST_CHECK(it->second.get("enabled", std::string("false")) == "true");
    BOOST_CHECK(it->second.get("missing_key", std::string("def")) == "def"); // default fallback
}

BOOST_AUTO_TEST_CASE(read_json_parses_palette_json_with_2_level_iteration) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    BOOST_REQUIRE_NO_THROW(read_json(test_data_file("palette.json"), t));

    std::vector<std::string> groups;
    for (const auto& [group, roles] : t) {
        groups.push_back(group);
        for (const auto& [role, colour] : roles) {
            BOOST_CHECK(!colour.get_value<std::string>().empty());
        }
    }
    BOOST_REQUIRE(groups.size() == 3);
    BOOST_CHECK(groups[0] == "active");
    BOOST_CHECK(groups[1] == "inactive");
    BOOST_CHECK(groups[2] == "disabled");
}

BOOST_AUTO_TEST_CASE(read_json_parses_highlighter_json_with_find_and_iterate_rules) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    BOOST_REQUIRE_NO_THROW(read_json(test_data_file("highlighter.json"), t));

    // Highlighter.cpp pattern: pt.find(id) / pt.not_found()
    auto itTop = t.find("ecfscript");
    BOOST_REQUIRE(itTop != t.end());

    std::vector<std::string> patterns;
    for (const auto& [rule_key, rule_node] : itTop->second) {
        auto itPat = rule_node.find("pattern");
        if (itPat != rule_node.end()) {
            patterns.push_back(itPat->second.get_value<std::string>());
        }
    }
    BOOST_CHECK(patterns.size() == 2);

    // Unknown group exists but is empty
    auto itUnknown = t.find("unknown_group");
    BOOST_REQUIRE(itUnknown != t.end());
    int count = 0;
    for ([[maybe_unused]] const auto& item : itUnknown->second) {
        ++count;
    }
    BOOST_CHECK(count == 0);
}

BOOST_AUTO_TEST_CASE(write_json_produces_a_readable_file_with_read_json_round_trip) {
    ECF_NAME_THIS_TEST();

    using ecf::PTree;

    // Build a tree in memory
    PTree original;
    original.put("version", std::string("2.0"));
    original.put("server.host", std::string("oper.ecflow"));
    original.put("server.port", 3141);

    PTree favs;
    favs.push_back_array_element(PTree("host1:3141"));
    favs.push_back_array_element(PTree("host2:3141"));
    original.put_child("favourites", favs);

    // Write to a temp file
    auto tmp = fs::temp_directory_path() / "pprop_roundtrip_test.json";
    BOOST_REQUIRE_NO_THROW(write_json(tmp.string(), original));
    BOOST_CHECK(fs::exists(tmp));

    // Re-read
    PTree restored;
    BOOST_REQUIRE_NO_THROW(read_json(tmp.string(), restored));

    BOOST_CHECK(restored.get<std::string>("version") == "2.0");
    BOOST_CHECK(restored.get<std::string>("server.host") == "oper.ecflow");
    BOOST_CHECK(restored.get<int>("server.port") == 3141);

    auto opt = restored.get_child_optional("favourites");
    BOOST_REQUIRE(opt.has_value());
    std::vector<std::string> hosts;
    for (const auto& [k, v] : *opt) {
        hosts.push_back(v.get_value<std::string>());
    }
    BOOST_CHECK(hosts == std::vector<std::string>({"host1:3141", "host2:3141"}));

    fs::remove(tmp);
}

BOOST_AUTO_TEST_CASE(write_json_produces_pretty_printed_output) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    t.put("a", std::string("1"));
    t.put("b", std::string("2"));

    auto tmp = fs::temp_directory_path() / "pprop_pretty_test.json";
    write_json(tmp.string(), t);

    std::ifstream fs_in(tmp);
    BOOST_REQUIRE(fs_in.is_open());
    std::string content((std::istreambuf_iterator<char>(fs_in)), std::istreambuf_iterator<char>());

    // Should contain newlines (pretty-printed, not compact)
    BOOST_CHECK(content.find('\n') != std::string::npos);

    fs::remove(tmp);
}

BOOST_AUTO_TEST_CASE(full_settings_json_round_trip_preserves_array_order) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    read_json(test_data_file("settings.json"), t);

    auto tmp = fs::temp_directory_path() / "pprop_settings_roundtrip.json";
    write_json(tmp.string(), t);

    ecf::PTree t2;
    read_json(tmp.string(), t2);

    // Array insertion order must be preserved
    auto opt1 = t.get_child_optional("favourites");
    auto opt2 = t2.get_child_optional("favourites");
    BOOST_REQUIRE(opt1.has_value());
    BOOST_REQUIRE(opt2.has_value());

    std::vector<std::string> h1, h2;
    for (const auto& [k, v] : *opt1) {
        h1.push_back(v.get_value<std::string>());
    }
    for (const auto& [k, v] : *opt2) {
        h2.push_back(v.get_value<std::string>());
    }
    BOOST_CHECK(h1 == h2);

    // Object key order must be preserved
    std::vector<std::string> k1, k2;
    for (const auto& [k, v] : t) {
        k1.push_back(k);
    }
    for (const auto& [k, v] : t2) {
        k2.push_back(k);
    }
    BOOST_CHECK(k1 == k2);

    fs::remove(tmp);
}

BOOST_AUTO_TEST_CASE(read_json_throws_JsonParseError_for_nonexistent_file) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    BOOST_CHECK_THROW(read_json("/no/such/file.json", t), ecf::PTreeParseError);
}

BOOST_AUTO_TEST_CASE(read_json_throws_JsonParseError_for_malformed_json) {
    ECF_NAME_THIS_TEST();

    auto tmp = fs::temp_directory_path() / "pprop_bad.json";
    {
        std::ofstream out(tmp);
        out << "{ this is not : valid json ,,, }";
    }
    ecf::PTree t;
    BOOST_CHECK_THROW(read_json(tmp.string(), t), ecf::PTreeParseError);
    fs::remove(tmp);
}

BOOST_AUTO_TEST_CASE(JsonParseError_is_a_std_runtime_error) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    bool caught = false;
    try {
        read_json("/no/such/file.json", t);
    }
    catch (const std::runtime_error&) {
        caught = true;
    }
    BOOST_CHECK(caught);
}

BOOST_AUTO_TEST_CASE(write_json_throws_JsonParseError_for_unwritable_path) {
    ECF_NAME_THIS_TEST();

    ecf::PTree t;
    t.put("x", std::string("1"));
    BOOST_CHECK_THROW(write_json("/no/such/directory/file.json", t), ecf::PTreeParseError);
}

BOOST_AUTO_TEST_CASE(get_or_create_child_on_array_via_put_string_throws) {
    ECF_NAME_THIS_TEST();

    ecf::PTree arr;
    arr.push_back_array_element(ecf::PTree(std::string("alpha")));
    arr.push_back_array_element(ecf::PTree(std::string("beta")));

    BOOST_CHECK_THROW(arr.put("key", std::string("x")), ecf::PTreeInvalidStateError);
}

BOOST_AUTO_TEST_CASE(get_or_create_child_on_array_via_put_int_throws) {
    ECF_NAME_THIS_TEST();

    ecf::PTree arr;
    arr.push_back_array_element(ecf::PTree(std::string("alpha")));

    BOOST_CHECK_THROW(arr.put("count", 42), ecf::PTreeInvalidStateError);
}

BOOST_AUTO_TEST_CASE(get_or_create_child_on_array_via_put_child_throws) {
    ECF_NAME_THIS_TEST();

    ecf::PTree arr;
    arr.push_back_array_element(ecf::PTree(std::string("alpha")));

    ecf::PTree child;
    child.put("x", std::string("1"));
    BOOST_CHECK_THROW(arr.put_child("sub", std::move(child)), ecf::PTreeInvalidStateError);
}

BOOST_AUTO_TEST_CASE(get_or_create_child_error_message_contains_key_name) {
    ECF_NAME_THIS_TEST();

    ecf::PTree arr;
    arr.push_back_array_element(ecf::PTree(std::string("v")));

    try {
        arr.put("offending_key", std::string("x"));
        BOOST_FAIL("expected PTreeInvalidStateError");
    }
    catch (const ecf::PTreeInvalidStateError& e) {
        BOOST_CHECK(std::string(e.what()).find("offending_key") != std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(get_or_create_child_array_is_not_corrupted_after_throw) {
    ECF_NAME_THIS_TEST();

    ecf::PTree arr;
    arr.push_back_array_element(ecf::PTree(std::string("first")));
    arr.push_back_array_element(ecf::PTree(std::string("second")));

    try {
        arr.put("key", std::string("x"));
    }
    catch (const ecf::PTreeInvalidStateError&) { /* expected */
    }

    // Must still be an array
    BOOST_REQUIRE(arr.is_array());

    // Must still have exactly 2 anonymous elements with the original values
    std::vector<std::string> elems;
    for (const auto& [k, v] : arr) {
        BOOST_CHECK(k.empty()); // array elements have empty names
        elems.push_back(v.get_value<std::string>());
    }
    BOOST_REQUIRE_EQUAL(elems.size(), 2u);
    BOOST_CHECK_EQUAL(elems[0], "first");
    BOOST_CHECK_EQUAL(elems[1], "second");
}

BOOST_AUTO_TEST_CASE(get_or_create_child_intermediate_array_in_dotted_path_throws) {
    ECF_NAME_THIS_TEST();

    // Build: root → tabs (array of two strings)
    ecf::PTree tabs;
    tabs.push_back_array_element(ecf::PTree(std::string("tab1")));
    tabs.push_back_array_element(ecf::PTree(std::string("tab2")));
    ecf::PTree root;
    root.put_child("tabs", std::move(tabs));

    // Navigating INTO the array via a dotted path must throw
    BOOST_CHECK_THROW(root.put("tabs.count", 2), ecf::PTreeInvalidStateError);
    BOOST_CHECK_THROW(root.put("tabs.name", std::string("list")), ecf::PTreeInvalidStateError);

    // The tabs array must remain intact
    auto t = root.get_child_optional("tabs");
    BOOST_REQUIRE(t.has_value());
    BOOST_REQUIRE(t->is_array());
    int count = 0;
    for ([[maybe_unused]] const auto& child : *t) {
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 2);
}

BOOST_AUTO_TEST_CASE(put_child_replacing_an_array_node_does_not_throw) {
    ECF_NAME_THIS_TEST();

    ecf::PTree old_arr;
    old_arr.push_back_array_element(ecf::PTree(std::string("old")));
    ecf::PTree root;
    root.put_child("list", std::move(old_arr));
    BOOST_REQUIRE(root.get_child_optional("list")->is_array());

    // Replace the array with a plain object subtree
    ecf::PTree replacement;
    replacement.put("name", std::string("replaced"));
    BOOST_REQUIRE_NO_THROW(root.put_child("list", std::move(replacement)));

    BOOST_CHECK_EQUAL(root.get<std::string>("list.name", ""), "replaced");
    BOOST_CHECK(!root.get_child_optional("list")->is_array());
}

BOOST_AUTO_TEST_CASE(PTreeInvalidStateError_from_array_guard_is_catchable_as_std_runtime_error) {
    ECF_NAME_THIS_TEST();

    ecf::PTree arr;
    arr.push_back_array_element(ecf::PTree(std::string("v")));

    bool caught = false;
    try {
        arr.put("key", std::string("x"));
    }
    catch (const std::runtime_error&) {
        caught = true;
    }
    BOOST_CHECK(caught);
}

BOOST_AUTO_TEST_CASE(get_value_bool_recognises_true_and_1_as_true) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::PTree(std::string("true")).get_value<bool>() == true);
    BOOST_CHECK(ecf::PTree(std::string("1")).get_value<bool>() == true);
}

BOOST_AUTO_TEST_CASE(get_value_bool_recognises_false_and_0_as_false) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::PTree(std::string("false")).get_value<bool>() == false);
    BOOST_CHECK(ecf::PTree(std::string("0")).get_value<bool>() == false);
}

BOOST_AUTO_TEST_CASE(get_value_bool_throws_PTreeValueError_for_unrecognised_string) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_THROW(ecf::PTree(std::string("yes")).get_value<bool>(), ecf::PTreeValueError);
    BOOST_CHECK_THROW(ecf::PTree(std::string("no")).get_value<bool>(), ecf::PTreeValueError);
    BOOST_CHECK_THROW(ecf::PTree(std::string("True")).get_value<bool>(), ecf::PTreeValueError);
    BOOST_CHECK_THROW(ecf::PTree(std::string("")).get_value<bool>(), ecf::PTreeValueError);
}

BOOST_AUTO_TEST_CASE(get_value_bool_throws_PTreeValueError_for_int_node) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_THROW(ecf::PTree(1).get_value<bool>(), ecf::PTreeValueError);
    BOOST_CHECK_THROW(ecf::PTree(0).get_value<bool>(), ecf::PTreeValueError);
}

BOOST_AUTO_TEST_CASE(get_value_bool_throws_PTreeValueError_for_null_node) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_THROW(ecf::PTree{}.get_value<bool>(), ecf::PTreeValueError);
}

BOOST_AUTO_TEST_CASE(read_json_number_large_unsigned_branch_is_exercised_without_crash) {
    ECF_NAME_THIS_TEST();

    auto tmp = fs::temp_directory_path() / "ptree_n6_unsigned.json";
    {
        std::ofstream out(tmp);
        // UINT64_MAX = 18446744073709551615  (> INT64_MAX, triggers number_unsigned)
        // INT64_MAX+1 = 9223372036854775808  (also triggers number_unsigned)
        out << R"({ "uint64max": 18446744073709551615, "int64_plus1": 9223372036854775808 })";
    }

    ecf::PTree t;
    BOOST_REQUIRE_NO_THROW(read_json(tmp.string(), t));

    // Both keys must be present
    // These values are valid JSON numbers, but too big for int64_t, so will return invalid/different values
    BOOST_CHECK(t.contains("uint64max"));
    BOOST_CHECK(t.contains("int64_plus1"));

    fs::remove(tmp);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
