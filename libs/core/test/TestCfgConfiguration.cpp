// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/Configuration.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::Configuration;
using ecf::cfg::ConfigurationError;
using ecf::cfg::Key;
using ecf::cfg::Layer;
using ecf::cfg::Value;
using ecf::cfg::detail::is_settable_v;

namespace layers = ecf::cfg::layers;

namespace {

///
/// @brief Creates an empty configuration with the layers of the server, from lowest to highest precedence.
///
/// @return The configuration.
///
Configuration make_configuration() {
    return Configuration({Layer(layers::defaults),
                          Layer(layers::env_cfg),
                          Layer(layers::ecf_cfg),
                          Layer(layers::env),
                          Layer(layers::cli)});
}

///
/// @brief Checks that the given callable throws a ConfigurationError whose message contains the given fragment.
///
/// @tparam F Callable taking no arguments
/// @param[in] f The callable expected to throw
/// @param[in] fragment Text expected in the message of the error
///
///
/// @brief Creates the value expected from a configuration.
///
/// @tparam T The type of the value, stored as is in the alternative of the same type
/// @param[in] value The value
/// @return The optional value, as returned by Configuration::get.
///
template <typename T>
std::optional<Value> expected(T value) {
    return Value(std::in_place_type<T>, std::move(value));
}

template <typename F>
void check_error(F f, const std::string& fragment) {
    BOOST_CHECK_EXCEPTION(f(), ConfigurationError, [&](const ConfigurationError& e) {
        const std::string message = e.what();
        BOOST_TEST_INFO("message: " << message);
        return message.find(fragment) != std::string::npos;
    });
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgConfiguration)

//
// Keys
//

BOOST_AUTO_TEST_CASE(test_key_of_option) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"protocol", "log.extras", "server.tcp_protocol"}) {
        BOOST_TEST_CONTEXT("key: " << text) {
            for (const auto& key : {Key(text), Key::for_option(text), Key::for_option(std::string_view(text))}) {
                BOOST_CHECK_EQUAL(key.key(), text);
                BOOST_CHECK(!key.is_variable());
                const auto [path, name] = key.components();
                BOOST_CHECK_EQUAL(path, text);
                BOOST_CHECK_EQUAL(name, "");
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_key_of_variable) {
    ECF_NAME_THIS_TEST();

    const std::string_view path_view("server.variables");
    const std::string_view name_view("ECF_PORT");
    for (const auto& key : {Key("server.variables@ECF_PORT"),
                            Key::for_variable("server.variables", "ECF_PORT"),
                            Key::for_variable(path_view, name_view)}) {
        BOOST_CHECK_EQUAL(key.key(), "server.variables@ECF_PORT");
        BOOST_CHECK(key.is_variable());
        const auto [path, name] = key.components();
        BOOST_CHECK_EQUAL(path, "server.variables");
        BOOST_CHECK_EQUAL(name, "ECF_PORT");
    }
}

BOOST_AUTO_TEST_CASE(test_key_comparison) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(Key("log.extras") == Key::for_option("log.extras"));
    BOOST_CHECK(Key("a@ECF_PORT") == Key::for_variable("a", "ECF_PORT"));
    BOOST_CHECK(Key("a@ECF_PORT") != Key("b@ECF_PORT"));
    BOOST_CHECK(Key("a@ECF_PORT") != Key("a@ECF_HOME"));
    BOOST_CHECK(Key("a") < Key("b"));
}

BOOST_AUTO_TEST_CASE(test_key_rejects_malformed_text) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"",
                                   "Protocol",
                                   "log..extras",
                                   "log.extras.",
                                   ".log",
                                   "log extras",
                                   "log#extras",
                                   "@ECF_PORT",
                                   "server@",
                                   "server@ECF@PORT",
                                   "server@ECF PORT",
                                   "server@ECF.PORT",
                                   "Server@ECF_PORT",
                                   "server..variables@ECF_PORT"}) {
        BOOST_TEST_CONTEXT("key: '" << text << "'") {
            check_error([&] { (void)Key(text); }, "key");
        }
    }
    check_error([] { (void)Key::for_option("log@extras"); }, "key");
    check_error([] { (void)Key::for_variable("server", "ECF@PORT"); }, "key");
    check_error([] { (void)Key::for_variable("", "ECF_PORT"); }, "key");
}

//
// Construction
//

BOOST_AUTO_TEST_CASE(test_layer_names_of_ecflow) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(layers::defaults, "defaults");
    BOOST_CHECK_EQUAL(layers::env_cfg, "env.cfg");
    BOOST_CHECK_EQUAL(layers::ecf_cfg, "ecf.cfg");
    BOOST_CHECK_EQUAL(layers::env, "env");
    BOOST_CHECK_EQUAL(layers::cli, "cli");
}

BOOST_AUTO_TEST_CASE(test_layer_holds_name_and_entries) {
    ECF_NAME_THIS_TEST();

    const Layer empty(layers::env);
    BOOST_CHECK_EQUAL(empty.name, "env");
    BOOST_CHECK(empty.entries.empty());

    const Layer filled(layers::defaults, {{Key("server.variables@ECF_PORT"), std::int64_t{3141}}});
    BOOST_CHECK_EQUAL(filled.name, "defaults");
    BOOST_REQUIRE_EQUAL(filled.entries.size(), 1U);
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(filled.entries.at(Key("server.variables@ECF_PORT"))), 3141);
}

BOOST_AUTO_TEST_CASE(test_configuration_starts_from_given_layers) {
    ECF_NAME_THIS_TEST();

    const Key port("server.variables@ECF_PORT");
    Configuration configuration({Layer(layers::defaults, {{port, std::int64_t{3141}}}), Layer(layers::cli)});
    BOOST_CHECK(configuration.get(port) == expected<std::int64_t>(3141));
    BOOST_CHECK(configuration.source(port) == std::string(layers::defaults));

    configuration.set(layers::cli, port, 3144);
    BOOST_CHECK(configuration.get(port) == expected<std::int64_t>(3144));
    BOOST_CHECK(configuration.source(port) == std::string(layers::cli));
}

BOOST_AUTO_TEST_CASE(test_configuration_rejects_invalid_layers) {
    ECF_NAME_THIS_TEST();

    check_error([] { Configuration({}); }, "layer");
    check_error([] { Configuration({Layer("default"), Layer("environment"), Layer("default")}); }, "default");
    check_error([] { Configuration({Layer("")}); }, "layer");
}

BOOST_AUTO_TEST_CASE(test_configuration_accepts_any_layer_name) {
    ECF_NAME_THIS_TEST();

    // No convention is imposed on the names of the layers: any non-empty, unique name is accepted
    for (const std::string_view name : {"Bad", "Env", "env..cfg", ".cfg", "env.", "env cfg", "env#cfg", "env@cfg"}) {
        BOOST_TEST_CONTEXT("layer: '" << name << "'") {
            Configuration configuration({Layer(name)});
            configuration.set(name, Key("log.extras"), true);
            BOOST_CHECK(configuration.source(Key("log.extras")) == std::string(name));
        }
    }
}

//
// Setting and getting values
//

BOOST_AUTO_TEST_CASE(test_set_and_get_typed_values) {
    ECF_NAME_THIS_TEST();

    auto configuration = make_configuration();
    configuration.set(layers::env, Key("server.variables@ECF_PORT"), std::int64_t{3143});
    configuration.set(layers::ecf_cfg, Key("log.extras"), true);
    configuration.set(layers::defaults, Key("server.variables@ECF_HOME"), std::string("."));

    BOOST_CHECK(configuration.get(Key("server.variables@ECF_PORT")) == expected<std::int64_t>(3143));
    BOOST_CHECK(configuration.get(Key("log.extras")) == expected(true));
    BOOST_CHECK(configuration.get(Key("server.variables@ECF_HOME")) == expected<std::string>("."));
}

// Each argument type is stored as exactly one alternative of Value; other types are refused at compile time
static_assert(is_settable_v<Value>);
static_assert(is_settable_v<bool>);
static_assert(is_settable_v<int>);
static_assert(is_settable_v<long>);
static_assert(is_settable_v<std::int64_t>);
static_assert(is_settable_v<unsigned>);
static_assert(is_settable_v<std::uint32_t>);
static_assert(is_settable_v<const char*>);
static_assert(is_settable_v<const char (&)[4]>);
static_assert(is_settable_v<std::string>);
static_assert(is_settable_v<const std::string&>);
static_assert(is_settable_v<std::string_view>);
static_assert(!is_settable_v<char>);
static_assert(!is_settable_v<signed char>);
static_assert(!is_settable_v<unsigned char>);
static_assert(!is_settable_v<std::uint64_t>);
static_assert(!is_settable_v<std::size_t>);
static_assert(!is_settable_v<double>);
static_assert(!is_settable_v<std::vector<int>>);

BOOST_AUTO_TEST_CASE(test_set_converts_each_argument_to_one_alternative) {
    ECF_NAME_THIS_TEST();

    auto configuration = make_configuration();
    const std::string text("text");
    const std::string_view view("view");
    configuration.set(layers::defaults, Key("int"), 3143);
    configuration.set(layers::defaults, Key("long"), 3143L);
    configuration.set(layers::defaults, Key("int64"), std::int64_t{-1});
    configuration.set(layers::defaults, Key("unsigned"), std::uint32_t{4294967295U});
    configuration.set(layers::defaults, Key("bool"), false);
    configuration.set(layers::defaults, Key("literal"), "%");
    configuration.set(layers::defaults, Key("string"), text);
    configuration.set(layers::defaults, Key("view"), view);
    configuration.set(layers::defaults, Key("value"), Value{std::int64_t{7}});

    BOOST_CHECK(configuration.get(Key("int")) == expected<std::int64_t>(3143));
    BOOST_CHECK(configuration.get(Key("long")) == expected<std::int64_t>(3143));
    BOOST_CHECK(configuration.get(Key("int64")) == expected<std::int64_t>(-1));
    BOOST_CHECK(configuration.get(Key("unsigned")) == expected<std::int64_t>(4294967295LL));
    BOOST_CHECK(configuration.get(Key("bool")) == expected(false));
    BOOST_CHECK(configuration.get(Key("literal")) == expected<std::string>("%"));
    BOOST_CHECK(configuration.get(Key("string")) == expected<std::string>("text"));
    BOOST_CHECK(configuration.get(Key("view")) == expected<std::string>("view"));
    BOOST_CHECK(configuration.get(Key("value")) == expected<std::int64_t>(7));
}

BOOST_AUTO_TEST_CASE(test_set_text_is_never_taken_as_boolean) {
    ECF_NAME_THIS_TEST();

    // A string literal converts to bool before std::string; the value must stay text
    auto configuration = make_configuration();
    configuration.set(layers::defaults, Key("server.variables@ECF_MICRODEF"), "%");

    BOOST_CHECK(configuration.get(Key("server.variables@ECF_MICRODEF")) == expected<std::string>("%"));
}

BOOST_AUTO_TEST_CASE(test_set_twice_in_layer_is_refused) {
    ECF_NAME_THIS_TEST();

    auto configuration = make_configuration();
    configuration.set(layers::ecf_cfg, Key("server.variables@ECF_PORT"), std::int64_t{3142});
    check_error([&] { configuration.set(layers::ecf_cfg, Key("server.variables@ECF_PORT"), std::int64_t{3143}); },
                "server.variables@ECF_PORT");
    BOOST_CHECK(configuration.get(Key("server.variables@ECF_PORT")) == expected<std::int64_t>(3142));
}

BOOST_AUTO_TEST_CASE(test_set_in_unknown_layer_is_refused) {
    ECF_NAME_THIS_TEST();

    auto configuration = make_configuration();
    check_error([&] { configuration.set("unknown", Key("log.extras"), true); }, "unknown");
}

BOOST_AUTO_TEST_CASE(test_effective_value_comes_from_highest_layer) {
    ECF_NAME_THIS_TEST();

    // For each combination of layers holding the key, the highest one wins, for variables and options alike
    const std::vector<std::string_view> names{
        layers::defaults, layers::env_cfg, layers::ecf_cfg, layers::env, layers::cli};
    for (const auto& key : {Key("server.variables@ECF_HOME"), Key("log.extras")}) {
        for (unsigned mask = 1; mask < (1U << names.size()); ++mask) {
            BOOST_TEST_CONTEXT("key: " << key.key() << ", layer mask: " << mask) {
                auto configuration = make_configuration();
                std::string highest;
                for (std::size_t i = 0; i < names.size(); ++i) {
                    if (mask & (1U << i)) {
                        configuration.set(names[i], key, std::string(names[i]) + " value");
                        highest = names[i];
                    }
                }
                BOOST_CHECK(configuration.get(key) == expected<std::string>(highest + " value"));
                BOOST_CHECK(configuration.source(key) == highest);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_value_of_selected_layer_ignores_precedence) {
    ECF_NAME_THIS_TEST();

    auto configuration = make_configuration();
    const Key key("server.variables@ECF_SSL");
    configuration.set(layers::defaults, key, false);
    configuration.set(layers::env, key, true);

    BOOST_CHECK(configuration.get(key) == expected(true));
    BOOST_CHECK(configuration.get(key, layers::defaults) == expected(false));
    BOOST_CHECK(configuration.get(key, layers::env) == expected(true));
    BOOST_CHECK(!configuration.get(key, layers::ecf_cfg).has_value());
    // An unknown layer is an error, not an absent value
    check_error([&] { (void)configuration.get(key, "unknown"); }, "unknown");
}

BOOST_AUTO_TEST_CASE(test_layer_filled_later_takes_its_precedence) {
    ECF_NAME_THIS_TEST();

    // A layer filled after another layer keeps its own place in the precedence (R6/R7)
    auto configuration = make_configuration();
    const Key interval("server.variables@ECF_INTERVAL");
    const Key port("server.variables@ECF_PORT");
    configuration.set(layers::defaults, interval, std::int64_t{60});
    configuration.set(layers::env, port, std::int64_t{3143});
    BOOST_CHECK(configuration.get(interval) == expected<std::int64_t>(60));

    configuration.set(layers::ecf_cfg, interval, 17);
    configuration.set(layers::ecf_cfg, port, 3142);
    BOOST_CHECK(configuration.get(interval) == expected<std::int64_t>(17));
    BOOST_CHECK(configuration.source(interval) == std::string(layers::ecf_cfg));
    BOOST_CHECK(configuration.get(port) == expected<std::int64_t>(3143));
    BOOST_CHECK(configuration.source(port) == std::string(layers::env));
}

BOOST_AUTO_TEST_CASE(test_missing_key) {
    ECF_NAME_THIS_TEST();

    const auto configuration = make_configuration();
    const Key key("server.variables@ECF_PORT");
    BOOST_CHECK(!configuration.get(key).has_value());
    BOOST_CHECK(!configuration.get(key, layers::defaults).has_value());
    BOOST_CHECK(!configuration.source(key).has_value());
}

BOOST_AUTO_TEST_CASE(test_text_holding_a_number_stays_text) {
    ECF_NAME_THIS_TEST();

    // No conversion takes place: text holding a number is not an integer
    auto configuration = make_configuration();
    const Key key("server.variables@ECF_PORT");
    configuration.set(layers::ecf_cfg, key, "3142");
    BOOST_CHECK(configuration.get(key) == expected<std::string>("3142"));
}

BOOST_AUTO_TEST_CASE(test_keys_lists_every_key_once) {
    ECF_NAME_THIS_TEST();

    auto configuration = make_configuration();
    configuration.set(layers::defaults, Key("server.variables@ECF_PORT"), std::int64_t{3141});
    configuration.set(layers::defaults, Key("server.variables@ECF_HOME"), ".");
    configuration.set(layers::cli, Key("server.variables@ECF_PORT"), std::int64_t{3144});
    configuration.set(layers::ecf_cfg, Key("log.extras"), true);

    std::vector<std::string> actual;
    for (const auto& key : configuration.keys()) {
        actual.push_back(key.key());
    }
    // Ordered by key, whatever the layers holding them
    const std::vector<std::string> expected{"log.extras", "server.variables@ECF_HOME", "server.variables@ECF_PORT"};
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
