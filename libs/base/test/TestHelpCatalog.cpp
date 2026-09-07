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

#include "ecflow/base/HelpCatalog.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests ecf::HelpCatalog's lazy access to the embedded CLI help manifest
///

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_HelpCatalog)

BOOST_AUTO_TEST_CASE(test_manifest_parses_and_has_expected_top_level_shape) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json& manifest = ecf::HelpCatalog::manifest();

    BOOST_REQUIRE(manifest.is_object());
    BOOST_CHECK_EQUAL(manifest.at("schema_version").get<int>(), 1);
    BOOST_CHECK(manifest.at("topics").is_array());
    BOOST_CHECK(manifest.at("commands").is_array());
    BOOST_CHECK(manifest.at("options").is_array());
    BOOST_CHECK(manifest.at("environment_variables").is_array());
    BOOST_CHECK(manifest.at("definitions").is_array());
}

BOOST_AUTO_TEST_CASE(test_manifest_contains_known_entries) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json& manifest = ecf::HelpCatalog::manifest();

    auto has_name = [](const nlohmann::json& array, const std::string& name) {
        for (const auto& entry : array) {
            if (entry.at("name") == name) {
                return true;
            }
        }
        return false;
    };

    BOOST_CHECK(has_name(manifest.at("commands"), "abort"));
    BOOST_CHECK(has_name(manifest.at("options"), "host"));
    BOOST_CHECK(has_name(manifest.at("environment_variables"), "ECF_HOST"));
    BOOST_CHECK(has_name(manifest.at("topics"), "summary"));
    BOOST_CHECK(has_name(manifest.at("topics"), "definition"));
    BOOST_CHECK(has_name(manifest.at("definitions"), "trigger"));
    BOOST_CHECK(has_name(manifest.at("definitions"), "task"));
}

BOOST_AUTO_TEST_CASE(test_manifest_is_parsed_once_and_cached) {
    ECF_NAME_THIS_TEST();

    // Same reference on every call confirms the manifest is parsed lazily,
    // once, and cached -- not re-parsed on every invocation.
    const nlohmann::json& first  = ecf::HelpCatalog::manifest();
    const nlohmann::json& second = ecf::HelpCatalog::manifest();
    BOOST_CHECK_EQUAL(&first, &second);
}

BOOST_AUTO_TEST_CASE(test_find_command_by_exact_name) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json* found = ecf::HelpCatalog::find_command("abort");
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_EQUAL(found->at("implementation").get<std::string>(), "AbortCmd");

    BOOST_CHECK(ecf::HelpCatalog::find_command("no-such-command") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_find_option_by_exact_name) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json* found = ecf::HelpCatalog::find_option("host");
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_EQUAL(found->at("kind").get<std::string>(), "global-option");

    BOOST_CHECK(ecf::HelpCatalog::find_option("no-such-option") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_find_topic_by_exact_name) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json* found = ecf::HelpCatalog::find_topic("summary");
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK(found->contains("summary"));

    BOOST_CHECK(ecf::HelpCatalog::find_topic("no-such-topic") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_summary_for_command_and_option) {
    ECF_NAME_THIS_TEST();

    std::optional<std::string> command_summary = ecf::HelpCatalog::summary_for("abort");
    BOOST_REQUIRE(command_summary.has_value());
    BOOST_CHECK_EQUAL(*command_summary, ecf::HelpCatalog::find_command("abort")->at("summary").get<std::string>());

    std::optional<std::string> option_summary = ecf::HelpCatalog::summary_for("host");
    BOOST_REQUIRE(option_summary.has_value());
    BOOST_CHECK_EQUAL(*option_summary, ecf::HelpCatalog::find_option("host")->at("summary").get<std::string>());

    BOOST_CHECK(!ecf::HelpCatalog::summary_for("no-such-name").has_value());
}

BOOST_AUTO_TEST_CASE(test_description_for_command_and_option) {
    ECF_NAME_THIS_TEST();

    std::optional<std::string> command_description = ecf::HelpCatalog::description_for("abort");
    BOOST_REQUIRE(command_description.has_value());
    BOOST_CHECK(!command_description->empty());

    std::optional<std::string> option_description = ecf::HelpCatalog::description_for("host");
    BOOST_REQUIRE(option_description.has_value());
    BOOST_CHECK(!option_description->empty());

    BOOST_CHECK(!ecf::HelpCatalog::description_for("no-such-name").has_value());
}

BOOST_AUTO_TEST_CASE(test_find_definition_item_by_exact_name) {
    ECF_NAME_THIS_TEST();

    const nlohmann::json* found = ecf::HelpCatalog::find_definition_item("trigger");
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_EQUAL(found->at("kind").get<std::string>(), "attribute");

    const nlohmann::json* node = ecf::HelpCatalog::find_definition_item("task");
    BOOST_REQUIRE(node != nullptr);
    BOOST_CHECK_EQUAL(node->at("kind").get<std::string>(), "node");

    BOOST_CHECK(ecf::HelpCatalog::find_definition_item("no-such-definition-item") == nullptr);
}

BOOST_AUTO_TEST_CASE(test_summary_and_description_for_definition_item) {
    ECF_NAME_THIS_TEST();

    std::optional<std::string> attribute_summary = ecf::HelpCatalog::summary_for_definition_item("trigger");
    BOOST_REQUIRE(attribute_summary.has_value());
    BOOST_CHECK_EQUAL(*attribute_summary,
                      ecf::HelpCatalog::find_definition_item("trigger")->at("summary").get<std::string>());

    std::optional<std::string> node_summary = ecf::HelpCatalog::summary_for_definition_item("task");
    BOOST_REQUIRE(node_summary.has_value());
    BOOST_CHECK_EQUAL(*node_summary, ecf::HelpCatalog::find_definition_item("task")->at("summary").get<std::string>());

    std::optional<std::string> attribute_description = ecf::HelpCatalog::description_for_definition_item("trigger");
    BOOST_REQUIRE(attribute_description.has_value());
    BOOST_CHECK(!attribute_description->empty());

    BOOST_CHECK(!ecf::HelpCatalog::summary_for_definition_item("no-such-definition-item").has_value());
    BOOST_CHECK(!ecf::HelpCatalog::description_for_definition_item("no-such-definition-item").has_value());
}

BOOST_AUTO_TEST_CASE(test_definition_item_lookup_never_resolves_a_colliding_command) {
    ECF_NAME_THIS_TEST();

    // "event"/"label"/"meter"/"queue"/"complete" are both task commands and definition-item
    // attribute names; the two lookup families must stay independent so that a name collision
    // never lets one resolve the other's entry.
    for (const std::string& name : {"event", "label", "meter", "queue", "complete"}) {
        const nlohmann::json* command = ecf::HelpCatalog::find_command(name);
        BOOST_REQUIRE_MESSAGE(command != nullptr, "expected '" << name << "' to still be a command");
        BOOST_CHECK_EQUAL(command->at("kind").get<std::string>(), "task");

        const nlohmann::json* definition_item = ecf::HelpCatalog::find_definition_item(name);
        BOOST_REQUIRE_MESSAGE(definition_item != nullptr, "expected '" << name << "' to also be a definition item");
        BOOST_CHECK_EQUAL(definition_item->at("kind").get<std::string>(), "attribute");

        // summary_for()/description_for() (command/option lookup) must not see the definition item,
        // and must return the same text as before this collision-safe lookup family existed.
        BOOST_CHECK_EQUAL(*ecf::HelpCatalog::summary_for(name), command->at("summary").get<std::string>());
    }

    // "task" is both a topic ("list task commands") and a definition-item node type.
    BOOST_REQUIRE(ecf::HelpCatalog::find_topic("task") != nullptr);
    BOOST_REQUIRE(ecf::HelpCatalog::find_definition_item("task") != nullptr);
}

BOOST_AUTO_TEST_CASE(test_description_for_joins_lines_with_newline) {
    ECF_NAME_THIS_TEST();

    // The description holds one line per array element; description_for() joins them with a single
    // newline, so the reconstruction equals the elements joined by '\n', verbatim.
    const nlohmann::json* entry = ecf::HelpCatalog::find_command("abort");
    BOOST_REQUIRE(entry != nullptr);

    std::string expected;
    bool first = true;
    for (const auto& line : entry->at("description")) {
        if (!first) {
            expected += "\n";
        }
        first = false;
        expected += line.get<std::string>();
    }

    std::optional<std::string> actual = ecf::HelpCatalog::description_for("abort");
    BOOST_REQUIRE(actual.has_value());
    BOOST_CHECK_EQUAL(*actual, expected);

    // A blank line between paragraphs is stored as an empty element and reappears as "\n\n".
    BOOST_CHECK(actual->find("\n\n") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
