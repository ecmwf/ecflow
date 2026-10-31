// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <limits>
#include <list>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Checks bounded CLI completion without runtime file access or a listening socket.
///
/// @param[in] options Command-line tokens; PORT and OTHER_PORT placeholders select distinct valid port values
/// @param[in] expected_status Required process exit status
/// @param[in] fragments Required fragments of informational output or the original CLI diagnostic
/// @param[in] scenario Invalid config, environment or filesystem setting to bypass for informational requests
///
void check_server_cli_exit(std::vector<std::string> options,
                           int expected_status,
                           const std::vector<std::string>& fragments,
                           const std::string& scenario = "clean") {
    using namespace ecf::test::scaffold;

    auto startup           = MakeDirectory{}.create();
    const auto config_path = startup.path() / "server_environment.cfg";

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto port = ecf::test::foolproof::reserve_port();
    for (auto& option : options) {
        for (const auto& [placeholder, value] : std::vector<std::pair<std::string, std::string>>{
                 {"OTHER_PORT", std::to_string(port.value() + 1)}, {"PORT", std::to_string(port.value())}}) {
            const auto position = option.find(placeholder);
            if (position != std::string::npos) {
                option.replace(position, placeholder.size(), value);
            }
        }
    }

    std::list<WithTestFile> files;
    std::string config = "ECF_PORT = " + std::to_string(port.value()) + "\n";
    for (const auto& [name, basename] :
         std::vector<std::pair<std::string, std::string>>{{"ECF_LOG", "server.log"},
                                                          {"ECF_CHECK", "primary.check"},
                                                          {"ECF_CHECKOLD", "backup.check"},
                                                          {"ECF_PASSWD", "regular.passwd"},
                                                          {"ECF_CUSTOM_PASSWD", "custom.passwd"},
                                                          {"ECF_LISTS", "users.lists"}}) {
        const auto path = startup.path() / basename;
        config += name + " = " + path.string() + "\n";
        if (name == "ECF_PASSWD" || name == "ECF_CUSTOM_PASSWD" || name == "ECF_LISTS") {
            files.emplace_back(NamedTestFile{path.string()}, "malformed security file\n");
        }
    }

    std::list<WithTestEnvironmentVariable> overrides;
    if (scenario == "malformed_config") {
        config += "[unterminated\n";
    }
    else if (scenario == "unknown_config") {
        config += "UNKNOWN_SERVER_SETTING = 1\n";
    }
    else if (scenario == "invalid_config_integer") {
        config += "ECF_CHECKINTERVAL = invalid_config_integer\n";
    }
    else if (scenario == "invalid_mode") {
        config += "ECF_CHECKMODE = unsupported\n";
    }
    else if (scenario == "missing_home") {
        config += "ECF_HOME = " + (startup.path() / "missing home").string() + "\n";
    }
    else if (scenario == "empty_home") {
        config += "ECF_HOME =\n";
    }
    else if (scenario == "empty_command") {
        config += "ECF_JOB_CMD =\n";
    }
    else if (scenario == "invalid_port_env") {
        overrides.emplace_back("ECF_PORT", "invalid_environment_integer");
    }
    else if (scenario == "invalid_checkpoint_env") {
        overrides.emplace_back("ECF_CHECKINTERVAL", "invalid_environment_integer");
    }
    else if (scenario == "invalid_pruning_env") {
        overrides.emplace_back("ECF_PRUNE_NODE_LOG", "invalid_environment_integer");
    }
    else if (scenario == "invalid_threshold_env") {
        overrides.emplace_back("ECF_TASK_THRESHOLD", "invalid_environment_integer");
    }
    else if (scenario == "nonpositive_checkpoint_env") {
        overrides.emplace_back("ECF_CHECKINTERVAL", "0");
    }
    else if (scenario == "nonpositive_threshold_env") {
        overrides.emplace_back("ECF_TASK_THRESHOLD", "0");
    }
    else if (scenario == "invalid_ssl_env") {
        overrides.emplace_back("ECF_SSL", "unsupported_ssl_selection");
    }

    BOOST_SCOPE_EXIT_ALL(&) {
        if (scenario == "unreadable_config") {
            std::error_code error;
            fs::permissions(
                config_path, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, error);
            BOOST_CHECK_MESSAGE(!error, "Unable to restore config permissions: " << error.message());
        }
    };

    if (scenario == "directory_config") {
        fs::create_directories(config_path);
    }
    else {
        files.emplace_back(NamedTestFile{config_path.string()}, config);
        if (scenario == "unreadable_config") {
            fs::permissions(config_path, fs::perms::none, fs::perm_options::replace);
            if (::access(config_path.c_str(), R_OK) == 0) {
                ecf::test::scaffold::skip_unenforced_permissions("the unreadable config case");
                return;
            }
        }
    }

    Process process(ecf::test::foolproof::ecflow_server_executable(), options, startup.path());
    const auto [exited, status, output] = ecf::test::foolproof::wait_for_exit(process);
    BOOST_REQUIRE_MESSAGE(exited, "CLI request did not finish: " << output);
    BOOST_CHECK_EQUAL(status, expected_status);
    for (const auto& fragment : fragments) {
        BOOST_CHECK_MESSAGE(output.find(fragment) != std::string::npos, output);
    }
    for (const auto& basename : {"server.log", "primary.check", "backup.check"}) {
        BOOST_CHECK(!fs::exists(startup.path() / basename));
    }
    for (const auto& entry : fs::directory_iterator(startup.path())) {
        const auto name = entry.path().filename().string();
        BOOST_CHECK_MESSAGE(name == "server_environment.cfg" || name == "regular.passwd" || name == "custom.passwd" ||
                                name == "users.lists",
                            "CLI request created an unexpected runtime file: " << entry.path());
    }

    // Start-up announcements (AuthenticationService and ServerOptions) prove that the request went past the CLI
    BOOST_CHECK(output.find(" file located at '") == std::string::npos);
    BOOST_CHECK(output.find("SSL enabled:") == std::string::npos);

    const auto error = ecf::test::foolproof::probe_no_listener(port.value());
    BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                        "Expected no listener after CLI completion, found: " << error.message());
}

///
/// @brief Checks equivalent argument forms through a live server and its exported runtime configuration.
///
/// @param[in] equals_port Whether the port argument uses the equals form
/// @param[in] equals_interval Whether the scheduling argument uses the equals form
///
void check_server_argument_forms(bool equals_port, bool equals_interval) {
    using namespace ecf::test::scaffold;

    auto startup = MakeDirectory{}.create();

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = ecf::test::foolproof::reserve_port();
    // The losing port settings are reserved too, so that a precedence regression cannot bind another test's port
    auto config_decoy      = ecf::test::foolproof::reserve_port();
    auto environment_decoy = ecf::test::foolproof::reserve_port();
    WithTestEnvironmentVariable environment_port("ECF_PORT", std::to_string(environment_decoy.value()));
    WithTestFile config(NamedTestFile{(startup.path() / "server_environment.cfg").string()},
                        "ECF_PORT = " + std::to_string(config_decoy.value()) + "\nECF_INTERVAL = 19\n");

    std::vector<std::string> options;
    if (equals_port) {
        options.emplace_back("--port=" + std::to_string(port.value()));
    }
    else {
        options.insert(options.end(), {"--port", std::to_string(port.value())});
    }
    if (equals_interval) {
        options.emplace_back("--ecfinterval=17");
    }
    else {
        options.insert(options.end(), {"--ecfinterval", "17"});
    }

    Process process(ecf::test::foolproof::ecflow_server_executable(), options, startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        process.terminate();
    };

    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "Server did not start with the selected argument forms: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());

    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());
    BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_PORT"), std::to_string(port.value()));
    BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_INTERVAL"), "17");

    const auto prefix = host.value() + "." + std::to_string(port.value()) + ".";
    for (const auto& [name, suffix] :
         std::vector<std::pair<std::string, std::string>>{{"ECF_LOG", "ecf.log"},
                                                          {"ECF_CHECK", "ecf.check"},
                                                          {"ECF_CHECKOLD", "ecf.check.b"},
                                                          {"ECF_PASSWD", "ecf.passwd"},
                                                          {"ECF_CUSTOM_PASSWD", "ecf.custom_passwd"},
                                                          {"ECF_LISTS", "ecf.lists"}}) {
        const bool anchored = name == "ECF_LOG" || name == "ECF_CHECK" || name == "ECF_CHECKOLD";
        const auto expected = anchored ? (startup.path() / (prefix + suffix)).string() : prefix + suffix;
        BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable(name), expected);
    }
    BOOST_CHECK(fs::is_regular_file(startup.path() / (prefix + "ecf.log")));
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerOptions)

///
/// @brief Verifies that unknown CLI options are rejected with the offending option name.
///
BOOST_AUTO_TEST_CASE(test_server_options_unknown) {
    ECF_NAME_THIS_TEST();

    for (const auto& option : {"--unknown-server-option", "--version-extra"}) {
        BOOST_TEST_CONTEXT("Unknown option: " << option) {
            check_server_cli_exit({option}, 1, {"Invalid server command line", option});
        }
    }
}

///
/// @brief Verifies that numeric options without arguments fail before runtime setup.
///
BOOST_AUTO_TEST_CASE(test_server_options_missing_arguments) {
    ECF_NAME_THIS_TEST();

    for (const auto& option : {"--port", "--ecfinterval"}) {
        BOOST_TEST_CONTEXT("Missing argument: " << option) {
            check_server_cli_exit({option}, 1, {"Invalid server command line", option});
        }
    }
}

///
/// @brief Verifies rejection of nonnumeric, empty, overflowing and underflowing numeric arguments.
///
BOOST_AUTO_TEST_CASE(test_server_options_invalid_numbers) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::string> values = {
        "not_a_number",
        "",
        "--help",
        std::to_string(static_cast<long long>(std::numeric_limits<int>::max()) + 1),
        std::to_string(static_cast<long long>(std::numeric_limits<int>::min()) - 1)};
    for (const std::string option : {"--port", "--ecfinterval"}) {
        for (const auto& value : values) {
            BOOST_TEST_CONTEXT(option << " = '" << value << "'") {
                std::vector<std::string> fragments = {"Invalid server command line", option};
                if (!value.empty()) {
                    fragments.push_back(value);
                }
                check_server_cli_exit({option + "=" + value}, 1, fragments);
            }
        }
    }
}

///
/// @brief Verifies rejection of repeated numeric options in equals and separate-argument forms.
///
BOOST_AUTO_TEST_CASE(test_server_options_duplicate_numbers) {
    ECF_NAME_THIS_TEST();

    check_server_cli_exit({"--port=PORT", "--port=OTHER_PORT"}, 1, {"--port", "Invalid server command line"});
    check_server_cli_exit({"--port", "PORT", "--port", "OTHER_PORT"}, 1, {"--port", "Invalid server command line"});
    check_server_cli_exit(
        {"--ecfinterval=17", "--ecfinterval=23"}, 1, {"--ecfinterval", "Invalid server command line"});
    check_server_cli_exit(
        {"--ecfinterval", "17", "--ecfinterval", "23"}, 1, {"--ecfinterval", "Invalid server command line"});
}

///
/// @brief Verifies identical effective port, scheduling and path settings for all supported argument forms.
///
BOOST_AUTO_TEST_CASE(test_server_options_argument_forms) {
    ECF_NAME_THIS_TEST();

    for (const bool equals_port : {false, true}) {
        for (const bool equals_interval : {false, true}) {
            BOOST_TEST_CONTEXT("Equals port: " << equals_port << ", equals interval: " << equals_interval) {
                check_server_argument_forms(equals_port, equals_interval);
            }
        }
    }
}

///
/// @brief Verifies long and short informational options without log, security-file or listener side effects.
///
BOOST_AUTO_TEST_CASE(test_server_options_information) {
    ECF_NAME_THIS_TEST();

    for (const std::string option : {"--help", "-h", "--version", "-v"}) {
        BOOST_TEST_CONTEXT("Information option: " << option) {
            check_server_cli_exit({option}, 0, {option == "--help" || option == "-h" ? "Server options" : "Ecflow"});
        }
    }

#ifdef ECF_OPENSSL
    check_server_cli_exit({"--help", "--ssl"}, 0, {"Server options"}, "invalid_ssl_env");
    check_server_cli_exit({"--version", "--ssl"}, 0, {"Ecflow"}, "invalid_ssl_env");
#endif
}

///
/// @brief Verifies that informational requests bypass invalid configuration, environment and filesystem settings.
///
BOOST_AUTO_TEST_CASE(test_server_options_information_bypasses_settings) {
    ECF_NAME_THIS_TEST();

    for (const std::string option : {"--help", "--version"}) {
        for (const std::string scenario : {"malformed_config",
                                           "unknown_config",
                                           "invalid_config_integer",
                                           "invalid_mode",
                                           "directory_config",
                                           "unreadable_config",
                                           "missing_home",
                                           "empty_home",
                                           "empty_command",
                                           "invalid_port_env",
                                           "invalid_checkpoint_env",
                                           "invalid_pruning_env",
                                           "invalid_threshold_env",
                                           "nonpositive_checkpoint_env",
                                           "nonpositive_threshold_env",
                                           "invalid_ssl_env"}) {
            BOOST_TEST_CONTEXT(option << " with " << scenario) {
                check_server_cli_exit({option}, 0, {option == "--help" ? "Server options" : "Ecflow"}, scenario);
            }
        }
    }
}

///
/// @brief Verifies that informational options do not conceal malformed command-line syntax.
///
BOOST_AUTO_TEST_CASE(test_server_options_information_rejects_invalid_cli) {
    ECF_NAME_THIS_TEST();

    check_server_cli_exit(
        {"--help", "--unknown-server-option"}, 1, {"Invalid server command line", "--unknown-server-option"});
    check_server_cli_exit({"--help", "--port"}, 1, {"Invalid server command line", "--port"});
    check_server_cli_exit(
        {"--version", "--port=not_a_number"}, 1, {"Invalid server command line", "--port", "not_a_number"});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
