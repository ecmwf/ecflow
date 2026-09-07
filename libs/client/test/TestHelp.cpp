/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <sstream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/base/cts/CtsCmdRegistry.hpp"
#include "ecflow/client/Help.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// @brief Tests the manifest-driven help rendering exposed by the Help class.
///
/// @details The rendering helpers live in an anonymous namespace inside Help.cpp; they are
/// exercised here through the only public seam, streaming a Help built from the same
/// options_description the client assembles in production.
///

namespace {

///
/// @brief Builds the option description exactly as ClientOptions assembles it in production.
///
/// @details Registers every command option through CtsCmdRegistry and then appends the global
/// command-line options, so that the rendered help matches what ecflow_client produces.
///
/// @return The assembled option description.
///
boost::program_options::options_description make_description() {
    namespace po = boost::program_options;

    po::options_description desc("Client options");

    CtsCmdRegistry registry;
    registry.addAllOptions(desc);

    desc.add_options()("rid", po::value<std::string>()->implicit_value(std::string{}));
    desc.add_options()("port", po::value<std::string>()->implicit_value(std::string{}));
    desc.add_options()("host", po::value<std::string>()->implicit_value(std::string{}));
    desc.add_options()("user", po::value<std::string>()->implicit_value(std::string{}));
    desc.add_options()("password", po::value<std::string>()->implicit_value(std::string{}));
#ifdef ECF_OPENSSL
    desc.add_options()("ssl", "");
#endif
    desc.add_options()("http", "");
    desc.add_options()("https", "");

    return desc;
}

///
/// @brief Renders the help output for the given topic.
///
/// @details Mirrors `ecflow_client --help=<topic>` by streaming a Help built from the
/// production option description.
///
/// @param[in] topic Help topic, such as "summary", "task", a command name, or an option name.
/// @return The rendered help text.
///
std::string render(const std::string& topic) {
    boost::program_options::options_description desc = make_description();
    std::ostringstream ss;
    ss << Help{desc, topic};
    return ss.str();
}

///
/// @brief Reports whether one string occurs within another.
///
/// @param[in] haystack Text to search.
/// @param[in] needle Substring to look for.
/// @return true when @p needle occurs anywhere in @p haystack, false otherwise.
///
bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// The banner sentences produced by make_client_env_description() and make_task_env_description().
const std::string client_env_banner = "The client considers, for both user and task commands";
const std::string task_env_banner   = "The following environment variables are used specifically by task commands";

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_Help)

BOOST_AUTO_TEST_CASE(test_summary_lists_task_and_user_commands) {
    ECF_NAME_THIS_TEST();

    std::string out = render("summary");
    BOOST_CHECK(contains(out, "Ecflow client commands:"));
    // abort is a task command, alter a user command; both appear in the full summary.
    BOOST_CHECK(contains(out, "abort"));
    BOOST_CHECK(contains(out, "alter"));
    // get_name_kind() labels the command kinds in the summary column.
    BOOST_CHECK(contains(out, "task"));
    BOOST_CHECK(contains(out, "user"));
}

BOOST_AUTO_TEST_CASE(test_task_topic_lists_task_commands) {
    ECF_NAME_THIS_TEST();

    std::string out = render("task");
    BOOST_CHECK(contains(out, "Ecflow task client commands:"));
    BOOST_CHECK(contains(out, "abort"));
}

BOOST_AUTO_TEST_CASE(test_user_topic_lists_user_commands) {
    ECF_NAME_THIS_TEST();

    std::string out = render("user");
    BOOST_CHECK(contains(out, "Ecflow user client commands:"));
    BOOST_CHECK(contains(out, "alter"));
}

BOOST_AUTO_TEST_CASE(test_command_help_for_task_command_appends_both_env_banners) {
    ECF_NAME_THIS_TEST();

    std::string out = render("abort");
    // Heading plus the manifest description.
    BOOST_CHECK(contains(out, "abort"));
    // A task command gets both the common banner and the task-specific banner.
    BOOST_CHECK(contains(out, client_env_banner));
    BOOST_CHECK(contains(out, "ECF_HOST"));
    BOOST_CHECK(contains(out, task_env_banner));
    BOOST_CHECK(contains(out, "ECF_NAME"));
}

BOOST_AUTO_TEST_CASE(test_command_help_for_user_command_omits_task_env_banner) {
    ECF_NAME_THIS_TEST();

    std::string out = render("alter");
    // A user command gets the common banner but not the task-specific one.
    BOOST_CHECK(contains(out, client_env_banner));
    BOOST_CHECK(!contains(out, task_env_banner));
}

BOOST_AUTO_TEST_CASE(test_command_help_for_option_omits_env_banner) {
    ECF_NAME_THIS_TEST();

    std::string out = render("host");
    // An option is neither a user nor a task command, so no environment banner is appended.
    BOOST_CHECK(contains(out, "host"));
    BOOST_CHECK(!contains(out, client_env_banner));
    BOOST_CHECK(!contains(out, task_env_banner));
}

BOOST_AUTO_TEST_CASE(test_unknown_topic_falls_back_to_command_list) {
    ECF_NAME_THIS_TEST();

    // The renamed-away 'child' topic, and any other unknown name, degrade to the command list.
    std::string out = render("child");
    BOOST_CHECK(contains(out, "No matching command found"));
}

BOOST_AUTO_TEST_CASE(test_env_banner_ssl_visibility_follows_openssl_build) {
    ECF_NAME_THIS_TEST();

    // ECF_SSL carries requires: ECF_OPENSSL in the manifest, so it is advertised in the common
    // environment-variable banner only when the client is built with OpenSSL.
    std::string out = render("abort");
#ifdef ECF_OPENSSL
    BOOST_CHECK(contains(out, "ECF_SSL"));
#else
    BOOST_CHECK(!contains(out, "ECF_SSL"));
#endif
}

BOOST_AUTO_TEST_CASE(test_help_definition_lists_all_definition_items) {
    ECF_NAME_THIS_TEST();

    std::string out = render("definition");
    BOOST_CHECK(contains(out, "Ecflow definition items:"));
    // "task" is a node type, "trigger" is an attribute; both must appear in the listing.
    BOOST_CHECK(contains(out, "task"));
    BOOST_CHECK(contains(out, "trigger"));
    BOOST_CHECK(contains(out, "node"));
    BOOST_CHECK(contains(out, "attribute"));
}

BOOST_AUTO_TEST_CASE(test_help_defs_item_renders_detail_without_env_banner) {
    ECF_NAME_THIS_TEST();

    std::string out = render("defs/trigger");
    // Name banner plus the manifest description.
    BOOST_CHECK(contains(out, "trigger"));
    BOOST_CHECK(contains(out, "A boolean expression keeping the node from executing"));
    // A definition item is not a CLI command, so it gets no environment-variable footer.
    BOOST_CHECK(!contains(out, client_env_banner));
    BOOST_CHECK(!contains(out, task_env_banner));
}

BOOST_AUTO_TEST_CASE(test_help_defs_unknown_falls_back_to_definition_item_list) {
    ECF_NAME_THIS_TEST();

    // Falls back to the definition-item list (not the command list), since that is what the
    // user was actually asking about.
    std::string out = render("defs/no-such-item");
    BOOST_CHECK(contains(out, "No matching definition item found"));
    BOOST_CHECK(contains(out, "trigger"));
}

BOOST_AUTO_TEST_CASE(test_bare_help_mentions_definition_topic_and_lists_items) {
    ECF_NAME_THIS_TEST();

    std::string out = render("");
    BOOST_CHECK(contains(out, "--help=definition"));
    BOOST_CHECK(contains(out, "--help=defs/<item>"));
    BOOST_CHECK(contains(out, "Definition:"));
    BOOST_CHECK(contains(out, "trigger"));
}

BOOST_AUTO_TEST_CASE(test_defs_prefix_never_collides_with_a_same_named_command) {
    ECF_NAME_THIS_TEST();

    // event/label/meter/queue/complete are both task commands and definition-item attribute
    // names. A bare render(name) must keep showing the command (unchanged), while
    // render("defs/" + name) must reach the attribute -- exercising, through the full CLI
    // rendering path, the same collision-safety guarantee HelpCatalog's own test covers at the
    // manifest-lookup level.
    struct Case
    {
        std::string name;
        std::string attribute_fingerprint;
    };
    const std::vector<Case> cases = {
        {"event", "event's initial value is true"},
        {"label", "Double-quoted initial text"},
        {"meter", "GUI display hint"},
        {"queue", "lighter-weight alternative to repeat"},
        {"complete", "AND-continuation"},
    };

    for (const auto& c : cases) {
        std::string command_out = render(c.name);
        BOOST_CHECK_MESSAGE(contains(command_out, "Argument(s):"), c.name << ": expected the command's help");
        BOOST_CHECK_MESSAGE(!contains(command_out, c.attribute_fingerprint),
                            c.name << ": command help unexpectedly contains attribute-only text");

        std::string attribute_out = render("defs/" + c.name);
        BOOST_CHECK_MESSAGE(contains(attribute_out, c.attribute_fingerprint),
                            c.name << ": expected the attribute's detail");
        BOOST_CHECK_MESSAGE(!contains(attribute_out, "Argument(s):"),
                            c.name << ": attribute detail unexpectedly contains the command's Argument(s) block");
    }

    // "task" is both a topic (list task commands) and a definition-item node type.
    BOOST_CHECK(contains(render("task"), "Ecflow task client commands:"));
    BOOST_CHECK(contains(render("defs/task"), "A leaf node in the node hierarchy."));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
