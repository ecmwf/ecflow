// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/EnvReader.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::cfg::Catalogue;
using ecf::cfg::ConfigurationError;
using ecf::cfg::EnvReader;
using ecf::cfg::Key;
using ecf::cfg::Layer;
using ecf::cfg::Names;
using ecf::cfg::Setting;
using ecf::cfg::Type;
using ecf::cfg::Value;
using ecf::test::scaffold::WithoutTestEnvironmentVariable;
using ecf::test::scaffold::WithTestEnvironmentVariable;

namespace checks = ecf::cfg::checks;
namespace layers = ecf::cfg::layers;

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
/// @brief Creates a catalogue with an integer, a text and a switch named in the environment, and a text that is not.
///
/// @return The catalogue.
///
Catalogue test_catalogue() {
    return Catalogue({
        Setting(Key("test.variables@PORT"),
                Type::Integer,
                value_of<std::int64_t>(3141),
                Names{std::nullopt, std::nullopt, "ECF_TEST_CFG_PORT", std::nullopt},
                checks::range(1024, 49150)),
        Setting(Key("test.variables@HOME"),
                Type::Text,
                std::nullopt,
                Names{std::nullopt, std::nullopt, "ECF_TEST_CFG_HOME", std::nullopt},
                checks::non_empty()),
        // Enabled when its variable is defined, whatever its text
        Setting(Key("test.variables@DEBUG"),
                Type::Boolean,
                value_of(false),
                Names{std::nullopt, std::nullopt, "ECF_TEST_CFG_DEBUG", std::nullopt},
                {},
                [](std::string_view) { return Value(true); }),
        Setting(Key("test.variables@LOG"),
                Type::Text,
                std::nullopt,
                Names{"ECF_TEST_CFG_LOG", std::nullopt, std::nullopt, std::nullopt}),
    });
}

///
/// @brief Reads the environment with a reader of the test catalogue.
///
/// @return The layer.
///
Layer read() {
    const auto catalogue = test_catalogue();
    return EnvReader(catalogue).read();
}

///
/// @brief Returns the message of the ConfigurationError thrown when reading the environment.
///
/// @return The message, or `(no error)` when the environment is read without error.
///
std::string error_of_read() {
    try {
        (void)read();
    }
    catch (const ConfigurationError& e) {
        return e.what();
    }
    return "(no error)";
}

///
/// @brief Clears every variable of the test catalogue for the duration of a test.
///
struct WithoutTestVariables
{
    WithoutTestEnvironmentVariable port{"ECF_TEST_CFG_PORT"};
    WithoutTestEnvironmentVariable home{"ECF_TEST_CFG_HOME"};
    WithoutTestEnvironmentVariable debug{"ECF_TEST_CFG_DEBUG"};
    WithoutTestEnvironmentVariable log{"ECF_TEST_CFG_LOG"};
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgEnvReader)

//
// Reading
//

BOOST_AUTO_TEST_CASE(test_read_defined_variables) {
    ECF_NAME_THIS_TEST();

    const WithoutTestVariables clean;
    const WithTestEnvironmentVariable port("ECF_TEST_CFG_PORT", "3142");
    const WithTestEnvironmentVariable home("ECF_TEST_CFG_HOME", "/home");

    const auto layer = read();
    BOOST_CHECK_EQUAL(layer.name, std::string(layers::env));
    BOOST_REQUIRE_EQUAL(layer.entries.size(), 2U);
    BOOST_CHECK(layer.entries.at(Key("test.variables@PORT")) == value_of<std::int64_t>(3142));
    BOOST_CHECK(layer.entries.at(Key("test.variables@HOME")) == value_of<std::string>("/home"));
}

BOOST_AUTO_TEST_CASE(test_read_only_the_variables_defined) {
    ECF_NAME_THIS_TEST();

    const WithoutTestVariables clean;

    const auto layer = read();
    BOOST_CHECK_EQUAL(layer.name, std::string(layers::env));
    BOOST_CHECK(layer.entries.empty());
}

BOOST_AUTO_TEST_CASE(test_read_with_custom_rule) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"", "0", "false", "1"}) {
        BOOST_TEST_CONTEXT("ECF_TEST_CFG_DEBUG: '" << text << "'") {
            const WithoutTestVariables clean;
            const WithTestEnvironmentVariable debug("ECF_TEST_CFG_DEBUG", text);

            const auto layer = read();
            BOOST_REQUIRE_EQUAL(layer.entries.size(), 1U);
            BOOST_CHECK(layer.entries.at(Key("test.variables@DEBUG")) == value_of(true));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_ignore_settings_without_variable) {
    ECF_NAME_THIS_TEST();

    // Only the settings named in the environment are read from the environment
    const WithoutTestVariables clean;
    const WithTestEnvironmentVariable log("ECF_TEST_CFG_LOG", "server.log");

    BOOST_CHECK(read().entries.empty());
}

//
// Refusing
//

BOOST_AUTO_TEST_CASE(test_refuse_values_of_settings) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::pair<std::string, std::string>> ports{
        {"3141x", "Environment variable ECF_TEST_CFG_PORT: invalid value '3141x': expected an integer"},
        {"80", "Environment variable ECF_TEST_CFG_PORT: invalid value '80': expected a value in [1024, 49150]"},
        {"", "Environment variable ECF_TEST_CFG_PORT: invalid value '': expected an integer"},
        {" 3141", "Environment variable ECF_TEST_CFG_PORT: invalid value ' 3141': expected an integer"},
    };
    for (const auto& [text, expected] : ports) {
        BOOST_TEST_CONTEXT("ECF_TEST_CFG_PORT: '" << text << "'") {
            const WithoutTestVariables clean;
            const WithTestEnvironmentVariable port("ECF_TEST_CFG_PORT", text);
            BOOST_CHECK_EQUAL(error_of_read(), expected);
        }
    }

    const WithoutTestVariables clean;
    const WithTestEnvironmentVariable home("ECF_TEST_CFG_HOME", "");
    BOOST_CHECK_EQUAL(error_of_read(),
                      "Environment variable ECF_TEST_CFG_HOME: invalid value '': expected a non-empty value");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
