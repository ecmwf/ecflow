// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <fstream>
#include <iterator>
#include <list>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>
#include <sys/wait.h>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Checks logged and exported settings against expectations derived only from fixture inputs.
///
/// @param[in] environment_overrides Whether environment values override conflicting config settings
///
void check_observed_configuration(bool environment_overrides) {
    using namespace ecf::test::scaffold;

    auto startup                = MakeDirectory{}.create();
    const auto config_home      = startup.path() / "config home";
    const auto environment_home = startup.path() / "environment home";
    const auto home             = environment_overrides ? environment_home : config_home;
    fs::create_directories(config_home);
    fs::create_directories(environment_home);

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();
    // The losing port settings are reserved too, so that a precedence regression cannot bind another test's port
    auto config_decoy           = MakePort{}.with(AutomaticPortValue{}).create();
    auto environment_decoy      = MakePort{}.with(AutomaticPortValue{}).create();
    const std::string port_text = std::to_string(port.value());
    const auto prefix           = host.value() + "." + port_text + ".";
    const std::string config_text =
        "# Keep this input byte-for-byte unchanged.\n\nECF_HOME = " + config_home.string() +
        "\nECF_PORT = " + std::to_string(config_decoy.value()) +
        "\nECF_LOG = config.log\nECF_CHECK = ecf.check\nECF_CHECKOLD = ecf.check.b\n"
        "ECF_PASSWD = config.passwd\nECF_CUSTOM_PASSWD = config.custom_passwd\nECF_LISTS = config.lists\n"
        "ECF_CHECKMODE = CHECK_NEVER\nECF_CHECKINTERVAL = 137\nECF_PRUNE_NODE_LOG = 19\n"
        "ECF_TASK_THRESHOLD = 333\nECF_INTERVAL = 17\nECF_JOB_CMD = configured_submit\n"
        "ECF_KILL_CMD = configured_kill\nECF_STATUS_CMD = configured_status\nECF_CHECK_CMD = configured_check\n"
        "ECF_URL_CMD = configured_url\nECF_URL_BASE = https://current.invalid\nECF_URL = current/path\nECF_MICRODEF = "
        "@\n";
    WithTestFile config(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config_text);

    const auto absolute_backup = startup.path() / "environment.backup";
    std::list<WithTestEnvironmentVariable> overrides;
    if (environment_overrides) {
        for (const auto& [name, value] :
             std::vector<std::pair<std::string, std::string>>{{"ECF_HOME", environment_home.string()},
                                                              {"ECF_PORT", std::to_string(environment_decoy.value())},
                                                              {"ECF_LOG", "environment.log"},
                                                              {"ECF_CHECK", "environment.primary"},
                                                              {"ECF_CHECKOLD", absolute_backup.string()},
                                                              {"ECF_PASSWD", "environment.passwd"},
                                                              {"ECF_CUSTOM_PASSWD", "environment.custom_passwd"},
                                                              {"ECF_LISTS", "environment.lists"},
                                                              {"ECF_CHECKINTERVAL", "251"},
                                                              {"ECF_PRUNE_NODE_LOG", "31"},
                                                              {"ECF_TASK_THRESHOLD", "555"}}) {
            overrides.emplace_back(name, value);
        }
    }

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + port_text, "--ecfinterval=23", "--dis_job_gen"},
                    startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };

    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "Observed server did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    BOOST_REQUIRE(client.defs());
    BOOST_REQUIRE_EQUAL(process.terminate(), 0);

    const auto log_path = home / (environment_overrides ? "environment.log" : "config.log");
    const auto primary  = home / (environment_overrides ? "environment.primary" : prefix + "ecf.check");
    const auto backup   = environment_overrides ? absolute_backup : home / (prefix + "ecf.check.b");
    std::vector<std::string> lines;
    std::istringstream log(ecf::test::foolproof::read_file_bytes(log_path));
    for (std::string line; std::getline(log, line);) {
        const auto delimiter = line.find("] ");
        lines.push_back(delimiter == std::string::npos ? line : line.substr(delimiter + 2));
    }

    for (const auto& [name, value] : std::vector<std::pair<std::string, std::string>>{
             {"ECF_HOME", home.string()},
             {"ECF_PORT", port_text},
             {"ECF_HOST", host.value()},
             {"ECF_LOG", log_path.string()},
             {"ECF_CHECK", primary.string()},
             {"ECF_CHECKOLD", backup.string()},
             {"ECF_PASSWD", environment_overrides ? "environment.passwd" : "config.passwd"},
             {"ECF_CUSTOM_PASSWD", environment_overrides ? "environment.custom_passwd" : "config.custom_passwd"},
             {"ECF_LISTS", environment_overrides ? "environment.lists" : "config.lists"},
             {"ECF_INTERVAL", "23"},
             {"ECF_JOB_CMD", "configured_submit"},
             {"ECF_KILL_CMD", "configured_kill"},
             {"ECF_STATUS_CMD", "configured_status"},
             {"ECF_CHECK_CMD", "configured_check"},
             {"ECF_URL_CMD", "configured_url"},
             {"ECF_URL_BASE", "https://current.invalid"},
             {"ECF_URL", "current/path"},
             {"ECF_MICRO", "@"}}) {
        BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable(name), value);
        const std::string label  = name == "ECF_PORT"       ? "Port"
                                   : name == "ECF_HOST"     ? "Host"
                                   : name == "ECF_INTERVAL" ? "Job scheduling interval"
                                                            : name;
        const auto expected_line = "  " + label + " = " + value;
        BOOST_CHECK_MESSAGE(std::find(lines.begin(), lines.end(), expected_line) != lines.end(),
                            "Missing independently expected log entry: " << expected_line);
    }
    BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_PID"), std::to_string(process.pid()));
    BOOST_CHECK_EQUAL(client.defs()->server_state().find_variable("ECF_VERSION"), ecf::Version::full());
    for (const auto& [name, value] :
         std::vector<std::pair<std::string, std::string>>{{"ECF_CHECKINTERVAL", environment_overrides ? "251" : "137"},
                                                          {"ECF_PRUNE_NODE_LOG", environment_overrides ? "31" : "19"},
                                                          {"ECF_TASK_THRESHOLD", environment_overrides ? "555" : "333"},
                                                          {"ECF_CHECKMODE", "CHECK_NEVER"},
                                                          {"ECF_DEBUG_SERVER", "disabled"},
                                                          {"Connection", "TCP/IP v4"},
                                                          {"Job generation", "disabled"}}) {
        const auto expected_line = "  " + name + " = " + value;
        BOOST_CHECK_MESSAGE(std::find(lines.begin(), lines.end(), expected_line) != lines.end(),
                            "Missing independently expected log entry: " << expected_line);
    }
    BOOST_CHECK_EQUAL(ecf::test::foolproof::read_file_bytes(config.path()), config_text);
    BOOST_CHECK(!fs::exists(primary));
    BOOST_CHECK(!fs::exists(backup));
}

///
/// @brief Verifies original failure diagnostics, child reaping and release of only the owned port reservation.
///
/// @param[in] stage Startup stage at which the fixture must fail
/// @param[in] foreign_lock Existing unrelated lock that must remain unchanged
///
void check_failed_start_cleanup(const std::string& stage, const fs::path& foreign_lock) {
    using namespace ecf::test::scaffold;

    auto startup    = MakeDirectory{}.create();
    const auto home = startup.path() / "home";
    fs::create_directories(home);

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    fs::path owned_lock;
    std::string owned_lock_contents;
    unsigned selected_port = 0;
    {
        auto port     = MakePort{}.with(AutomaticPortValue{}).create();
        owned_lock    = port.lock_location();
        selected_port = port.value();
        BOOST_REQUIRE(fs::is_regular_file(owned_lock));
        owned_lock_contents = ecf::test::foolproof::read_file_bytes(owned_lock);

        std::vector<std::string> options = {"--port=" + std::to_string(selected_port)};
        std::string config =
            "ECF_LOG = " + (home / "server.log").string() +
            "\nECF_CHECKMODE = CHECK_NEVER\nECF_CHECK = primary.check\nECF_CHECKOLD = backup.check\n"
            "ECF_PASSWD = missing.passwd\nECF_CUSTOM_PASSWD = missing.custom_passwd\nECF_LISTS = missing.lists\n";
        config += "ECF_HOME = " + (stage == "home" ? (startup.path() / "missing home").string() : home.string()) + "\n";
        std::vector<std::string> fragments;
        std::list<WithTestFile> inputs;
        if (stage == "cli") {
            options.emplace_back("--unknown-observation-option");
            fragments = {"Invalid server command line", "--unknown-observation-option"};
        }
        else if (stage == "config") {
            config += "UNKNOWN_OBSERVATION_SETTING = 1\n";
            fragments = {
                "Could not parse server configuration file", "server_environment.cfg", "UNKNOWN_OBSERVATION_SETTING"};
        }
        else if (stage == "home") {
            fragments = {"ECF_HOME", (startup.path() / "missing home").string()};
        }
        else if (stage == "security") {
            inputs.emplace_back(NamedTestFile{(home / "missing.passwd").string()}, "not_a_version\n");
            fragments = {"missing.passwd", "could not be parsed", "not_a_version"};
        }
        else {
            inputs.emplace_back(NamedTestFile{(home / "primary.check").string()}, "UNKNOWN_OBSERVATION_CHECKPOINT\n");
            fragments = {"ECF_CHECK", "primary.check", "UNKNOWN_OBSERVATION_CHECKPOINT", "no usable checkpoint"};
        }
        WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config);

        Process process(ecf::test::foolproof::ecflow_server_executable(), options, startup.path());
        const auto pid                      = process.pid();
        const auto [exited, status, output] = ecf::test::foolproof::wait_for_exit(process);
        BOOST_REQUIRE_MESSAGE(exited, "Failed startup did not finish: " << output);
        BOOST_CHECK_EQUAL(status, 1);
        for (const auto& fragment : fragments) {
            BOOST_CHECK_MESSAGE(output.find(fragment) != std::string::npos, output);
        }
        BOOST_CHECK_EQUAL(fs::exists(home / "server.log"), stage == "security" || stage == "checkpoint");
        BOOST_CHECK_EQUAL(ecf::test::foolproof::read_file_bytes(configuration.path()), config);
        BOOST_CHECK(!fs::exists(home / "backup.check"));

        int native_status          = 0;
        errno                      = 0;
        const auto remaining_child = ::waitpid(pid, &native_status, WNOHANG);
        const auto wait_error      = errno;
        BOOST_CHECK_EQUAL(remaining_child, -1);
        BOOST_CHECK_EQUAL(wait_error, ECHILD);
        BOOST_CHECK_EQUAL(process.wait(), 1);
        BOOST_CHECK(fs::is_regular_file(owned_lock));
        BOOST_CHECK_EQUAL(ecf::test::foolproof::read_file_bytes(owned_lock), owned_lock_contents);

        const auto error = ecf::test::foolproof::probe_no_listener(selected_port);
        BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                            "Failed startup left a listener: " << error.message());
    }

    // Another test may legitimately acquire the released port immediately.
    // A remaining lock must not be the reservation owned by this test.
    std::ifstream remaining_lock(owned_lock, std::ios::binary);
    bool owned_reservation_remains = false;
    if (remaining_lock.is_open()) {
        const std::string contents{std::istreambuf_iterator<char>(remaining_lock), std::istreambuf_iterator<char>()};
        owned_reservation_remains = contents == owned_lock_contents;
    }
    BOOST_CHECK_MESSAGE(!owned_reservation_remains, "Failed startup leaked its owned port lock: " << owned_lock);
    BOOST_CHECK_EQUAL(ecf::test::foolproof::read_file_bytes(foreign_lock), "preserve unrelated lock\n");
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerObservability)

///
/// @brief Verifies independent expectations for logged and exported startup settings without input mutation.
///
BOOST_AUTO_TEST_CASE(test_server_observability_logged_and_exported_configuration) {
    ECF_NAME_THIS_TEST();

    for (const bool environment_overrides : {false, true}) {
        BOOST_TEST_CONTEXT("Environment overrides: " << environment_overrides) {
            check_observed_configuration(environment_overrides);
        }
    }
}

///
/// @brief Verifies failed startup diagnostics and cleanup while unrelated child and lock resources remain intact.
///
BOOST_AUTO_TEST_CASE(test_server_observability_failed_start_diagnostics_and_cleanup) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    const auto foreign_path =
        LockFile::lock_directory() / ("observability-foreign-" + std::to_string(::getpid()) + ".lock");
    BOOST_REQUIRE(!fs::exists(foreign_path));
    WithTestFile foreign_lock(NamedTestFile{foreign_path.string()}, "preserve unrelated lock\n");
    Process unrelated("/bin/sh", {"-c", "exec sleep 300"});
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(unrelated.terminate(), 0);
    };

    for (const std::string stage : {"cli", "config", "home", "security", "checkpoint"}) {
        BOOST_TEST_CONTEXT("Failed startup stage: " << stage) {
            check_failed_start_cleanup(stage, foreign_path);
            BOOST_CHECK_EQUAL(::kill(unrelated.pid(), 0), 0);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
