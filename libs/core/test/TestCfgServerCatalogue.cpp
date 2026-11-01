// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/cfg/ServerCatalogue.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::ConfigurationError;
using ecf::cfg::Key;
using ecf::cfg::Names;
using ecf::cfg::server_catalogue;
using ecf::cfg::Type;
using ecf::cfg::Value;

namespace {

///
/// @brief Creates a value holding the given alternative.
///
/// @tparam T The alternative of Value
/// @param[in] value The value
/// @return The value.
///
template <typename T>
Value value_of(T value) {
    return Value(std::in_place_type<T>, std::move(value));
}

///
/// @brief Describes a setting expected in the server catalogue.
///
struct Expected
{
    std::string key;
    Type type;
    std::optional<Value> default_value;
    Names names;
};

///
/// @brief Lists every input of the server, as available in develop.
///
/// @return The expected settings.
///
std::vector<Expected> expected_settings() {
    auto text                    = [](const std::string& v) { return std::optional<Value>(value_of<std::string>(v)); };
    auto integer                 = [](std::int64_t v) { return std::optional<Value>(value_of<std::int64_t>(v)); };
    auto boolean                 = [](bool v) { return std::optional<Value>(value_of<bool>(v)); };
    auto variable                = [](const std::string& name) { return "ecflow.server.variables@" + name; };
    auto in_file_and_environment = [](const std::string& name) { return Names{name, name, std::nullopt}; };
    auto in_file_only            = [](const std::string& name) { return Names{name, std::nullopt, std::nullopt}; };

    return {
        // server_environment.cfg, overridden by the environment
        {variable("ECF_HOME"), Type::Text, text(Ecf::HOME()), in_file_and_environment("ECF_HOME")},
        {variable("ECF_PORT"), Type::Integer, integer(Ecf::PORT()), Names{"ECF_PORT", "ECF_PORT", "port"}},
        {variable("ECF_CHECK"), Type::Text, text(Ecf::CHECKPT()), in_file_and_environment("ECF_CHECK")},
        {variable("ECF_CHECKOLD"), Type::Text, text(Ecf::BACKUP_CHECKPT()), in_file_and_environment("ECF_CHECKOLD")},
        {variable("ECF_CHECKINTERVAL"),
         Type::Integer,
         integer(ecf::CheckPt::default_interval()),
         in_file_and_environment("ECF_CHECKINTERVAL")},
        {variable("ECF_LOG"), Type::Text, text(Ecf::LOG_FILE()), in_file_and_environment("ECF_LOG")},
        {variable("ECF_LISTS"),
         Type::Text,
         text(ecf::string_constants::white_list_file),
         in_file_and_environment("ECF_LISTS")},
        {variable("ECF_PASSWD"), Type::Text, text(Ecf::PASSWD_FILE()), in_file_and_environment("ECF_PASSWD")},
        {variable("ECF_CUSTOM_PASSWD"),
         Type::Text,
         text(Ecf::CUSTOM_PASSWD_FILE()),
         in_file_and_environment("ECF_CUSTOM_PASSWD")},
        {variable("ECF_TASK_THRESHOLD"),
         Type::Integer,
         integer(Ecf::TASK_THRESHOLD()),
         in_file_and_environment("ECF_TASK_THRESHOLD")},
        {variable("ECF_PRUNE_NODE_LOG"),
         Type::Integer,
         integer(Ecf::PRUNE_NODE_LOG()),
         in_file_and_environment("ECF_PRUNE_NODE_LOG")},
        // server_environment.cfg only, possibly overridden by the command line
        {variable("ECF_INTERVAL"),
         Type::Integer,
         integer(Ecf::SUBMIT_INTERVAL()),
         Names{"ECF_INTERVAL", std::nullopt, "ecfinterval"}},
        {variable("ECF_CHECKMODE"), Type::Text, text(Ecf::CHECK_MODE()), in_file_only("ECF_CHECKMODE")},
        {variable("ECF_JOB_CMD"), Type::Text, text(Ecf::JOB_CMD()), in_file_only("ECF_JOB_CMD")},
        {variable("ECF_KILL_CMD"), Type::Text, text(Ecf::KILL_CMD()), in_file_only("ECF_KILL_CMD")},
        {variable("ECF_STATUS_CMD"), Type::Text, text(Ecf::STATUS_CMD()), in_file_only("ECF_STATUS_CMD")},
        {variable("ECF_CHECK_CMD"), Type::Text, text(Ecf::CHECK_CMD()), in_file_only("ECF_CHECK_CMD")},
        {variable("ECF_URL_CMD"), Type::Text, text(Ecf::URL_CMD()), in_file_only("ECF_URL_CMD")},
        {variable("ECF_URL_BASE"), Type::Text, text(Ecf::URL_BASE()), in_file_only("ECF_URL_BASE")},
        {variable("ECF_URL"), Type::Text, text(Ecf::URL()), in_file_only("ECF_URL")},
        {variable("ECF_MICRODEF"), Type::Text, text(Ecf::MICRO()), in_file_only("ECF_MICRODEF")},
        // Environment only, possibly overridden by the command line
        {variable("ECF_SSL"), Type::Text, std::nullopt, Names{std::nullopt, "ECF_SSL", std::nullopt}},
        {variable("ECF_DEBUG_SERVER"), Type::Boolean, boolean(false), Names{std::nullopt, "ECF_DEBUG_SERVER", "debug"}},
        // Command line only
        {"ecflow.server.http", Type::Boolean, boolean(false), Names{std::nullopt, std::nullopt, "http"}},
        {"ecflow.server.ipv6", Type::Boolean, boolean(false), Names{std::nullopt, std::nullopt, "v6"}},
        {"ecflow.server.ssl", Type::Boolean, boolean(false), Names{std::nullopt, std::nullopt, "ssl"}},
        {"ecflow.server.job_generation",
         Type::Boolean,
         boolean(true),
         Names{std::nullopt, std::nullopt, "dis_job_gen"}},
    };
}

///
/// @brief Checks that parsing the given text for the given key throws a ConfigurationError naming the key.
///
/// @param[in] key The key
/// @param[in] text The text
///
void check_refused(const std::string& key, const std::string& text) {
    BOOST_TEST_CONTEXT("key: " << key << ", text: '" << text << "'") {
        BOOST_CHECK_EXCEPTION(
            (void)server_catalogue().parse(Key(key), text), ConfigurationError, [&](const ConfigurationError& e) {
                return std::string(e.what()).find(key) != std::string::npos;
            });
    }
}

///
/// @brief Checks that parsing the given text for the given key gives the expected value.
///
/// @param[in] key The key
/// @param[in] text The text
/// @param[in] expected The expected value
///
void check_accepted(const std::string& key, const std::string& text, const Value& expected) {
    BOOST_TEST_CONTEXT("key: " << key << ", text: '" << text << "'") {
        BOOST_CHECK(server_catalogue().parse(Key(key), text) == expected);
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgServerCatalogue)

BOOST_AUTO_TEST_CASE(test_server_catalogue_describes_every_input) {
    ECF_NAME_THIS_TEST();

    const auto& catalogue = server_catalogue();
    const auto expected   = expected_settings();
    BOOST_CHECK_EQUAL(catalogue.settings().size(), expected.size());

    for (const auto& e : expected) {
        BOOST_TEST_CONTEXT("key: " << e.key) {
            const auto* setting = catalogue.find(Key(e.key));
            BOOST_REQUIRE(setting);
            BOOST_CHECK(setting->type() == e.type);
            BOOST_CHECK(setting->default_value() == e.default_value);
            BOOST_CHECK(setting->names().file == e.names.file);
            BOOST_CHECK(setting->names().environment == e.names.environment);
            BOOST_CHECK(setting->names().option == e.names.option);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_server_catalogue_names_each_input_once_per_source) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> file, environment, option;
    for (const auto& setting : server_catalogue().settings()) {
        if (setting.names().file) {
            file.push_back(*setting.names().file);
        }
        if (setting.names().environment) {
            environment.push_back(*setting.names().environment);
        }
        if (setting.names().option) {
            option.push_back(*setting.names().option);
        }
    }
    for (auto* names : {&file, &environment, &option}) {
        std::sort(names->begin(), names->end());
        BOOST_CHECK(std::adjacent_find(names->begin(), names->end()) == names->end());
    }
    // The 21 settings accepted in server_environment.cfg
    BOOST_CHECK_EQUAL(file.size(), 21U);
}

BOOST_AUTO_TEST_CASE(test_server_catalogue_integer_ranges) {
    ECF_NAME_THIS_TEST();

    const std::string port = "ecflow.server.variables@ECF_PORT";
    check_accepted(port, "1024", value_of<std::int64_t>(1024));
    check_accepted(port, "49150", value_of<std::int64_t>(49150));
    check_refused(port, "1023");
    check_refused(port, "49151");
    check_refused(port, "3141x");

    const std::string interval = "ecflow.server.variables@ECF_INTERVAL";
    check_accepted(interval, "1", value_of<std::int64_t>(1));
    check_accepted(interval, "60", value_of<std::int64_t>(60));
    check_refused(interval, "0");
    check_refused(interval, "61");

    for (const std::string key :
         {"ecflow.server.variables@ECF_CHECKINTERVAL", "ecflow.server.variables@ECF_TASK_THRESHOLD"}) {
        check_accepted(key, "1", value_of<std::int64_t>(1));
        check_refused(key, "0");
        check_refused(key, "-1");
    }

    const std::string prune = "ecflow.server.variables@ECF_PRUNE_NODE_LOG";
    check_accepted(prune, "0", value_of<std::int64_t>(0));
    check_refused(prune, "-1");
}

BOOST_AUTO_TEST_CASE(test_server_catalogue_check_mode) {
    ECF_NAME_THIS_TEST();

    const std::string mode = "ecflow.server.variables@ECF_CHECKMODE";
    for (const std::string value : {"CHECK_NEVER", "CHECK_ON_TIME", "CHECK_ALWAYS"}) {
        check_accepted(mode, value, value_of(value));
    }
    for (const std::string value : {"", "check_never", "NEVER"}) {
        check_refused(mode, value);
    }
}

BOOST_AUTO_TEST_CASE(test_server_catalogue_refuses_empty_text) {
    ECF_NAME_THIS_TEST();

    // Every text setting, except ECF_SSL, is required to be non-empty
    for (const auto& setting : server_catalogue().settings()) {
        if (setting.type() != Type::Text) {
            continue;
        }
        if (setting.key() == Key("ecflow.server.variables@ECF_SSL")) {
            check_accepted(setting.key().key(), "", value_of<std::string>(""));
        }
        else {
            check_refused(setting.key().key(), "");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_server_catalogue_switches) {
    ECF_NAME_THIS_TEST();

    // ECF_DEBUG_SERVER enables debug when defined, whatever its text
    for (const std::string text : {"", "0", "false", "1"}) {
        check_accepted("ecflow.server.variables@ECF_DEBUG_SERVER", text, value_of(true));
    }
    // Command-line switches take no text: their presence decides the value
    check_accepted("ecflow.server.http", "", value_of(true));
    check_accepted("ecflow.server.ipv6", "", value_of(true));
    check_accepted("ecflow.server.ssl", "", value_of(true));
    check_accepted("ecflow.server.job_generation", "", value_of(false));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
