// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "ecflow/attribute/Permissions.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Memento.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::Permissions;

namespace {

const char* definitions = R"(suite s
  permissions [{"user": "alice", "permissions": "rwxops"}, {"user": "bob", "permissions": "r"}]
  family f
    permissions [{"user":"alice","permissions":"rw"}] # a comment
    task t
      permissions [{"user":"alice","permissions":"r"}]
  endfamily
  task u
endsuite
)";

// A task holding every optional member that precedes the attribute in the serialisation of a node
const char* rich_definitions = R"(suite s
  family f
    task t
      defstatus complete
      late -s +00:15 -a 20:00 -c +02:00
      complete t2 == complete
      trigger t2 == complete
      repeat integer i 1 3
      edit V 1
      inlimit l 1
      label L "x"
      meter M 0 10 5
      event e
      time 10:00
      today 11:00
      date 1.1.2030
      day monday
      cron 23:00
      autocancel 10
      autorestore /s/f
      zombie user:fob::
      verify complete:1
      queue q a b
      generic g x
    task t2
  endfamily
  limit l 2
endsuite
)";

// The serialisation of the task /s/f/t of rich_definitions, which has no Permissions attribute, as written before the
// attribute existed
const char* rich_task_fixture = R"({
"4Task": {
"cereal_class_version": 0,
"value0": {
"cereal_class_version": 0,
"value0": {
"cereal_class_version": 0,
"n_": "t",
"d_st_": {
"st_": 1
},
"vars_": [
{
"n_": "V",
"v_": "1"
}
],
"c_expr_": {
"ptr_wrapper": {
"valid": 1,
"data": {
"vec_": [
{
"exp_": "t2 == complete"
}
]
}
}
},
"t_expr_": {
"ptr_wrapper": {
"valid": 1,
"data": {
"vec_": [
{
"exp_": "t2 == complete"
}
]
}
}
},
"meters_": [
{
"min_": 0,
"max_": 10,
"v_": 0,
"n_": "M",
"cc_": 5
}
],
"events_": [
{
"n_": "e"
}
],
"labels_": [
{
"n_": "L",
"v_": "x"
}
],
"times_": [
{
"ts_": {
"cereal_class_version": 0,
"start_": {
"h_": 10,
"m_": 0
}
}
}
],
"todays_": [
{
"ts_": {
"start_": {
"h_": 11,
"m_": 0
}
}
}
],
"crons_": [
{
"cereal_class_version": 0,
"timeSeries_": {
"start_": {
"h_": 23,
"m_": 0
}
}
}
],
"dates_": [
{
"day_": 1,
"month_": 1,
"year_": 2030
}
],
"days_": [
{
"day_": 1
}
],
"late_": {
"ptr_wrapper": {
"valid": 1,
"data": {
"s_": {
"h_": 0,
"m_": 15
},
"a_": {
"h_": 20,
"m_": 0
},
"c_": {
"h_": 2,
"m_": 0
},
"c_is_rel_": true
}
}
},
"misc_attrs_": {
"ptr_wrapper": {
"valid": 1,
"data": {
"cereal_class_version": 0,
"zombies_": [
{
"cereal_class_version": 0,
"child_cmds_": [],
"zombie_type_": 0,
"action_": 0,
"zombie_lifetime_": 300
}
],
"verifys_": [
{
"state_": 1,
"expected_": 1,
"actual_": 0
}
],
"queues_": [
{
"cereal_class_version": 0,
"theQueue_": [
"a",
"b"
],
"state_vec_": [
2,
2
],
"name_": "q",
"currentIndex_": 0
}
],
"generics_": [
{
"name_": "g",
"values_": [
"x"
]
}
]
}
}
},
"repeat_": {
"cereal_class_version": 0,
"type_": {
"polymorphic_id": 2147483649,
"polymorphic_name": "RepeatInteger",
"ptr_wrapper": {
"valid": 1,
"data": {
"cereal_class_version": 0,
"value0": {
"name_": "i"
},
"start_": 1,
"end_": 3,
"delta_": 1,
"value_": 1
}
}
}
},
"inLimitMgr_": {
"vec_": [
{
"n_": "l"
}
]
},
"auto_cancel_": {
"ptr_wrapper": {
"valid": 1,
"data": {
"cereal_class_version": 0,
"time_": {
"h_": 240,
"m_": 0
},
"days_": true
}
}
},
"auto_restore_": {
"ptr_wrapper": {
"valid": 1,
"data": {
"cereal_class_version": 0,
"nodes_to_restore_": [
"/s/f"
]
}
}
}
}
}
}
})";

// Compares two cereal forms member by member, in order, independently of the white space of the archive (which
// depends on the build configuration)
bool same_members(const std::string& lhs, const std::string& rhs) {
    return nlohmann::ordered_json::parse(lhs) == nlohmann::ordered_json::parse(rhs);
}

defs_ptr parse(const std::string& text) {
    auto defs = Defs::create();
    std::string error;
    std::string warning;
    BOOST_REQUIRE_MESSAGE(defs->restore_from_string(text, error, warning), error);
    return defs;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_PermissionsAttr)

///
/// @brief Verifies that a node carries at most one attribute, and that a node without one reports none.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_add_to_node) {
    ECF_NAME_THIS_TEST();

    Task task("t");
    BOOST_CHECK(task.permissions() == nullptr);

    task.add_permissions(Permissions::parse_cli("alice:r"));
    BOOST_REQUIRE(task.permissions() != nullptr);
    BOOST_CHECK_EQUAL(task.permissions()->to_cli(), "alice:r");

    BOOST_CHECK_THROW(task.add_permissions(Permissions::parse_cli("bob:r")), std::runtime_error);
}

///
/// @brief Verifies that the attribute is parsed on suites, families and tasks, and printed in the JSON form.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_parse_definitions) {
    ECF_NAME_THIS_TEST();

    auto defs = parse(definitions);

    BOOST_REQUIRE(defs->findAbsNode("/s")->permissions() != nullptr);
    BOOST_CHECK_EQUAL(defs->findAbsNode("/s")->permissions()->to_cli(), "alice:rwxops,bob:r");
    BOOST_CHECK_EQUAL(defs->findAbsNode("/s/f")->permissions()->to_cli(), "alice:rw");
    BOOST_CHECK_EQUAL(defs->findAbsNode("/s/f/t")->permissions()->to_cli(), "alice:r");
    BOOST_CHECK(defs->findAbsNode("/s/u")->permissions() == nullptr);

    const auto text = ecf::as_string(*defs, PrintStyle::DEFS);
    BOOST_CHECK_MESSAGE(text.find(R"(permissions [{"permissions":"rw","user":"alice"}])") != std::string::npos, text);
}

///
/// @brief Verifies that the attribute survives a write and parse round trip in every print style.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_round_trip_in_every_style) {
    ECF_NAME_THIS_TEST();

    auto defs = parse(definitions);

    for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
        auto copy = parse(ecf::as_string(*defs, style));
        for (const std::string path : {"/s", "/s/f", "/s/f/t", "/s/u"}) {
            const auto* expected = defs->findAbsNode(path)->permissions();
            const auto* actual   = copy->findAbsNode(path)->permissions();
            BOOST_REQUIRE_EQUAL(expected == nullptr, actual == nullptr);
            if (expected) {
                BOOST_CHECK_MESSAGE(*expected == *actual, path << " in style " << PrintStyle::to_string(style));
            }
        }
    }
}

///
/// @brief Verifies that an alias carries the attribute through the state styles.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_on_alias) {
    ECF_NAME_THIS_TEST();

    auto defs = parse(definitions);
    auto task = defs->findAbsNode("/s/f/t")->isTask();
    BOOST_REQUIRE(task);
    auto alias = task->add_alias_only();
    alias->add_permissions(Permissions::parse_cli("carol:rx"));

    auto copy         = parse(ecf::as_string(*defs, PrintStyle::MIGRATE));
    auto copied_alias = copy->findAbsNode(alias->absNodePath());
    BOOST_REQUIRE(copied_alias);
    BOOST_REQUIRE(copied_alias->permissions() != nullptr);
    BOOST_CHECK_EQUAL(copied_alias->permissions()->to_cli(), "carol:rx");
}

///
/// @brief Verifies that invalid attributes in definitions are rejected, with the offending line.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_parse_rejects_invalid) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::string> invalid{
        "suite s\n  permissions\nendsuite\n",
        "suite s\n  permissions []\nendsuite\n",
        "suite s\n  permissions alice:r\nendsuite\n",
        "suite s\n  permissions [{\"user\":\"alice\",\"permissions\":\"rq\"}]\nendsuite\n",
        "suite s\n  permissions [{\"user\":\"*\",\"permissions\":\"r\"}]\nendsuite\n",
        // a second attribute on the same node
        std::string("suite s\n  permissions [{\"user\":\"alice\",\"permissions\":\"r\"}]\n") +
            "  permissions [{\"user\":\"bob\",\"permissions\":\"r\"}]\nendsuite\n",
    };
    for (const auto& text : invalid) {
        auto defs = Defs::create();
        std::string error;
        std::string warning;
        BOOST_CHECK_MESSAGE(!defs->restore_from_string(text, error, warning), "accepted:\n" << text);
        BOOST_CHECK_MESSAGE(error.find("permissions") != std::string::npos, error);
    }
}

///
/// @brief Verifies that copies of nodes and definitions, and assignment, carry an equal attribute.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_copy_and_assignment) {
    ECF_NAME_THIS_TEST();

    auto defs = parse(definitions);

    Defs copy(*defs);
    BOOST_CHECK(copy == *defs);
    BOOST_REQUIRE(copy.findAbsNode("/s/f")->permissions() != nullptr);
    BOOST_CHECK(*copy.findAbsNode("/s/f")->permissions() == *defs->findAbsNode("/s/f")->permissions());

    const auto& suite = *defs->findSuite("s");
    Suite suite_copy(suite);
    BOOST_REQUIRE(suite_copy.permissions() != nullptr);
    BOOST_CHECK(*suite_copy.permissions() == *suite.permissions());

    Suite assigned("other");
    assigned = suite;
    BOOST_REQUIRE(assigned.permissions() != nullptr);
    BOOST_CHECK(*assigned.permissions() == *suite.permissions());

    Task task("t");
    task.add_permissions(Permissions::parse_cli("alice:r"));
    Task task_assigned("t");
    task_assigned = task;
    BOOST_REQUIRE(task_assigned.permissions() != nullptr);
    Task task_with_attribute("t");
    task_with_attribute.add_permissions(Permissions::parse_cli("bob:r"));
    task_with_attribute = Task("t");
    BOOST_CHECK(task_with_attribute.permissions() == nullptr);
}

///
/// @brief Verifies that definitions that differ only in an attribute compare unequal.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_inequality) {
    ECF_NAME_THIS_TEST();

    auto a = parse(definitions);
    auto b = parse(definitions);
    BOOST_CHECK(*a == *b);

    b->findAbsNode("/s/u")->add_permissions(Permissions::parse_cli("alice:r"));
    BOOST_CHECK(!(*a == *b));

    auto c = parse(definitions);
    auto d = parse(std::string(definitions).replace(std::string(definitions).find("\"r\"}]\n  family"), 5, "\"w\"}]"));
    BOOST_CHECK(!(*c == *d));
}

///
/// @brief Verifies that nodes differing only in the attribute compare unequal, whatever other attributes they hold.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_inequality_with_every_attribute) {
    ECF_NAME_THIS_TEST();

    auto a = parse(rich_definitions);
    auto b = parse(rich_definitions);
    BOOST_REQUIRE(*a == *b);

    const auto& ta = *a->findAbsNode("/s/f/t");
    auto& tb       = *b->findAbsNode("/s/f/t");
    tb.add_permissions(Permissions::parse_cli("alice:r"));
    BOOST_CHECK(!(ta == tb));
    BOOST_CHECK(!(tb == ta));
    BOOST_CHECK(!(*a == *b));
    BOOST_CHECK(!(*b == *a));

    auto c = parse(rich_definitions);
    c->findAbsNode("/s/f/t")->add_permissions(Permissions::parse_cli("alice:w"));
    BOOST_CHECK(!(tb == *c->findAbsNode("/s/f/t")));
    BOOST_CHECK(!(*b == *c));
}

///
/// @brief Verifies that rebuilding the attributes of a node during incremental synchronisation keeps the attribute.
///
/// The attribute has no memento: its changes reach clients through a full synchronisation.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_kept_when_attributes_are_rebuilt) {
    ECF_NAME_THIS_TEST();

    auto defs = parse(definitions);
    defs->findAbsNode("/s/f")->addVariable(Variable("V", "1"));

    CompoundMemento memento("/s/f");
    memento.clear_attributes();
    memento.incremental_sync(defs);

    BOOST_CHECK(defs->findAbsNode("/s/f")->variables().empty()); // the attributes were rebuilt
    BOOST_REQUIRE(defs->findAbsNode("/s/f")->permissions() != nullptr);
    BOOST_CHECK_EQUAL(defs->findAbsNode("/s/f")->permissions()->to_cli(), "alice:rw");
}

///
/// @brief Verifies that a node without the attribute serialises exactly as before the attribute existed, and that
///        the earlier form loads unchanged.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_cereal_unchanged_without_attribute) {
    ECF_NAME_THIS_TEST();

    auto defs  = parse(rich_definitions);
    auto& task = *defs->findAbsNode("/s/f/t")->isTask();
    std::string actual;
    ecf::save_as_string(actual, task);
    BOOST_CHECK(same_members(actual, rich_task_fixture));

    Task restored;
    ecf::restore_from_string(rich_task_fixture, restored);
    BOOST_CHECK(restored.permissions() == nullptr);

    // A node that held an attribute, read from the earlier form, holds none
    Task reused;
    reused.add_permissions(ecf::Permissions::parse_cli("alice:r"));
    ecf::restore_from_string(rich_task_fixture, reused);
    BOOST_CHECK(reused.permissions() == nullptr);
    std::string again;
    ecf::save_as_string(again, restored);
    BOOST_CHECK(same_members(again, rich_task_fixture));
}

///
/// @brief Verifies that the attribute is the last member of the serialised node, after every other optional member.
///
/// Together with test_permissions_attr_cereal_unknown_last_member_is_skipped, this guarantees that a reader
/// unaware of the attribute reads every other member unchanged.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_cereal_is_last_member) {
    ECF_NAME_THIS_TEST();

    auto defs  = parse(rich_definitions);
    auto& task = *defs->findAbsNode("/s/f/t")->isTask();
    task.add_permissions(Permissions::parse_cli("alice:r"));
    std::string data;
    ecf::save_as_string(data, task);

    // Finds the object holding the given member, depth first
    std::function<const nlohmann::ordered_json*(const nlohmann::ordered_json&)> holder =
        [&holder](const nlohmann::ordered_json& j) -> const nlohmann::ordered_json* {
        if (!j.is_object()) {
            return nullptr;
        }
        if (j.contains("permissions_")) {
            return &j;
        }
        for (const auto& item : j.items()) {
            if (const auto* found = holder(item.value()); found) {
                return found;
            }
        }
        return nullptr;
    };

    const auto json  = nlohmann::ordered_json::parse(data);
    const auto* node = holder(json);
    BOOST_REQUIRE_MESSAGE(node != nullptr, data);
    BOOST_CHECK(node->contains("auto_restore_"));
    BOOST_CHECK(node->contains("auto_cancel_"));
    BOOST_CHECK(node->contains("misc_attrs_"));
    BOOST_CHECK(node->contains("repeat_"));
    BOOST_CHECK_EQUAL(std::prev(node->end()).key(), "permissions_");
}

///
/// @brief Verifies the cereal form: absent without the attribute, last member with it, and a lossless round trip.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_cereal) {
    ECF_NAME_THIS_TEST();

    auto defs = parse(definitions);

    std::string without;
    ecf::save_as_string(without, *defs->findAbsNode("/s/u"));
    BOOST_CHECK_MESSAGE(without.find("permissions_") == std::string::npos, without);

    Task task("t");
    task.addVariable(Variable("V", "1"));
    task.add_permissions(Permissions::parse_cli("alice:rs,bob:r"));
    std::string with;
    ecf::save_as_string(with, task);
    BOOST_REQUIRE_MESSAGE(with.find("\"permissions_\"") != std::string::npos, with);

    Task restored;
    ecf::restore_from_string(with, restored);
    BOOST_REQUIRE(restored.permissions() != nullptr);
    BOOST_CHECK(*restored.permissions() == *task.permissions());
    BOOST_CHECK_EQUAL(restored.variables().size(), 1u);
}

namespace {

// Stand-ins for a node as serialised by a version without the attribute (Before) and with it (After)
struct Before
{
    int a{0};
    std::string b;
    template <class Archive>
    void serialize(Archive& ar) {
        CEREAL_OPTIONAL_NVP(ar, a, [this]() { return a != 0; });
        CEREAL_OPTIONAL_NVP(ar, b, [this]() { return !b.empty(); });
    }
};

struct After
{
    int a{0};
    std::string b;
    std::string permissions_;
    template <class Archive>
    void serialize(Archive& ar) {
        CEREAL_OPTIONAL_NVP(ar, a, [this]() { return a != 0; });
        CEREAL_OPTIONAL_NVP(ar, b, [this]() { return !b.empty(); });
        CEREAL_OPTIONAL_NVP(ar, permissions_, [this]() { return !permissions_.empty(); });
    }
};

struct Holder
{
    After node;
    int next{0};
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(node), CEREAL_NVP(next));
    }
};

struct OldHolder
{
    Before node;
    int next{0};
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(node), CEREAL_NVP(next));
    }
};

} // namespace

///
/// @brief Verifies that a reader unaware of a last optional member reads the other members unchanged.
///
/// This is the property the placement of the member, after every other member of Node, relies on.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_cereal_unknown_last_member_is_skipped) {
    ECF_NAME_THIS_TEST();

    Holder holder{After{1, "x", "[...]"}, 7};
    std::string data;
    ecf::save_as_string(data, holder);

    OldHolder old;
    ecf::restore_from_string(data, old);
    BOOST_CHECK_EQUAL(old.node.a, 1);
    BOOST_CHECK_EQUAL(old.node.b, "x");
    BOOST_CHECK_EQUAL(old.next, 7);
}

///
/// @brief Verifies that a cereal form holding an invalid attribute is rejected rather than read as no attribute.
///
BOOST_AUTO_TEST_CASE(test_permissions_attr_cereal_rejects_invalid) {
    ECF_NAME_THIS_TEST();

    Task task("t");
    task.add_permissions(Permissions::parse_cli("alice:r"));
    std::string with;
    ecf::save_as_string(with, task);

    const std::string valid = R"(\"permissions\":\"r\")";
    const auto at           = with.find(valid);
    BOOST_REQUIRE_MESSAGE(at != std::string::npos, with);
    std::string corrupted = with;
    corrupted.replace(at, valid.size(), R"(\"permissions\":\"q\")");

    Task restored;
    BOOST_CHECK_THROW(ecf::restore_from_string(corrupted, restored), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
