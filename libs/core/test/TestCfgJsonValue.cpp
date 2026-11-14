// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/Catalogue.hpp"
#include "ecflow/core/cfg/Configuration.hpp"
#include "ecflow/core/cfg/JsonReader.hpp"
#include "ecflow/core/cfg/Mapper.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::Catalogue;
using ecf::cfg::Configuration;
using ecf::cfg::ConfigurationError;
using ecf::cfg::InvalidValue;
using ecf::cfg::Json;
using ecf::cfg::JsonReader;
using ecf::cfg::Key;
using ecf::cfg::Layer;
using ecf::cfg::Mapper;
using ecf::cfg::Names;
using ecf::cfg::Setting;
using ecf::cfg::Type;
using ecf::cfg::Value;

namespace layers = ecf::cfg::layers;

namespace {

const std::string origin = "host.3141.ecf.cfg";

const Key list_key("ecflow.server.list");
const Key port_key("ecflow.server.variables@ECF_PORT");

///
/// @brief Creates a catalogue with a structured setting, available in every layer, and an integer setting.
///
/// @return The catalogue.
///
Catalogue test_catalogue() {
    return Catalogue({
        Setting(list_key, Type::Json, std::nullopt, Names{"ECF_LIST", "/ecflow/server/list", "ECF_LIST", "list"}),
        Setting(port_key,
                Type::Integer,
                Value(std::in_place_type<std::int64_t>, 3141),
                Names{std::nullopt, "/ecflow/server/variables/ECF_PORT", std::nullopt, std::nullopt}),
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
/// @brief Checks that mapping the given JSON text is refused with a message holding the given fragment.
///
/// @param[in] text The JSON text
/// @param[in] fragment A text expected in the message of the error
///
void check_refused(const std::string& text, const std::string& fragment) {
    BOOST_TEST_CONTEXT("text: " << text) {
        BOOST_CHECK_EXCEPTION((void)map(text), ConfigurationError, [&](const ConfigurationError& e) {
            BOOST_TEST_INFO("message: " << e.what());
            return std::string(e.what()).find(fragment) != std::string::npos;
        });
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgJsonValue)

///
/// @brief Verifies that an array read from the configuration file is held as compact JSON text, in document order.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_maps_an_array) {
    ECF_NAME_THIS_TEST();

    const auto layer = map(R"({
        "version": 1,
        "ecflow": { "server": {
            "list": [ { "user": "bob", "permissions": "r" }, { "user": "alice", "permissions": "rwx", "n": 1.5 } ],
            "variables": { "ECF_PORT": 3142 }
        } }
    })");

    BOOST_REQUIRE_EQUAL(layer.entries.size(), 2U);
    BOOST_CHECK(layer.entries.at(list_key) ==
                Value(Json{R"([{"user":"bob","permissions":"r"},{"user":"alice","permissions":"rwx","n":1.5}])"}));
    BOOST_CHECK(layer.entries.at(port_key) == Value(std::in_place_type<std::int64_t>, 3142));
}

///
/// @brief Verifies that an object, nested values, strings needing escapes, null and booleans are kept.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_maps_an_object) {
    ECF_NAME_THIS_TEST();

    const auto layer = map(R"({ "version": 1, "ecflow": { "server": {
        "list": { "b": [true, null, -2], "a": "x\"y\\zé" } } } })");

    BOOST_CHECK(layer.entries.at(list_key) == Value(Json{"{\"b\":[true,null,-2],\"a\":\"x\\\"y\\\\z\xc3\xa9\"}"}));
}

///
/// @brief Verifies that empty arrays and objects, alone or nested, are kept apart from each other and from null.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_empty_containers) {
    ECF_NAME_THIS_TEST();

    const auto mapped = [](const std::string& value) {
        return map(R"({ "version": 1, "ecflow": { "server": { "list": )" + value + " } } }").entries.at(list_key);
    };
    BOOST_CHECK(mapped("[]") == Value(Json{"[]"}));
    BOOST_CHECK(mapped("{ }") == Value(Json{"{}"}));
    BOOST_CHECK(mapped("[ [ ], { }, null, { \"a\": [] } ]") == Value(Json{R"([[],{},null,{"a":[]}])"}));

    const auto catalogue = test_catalogue();
    BOOST_CHECK(catalogue.parse(list_key, " [ ] ") == Value(Json{"[]"}));
    BOOST_CHECK(catalogue.parse(list_key, "{}") == Value(Json{"{}"}));
    BOOST_CHECK(catalogue.parse(list_key, R"({"a": {}, "b": [[]]})") == Value(Json{R"({"a":{},"b":[[]]})"}));
}

///
/// @brief Verifies that a structured value is stored as such by Configuration::set, whatever the argument category.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_set_and_get) {
    ECF_NAME_THIS_TEST();

    Configuration configuration({Layer(layers::defaults), Layer(layers::ecf_cfg), Layer(layers::cli)});

    Json lvalue{"[1]"};
    const Json const_lvalue{"[2]"};
    configuration.set(layers::defaults, list_key, lvalue);
    BOOST_CHECK(configuration.get(list_key) == Value(Json{"[1]"}));
    configuration.set(layers::ecf_cfg, list_key, const_lvalue);
    BOOST_CHECK(configuration.get(list_key) == Value(Json{"[2]"}));
    configuration.set(layers::cli, list_key, Json{"[3]"});
    const auto value = configuration.get(list_key);
    BOOST_REQUIRE(value.has_value());
    BOOST_REQUIRE(std::holds_alternative<Json>(*value));
    BOOST_CHECK_EQUAL(std::get<Json>(*value).text, "[3]");
    BOOST_CHECK_EQUAL(lvalue.text, "[1]"); // an lvalue is copied, not moved
}

///
/// @brief Verifies that a scalar given for a structured setting, and a structure given for a scalar one, are refused.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_type_errors) {
    ECF_NAME_THIS_TEST();

    check_refused(R"({ "version": 1, "ecflow": { "server": { "list": "alice:r" } } })",
                  "/ecflow/server/list: expected an array or an object, not a string");
    check_refused(R"({ "version": 1, "ecflow": { "server": { "list": 1 } } })",
                  "/ecflow/server/list: expected an array or an object, not an integer");
    check_refused(R"({ "version": 1, "ecflow": { "server": { "list": null } } })",
                  "/ecflow/server/list: expected an array or an object, not null");
    check_refused(R"({ "version": 1, "ecflow": { "server": { "variables": { "ECF_PORT": [3142] } } } })",
                  "/ecflow/server/variables/ECF_PORT: expected an integer, not an array");
}

///
/// @brief Verifies that a structured setting given as text (environment, command line) is parsed and checked.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_parsed_from_text) {
    ECF_NAME_THIS_TEST();

    const auto catalogue = test_catalogue();

    BOOST_CHECK(catalogue.parse(list_key, R"([ {"user": "alice"} ])") == Value(Json{R"([{"user":"alice"}])"}));
    BOOST_CHECK(catalogue.parse(list_key, R"({"a": [1, 2]})") == Value(Json{R"({"a":[1,2]})"}));

    for (const std::string invalid : {"", "alice:r", "[", "[1] x", "\"text\"", "42", "true", "null"}) {
        BOOST_TEST_CONTEXT("text: " << invalid) {
            BOOST_CHECK_THROW((void)catalogue.parse(list_key, invalid), InvalidValue);
        }
    }
}

///
/// @brief Verifies that a structured value is typed: it is refused for other settings, and other values for it.
///
BOOST_AUTO_TEST_CASE(test_cfg_json_value_is_typed) {
    ECF_NAME_THIS_TEST();

    const auto catalogue = test_catalogue();

    BOOST_CHECK_NO_THROW(catalogue.check(list_key, Value(Json{"[]"})));
    BOOST_CHECK_THROW(catalogue.check(list_key, Value(std::in_place_type<std::string>, "[]")), InvalidValue);
    BOOST_CHECK_THROW(catalogue.check(port_key, Value(Json{"[]"})), InvalidValue);
    BOOST_CHECK_THROW(Setting(list_key, Type::Json, Value(std::in_place_type<std::string>, "[]"), Names{}),
                      ConfigurationError);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
