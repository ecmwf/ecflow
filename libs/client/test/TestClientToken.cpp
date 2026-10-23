// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <sstream>
#include <string>

#include <boost/io/ios_state.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf;
using namespace ecf::test::scaffold;

namespace {

///
/// @brief Pings, over HTTP, a local port on which nothing listens, and returns what was written to std::cerr.
///
/// @param[in] port   The (reserved, unused) port to ping
/// @param[in] debug  Whether the client runs in debug mode
/// @return The text written to std::cerr during the request
///
std::string ping_and_capture_errors(const std::string& port, bool debug) {
    std::ostringstream errors;
    {
        boost::io::ios_rdbuf_saver capture(std::cerr, errors.rdbuf());

        ClientInvoker client(ecf::string_constants::localhost, port);
        client.set_throw_on_error(false);
        client.set_connection_attempts(1);
        client.enable_http();
        client.debug(debug);

        BOOST_REQUIRE_MESSAGE(client.pingServer() == 1, "expected the request to fail");
    }
    return errors.str();
}

///
/// @brief Creates a tokens file holding a single basic token, for the servers matching the given pattern.
///
/// @param[in] directory  The directory where the tokens file is created
/// @param[in] server     The regular expression matched against the server URL
/// @return The tokens file, removed on destruction
///
File make_tokens_file(const Directory& directory, const std::string& server) {
    const std::string content = R"({"version": 1, "tokens": [{"type": "basic", "server": ")" + server +
                                R"(", "api": {"username": "someone", "password": "secret"}}]})";
    return MakeTestFile{}.with(SpecificFileLocation{"tokens.ecflowapirc", directory}).with(content).create();
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientToken)

BOOST_AUTO_TEST_CASE(test_missing_tokens_file_is_reported_only_in_debug_mode) {
    ECF_NAME_THIS_TEST();

    auto directory      = MakeDirectory{}.create();
    const auto location = (directory.path() / "missing.ecflowapirc").string();
    WithTestEnvironmentVariable tokens("ECF_AUTHTOKENS", location);

    const auto reserved = MakePort{}.with(AutomaticPortValue{}).create();
    const auto port     = std::to_string(reserved.value());

    {
        const auto errors = ping_and_capture_errors(port, false);
        BOOST_CHECK_MESSAGE(errors.find("token") == std::string::npos, errors);
    }
    {
        const auto errors = ping_and_capture_errors(port, true);
        BOOST_CHECK_MESSAGE(errors.find("token") != std::string::npos, errors);
        BOOST_CHECK_MESSAGE(errors.find(location) != std::string::npos, errors);
    }
}

BOOST_AUTO_TEST_CASE(test_unmatched_token_is_reported_with_the_tokens_file_only_in_debug_mode) {
    ECF_NAME_THIS_TEST();

    auto directory      = MakeDirectory{}.create();
    const auto file     = make_tokens_file(directory, "https://elsewhere:.*");
    const auto location = file.path().string();
    WithTestEnvironmentVariable tokens("ECF_AUTHTOKENS", location);

    const auto reserved = MakePort{}.with(AutomaticPortValue{}).create();
    const auto port     = std::to_string(reserved.value());

    {
        const auto errors = ping_and_capture_errors(port, false);
        BOOST_CHECK_MESSAGE(errors.find("token") == std::string::npos, errors);
    }
    {
        const auto errors = ping_and_capture_errors(port, true);
        const auto url    = "http://" + std::string{ecf::string_constants::localhost} + ":" + port;
        BOOST_CHECK_MESSAGE(errors.find("Unable to find token for URL: " + url + ", in tokens file: " + location) !=
                                std::string::npos,
                            errors);
    }
}

BOOST_AUTO_TEST_CASE(test_matched_token_is_reported_with_the_tokens_file_only_in_debug_mode) {
    ECF_NAME_THIS_TEST();

    auto directory      = MakeDirectory{}.create();
    const auto file     = make_tokens_file(directory, "http://.*");
    const auto location = file.path().string();
    WithTestEnvironmentVariable tokens("ECF_AUTHTOKENS", location);

    const auto reserved = MakePort{}.with(AutomaticPortValue{}).create();
    const auto port     = std::to_string(reserved.value());

    {
        const auto errors = ping_and_capture_errors(port, false);
        BOOST_CHECK_MESSAGE(errors.find("token") == std::string::npos, errors);
    }
    {
        const auto errors = ping_and_capture_errors(port, true);
        const auto url    = "http://" + std::string{ecf::string_constants::localhost} + ":" + port;
        BOOST_CHECK_MESSAGE(
            errors.find("Found token for URL: " + url + ", in tokens file: " + location) != std::string::npos, errors);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
