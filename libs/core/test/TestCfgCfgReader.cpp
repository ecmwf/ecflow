// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/CfgReader.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::cfg::Catalogue;
using ecf::cfg::CfgReader;
using ecf::cfg::ConfigurationError;
using ecf::cfg::Key;
using ecf::cfg::Layer;
using ecf::cfg::Names;
using ecf::cfg::Setting;
using ecf::cfg::Type;
using ecf::cfg::Value;

namespace checks   = ecf::cfg::checks;
namespace layers   = ecf::cfg::layers;
namespace scaffold = ecf::test::scaffold;

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
/// @brief Creates a catalogue with an integer and a text named in the file, and a boolean that is not.
///
/// @return The catalogue.
///
Catalogue test_catalogue() {
    return Catalogue({
        Setting(Key("test.variables@PORT"),
                Type::Integer,
                value_of<std::int64_t>(3141),
                Names{"TEST_PORT", std::nullopt, std::nullopt, std::nullopt},
                checks::range(1024, 49150)),
        Setting(Key("test.variables@HOME"),
                Type::Text,
                std::nullopt,
                Names{"TEST_HOME", std::nullopt, std::nullopt, std::nullopt},
                checks::non_empty()),
        Setting(Key("test.variables@DEBUG"),
                Type::Boolean,
                value_of(false),
                Names{std::nullopt, std::nullopt, "TEST_DEBUG", std::nullopt}),
    });
}

///
/// @brief Reads a file of the given content with a reader of the test catalogue.
///
/// @param[in] content The content of the file
/// @return The layer.
///
Layer read(const std::string& content) {
    const scaffold::WithTestFile file(scaffold::AutomaticTestFile{"server_environment.cfg"}, content);
    const auto catalogue = test_catalogue();
    return CfgReader(catalogue).read(file.path());
}

///
/// @brief Returns the message of the ConfigurationError thrown when reading the given file.
///
/// @param[in] file The file
/// @return The message, or `(no error)` when the file is read without error.
///
std::string error_of(const std::filesystem::path& file) {
    const auto catalogue = test_catalogue();
    try {
        (void)CfgReader(catalogue).read(file);
    }
    catch (const ConfigurationError& e) {
        return e.what();
    }
    return "(no error)";
}

///
/// @brief Returns the message of the ConfigurationError thrown when reading a file of the given content.
///
/// @param[in] content The content of the file
/// @param[out] path The path of the file, as named in the message
/// @return The message, or `(no error)` when the file is read without error.
///
std::string error_of(const std::string& content, std::string& path) {
    const scaffold::WithTestFile file(scaffold::AutomaticTestFile{"server_environment.cfg"}, content);
    path = file.path().string();
    return error_of(file.path());
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgCfgReader)

//
// Reading
//

BOOST_AUTO_TEST_CASE(test_read_settings) {
    ECF_NAME_THIS_TEST();

    const auto layer = read("TEST_PORT = 3142\nTEST_HOME=/home  # The home\n");

    BOOST_CHECK_EQUAL(layer.name, std::string(layers::env_cfg));
    BOOST_REQUIRE_EQUAL(layer.entries.size(), 2U);
    BOOST_CHECK(layer.entries.at(Key("test.variables@PORT")) == value_of<std::int64_t>(3142));
    BOOST_CHECK(layer.entries.at(Key("test.variables@HOME")) == value_of<std::string>("/home"));
}

BOOST_AUTO_TEST_CASE(test_read_only_the_settings_present) {
    ECF_NAME_THIS_TEST();

    // Neither defaults nor anything else is added: the layer holds what the file states
    for (const std::string content : {"", "\n", "# Only a comment\n"}) {
        BOOST_TEST_CONTEXT("content: '" << content << "'") {
            const auto layer = read(content);
            BOOST_CHECK_EQUAL(layer.name, std::string(layers::env_cfg));
            BOOST_CHECK(layer.entries.empty());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_read_missing_file_as_empty) {
    ECF_NAME_THIS_TEST();

    const auto catalogue = test_catalogue();
    const auto layer     = CfgReader(catalogue).read("missing.server_environment.cfg");
    BOOST_CHECK_EQUAL(layer.name, std::string(layers::env_cfg));
    BOOST_CHECK(layer.entries.empty());
}

//
// Refusing
//

BOOST_AUTO_TEST_CASE(test_refuse_values_of_settings) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::pair<std::string, std::string>> cases{
        {"TEST_PORT = 3141x\n", "TEST_PORT: invalid value '3141x': expected an integer"},
        {"TEST_PORT = 80\n", "TEST_PORT: invalid value '80': expected a value in [1024, 49150]"},
        {"TEST_PORT =\n", "TEST_PORT: invalid value '': expected an integer"},
        {"TEST_HOME =\n", "TEST_HOME: invalid value '': expected a non-empty value"},
        {"TEST_HOME = /home\nTEST_PORT = 99999999999999999999\n",
         "TEST_PORT: invalid value '99999999999999999999': expected an integer"},
    };
    for (const auto& [content, reason] : cases) {
        BOOST_TEST_CONTEXT("content: " << content) {
            std::string path;
            const auto message = error_of(content, path);
            BOOST_CHECK_EQUAL(message, "Could not parse server configuration file '" + path + "': " + reason);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_malformed_files) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::pair<std::string, std::string>> cases{
        {"UNKNOWN = 1\n", "UNKNOWN"},
        // Only the settings named in the file can be set in the file
        {"TEST_DEBUG = true\n", "TEST_DEBUG"},
        {"TEST_PORT = 3141\nTEST_PORT = 3142\n", "TEST_PORT"},
        {"[unterminated\n", "unterminated"},
    };
    for (const auto& [content, fragment] : cases) {
        BOOST_TEST_CONTEXT("content: " << content) {
            std::string path;
            const auto message = error_of(content, path);
            BOOST_TEST_INFO("message: " << message);
            BOOST_CHECK(message.rfind("Could not parse server configuration file '" + path + "': ", 0) == 0);
            BOOST_CHECK(message.find(fragment) != std::string::npos);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_directory) {
    ECF_NAME_THIS_TEST();

    const scaffold::MakeDirectory make;
    const auto directory = make.create();
    BOOST_CHECK_EQUAL(error_of(directory.path()),
                      "Could not parse server configuration file '" + directory.path().string() +
                          "': Configuration path is not a regular file");
}

BOOST_AUTO_TEST_CASE(test_refuse_unreadable_file) {
    ECF_NAME_THIS_TEST();

    const scaffold::WithTestFile file(scaffold::AutomaticTestFile{"server_environment.cfg"}, "TEST_PORT = 3142\n");
    std::filesystem::permissions(file.path(), std::filesystem::perms::none);

    if (std::ifstream(file.path())) {
        // e.g. when running as root, permissions do not prevent reading
        BOOST_TEST_MESSAGE("Skipped: the file remains readable without permissions");
    }
    else {
        BOOST_CHECK_EQUAL(error_of(file.path()),
                          "Could not parse server configuration file '" + file.path().string() +
                              "': Cannot open configuration file for reading");
    }

    std::filesystem::permissions(file.path(), std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
