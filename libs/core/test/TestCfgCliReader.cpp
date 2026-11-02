// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/CliReader.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::cfg::Catalogue;
using ecf::cfg::CliReader;
using ecf::cfg::ConfigurationError;
using ecf::cfg::Key;
using ecf::cfg::Layer;
using ecf::cfg::Names;
using ecf::cfg::Setting;
using ecf::cfg::Type;
using ecf::cfg::Value;

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
/// @brief Creates a catalogue with an integer and two switches named on the command line, and a text that is not.
///
/// @return The catalogue.
///
Catalogue test_catalogue() {
    return Catalogue({
        Setting(Key("test.variables@PORT"),
                Type::Integer,
                value_of<std::int64_t>(3141),
                Names{std::nullopt, std::nullopt, std::nullopt, "port"},
                checks::range(1024, 49150)),
        // Enabled when its option is given
        Setting(Key("test.http"),
                Type::Boolean,
                value_of(false),
                Names{std::nullopt, std::nullopt, std::nullopt, "http"},
                {},
                [](std::string_view) { return Value(true); }),
        // Disabled when its option is given
        Setting(Key("test.job_generation"),
                Type::Boolean,
                value_of(true),
                Names{std::nullopt, std::nullopt, std::nullopt, "dis_job_gen"},
                {},
                [](std::string_view) { return Value(false); }),
        Setting(Key("test.variables@HOME"),
                Type::Text,
                std::nullopt,
                Names{std::nullopt, std::nullopt, "ECF_TEST_CFG_HOME", std::nullopt}),
    });
}

///
/// @brief Reads the given options with a reader of the test catalogue.
///
/// @param[in] options The options, by name, with their text
/// @return The layer.
///
Layer read(const std::map<std::string, std::string>& options) {
    const auto catalogue = test_catalogue();
    return CliReader(catalogue).read(options);
}

///
/// @brief Returns the message of the ConfigurationError thrown when reading the given options.
///
/// @param[in] options The options, by name, with their text
/// @return The message, or `(no error)` when the options are read without error.
///
std::string error_of_read(const std::map<std::string, std::string>& options) {
    try {
        (void)read(options);
    }
    catch (const ConfigurationError& e) {
        return e.what();
    }
    return "(no error)";
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgCliReader)

//
// Reading
//

BOOST_AUTO_TEST_CASE(test_read_given_options) {
    ECF_NAME_THIS_TEST();

    const auto layer = read({{"port", "3142"}, {"http", ""}, {"dis_job_gen", ""}});
    BOOST_CHECK_EQUAL(layer.name, std::string(layers::cli));
    BOOST_REQUIRE_EQUAL(layer.entries.size(), 3U);
    BOOST_CHECK(layer.entries.at(Key("test.variables@PORT")) == value_of<std::int64_t>(3142));
    BOOST_CHECK(layer.entries.at(Key("test.http")) == value_of(true));
    BOOST_CHECK(layer.entries.at(Key("test.job_generation")) == value_of(false));
}

BOOST_AUTO_TEST_CASE(test_read_only_the_options_given) {
    ECF_NAME_THIS_TEST();

    const auto layer = read({});
    BOOST_CHECK_EQUAL(layer.name, std::string(layers::cli));
    BOOST_CHECK(layer.entries.empty());
}

BOOST_AUTO_TEST_CASE(test_ignore_options_without_setting) {
    ECF_NAME_THIS_TEST();

    // Informational options, and names of other origins, are not settings of the command line
    BOOST_CHECK(read({{"help", ""}, {"version", ""}, {"ECF_TEST_CFG_HOME", "/home"}}).entries.empty());
}

//
// Refusing
//

BOOST_AUTO_TEST_CASE(test_refuse_values_of_settings) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::pair<std::string, std::string>> ports{
        {"80", "Invalid server command line: --port: invalid value '80': expected a value in [1024, 49150]"},
        {"49151", "Invalid server command line: --port: invalid value '49151': expected a value in [1024, 49150]"},
        {"3141x", "Invalid server command line: --port: invalid value '3141x': expected an integer"},
        {"", "Invalid server command line: --port: invalid value '': expected an integer"},
    };
    for (const auto& [text, expected] : ports) {
        BOOST_TEST_CONTEXT("--port: '" << text << "'") {
            BOOST_CHECK_EQUAL(error_of_read({{"port", text}}), expected);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
