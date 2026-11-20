// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/VariableLookup.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

namespace {

///
/// @brief Builds a small definition whose task sees user, repeat, generated and server variables.
///
struct Fixture
{
    Fixture() {
        defs.server_state().add_or_update_user_variables("SERVER_ONLY", "from-server");
        defs.server_state().add_or_update_user_variables("SERVER_EMPTY", "");
        // A server user variable hides the server variable of the same name
        defs.server_state().add_or_update_user_variables("ECF_PORT", "4141");
        auto s = defs.add_suite("suite");
        // A repeat named ECF on the suite generates ECF_JULIAN, which the suite also generates
        s->addRepeat(RepeatDate("ECF", 20200101, 20201231, 1));
        s->addVariable(Variable("ECF_HOME", "/home/ecf"));
        s->addVariable(Variable("AVI", "avi"));
        s->addVariable(Variable("NESTED", "%AVI%-x"));
        s->addVariable(Variable("SELF", "%SELF%"));
        s->addVariable(Variable("EMPTY", ""));
        s->addVariable(Variable("PASSWORD", "secret"));
        s->addVariable(Variable("ECF_NAME", "user-override-is-ignored"));
        auto f = s->add_family("family");
        f->addVariable(Variable("AVI", "avi-in-family"));
        f->addRepeat(RepeatInteger("YMD", 1, 10, 1));
        task = f->add_task("task");
        task->addVariable(Variable("LOCAL", "local"));
        task->addVariable(Variable("AVI", "avi-in-task"));
        task->addRepeat(RepeatInteger("STEP", 0, 5, 1));
        alias = task->add_alias_only();
        alias->addVariable(Variable("A:0", "colon-name"));
        s->update_generated_variables();
        task->update_generated_variables();
        alias->update_generated_variables();
    }

    Defs defs;
    task_ptr task;
    alias_ptr alias;
};

///
/// @brief Substitutes a command through the node, and through a map of the same node's variables.
///
void check_same(const Node& node, const std::string& command, char micro = '%') {
    std::string through_node = command;
    NameValueMap none;
    Node::SubstitutionRecord node_record;
    const bool node_ok = node.variable_substitution(through_node, none, micro, &node_record);

    auto lookup             = MapVariableLookup::from_node(node);
    std::string through_map = command;
    SubstitutionRecord map_record;
    const bool map_ok = variable_substitution(through_map, lookup, none, micro, &map_record);

    BOOST_CHECK_MESSAGE(node_ok == map_ok, "success differs for '" << command << "'");
    BOOST_CHECK_MESSAGE(through_node == through_map,
                        "'" << command << "' gives '" << through_node << "' through the node but '" << through_map
                            << "' through the map");
    BOOST_CHECK_MESSAGE(node_record.masked_command == map_record.masked_command,
                        "masked command differs for '" << command << "'");
    BOOST_CHECK_MESSAGE(node_record.variables == map_record.variables,
                        "recorded variables differ for '" << command << "'");
}

///
/// @brief Checks that both lookups of the map give what the node gives, for the given names.
///
void check_lookups(const Node& node, const std::vector<std::string>& names) {
    auto lookup = MapVariableLookup::from_node(node);
    for (const auto& name : names) {
        std::string from_node;
        std::string from_map;
        const bool found_node = node.findParentVariableValue(name, from_node);
        const bool found_map  = lookup.find(name, from_map);
        BOOST_CHECK_MESSAGE(found_node == found_map && from_node == from_map,
                            "find(" << name << "): node '" << from_node << "' map '" << from_map << "'");

        from_node.clear();
        from_map.clear();
        const bool gen_node = node.find_parent_gen_variable_value(name, from_node);
        const bool gen_map  = lookup.find_generated(name, from_map);
        BOOST_CHECK_MESSAGE(gen_node == gen_map && from_node == from_map,
                            "find_generated(" << name << "): node '" << from_node << "' map '" << from_map << "'");
    }
    BOOST_CHECK(lookup.allows_colon_names() == (node.isAlias() != nullptr));
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_VariableLookup)

BOOST_AUTO_TEST_CASE(test_map_lookup_substitutes_as_the_node_does) {
    ECF_NAME_THIS_TEST();

    Fixture fixture;
    const std::vector<std::string> commands = {
        "plain text",
        "%AVI%",                    // nearest definition wins
        "%LOCAL% %AVI% %ECF_HOME%", // several references
        "%NESTED%",                 // a reference in a value
        "%MISSING%",                // an undefined variable fails
        "%MISSING:default value%",  // a default for an undefined variable
        "%AVI:default%",            // a default for a defined variable
        "%EMPTY%|",                 // an empty value is a value
        "printf %%02d %YMD%",       // a doubled micro character, and a repeat
        "%SELF%",                   // a reference to itself fails
        "%ECF_NAME% %ECF_TRYNO%",   // generated variables take precedence over user ones
        "%ECF_JOB% %ECF_SCRIPT%",   // other generated variables
        "%SERVER_ONLY%",            // a server variable
        "%PASSWORD% and %AVI%",     // a credential is masked in the record
        "%unterminated",            // a single micro character
        "%ECF_HOST% %ECF_PORT%",    // generated variables found on the server only
        "%SUITE% %FAMILY% %TASK%",  // generated variables of the parents
        "%ECF_JULIAN% %ECF_DATE%",  // ECF_JULIAN is also generated by the repeat ECF; ECF_DATE by the suite only
        "%STEP% %YMD%",             // repeats on several levels
        "%SERVER_EMPTY%|",          // a server variable with an empty value
    };
    for (const auto& command : commands) {
        check_same(*fixture.task, command);
    }
}

BOOST_AUTO_TEST_CASE(test_map_lookup_finds_what_the_node_finds) {
    ECF_NAME_THIS_TEST();

    Fixture fixture;
    const std::vector<std::string> names = {"AVI",        "LOCAL",       "STEP",         "YMD",         "ECF",
                                            "ECF_JULIAN", "ECF_DATE",    "ECF_NAME",     "ECF_TRYNO",   "ECF_HOST",
                                            "ECF_PORT",   "SERVER_ONLY", "SERVER_EMPTY", "SUITE",       "FAMILY",
                                            "TASK",       "ECF_JOB",     "ECF_SCRIPT",   "NOT_DEFINED", "A:0"};
    check_lookups(*fixture.task, names);
    check_lookups(*fixture.alias, names);
}

BOOST_AUTO_TEST_CASE(test_map_lookup_substitutes_for_an_alias) {
    ECF_NAME_THIS_TEST();

    Fixture fixture;
    check_same(*fixture.alias, "%A:0% %A:fallback% %LOCAL% %ECF_NAME%");
}

BOOST_AUTO_TEST_CASE(test_map_lookup_substitutes_with_another_micro_character) {
    ECF_NAME_THIS_TEST();

    Fixture fixture;
    check_same(*fixture.task, "&AVI& %AVI% &&", '&');
}

BOOST_AUTO_TEST_CASE(test_map_lookup_from_plain_maps) {
    ECF_NAME_THIS_TEST();

    MapVariableLookup lookup({{"A", "1"}, {"B", "%A%2"}, {"ECF_NAME", "user"}}, {{"ECF_NAME", "/suite/task"}});
    NameValueMap none;

    std::string cmd = "%B% %ECF_NAME%";
    BOOST_CHECK(detail::substitute_variables(cmd, lookup, none, '%', nullptr, false));
    BOOST_CHECK_EQUAL(cmd, "12 /suite/task");

    cmd = "%C%";
    BOOST_CHECK(!detail::substitute_variables(cmd, lookup, none, '%', nullptr, false));
}

BOOST_AUTO_TEST_CASE(test_map_lookup_gives_user_edit_variables_precedence) {
    ECF_NAME_THIS_TEST();

    MapVariableLookup lookup({{"A", "1"}}, {});
    NameValueMap user_edit{{"A", "edited"}};

    std::string cmd = "%A%";
    BOOST_CHECK(detail::substitute_variables(cmd, lookup, user_edit, '%', nullptr, false));
    BOOST_CHECK_EQUAL(cmd, "edited");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
