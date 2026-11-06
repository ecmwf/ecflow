// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::Catalogue;
using ecf::cfg::ConfigurationError;
using ecf::cfg::InvalidValue;
using ecf::cfg::Key;
using ecf::cfg::Names;
using ecf::cfg::Setting;
using ecf::cfg::Type;
using ecf::cfg::Value;

namespace checks = ecf::cfg::checks;

namespace {

///
/// @brief Checks that the given callable throws a ConfigurationError whose message contains the given fragment.
///
/// @tparam F Callable taking no arguments
/// @param[in] f The callable expected to throw
/// @param[in] fragment Text expected in the message of the error
///
template <typename F>
void check_error(F f, const std::string& fragment) {
    BOOST_CHECK_EXCEPTION(f(), ConfigurationError, [&](const ConfigurationError& e) {
        const std::string message = e.what();
        BOOST_TEST_INFO("message: " << message);
        return message.find(fragment) != std::string::npos;
    });
}

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

const Key port_key("ecflow.server.variables@ECF_PORT");

///
/// @brief Creates a setting resembling the port of the server.
///
/// @return The setting: an integer in [1024, 49150], defaulting to 3141, with a name in every source.
///
Setting port_setting() {
    return Setting(port_key,
                   Type::Integer,
                   value_of<std::int64_t>(3141),
                   Names{"ECF_PORT", "/ecflow/server/variables/ECF_PORT", "ECF_PORT", "port"},
                   checks::range(1024, 49150));
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgCatalogue)

//
// Setting
//

BOOST_AUTO_TEST_CASE(test_setting_describes_an_input) {
    ECF_NAME_THIS_TEST();

    const auto setting = port_setting();
    BOOST_CHECK(setting.key() == port_key);
    BOOST_CHECK(setting.type() == Type::Integer);
    BOOST_REQUIRE(setting.default_value().has_value());
    BOOST_CHECK(*setting.default_value() == value_of<std::int64_t>(3141));
    BOOST_CHECK(setting.names().env_cfg == std::optional<std::string>("ECF_PORT"));
    BOOST_CHECK(setting.names().ecf_cfg == std::optional<std::string>("/ecflow/server/variables/ECF_PORT"));
    BOOST_CHECK(setting.names().env == std::optional<std::string>("ECF_PORT"));
    BOOST_CHECK(setting.names().cli == std::optional<std::string>("port"));
}

BOOST_AUTO_TEST_CASE(test_setting_without_default_or_names) {
    ECF_NAME_THIS_TEST();

    const Setting setting(Key("ecflow.server.http"), Type::Boolean, std::nullopt, Names{});
    BOOST_CHECK(!setting.default_value().has_value());
    BOOST_CHECK(!setting.names().env_cfg.has_value());
    BOOST_CHECK(!setting.names().ecf_cfg.has_value());
    BOOST_CHECK(!setting.names().env.has_value());
    BOOST_CHECK(!setting.names().cli.has_value());
}

BOOST_AUTO_TEST_CASE(test_setting_refuses_invalid_default) {
    ECF_NAME_THIS_TEST();

    // The default must hold the type of the setting, and pass its check
    check_error([] { Setting(port_key, Type::Integer, value_of<std::string>("3141"), Names{}); }, "ECF_PORT");
    check_error(
        [] { Setting(port_key, Type::Integer, value_of<std::int64_t>(80), Names{}, checks::range(1024, 49150)); },
        "ECF_PORT");
}

//
// Parsing text
//

BOOST_AUTO_TEST_CASE(test_parse_integer) {
    ECF_NAME_THIS_TEST();

    const auto setting = Setting(port_key, Type::Integer, std::nullopt, Names{});
    BOOST_CHECK(setting.parse("3141") == value_of<std::int64_t>(3141));
    BOOST_CHECK(setting.parse("-5") == value_of<std::int64_t>(-5));
    BOOST_CHECK(setting.parse("0") == value_of<std::int64_t>(0));
    BOOST_CHECK(setting.parse("9223372036854775807") == value_of<std::int64_t>(INT64_MAX));
}

BOOST_AUTO_TEST_CASE(test_parse_integer_refuses_ambiguous_text) {
    ECF_NAME_THIS_TEST();

    const auto setting = Setting(port_key, Type::Integer, std::nullopt, Names{});
    for (const std::string text :
         {"", " 3141", "3141 ", "3141x", "+3141", "0x10", "3.0", "1e3", "9223372036854775808", "-"}) {
        BOOST_TEST_CONTEXT("text: '" << text << "'") {
            check_error([&] { (void)setting.parse(text); }, "ECF_PORT");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parse_boolean) {
    ECF_NAME_THIS_TEST();

    const auto setting = Setting(Key("ecflow.server.http"), Type::Boolean, std::nullopt, Names{});
    BOOST_CHECK(setting.parse("true") == value_of(true));
    BOOST_CHECK(setting.parse("false") == value_of(false));
    for (const std::string text : {"", "1", "0", "yes", "True", "FALSE", " true"}) {
        BOOST_TEST_CONTEXT("text: '" << text << "'") {
            check_error([&] { (void)setting.parse(text); }, "ecflow.server.http");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parse_text_keeps_the_text) {
    ECF_NAME_THIS_TEST();

    const auto setting = Setting(Key("ecflow.server.variables@ECF_MICRODEF"), Type::Text, std::nullopt, Names{});
    for (const std::string text : {"%", "", " spaced ", "3141", "true"}) {
        BOOST_TEST_CONTEXT("text: '" << text << "'") {
            BOOST_CHECK(setting.parse(text) == value_of<std::string>(text));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parse_with_custom_rule) {
    ECF_NAME_THIS_TEST();

    // e.g. an environment variable whose mere presence enables an option, whatever its text
    const auto setting = Setting(Key("ecflow.server.variables@ECF_DEBUG_SERVER"),
                                 Type::Boolean,
                                 value_of(false),
                                 Names{std::nullopt, std::nullopt, "ECF_DEBUG_SERVER", "debug"},
                                 {},
                                 [](std::string_view) { return Value(true); });
    for (const std::string text : {"", "0", "false", "anything"}) {
        BOOST_TEST_CONTEXT("text: '" << text << "'") {
            BOOST_CHECK(setting.parse(text) == value_of(true));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parse_applies_the_check) {
    ECF_NAME_THIS_TEST();

    const auto setting = port_setting();
    BOOST_CHECK(setting.parse("1024") == value_of<std::int64_t>(1024));
    check_error([&] { (void)setting.parse("80"); }, "ECF_PORT");
}

//
// Checking values
//

BOOST_AUTO_TEST_CASE(test_check_refuses_value_of_another_type) {
    ECF_NAME_THIS_TEST();

    const auto setting = port_setting();
    setting.check(value_of<std::int64_t>(3141));
    check_error([&] { setting.check(value_of<std::string>("3141")); }, "ECF_PORT");
    check_error([&] { setting.check(value_of(true)); }, "ECF_PORT");
}

BOOST_AUTO_TEST_CASE(test_check_range) {
    ECF_NAME_THIS_TEST();

    const auto setting = port_setting();
    for (const std::int64_t value : {1024, 3141, 49150}) {
        setting.check(value_of(value));
    }
    for (const std::int64_t value : {1023, 49151, 0, -1}) {
        BOOST_TEST_CONTEXT("value: " << value) {
            check_error([&] { setting.check(value_of(value)); }, "[1024, 49150]");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_check_at_least) {
    ECF_NAME_THIS_TEST();

    const Setting setting(
        Key("ecflow.server.variables@ECF_PRUNE_NODE_LOG"), Type::Integer, std::nullopt, Names{}, checks::at_least(0));
    setting.check(value_of<std::int64_t>(0));
    setting.check(value_of<std::int64_t>(180));
    check_error([&] { setting.check(value_of<std::int64_t>(-1)); }, "at least 0");
}

BOOST_AUTO_TEST_CASE(test_check_non_empty) {
    ECF_NAME_THIS_TEST();

    const Setting setting(
        Key("ecflow.server.variables@ECF_JOB_CMD"), Type::Text, std::nullopt, Names{}, checks::non_empty());
    setting.check(value_of<std::string>("%ECF_JOB%"));
    check_error([&] { setting.check(value_of<std::string>("")); }, "empty");
}

BOOST_AUTO_TEST_CASE(test_check_one_of) {
    ECF_NAME_THIS_TEST();

    const Setting setting(Key("ecflow.server.variables@ECF_CHECKMODE"),
                          Type::Text,
                          std::nullopt,
                          Names{},
                          checks::one_of({"CHECK_NEVER", "CHECK_ON_TIME", "CHECK_ALWAYS"}));
    for (const std::string value : {"CHECK_NEVER", "CHECK_ON_TIME", "CHECK_ALWAYS"}) {
        setting.check(value_of(value));
    }
    for (const std::string value : {"", "check_never", "CHECK_ON_TIME ", "NEVER"}) {
        BOOST_TEST_CONTEXT("value: '" << value << "'") {
            check_error([&] { setting.check(value_of(value)); }, "CHECK_NEVER, CHECK_ON_TIME, CHECK_ALWAYS");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_check_only_when_supported) {
    ECF_NAME_THIS_TEST();

    const auto reason = std::string("this server was built without SSL support");

    // A supported feature accepts every value
    const Setting supported(
        Key("ecflow.server.ssl"), Type::Boolean, value_of(false), Names{}, checks::only_when_supported(true, reason));
    supported.check(value_of(true));
    supported.check(value_of(false));

    // An unsupported feature accepts only false, i.e. not requested
    const Setting option(
        Key("ecflow.server.ssl"), Type::Boolean, value_of(false), Names{}, checks::only_when_supported(false, reason));
    option.check(value_of(false));
    check_error([&] { option.check(value_of(true)); }, reason);

    const Setting variable(Key("ecflow.server.variables@ECF_SSL"),
                           Type::Text,
                           std::nullopt,
                           Names{},
                           checks::only_when_supported(false, reason));
    for (const std::string value : {"", "1", "host.port"}) {
        BOOST_TEST_CONTEXT("value: '" << value << "'") {
            check_error([&] { variable.check(value_of(value)); }, reason);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_refusals_carry_their_parts) {
    ECF_NAME_THIS_TEST();

    const auto setting = port_setting();
    auto refusal       = [](auto f) -> InvalidValue {
        try {
            f();
        }
        catch (const InvalidValue& e) {
            return e;
        }
        BOOST_FAIL("expected InvalidValue");
        return InvalidValue(Key("none"), "", "");
    };

    // Parsing shows the text as given, in single quotes
    const auto malformed = refusal([&] { (void)setting.parse("3141x"); });
    BOOST_CHECK(malformed.key() == port_key);
    BOOST_CHECK_EQUAL(malformed.value(), "'3141x'");
    BOOST_CHECK_EQUAL(malformed.reason(), "expected an integer");

    const auto out_of_range = refusal([&] { (void)setting.parse("80"); });
    BOOST_CHECK_EQUAL(out_of_range.value(), "'80'");
    BOOST_CHECK_EQUAL(out_of_range.reason(), "expected a value in [1024, 49150]");
    BOOST_CHECK_EQUAL(std::string(out_of_range.what()),
                      "Invalid value '80' for 'ecflow.server.variables@ECF_PORT': expected a value in [1024, 49150]");

    // Checking shows the value
    const auto checked = refusal([&] { setting.check(value_of<std::int64_t>(80)); });
    BOOST_CHECK_EQUAL(checked.value(), "80");
    BOOST_CHECK_EQUAL(checked.reason(), "expected a value in [1024, 49150]");

    const auto mistyped = refusal([&] { setting.check(value_of<std::string>("3141")); });
    BOOST_CHECK_EQUAL(mistyped.value(), "'3141'");
    BOOST_CHECK_EQUAL(mistyped.reason(), "expected an integer, not a text");
}

//
// Catalogue
//

BOOST_AUTO_TEST_CASE(test_catalogue_finds_settings_by_key) {
    ECF_NAME_THIS_TEST();

    const Catalogue catalogue(
        {port_setting(), Setting(Key("ecflow.server.http"), Type::Boolean, value_of(false), Names{})});
    BOOST_REQUIRE(catalogue.find(port_key));
    BOOST_CHECK(catalogue.find(port_key)->type() == Type::Integer);
    BOOST_REQUIRE(catalogue.find(Key("ecflow.server.http")));
    BOOST_CHECK(catalogue.find(Key("ecflow.server.unknown")) == nullptr);

    std::vector<std::string> keys;
    for (const auto& setting : catalogue.settings()) {
        keys.push_back(setting.key().key());
    }
    // Ordered by key
    const std::vector<std::string> expected{"ecflow.server.http", "ecflow.server.variables@ECF_PORT"};
    BOOST_CHECK_EQUAL_COLLECTIONS(keys.begin(), keys.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_catalogue_refuses_duplicate_keys) {
    ECF_NAME_THIS_TEST();

    check_error([] { Catalogue({port_setting(), port_setting()}); }, "ECF_PORT");
}

BOOST_AUTO_TEST_CASE(test_path_of_key) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(ecf::cfg::path_of(Key("a")), "/a");
    BOOST_CHECK_EQUAL(ecf::cfg::path_of(Key("a.b.c")), "/a/b/c");
    BOOST_CHECK_EQUAL(ecf::cfg::path_of(Key("a.b@c")), "/a/b/c");
    BOOST_CHECK_EQUAL(ecf::cfg::path_of(Key("ecflow.server.variables@ECF_PORT")), "/ecflow/server/variables/ECF_PORT");
}

BOOST_AUTO_TEST_CASE(test_setting_refuses_name_in_file_other_than_its_path) {
    ECF_NAME_THIS_TEST();

    // The name in <host>.<port>.ecf.cfg is the path designated by the key
    for (const std::string name : {"/ecflow/server/variables/ECF_HOME",
                                   "ecflow/server/variables/ECF_PORT",
                                   "/ecflow/server/variables/ecf_port",
                                   "/ecflow/server/variables@ECF_PORT",
                                   "ecflow.server.variables@ECF_PORT",
                                   ""}) {
        BOOST_TEST_CONTEXT("name: '" << name << "'") {
            const Names names{std::nullopt, name, std::nullopt, std::nullopt};
            check_error([&] { Setting(port_key, Type::Integer, std::nullopt, names); },
                        "/ecflow/server/variables/ECF_PORT");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_catalogue_refuses_ambiguous_keys) {
    ECF_NAME_THIS_TEST();

    auto boolean = [](const std::string& key) { return Setting(Key(key), Type::Boolean, std::nullopt, Names{}); };

    // The same place
    check_error([&] { Catalogue({boolean("a.b.c"), boolean("a.b@c")}); }, "'a.b.c' and 'a.b@c'");
    // A place inside another
    check_error([&] { Catalogue({boolean("a.b"), boolean("a.b.c")}); }, "'a.b' and 'a.b.c'");
    check_error([&] { Catalogue({boolean("a.b"), boolean("a.b@c")}); }, "'a.b' and 'a.b@c'");
    check_error([&] { Catalogue({boolean("a@b"), boolean("a.b.c")}); }, "'a@b' and 'a.b.c'");
    check_error([&] { Catalogue({boolean("a.b"), boolean("a.b_c"), boolean("a.b.c.d")}); }, "'a.b' and 'a.b.c.d'");

    // Places side by side are accepted
    const Catalogue catalogue({boolean("a.b"), boolean("a.bc"), boolean("a.b_c"), boolean("a.c@b"), boolean("a@bd")});
    BOOST_CHECK_EQUAL(catalogue.settings().size(), 5U);
}

BOOST_AUTO_TEST_CASE(test_defaults_layer) {
    ECF_NAME_THIS_TEST();

    // Only the settings with a default are in the layer
    const Catalogue catalogue(
        {port_setting(), Setting(Key("ecflow.server.http"), Type::Boolean, std::nullopt, Names{})});
    const auto layer = ecf::cfg::defaults_layer(catalogue);
    BOOST_CHECK_EQUAL(layer.name, std::string(ecf::cfg::layers::defaults));
    BOOST_REQUIRE_EQUAL(layer.entries.size(), 1U);
    BOOST_CHECK(layer.entries.at(port_key) == value_of<std::int64_t>(3141));
}

BOOST_AUTO_TEST_CASE(test_catalogue_parses_and_checks_by_key) {
    ECF_NAME_THIS_TEST();

    const Catalogue catalogue({port_setting()});
    BOOST_CHECK(catalogue.parse(port_key, "3142") == value_of<std::int64_t>(3142));
    catalogue.check(port_key, value_of<std::int64_t>(3142));
    check_error([&] { (void)catalogue.parse(port_key, "80"); }, "ECF_PORT");

    // An unknown key is refused (R11)
    check_error([&] { (void)catalogue.parse(Key("ecflow.server.unknown"), "1"); }, "ecflow.server.unknown");
    check_error([&] { catalogue.check(Key("ecflow.server.unknown"), value_of(true)); }, "ecflow.server.unknown");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
