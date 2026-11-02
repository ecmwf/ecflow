// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/core/cfg/JsonReader.hpp"
#include "ecflow/core/cfg/Mapper.hpp"
#include "ecflow/core/cfg/ServerCatalogue.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::Catalogue;
using ecf::cfg::ConfigurationError;
using ecf::cfg::JsonReader;
using ecf::cfg::Key;
using ecf::cfg::Layer;
using ecf::cfg::Mapper;
using ecf::cfg::Names;
using ecf::cfg::Setting;
using ecf::cfg::Type;
using ecf::cfg::Value;

namespace checks = ecf::cfg::checks;
namespace layers = ecf::cfg::layers;

namespace {

const std::string origin = "host.3141.ecf.cfg";

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
/// @brief Creates a catalogue resembling that of the server, with a setting of each type, and a setting without
///   a name in the file.
///
/// @return The catalogue.
///
Catalogue test_catalogue() {
    return Catalogue({
        Setting(Key("ecflow.server.variables@ECF_PORT"),
                Type::Integer,
                value_of<std::int64_t>(3141),
                Names{std::nullopt, "/ecflow/server/variables/ECF_PORT"},
                checks::range(1024, 49150)),
        Setting(Key("ecflow.server.variables@ECF_HOME"),
                Type::Text,
                std::nullopt,
                Names{std::nullopt, "/ecflow/server/variables/ECF_HOME"},
                checks::non_empty()),
        Setting(Key("ecflow.server.http"), Type::Boolean, value_of(false), Names{std::nullopt, "/ecflow/server/http"}),
        // Without a name in the file
        Setting(Key("ecflow.server.ipv6"), Type::Boolean, value_of(false), Names{}),
    });
}

///
/// @brief Reads the given JSON text and maps it onto a layer of the test catalogue.
///
/// @param[in] text The JSON text
/// @return The layer.
///
Layer map(const std::string& text) {
    const auto catalogue = test_catalogue();
    return Mapper(catalogue).map(JsonReader().read(text, origin), layers::ecf_cfg, origin);
}

///
/// @brief Checks that mapping the given JSON text throws a ConfigurationError whose message contains every given
///   fragment.
///
/// @param[in] text The JSON text
/// @param[in] fragments Texts expected in the message of the error, besides the origin
///
void check_refused(const std::string& text, std::vector<std::string> fragments) {
    BOOST_TEST_CONTEXT("text: " << text) {
        fragments.push_back(origin);
        BOOST_CHECK_EXCEPTION((void)map(text), ConfigurationError, [&](const ConfigurationError& e) {
            const std::string message = e.what();
            BOOST_TEST_INFO("message: " << message);
            for (const auto& fragment : fragments) {
                if (message.find(fragment) == std::string::npos) {
                    return false;
                }
            }
            return true;
        });
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgMapper)

//
// Mapping
//

BOOST_AUTO_TEST_CASE(test_map_settings_to_keys) {
    ECF_NAME_THIS_TEST();

    const auto layer = map(R"({
        "version": 1,
        "ecflow": {
            "server": {
                "variables": { "ECF_PORT": 3142, "ECF_HOME": "/home" },
                "http": true
            }
        }
    })");

    BOOST_CHECK_EQUAL(layer.name, std::string(layers::ecf_cfg));
    BOOST_REQUIRE_EQUAL(layer.entries.size(), 3U);
    BOOST_CHECK(layer.entries.at(Key("ecflow.server.variables@ECF_PORT")) == value_of<std::int64_t>(3142));
    BOOST_CHECK(layer.entries.at(Key("ecflow.server.variables@ECF_HOME")) == value_of<std::string>("/home"));
    BOOST_CHECK(layer.entries.at(Key("ecflow.server.http")) == value_of(true));
}

BOOST_AUTO_TEST_CASE(test_map_only_the_settings_present) {
    ECF_NAME_THIS_TEST();

    // Neither defaults nor anything else is added: the layer holds what the file states
    BOOST_CHECK(map(R"({ "version": 1 })").entries.empty());
    BOOST_CHECK(map(R"({ "version": 1, "ecflow": {} })").entries.empty());
    BOOST_CHECK(map(R"({ "version": 1, "ecflow": { "server": { "variables": {} } } })").entries.empty());

    const auto layer = map(R"({ "ecflow": { "server": { "http": false } }, "version": 1 })");
    BOOST_REQUIRE_EQUAL(layer.entries.size(), 1U);
    BOOST_CHECK(layer.entries.at(Key("ecflow.server.http")) == value_of(false));
}

BOOST_AUTO_TEST_CASE(test_map_every_setting_of_the_server) {
    ECF_NAME_THIS_TEST();

    // Every setting of the server can be set in the file (R10); without SSL support, the file can only leave SSL
    // disabled, hence omits ECF_SSL
#ifdef ECF_OPENSSL
    const std::string ssl_variable = R"("ECF_SSL": "",)";
    const std::string ssl_option   = "true";
#else
    const std::string ssl_variable;
    const std::string ssl_option = "false";
#endif
    const auto tree = JsonReader().read(R"({
        "version": 1,
        "ecflow": {
            "server": {
                "variables": {
                    "ECF_HOME": "/home",
                    "ECF_HOST": "host",
                    "ECF_PORT": 3142,
                    "ECF_CHECK": "check",
                    "ECF_CHECKOLD": "check.b",
                    "ECF_CHECKINTERVAL": 60,
                    "ECF_CHECKMODE": "CHECK_ALWAYS",
                    "ECF_LOG": "log",
                    "ECF_LISTS": "lists",
                    "ECF_PASSWD": "passwd",
                    "ECF_CUSTOM_PASSWD": "custom_passwd",
                    "ECF_TASK_THRESHOLD": 100,
                    "ECF_PRUNE_NODE_LOG": 0,
                    "ECF_INTERVAL": 10,
                    "ECF_JOB_CMD": "job",
                    "ECF_KILL_CMD": "kill",
                    "ECF_STATUS_CMD": "status",
                    "ECF_CHECK_CMD": "check",
                    "ECF_URL_CMD": "url",
                    "ECF_URL_BASE": "base",
                    "ECF_URL": "url",
                    "ECF_MICRODEF": "@",)" + ssl_variable +
                                            R"("ECF_DEBUG_SERVER": true
                },
                "http": true,
                "ipv6": true,
                "ssl": )" + ssl_option +
                                            R"(,
                "job_generation": false
            }
        }
    })",
                                        origin);

    const auto& catalogue = ecf::cfg::server_catalogue();
    const auto layer      = Mapper(catalogue).map(tree, layers::ecf_cfg, origin);
    const auto omitted    = ssl_variable.empty() ? 1U : 0U;
    BOOST_CHECK_EQUAL(layer.entries.size(), catalogue.settings().size() - omitted);
    for (const auto& setting : catalogue.settings()) {
        if (ssl_variable.empty() && setting.key() == Key("ecflow.server.variables@ECF_SSL")) {
            continue;
        }
        BOOST_TEST_CONTEXT("key: " << setting.key().key()) {
            BOOST_CHECK(layer.entries.count(setting.key()) == 1);
        }
    }
}

//
// Refusing
//

BOOST_AUTO_TEST_CASE(test_refuse_missing_or_unsupported_version) {
    ECF_NAME_THIS_TEST();

    check_refused(R"({})", {"missing the version"});
    check_refused(R"({ "ecflow": {} })", {"missing the version"});
    check_refused(R"({ "ecflow": { "version": 1 } })", {"missing the version"});
    for (const std::string version : {"0", "2", "-1", R"("1")", "1.0", "true", "null", "[1]", "{}"}) {
        check_refused(R"({ "version": )" + version + " }", {"/version", "unsupported version"});
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_root_other_than_object) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"null", "1", R"("text")", "[]", "true"}) {
        check_refused(text, {"expected an object"});
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_unknown_settings) {
    ECF_NAME_THIS_TEST();

    check_refused(R"({ "version": 1, "unknown": 1 })", {"/unknown", "unknown setting"});
    check_refused(R"({ "version": 1, "ecflow": { "client": {} } })", {"/ecflow/client", "unknown setting"});
    check_refused(R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_NONE": "x" } } } })",
                  {"/ecflow/server/variables/ECF_NONE", "unknown setting"});
    // A setting without a name in the file cannot be set in the file
    check_refused(R"({ "version": 1, "ecflow": { "server": { "ipv6": true } } })",
                  {"/ecflow/server/ipv6", "unknown setting"});
    // Names are case-sensitive
    check_refused(R"({ "version": 1, "ecflow": { "server": { "variables": { "ecf_port": 3141 } } } })",
                  {"/ecflow/server/variables/ecf_port", "unknown setting"});
    // A name never spans several segments of a path
    check_refused(R"({ "version": 1, "ecflow": { "server.variables": { "ECF_PORT": 3141 } } })",
                  {"/ecflow/server.variables", "unknown setting"});
    check_refused(R"({ "version": 1, "ecflow.server": { "http": true } })", {"/ecflow.server", "unknown setting"});
    check_refused(R"({ "version": 1, "ecflow": { "": {} } })", {"/ecflow/", "unknown setting"});
}

BOOST_AUTO_TEST_CASE(test_refuse_values_of_another_type) {
    ECF_NAME_THIS_TEST();

    const std::string port = R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_PORT": %s } } } })";
    const std::string home = R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_HOME": %s } } } })";
    const std::string http = R"({ "version": 1, "ecflow": { "server": { "http": %s } } })";
    auto with = [](std::string text, const std::string& value) { return text.replace(text.find("%s"), 2, value); };

    for (const std::string value : {R"("3141")", "3141.0", "true", "null", "[3141]", "{}"}) {
        check_refused(with(port, value), {"/ecflow/server/variables/ECF_PORT", "expected an integer"});
    }
    for (const std::string value : {"1", "true", "null", R"(["/home"])", "{}"}) {
        check_refused(with(home, value), {"/ecflow/server/variables/ECF_HOME", "expected a string"});
    }
    for (const std::string value : {R"("true")", "1", "0", "null", "[true]", "{}"}) {
        check_refused(with(http, value), {"/ecflow/server/http", "expected a boolean"});
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_values_failing_the_check) {
    ECF_NAME_THIS_TEST();

    check_refused(R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_PORT": 80 } } } })",
                  {"/ecflow/server/variables/ECF_PORT", "[1024, 49150]"});
    check_refused(R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_HOME": "" } } } })",
                  {"/ecflow/server/variables/ECF_HOME", "empty"});
}

BOOST_AUTO_TEST_CASE(test_refuse_values_where_an_object_is_expected) {
    ECF_NAME_THIS_TEST();

    for (const std::string value : {"1", "true", "null", R"("server")", "[]"}) {
        check_refused(R"({ "version": 1, "ecflow": )" + value + " }", {"/ecflow", "expected an object"});
        check_refused(R"({ "version": 1, "ecflow": { "server": { "variables": )" + value + " } } }",
                      {"/ecflow/server/variables", "expected an object"});
    }
}

BOOST_AUTO_TEST_CASE(test_error_messages) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::pair<std::string, std::string>> cases{
        {R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_PORT": "3141" } } } })",
         origin + ": /ecflow/server/variables/ECF_PORT: expected an integer, not a string"},
        {R"({ "version": 1, "ecflow": { "server": { "http": 1.5 } } })",
         origin + ": /ecflow/server/http: expected a boolean, not a decimal number"},
        {R"({ "version": 1, "ecflow": { "server": [] } })",
         origin + ": /ecflow/server: expected an object, not an array"},
        {R"({ "version": 1, "other": 1 })", origin + ": /other: unknown setting"},
        {R"({ "version": 2 })", origin + ": /version: unsupported version of the format, expected 1"},
        {R"({})", origin + ": missing the version of the format, expected \"version\": 1"},
        {R"(null)", origin + ": expected an object, not null"},
        {R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_PORT": 80 } } } })",
         origin + ": /ecflow/server/variables/ECF_PORT: invalid value 80: expected a value in [1024, 49150]"},
        {R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_HOME": "" } } } })",
         origin + ": /ecflow/server/variables/ECF_HOME: invalid value '': expected a non-empty value"},
    };
    for (const auto& [text, expected] : cases) {
        BOOST_TEST_CONTEXT("text: " << text) {
            std::string message = "(no error)";
            try {
                (void)map(text);
            }
            catch (const ConfigurationError& e) {
                message = e.what();
            }
            BOOST_CHECK_EQUAL(message, expected);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
