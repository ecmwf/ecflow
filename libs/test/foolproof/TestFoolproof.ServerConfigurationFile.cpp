// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Returns the configuration file of this host for the given port.
///
/// @param[in] directory The start-up directory of the server
/// @param[in] port The port of the server
/// @return The path of `<host>.<port>.ecf.cfg` in the start-up directory.
///
fs::path configuration_file(const fs::path& directory, std::uint32_t port) {
    return directory / (ecf::Host{}.name() + "." + std::to_string(port) + ".ecf.cfg");
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerConfigurationFile)

///
/// @brief Verifies that a server starts configured by `<host>.<port>.ecf.cfg` alone, without server_environment.cfg.
///
BOOST_AUTO_TEST_CASE(test_server_configuration_file_alone) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();
    fs::create_directories(startup.path() / "home");

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = ecf::test::foolproof::reserve_port();

    WithTestFile configuration(NamedTestFile{configuration_file(startup.path(), port.value()).string()},
                               R"({
                                    "version": 1,
                                    "ecflow": {
                                      "server": {
                                        "variables": {
                                          "ECF_HOME": "home",
                                          "ECF_LOG": "server.log",
                                          "ECF_CHECKMODE": "CHECK_NEVER",
                                          "ECF_URL": "file/url"
                                        }
                                      }
                                    }
                                  })");

    Process process(
        ecf::test::foolproof::ecflow_server_executable(), {"--port=" + std::to_string(port.value())}, startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };

    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "Server configured by its file did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());

    const auto& server = client.defs()->server_state();
    BOOST_CHECK_EQUAL(server.find_variable("ECF_HOME"), (startup.path() / "home").string());
    BOOST_CHECK_EQUAL(server.find_variable("ECF_LOG"), (startup.path() / "home" / "server.log").string());
    BOOST_CHECK_EQUAL(server.find_variable("ECF_URL"), "file/url");
    BOOST_CHECK(fs::is_regular_file(startup.path() / "home" / "server.log"));
}

///
/// @brief Verifies that a server refuses a configuration file of another port, before creating its log.
///
BOOST_AUTO_TEST_CASE(test_server_configuration_file_of_another_port) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto port       = ecf::test::foolproof::reserve_port();
    const auto file = configuration_file(startup.path(), port.value());
    WithTestFile configuration(NamedTestFile{file.string()},
                               R"({"version": 1, "ecflow": {"server": {"variables": {"ECF_PORT": 1024,
                                                                                      "ECF_LOG": "server.log"}}}})");

    Process process(
        ecf::test::foolproof::ecflow_server_executable(), {"--port=" + std::to_string(port.value())}, startup.path());
    const auto [exited, status, output] = ecf::test::foolproof::wait_for_exit(process);
    BOOST_REQUIRE_MESSAGE(exited, "Configuration file of another port did not stop start-up: " << output);
    BOOST_CHECK_EQUAL(status, 1);
    BOOST_CHECK_MESSAGE(output.find(file.filename().string() +
                                    ": /ecflow/server/variables/ECF_PORT: invalid value 1024: expected the port of "
                                    "the file name, " +
                                    std::to_string(port.value())) != std::string::npos,
                        output);
    BOOST_CHECK(!fs::exists(startup.path() / "server.log"));

    const auto error = ecf::test::foolproof::probe_no_listener(port.value());
    BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                        "Expected connection refusal after failed startup, found: " << error.message());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
