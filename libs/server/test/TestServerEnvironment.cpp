// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <list>
#include <optional>
#include <stdexcept>
#include <vector>

#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/base/ServerProtocol.hpp"
#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/MaskCredentials.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/JobProfiler.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf;

// Imported individually, rather than the whole namespace, since the scaffold also declares a Host.
using ecf::test::scaffold::AutomaticTestFile;
using ecf::test::scaffold::NamedTestFile;
using ecf::test::scaffold::WithoutTestEnvironmentVariable;
using ecf::test::scaffold::WithTestEnvironmentVariable;
using ecf::test::scaffold::WithTestFile;

namespace {

///
/// @brief Removes the log file created by the given server environment.
///
/// @param[in] serverEnv The server environment whose log file is to be removed
///
void remove_log_file(const ServerEnvironment& serverEnv) {
    Host host;
    fs::remove(host.ecf_log_file(serverEnv.the_port()));
}

///
/// @brief Returns the lines of the last start-up written to the log, from 'Started at', without their prefix.
///
/// @return The lines, each without its `XXX:[time] ` prefix; empty when the log holds no start-up.
///
std::vector<std::string> last_startup_lines() {
    Log::instance()->flush();

    std::vector<std::string> lines;
    BOOST_REQUIRE(File::splitFileIntoLines(Log::instance()->path(), lines));

    auto started = std::find_if(lines.rbegin(), lines.rend(), [](const std::string& line) {
        return line.find("] Started at ") != std::string::npos;
    });
    if (started == lines.rend()) {
        return {};
    }

    std::vector<std::string> startup;
    for (auto line = std::prev(started.base()); line != lines.end(); ++line) {
        auto prefix_end = line->find("] ");
        startup.push_back(prefix_end == std::string::npos ? *line : line->substr(prefix_end + 2));
    }
    return startup;
}

///
/// @brief Returns the value of the given server variable.
///
/// @param[in] serverEnv The server environment providing the variables
/// @param[in] name The name of the variable
/// @return The value of the variable; empty when the variable is not defined.
///
std::string server_variable(const ServerEnvironment& serverEnv, const std::string& name) {
    std::vector<std::pair<std::string, std::string>> variables;
    serverEnv.variables(variables);
    for (const auto& [key, value] : variables) {
        if (key == name) {
            return value;
        }
    }
    return {};
}

///
/// @brief Returns the runtime configuration block expected in the log, for the given server environment.
///
/// @param[in] serverEnv The server environment that wrote the log
/// @param[in] connection The expected value of the Connection entry
/// @param[in] ssl_certificate The expected value of the SSL certificate entry; empty when SSL is not enabled
/// @return The lines of the block, starting with its title.
///
std::vector<std::string> expected_runtime_configuration(const ServerEnvironment& serverEnv,
                                                        const std::string& connection,
                                                        const std::string& ssl_certificate) {
    auto entry    = [](const std::string& key, const std::string& value) { return "  " + key + " = " + value; };
    auto variable = [&](const std::string& name) { return entry(name, server_variable(serverEnv, name)); };
    auto command  = [&](const std::string& name) {
        return entry(name, ecf::mask_credentials(server_variable(serverEnv, name)));
    };

    std::vector<std::string> block = {"Runtime Configuration",
                                      entry("Host", serverEnv.hostPort().first),
                                      entry("Port", serverEnv.the_port()),
                                      entry("Connection", connection)};
    if (!ssl_certificate.empty()) {
        block.push_back(entry("SSL certificate", ssl_certificate));
    }
    std::vector<std::string> settings = {
        variable("ECF_HOME"),
        variable("ECF_LOG"),
        variable("ECF_CHECK"),
        variable("ECF_CHECKOLD"),
        variable("ECF_LISTS"),
        variable("ECF_PASSWD"),
        variable("ECF_CUSTOM_PASSWD"),
        entry("ECF_CHECKINTERVAL", std::to_string(serverEnv.checkPtInterval())),
        entry("ECF_CHECKMODE", serverEnv.check_mode_str()),
        entry("ECF_PRUNE_NODE_LOG", std::to_string(serverEnv.ecf_prune_node_log())),
        entry("ECF_TASK_THRESHOLD", std::to_string(JobProfiler::task_threshold())),
        entry("ECF_DEBUG_SERVER", serverEnv.debug() ? "enabled" : "disabled"),
        command("ECF_JOB_CMD"),
        command("ECF_KILL_CMD"),
        command("ECF_STATUS_CMD"),
        command("ECF_CHECK_CMD"),
        command("ECF_URL_CMD"),
        command("ECF_URL_BASE"),
        command("ECF_URL"),
        command("ECF_MICRO"),
        entry("Job scheduling interval", std::to_string(serverEnv.submitJobsInterval())),
        entry("Job generation", serverEnv.jobGeneration() ? "enabled" : "disabled"),
        entry("Checkpoint save time alarm", std::to_string(serverEnv.checkpt_save_time_alarm()))};
    block.insert(block.end(), settings.begin(), settings.end());
    return block;
}

///
/// @brief Checks that the log holds the runtime configuration block, right after the version line.
///
/// @param[in] serverEnv The server environment that wrote the log
/// @param[in] connection The expected value of the Connection entry
/// @param[in] ssl_certificate The expected value of the SSL certificate entry; empty when SSL is not enabled
///
void check_runtime_configuration(const ServerEnvironment& serverEnv,
                                 const std::string& connection,
                                 const std::string& ssl_certificate = {}) {
    auto startup  = last_startup_lines();
    auto expected = expected_runtime_configuration(serverEnv, connection, ssl_certificate);

    BOOST_REQUIRE_MESSAGE(startup.size() >= 2 + expected.size(),
                          "Expected 'Started at', the version line and "
                              << expected.size() << " lines of runtime configuration, but found " << startup.size()
                              << " lines");
    std::vector<std::string> actual(startup.begin() + 2, startup.begin() + 2 + expected.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

///
/// @brief Checks server-home and checkpoint resolution across a working-directory change.
///
/// @param[in] from_environment Whether the environment overrides the home in the configuration file
/// @param[in] nested_home Whether a duplicate relative home exists beneath the selected home
/// @param[in] custom_checkpoints Whether to use an explicit relative checkpoint and an absolute backup path
///
void check_server_home_resolution(bool from_environment, bool nested_home, bool custom_checkpoints) {
    auto startup               = ecf::test::scaffold::MakeDirectory{}.create();
    const auto original_path   = fs::current_path();
    const auto absolute_home   = startup.path() / "absolute workspace";
    const auto absolute_backup = startup.path() / "backup.check";
    BOOST_SCOPE_EXIT_ALL(&) {
        Log::destroy();
        std::error_code error;
        fs::current_path(original_path, error);
        BOOST_CHECK_MESSAGE(!error, "Unable to restore test working directory: " << error.message());
    };

    WithoutTestEnvironmentVariable no_home("ECF_HOME");
    WithoutTestEnvironmentVariable no_log("ECF_LOG");
    WithoutTestEnvironmentVariable no_check("ECF_CHECK");
    WithoutTestEnvironmentVariable no_checkold("ECF_CHECKOLD");
    WithoutTestEnvironmentVariable no_passwd("ECF_PASSWD");
    WithoutTestEnvironmentVariable no_custom_passwd("ECF_CUSTOM_PASSWD");
    WithoutTestEnvironmentVariable no_lists("ECF_LISTS");
    WithTestEnvironmentVariable port("ECF_PORT", "3144");

    for (const auto& home : std::vector<std::string>{"workspace",
                                                     "./workspace",
                                                     "other/../workspace",
                                                     "workspace/",
                                                     "workspace with spaces",
                                                     absolute_home.string()}) {
        BOOST_TEST_CONTEXT("ECF_HOME=" << home) {
            fs::current_path(startup.path());
            // A relative home is exported in normal form, without dot segments or a trailing separator
            auto expected_home = (startup.path() / home).lexically_normal();
            if (expected_home.filename().empty()) {
                expected_home = expected_home.parent_path();
            }
            fs::create_directories(expected_home / "checkpoints");
            if (nested_home && fs::path(home).is_relative()) {
                fs::create_directories(expected_home / home);
            }

            const auto checkpoint   = custom_checkpoints ? "checkpoints/primary.check" : Ecf::CHECKPT();
            const auto backup       = custom_checkpoints ? absolute_backup.string() : Ecf::BACKUP_CHECKPT();
            std::string config_text = "ECF_HOME = " + (from_environment ? std::string(".") : home) + "\n";
            config_text += "ECF_LOG = server.log\n";
            config_text += "ECF_CHECK = " + checkpoint + "\n";
            config_text += "ECF_CHECKOLD = " + backup + "\n";
            WithTestFile config(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config_text);
            std::optional<WithTestEnvironmentVariable> home_override;
            if (from_environment) {
                home_override.emplace("ECF_HOME", home);
            }

            const std::vector<std::string> args = {"ServerEnvironment"};
            ServerEnvironment serverEnv(args);
            std::string error;
            BOOST_REQUIRE_MESSAGE(serverEnv.valid(error), error);
            BOOST_CHECK_EQUAL(serverEnv.ecf_home(), expected_home.string());
            BOOST_CHECK(fs::equivalent(fs::current_path(), expected_home));
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_HOME"), expected_home.string());

            Host host;
            const auto expected_check =
                expected_home / (custom_checkpoints ? std::string(checkpoint) : host.ecf_checkpt_file("3144"));
            const auto expected_backup =
                custom_checkpoints ? absolute_backup : expected_home / host.ecf_backup_checkpt_file("3144");
            BOOST_CHECK_EQUAL(serverEnv.checkPtFilename(), expected_check.string());
            BOOST_CHECK_EQUAL(serverEnv.oldCheckPtFilename(), expected_backup.string());
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_CHECK"), expected_check.string());
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_CHECKOLD"), expected_backup.string());
            BOOST_CHECK(fs::exists(expected_home / "server.log"));
            BOOST_CHECK(!fs::exists(startup.path() / "server.log"));
        }
    }
}

///
/// @brief Checks that a configuration parsing failure stops initialisation before environment overrides.
///
/// @param[in] content Invalid configuration content to parse
/// @param[in] expected_error Fragment of the original parser diagnostic that must be preserved
/// @param[in] invalid_environment Whether an invalid port override competes with the configuration error
///
void check_server_config_parse_error(const std::string& content,
                                     const std::string& expected_error,
                                     bool invalid_environment = false) {
    auto directory           = ecf::test::scaffold::MakeDirectory{}.create();
    const auto original_path = fs::current_path();
    const auto log_path      = directory.path() / "server.log";
    WithTestFile config(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
    WithTestEnvironmentVariable home("ECF_HOME", original_path.string());
    WithTestEnvironmentVariable port("ECF_PORT", invalid_environment ? "invalid_environment_port" : "3144");
    WithTestEnvironmentVariable log("ECF_LOG", log_path.string());
    WithTestEnvironmentVariable checkpoint("ECF_CHECK", (directory.path() / "primary.check").string());
    WithTestEnvironmentVariable backup("ECF_CHECKOLD", (directory.path() / "backup.check").string());
    WithTestEnvironmentVariable passwd("ECF_PASSWD", (directory.path() / "missing.passwd").string());
    WithTestEnvironmentVariable custom_passwd("ECF_CUSTOM_PASSWD",
                                              (directory.path() / "missing.custom_passwd").string());
    WithTestEnvironmentVariable lists("ECF_LISTS", (directory.path() / "missing.lists").string());

    const std::vector<std::string> args = {"ServerEnvironment"};
    BOOST_CHECK_EXCEPTION(ServerEnvironment(args, config.path().string()),
                          ServerEnvironmentException,
                          [&](const ServerEnvironmentException& error) {
                              const std::string message = error.what();
                              return message.find("Could not parse server configuration file") != std::string::npos &&
                                     message.find(config.path().string()) != std::string::npos &&
                                     message.find(expected_error) != std::string::npos &&
                                     message.find("invalid_environment_port") == std::string::npos;
                          });
    BOOST_CHECK(fs::current_path() == original_path);
    BOOST_CHECK(!fs::exists(log_path));
    BOOST_CHECK(!Log::instance());
}

///
/// @brief Checks validation of the effective checkpoint interval after environment overrides.
///
/// @param[in] configured_interval Optional interval in the configuration file; absent selects the default
/// @param[in] environment_interval Optional environment override; absent leaves the configuration value unchanged
///
void check_server_checkpoint_interval(std::optional<int> configured_interval, std::optional<int> environment_interval) {
    auto directory      = ecf::test::scaffold::MakeDirectory{}.create();
    std::string content = "ECF_INTERVAL = 17\n";
    if (configured_interval) {
        content += "ECF_CHECKINTERVAL = " + std::to_string(*configured_interval) + "\n";
    }
    WithTestFile config(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
    WithTestEnvironmentVariable home("ECF_HOME", fs::current_path().string());
    WithTestEnvironmentVariable port("ECF_PORT", "3144");
    WithTestEnvironmentVariable log("ECF_LOG", (directory.path() / "server.log").string());
    WithTestEnvironmentVariable checkpoint("ECF_CHECK", (directory.path() / "primary.check").string());
    WithTestEnvironmentVariable backup("ECF_CHECKOLD", (directory.path() / "backup.check").string());
    WithTestEnvironmentVariable passwd("ECF_PASSWD", (directory.path() / "missing.passwd").string());
    WithTestEnvironmentVariable custom_passwd("ECF_CUSTOM_PASSWD",
                                              (directory.path() / "missing.custom_passwd").string());
    WithTestEnvironmentVariable lists("ECF_LISTS", (directory.path() / "missing.lists").string());
    WithoutTestEnvironmentVariable no_interval("ECF_CHECKINTERVAL");
    WithoutTestEnvironmentVariable no_prune("ECF_PRUNE_NODE_LOG");
    WithoutTestEnvironmentVariable no_threshold("ECF_TASK_THRESHOLD");
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");
    WithoutTestEnvironmentVariable no_debug("ECF_DEBUG_SERVER");
    std::optional<WithTestEnvironmentVariable> interval_override;
    if (environment_interval) {
        interval_override.emplace("ECF_CHECKINTERVAL", std::to_string(*environment_interval));
    }

    const auto expected = environment_interval.value_or(configured_interval.value_or(CheckPt::default_interval()));
    const std::vector<std::string> args = {"ServerEnvironment"};
    ServerEnvironment serverEnv(args, config.path().string());
    BOOST_CHECK_EQUAL(serverEnv.checkPtInterval(), expected);
    std::string error;
    const bool valid = serverEnv.valid(error);
    BOOST_CHECK_EQUAL(valid, expected > 0);
    if (expected > 0) {
        BOOST_CHECK_MESSAGE(error.empty(), error);
    }
    else {
        BOOST_CHECK_MESSAGE(error.find("ECF_CHECKINTERVAL") != std::string::npos, error);
        BOOST_CHECK_MESSAGE(error.find("positive integer") != std::string::npos, error);
        BOOST_CHECK_MESSAGE(error.find(std::to_string(expected)) != std::string::npos, error);
    }
}

///
/// @brief Checks configuration discovery using the default filename in an isolated startup directory.
///
/// @param[in] startup_content Optional startup-directory configuration; absent leaves the file missing
/// @param[in] home_content Optional configuration in a separate server home
/// @param[in] config_selects_home Whether the startup configuration selects the separate home
/// @param[in] expected_interval Independently expected checkpoint interval in seconds
/// @param[in] expected_schedule Independently expected scheduling interval in seconds
///
void check_server_config_discovery(const std::optional<std::string>& startup_content,
                                   const std::optional<std::string>& home_content,
                                   bool config_selects_home,
                                   int expected_interval,
                                   int expected_schedule) {
    auto directory           = ecf::test::scaffold::MakeDirectory{}.create();
    const auto original_path = fs::current_path();
    const auto home_path     = directory.path() / "server home";
    const auto expected_home = home_content || config_selects_home ? home_path : directory.path();
    BOOST_SCOPE_EXIT_ALL(&) {
        Log::destroy();
        std::error_code error;
        fs::current_path(original_path, error);
        BOOST_CHECK_MESSAGE(!error, "Unable to restore test working directory: " << error.message());
    };

    WithoutTestEnvironmentVariable no_home("ECF_HOME");
    WithoutTestEnvironmentVariable no_port("ECF_PORT");
    WithoutTestEnvironmentVariable no_log("ECF_LOG");
    WithoutTestEnvironmentVariable no_check("ECF_CHECK");
    WithoutTestEnvironmentVariable no_checkold("ECF_CHECKOLD");
    WithoutTestEnvironmentVariable no_passwd("ECF_PASSWD");
    WithoutTestEnvironmentVariable no_custom_passwd("ECF_CUSTOM_PASSWD");
    WithoutTestEnvironmentVariable no_lists("ECF_LISTS");
    WithoutTestEnvironmentVariable no_interval("ECF_CHECKINTERVAL");
    WithoutTestEnvironmentVariable no_prune("ECF_PRUNE_NODE_LOG");
    WithoutTestEnvironmentVariable no_threshold("ECF_TASK_THRESHOLD");
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");
    WithoutTestEnvironmentVariable no_debug("ECF_DEBUG_SERVER");
    std::optional<WithTestEnvironmentVariable> home_override;
    std::optional<WithTestFile> startup_config;
    std::optional<WithTestFile> home_config;
    fs::create_directories(home_path);
    if (startup_content) {
        const auto content = (config_selects_home ? "ECF_HOME = " + home_path.string() + "\n" : "") + *startup_content;
        startup_config.emplace(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
    }
    if (home_content) {
        home_config.emplace(NamedTestFile{(home_path / "server_environment.cfg").string()}, *home_content);
        if (!config_selects_home) {
            home_override.emplace("ECF_HOME", home_path.string());
        }
    }
    fs::current_path(directory.path());

    const std::vector<std::string> args = {"ServerEnvironment"};
    ServerEnvironment serverEnv(args);
    std::string error;
    BOOST_REQUIRE_MESSAGE(serverEnv.valid(error), error);
    BOOST_CHECK_EQUAL(serverEnv.the_port(), "3141");
    BOOST_CHECK_EQUAL(serverEnv.checkPtInterval(), expected_interval);
    BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), expected_schedule);
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_INTERVAL"), std::to_string(expected_schedule));
    BOOST_CHECK_EQUAL(serverEnv.ecf_home(), expected_home.string());
    BOOST_CHECK(fs::equivalent(fs::current_path(), expected_home));
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_HOME"), expected_home.string());

    Host host;
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_LOG"), (expected_home / host.ecf_log_file("3141")).string());
    BOOST_CHECK_EQUAL(serverEnv.checkPtFilename(), (expected_home / host.ecf_checkpt_file("3141")).string());
    BOOST_CHECK_EQUAL(serverEnv.oldCheckPtFilename(), (expected_home / host.ecf_backup_checkpt_file("3141")).string());
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_CHECK"),
                      (expected_home / host.ecf_checkpt_file("3141")).string());
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_CHECKOLD"),
                      (expected_home / host.ecf_backup_checkpt_file("3141")).string());
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_PASSWD"), host.name() + ".3141.ecf.passwd");
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_CUSTOM_PASSWD"), host.name() + ".3141.ecf.custom_passwd");
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_LISTS"), host.ecf_lists_file("3141"));
    BOOST_CHECK(fs::is_regular_file(expected_home / host.ecf_log_file("3141")));
    if (expected_home != directory.path()) {
        BOOST_CHECK(!fs::exists(directory.path() / host.ecf_log_file("3141")));
    }
}

///
/// @brief Runs precedence assertions with isolated configuration, environment and command-line inputs.
///
/// @tparam Check Callable accepting the server environment and the startup directory
/// @param[in] content Configuration-file content
/// @param[in] environment Explicit environment overrides
/// @param[in] args Command-line tokens including the executable name
/// @param[in] check Assertions to run after successful initialisation and validation
///
template <typename Check>
void check_server_precedence(const std::string& content,
                             const std::vector<std::pair<std::string, std::string>>& environment,
                             const std::vector<std::string>& args,
                             Check check) {
    auto directory           = ecf::test::scaffold::MakeDirectory{}.create();
    const auto original_path = fs::current_path();
    const auto old_threshold = JobProfiler::task_threshold();
    BOOST_SCOPE_EXIT_ALL(&) {
        Log::destroy();
        JobProfiler::set_task_threshold(old_threshold);
        std::error_code error;
        fs::current_path(original_path, error);
        BOOST_CHECK_MESSAGE(!error, "Unable to restore test working directory: " << error.message());
    };

    std::list<WithoutTestEnvironmentVariable> cleared;
    for (const auto* name :
         {"ECF_HOME",           "ECF_PORT",          "ECF_LOG",          "ECF_CHECK",         "ECF_CHECKOLD",
          "ECF_PASSWD",         "ECF_CUSTOM_PASSWD", "ECF_LISTS",        "ECF_CHECKINTERVAL", "ECF_PRUNE_NODE_LOG",
          "ECF_TASK_THRESHOLD", "ECF_SSL",           "ECF_DEBUG_SERVER", "ECF_INTERVAL",      "ECF_CHECKMODE",
          "ECF_JOB_CMD",        "ECF_KILL_CMD",      "ECF_STATUS_CMD",   "ECF_CHECK_CMD",     "ECF_URL_CMD",
          "ECF_URL_BASE",       "ECF_URL",           "ECF_MICRODEF",     "ECF_MICRO"}) {
        cleared.emplace_back(name);
    }
    std::list<WithTestEnvironmentVariable> overrides;
    for (const auto& [name, value] : environment) {
        overrides.emplace_back(name, value);
    }
    JobProfiler::set_task_threshold(4000);
    fs::create_directories(directory.path() / "config home");
    fs::create_directories(directory.path() / "environment home");
    WithTestFile config(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
    fs::current_path(directory.path());

    ServerEnvironment serverEnv(args);
    std::string error;
    BOOST_REQUIRE_MESSAGE(serverEnv.valid(error), error);
    check(serverEnv, directory.path());
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_ServerEnvironment)

///
/// @brief Verifies home resolution from configuration before constructing default checkpoint paths.
///
BOOST_AUTO_TEST_CASE(test_server_home_resolution_from_config) {
    ECF_NAME_THIS_TEST();
    check_server_home_resolution(false, false, false);
}

///
/// @brief Verifies home resolution from environment overrides before changing directory.
///
BOOST_AUTO_TEST_CASE(test_server_home_resolution_from_environment) {
    ECF_NAME_THIS_TEST();
    check_server_home_resolution(true, false, false);
}

///
/// @brief Verifies that a duplicate nested home does not conceal incorrect path resolution.
///
BOOST_AUTO_TEST_CASE(test_server_home_resolution_with_nested_home) {
    ECF_NAME_THIS_TEST();
    check_server_home_resolution(false, true, false);
}

///
/// @brief Verifies explicit relative checkpoint paths and absolute backup paths with a selected home.
///
BOOST_AUTO_TEST_CASE(test_server_home_resolution_with_explicit_checkpoints) {
    ECF_NAME_THIS_TEST();
    check_server_home_resolution(true, true, true);
}

BOOST_AUTO_TEST_CASE(test_server_environment_ecfinterval) {
    ECF_NAME_THIS_TEST();

    // ecflow server interval is valid for range [1-60]
    std::string port = ecf::string_constants::default_port_number;
    for (int i = -10; i < 70; ++i) {
        std::string errorMsg;
        std::string argument          = "--ecfinterval=" + ecf::convert_to<std::string>(i);
        std::vector<std::string> args = {"ServerEnvironment", argument};
        ServerEnvironment serverEnv(args);
        bool valid = serverEnv.valid(errorMsg);
        if (i > 0 && i < 61) {
            BOOST_REQUIRE_MESSAGE(valid, "Server environment ecfinterval valid range is [1-60] " << errorMsg);
            BOOST_CHECK_MESSAGE(serverEnv.submitJobsInterval() == i,
                                "Expected submit jobs interval of " << i << " but found "
                                                                    << serverEnv.submitJobsInterval());
        }
        else {
            BOOST_CHECK_MESSAGE(!valid, "Server environment ecfinterval valid range is [1-60] " << errorMsg);
        }

        port = serverEnv.the_port();
    }

    Host h;
    fs::remove(h.ecf_log_file(port));
}

BOOST_AUTO_TEST_CASE(test_server_environment_port) {
    ECF_NAME_THIS_TEST();

    // The port numbers are divided into three ranges.\n";
    //  o the Well Known Ports, (require root permission)      0 -1023\n";
    //  o the Registered Ports,                             1024 -49151\n";
    //  o Dynamic and/or Private Ports.                    49151 -65535\n\n";
    //  Please set in the range 1024-49151 via argument or \n";
    Host h;
    {
        std::string errorMsg;
        std::vector<std::string> args = {"ServerEnvironment", "--port=0"};
        ServerEnvironment serverEnv(args);
        BOOST_CHECK_MESSAGE(!serverEnv.valid(errorMsg), " Server environment not valid " << errorMsg);
        fs::remove(h.ecf_log_file(serverEnv.the_port()));
    }
    {
        std::string errorMsg;
        std::vector<std::string> args = {"ServerEnvironment", "--port=1000"};
        ServerEnvironment serverEnv(args);
        BOOST_CHECK_MESSAGE(!serverEnv.valid(errorMsg), " Server environment not valid " << errorMsg);
        fs::remove(h.ecf_log_file(serverEnv.the_port()));
    }
    {
        std::string errorMsg;
        std::vector<std::string> args = {"ServerEnvironment", "--port=49151"};
        ServerEnvironment serverEnv(args);
        BOOST_CHECK_MESSAGE(!serverEnv.valid(errorMsg), " Server environment not valid " << errorMsg);
        fs::remove(h.ecf_log_file(serverEnv.the_port()));
    }
    {
        std::string errorMsg;
        std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
        ServerEnvironment serverEnv(args);
        BOOST_CHECK_MESSAGE(serverEnv.valid(errorMsg), " Server environment not valid " << errorMsg);
        BOOST_CHECK_MESSAGE(serverEnv.port() == 3144, "Expected 3144 but found " << serverEnv.port());
        fs::remove(h.ecf_log_file(serverEnv.the_port()));
    }
}

BOOST_AUTO_TEST_CASE(test_server_environment_log_file) {
    ECF_NAME_THIS_TEST();

    // Regression test log file creation

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args);

    BOOST_CHECK_MESSAGE(Log::instance(), "Log singleton not created");
    BOOST_CHECK_MESSAGE(fs::exists(Log::instance()->path()), "Log file not created");

    // Check that server variable ECF_LOG created and value is correct
    std::vector<std::pair<std::string, std::string>> server_vars;
    serverEnv.variables(server_vars);

    bool found_var = false;
    using mpair    = std::pair<std::string, std::string>;
    for (const mpair& p : server_vars) {
        if (ecf::environment::ECF_LOG == p.first) {
            BOOST_CHECK_MESSAGE(p.second == Log::instance()->path(),
                                "Expected " << Log::instance()->path() << " but found " << p.second);
            found_var = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(found_var, "Failed to find server variable ECF_LOG");

    // tear down remove the log file created by ServerEnvironment
    Host h;
    fs::remove(h.ecf_log_file(serverEnv.the_port()));

    /// Destroy Log singleton to avoid valgrind from complaining
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_logs_version_after_start_time) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args);
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");
    Log::instance()->flush();

    std::vector<std::string> lines;
    BOOST_REQUIRE(File::splitFileIntoLines(Log::instance()->path(), lines));

    // The log may hold earlier runs, so the last start-up is the one written by this test
    auto started = std::find_if(lines.rbegin(), lines.rend(), [](const std::string& line) {
        return line.find("] Started at ") != std::string::npos;
    });
    BOOST_REQUIRE_MESSAGE(started != lines.rend(), "Expected a 'Started at' line in the log");

    auto version = started.base(); // the line following 'Started at'
    BOOST_REQUIRE_MESSAGE(version != lines.end(), "Expected a line after 'Started at' in the log");
    BOOST_CHECK_MESSAGE(ecf::algorithm::starts_with(*version, "MSG:["),
                        "Expected a MSG line after 'Started at', but found: " << *version);
    BOOST_CHECK_MESSAGE(ecf::algorithm::ends_with(*version, "] " + Version::description()),
                        "Expected the version description after 'Started at', but found: " << *version);

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_logs_runtime_configuration_for_plain_connection) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args);
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");

    check_runtime_configuration(serverEnv, "TCP/IP v4");

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_logs_runtime_configuration_for_http_connection) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--http"};
    ServerEnvironment serverEnv(args);
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");

    check_runtime_configuration(serverEnv, "HTTP over TCP/IP v4");

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_logs_runtime_configuration_for_ipv6) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--v6"};
    ServerEnvironment serverEnv(args);
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");

    check_runtime_configuration(serverEnv, "TCP/IP v6");

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_logs_non_default_runtime_configuration) {
    ECF_NAME_THIS_TEST();

    WithTestFile config(NamedTestFile{"non_default.server_environment.cfg"},
                        "ECF_CHECKINTERVAL = 300\n"
                        "ECF_CHECKMODE = CHECK_ALWAYS\n"
                        "ECF_PRUNE_NODE_LOG = 7\n");

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--dis_job_gen"};
    ServerEnvironment serverEnv(args, "non_default.server_environment.cfg");
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");

    check_runtime_configuration(serverEnv, "TCP/IP v4");

    // The values are checked independently of the server environment that wrote them
    auto startup = last_startup_lines();
    for (const auto* line : {"  Job generation = disabled",
                             "  ECF_CHECKINTERVAL = 300",
                             "  ECF_CHECKMODE = CHECK_ALWAYS",
                             "  ECF_PRUNE_NODE_LOG = 7"}) {
        BOOST_CHECK_MESSAGE(std::find(startup.begin(), startup.end(), line) != startup.end(),
                            "Expected '" << line << "' in the runtime configuration");
    }

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_masks_credentials_in_runtime_configuration) {
    ECF_NAME_THIS_TEST();

    WithTestFile config(NamedTestFile{"masks_credentials.server_environment.cfg"},
                        "ECF_JOB_CMD = submit --token s3cr3t %ECF_JOB%\n"
                        "ECF_URL = https://user:s3cr3t@host/\n");

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args, "masks_credentials.server_environment.cfg");
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");

    auto startup = last_startup_lines();
    BOOST_CHECK(std::find(startup.begin(), startup.end(), "  ECF_JOB_CMD = submit --token *** %ECF_JOB%") !=
                startup.end());
    BOOST_CHECK(std::find(startup.begin(), startup.end(), "  ECF_URL = https://user:***@host/") != startup.end());
    for (const auto& line : startup) {
        BOOST_CHECK_MESSAGE(line.find("s3cr3t") == std::string::npos, "Unmasked credential in: " << line);
    }

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_config_file) {
    ECF_NAME_THIS_TEST();

    // Verify that the shipped configuration preserves the expected server variables and defaults.

    const auto config_path = File::test_data("libs/server/server_environment.cfg", "libs/server");
    BOOST_REQUIRE_MESSAGE(fs::is_regular_file(config_path), "Server configuration fixture not found: " << config_path);

    std::vector<std::string> args = {"ServerEnvironment"};
    ServerEnvironment serverEnv(args, config_path);

    std::vector<std::string> expected_variables = ServerEnvironment::expected_variables();

    std::vector<std::pair<std::string, std::string>> server_vars;
    serverEnv.variables(server_vars);
    for (const std::string& expected_var : expected_variables) {

        bool found_var = false;
        using s_pair   = std::pair<std::string, std::string>;
        for (const s_pair& p : server_vars) {
            if (expected_var == p.first) {
                found_var = true;
                break;
            }
        }
        BOOST_CHECK_MESSAGE(found_var, "Failed to find server var " << expected_var);
    }

    {
        // check other way, so that this test gets updated
        using mpair = std::pair<std::string, std::string>;
        for (const mpair& p : server_vars) {
            bool found_var = false;
            for (const std::string& expected_var : expected_variables) {
                if (expected_var == p.first) {
                    found_var = true;
                    break;
                }
            }
            BOOST_CHECK_MESSAGE(found_var, "Failed to update test for server var " << p.first);
        }
    }

    // Check the values in the server config file, are the *SAME* as the defaults, when config is *NOT* present
    // Please note do *NOT* use quotes for the values, otherwise quotes get added.
    // WRONG: ECF_MICRODEF = "%"
    // RIGHT: ECF_MICRODEF = %
    // o IGNORE ECF_CHECK: We *ONLY check those value in the config, that should not be altered.
    //                     since we add root path, and append with host/port
    // o ignore ECF_CHECKMODE: not a server variable
    //
    using mpair = std::pair<std::string, std::string>;
    for (const mpair& p : server_vars) {
        // std::cout << "server variables " << p.first << "  " << p.second << "\n";
        if (ecf::environment::ECF_HOME == p.first) {
            BOOST_CHECK_MESSAGE(p.second == fs::current_path().string(),
                                "for ECF_HOME expected " << fs::current_path().string() << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_PORT") == p.first && !ecf::environment::has("ECF_PORT")) {
            BOOST_CHECK_MESSAGE(p.second == ecf::string_constants::default_port_number,
                                "for ECF_PORT expected " << ecf::string_constants::default_port_number << " but found "
                                                         << p.second);
            continue;
        }
        if (std::string("ECF_CHECKINTERVAL") == p.first) {
            std::string expected = ecf::convert_to<std::string>(CheckPt::default_interval());
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_CHECKINTERVAL expected " << CheckPt::default_interval() << " but found "
                                                                  << p.second);
            continue;
        }
        if (std::string("ECF_INTERVAL") == p.first) {
            std::string expected = "60";
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_INTERVAL expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_JOB_CMD") == p.first) {
            std::string expected = Ecf::JOB_CMD();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_JOB_CMD expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_KILL_CMD") == p.first) {
            std::string expected = Ecf::KILL_CMD();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_KILL_CMD expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_STATUS_CMD") == p.first) {
            std::string expected = Ecf::STATUS_CMD();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_STATUS_CMD expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_CHECK_CMD") == p.first) {
            std::string expected = Ecf::CHECK_CMD();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_CHECK_CMD expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_URL_CMD") == p.first) {
            std::string expected = Ecf::URL_CMD();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_URL_CMD expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_URL_BASE") == p.first) {
            std::string expected = Ecf::URL_BASE();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_URL_BASE expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_URL") == p.first) {
            std::string expected = Ecf::URL();
            BOOST_CHECK_MESSAGE(p.second == expected, "for ECF_URL expected " << expected << " but found " << p.second);
            continue;
        }
        if (std::string("ECF_MICRODEF") == p.first) {
            std::string expected = Ecf::MICRO();
            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_MICRODEF expected " << expected << " but found " << p.second);
            continue;
        }

        if (std::string("ECF_PASSWD") == p.first) {

            Host host;
            std::string port = ecf::string_constants::default_port_number;
            if (ecf::environment::has("ECF_PORT")) {
                port = ecf::environment::get("ECF_PORT");
            }
            std::string expected = host.prefix_host_and_port(port, AuthenticationService::default_passwd_file());

            BOOST_CHECK_MESSAGE(p.second == expected,
                                "for ECF_PASSWD expected " << expected << " but found " << p.second);
            continue;
        }
    }

    // tear down remove the log file created by ServerEnvironment
    Host host;
    fs::remove(host.ecf_log_file(serverEnv.the_port()));
}

///
/// @brief Verifies that configuration-file values replace the built-in defaults.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_applies_non_default_values) {
    ECF_NAME_THIS_TEST();

    WithTestFile config(AutomaticTestFile{"server_environment.cfg"},
                        "ECF_CHECKINTERVAL = 137\n"
                        "ECF_CHECKMODE = CHECK_ALWAYS\n"
                        "ECF_INTERVAL = 17\n");
    WithTestFile log_file(AutomaticTestFile{"server-config.log"}, "");
    WithTestEnvironmentVariable ecf_log("ECF_LOG", log_file.path().string());
    WithoutTestEnvironmentVariable no_check_interval("ECF_CHECKINTERVAL");

    const std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args, config.path().string());

    BOOST_CHECK_EQUAL(serverEnv.checkPtInterval(), 137);
    BOOST_CHECK(serverEnv.checkMode() == CheckPt::ALWAYS);
    BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), 17);
    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_INTERVAL"), "17");
}

///
/// @brief Verifies that malformed configuration fails with its original parser diagnostic.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_rejects_parse_errors) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::pair<std::string, std::string>> invalid_configs = {
        {"[unterminated\n", "unterminated"},
        {"UNKNOWN_SERVER_OPTION = 1\n", "UNKNOWN_SERVER_OPTION"},
        {"ECF_PORT = not_a_config_port\n", "not_a_config_port"},
        {"ECF_PORT = 99999999999999999999999999999999999999\n", "99999999999999999999999999999999999999"},
        {"ECF_PORT = 3141\nECF_PORT = 3142\n", "ECF_PORT"},
        {"ECF_INTERVAL = 17\nECF_JOB_CMD = configured_submit\nECF_PORT = invalid_after_valid_entries\n",
         "invalid_after_valid_entries"}};
    for (const auto& [content, expected_error] : invalid_configs) {
        BOOST_TEST_CONTEXT("Invalid config: " << content) {
            check_server_config_parse_error(content, expected_error);
        }
    }
}

///
/// @brief Verifies that environment parsing cannot replace an earlier configuration error.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_error_precedes_environment_error) {
    ECF_NAME_THIS_TEST();
    check_server_config_parse_error("UNKNOWN_SERVER_OPTION = 1\n", "UNKNOWN_SERVER_OPTION", true);
}

///
/// @brief Verifies the supported checkpoint modes and the default when the setting is absent.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_check_modes) {
    ECF_NAME_THIS_TEST();

    auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    WithTestEnvironmentVariable home("ECF_HOME", fs::current_path().string());
    WithTestEnvironmentVariable port("ECF_PORT", "3144");
    const std::vector<std::pair<std::string, CheckPt::Mode>> modes = {{"CHECK_NEVER", CheckPt::NEVER},
                                                                      {"CHECK_ON_TIME", CheckPt::ON_TIME},
                                                                      {"CHECK_ALWAYS", CheckPt::ALWAYS},
                                                                      {"", CheckPt::ON_TIME}};
    for (const auto& [name, expected_mode] : modes) {
        BOOST_TEST_CONTEXT("Checkpoint mode: " << (name.empty() ? "absent" : name)) {
            std::string content = "ECF_INTERVAL = 17\n";
            if (!name.empty()) {
                content += "ECF_CHECKMODE =  " + name + "  # Mode comment\n";
            }
            WithTestFile config(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
            WithTestEnvironmentVariable log("ECF_LOG", (directory.path() / "server.log").string());
            const std::vector<std::string> args = {"ServerEnvironment"};
            ServerEnvironment serverEnv(args, config.path().string());

            BOOST_CHECK(serverEnv.checkMode() == expected_mode);
            BOOST_CHECK_EQUAL(serverEnv.check_mode_str(), name.empty() ? "CHECK_ON_TIME" : name);
            BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), 17);
        }
    }
}

///
/// @brief Verifies that unsupported or explicitly empty checkpoint modes cannot silently select the default.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_check_modes_reject_invalid_values) {
    ECF_NAME_THIS_TEST();

    for (const auto& value : std::vector<std::string>{
             "TYPO", "check_on_time", "UNDEFINED", "CHECK_NEVER_EXTRA", "0", "", "   ", "\"CHECK_ON_TIME\""}) {
        BOOST_TEST_CONTEXT("Invalid checkpoint mode: '" << value << "'") {
            const auto parsed_value   = value.find_first_not_of(' ') == std::string::npos ? std::string{} : value;
            const auto expected_error = "Invalid ECF_CHECKMODE value '" + parsed_value +
                                        "'. Expected CHECK_NEVER, CHECK_ON_TIME or CHECK_ALWAYS.";
            check_server_config_parse_error("ECF_CHECKMODE = " + value + "\n", expected_error);
        }
    }
}

///
/// @brief Verifies checkpoint-interval boundaries read from the configuration file.
///
BOOST_AUTO_TEST_CASE(test_server_checkpoint_interval_from_config) {
    ECF_NAME_THIS_TEST();

    for (const int interval :
         {std::numeric_limits<int>::min(), -120, -1, 0, 1, 120, 137, std::numeric_limits<int>::max()}) {
        BOOST_TEST_CONTEXT("Configured checkpoint interval: " << interval) {
            check_server_checkpoint_interval(interval, std::nullopt);
        }
    }
}

///
/// @brief Verifies checkpoint-interval boundaries selected by an environment override.
///
BOOST_AUTO_TEST_CASE(test_server_checkpoint_interval_from_environment) {
    ECF_NAME_THIS_TEST();

    for (const int interval :
         {std::numeric_limits<int>::min(), -120, -1, 0, 1, 120, 137, std::numeric_limits<int>::max()}) {
        BOOST_TEST_CONTEXT("Environment checkpoint interval: " << interval) {
            check_server_checkpoint_interval(300, interval);
        }
    }
}

///
/// @brief Verifies checkpoint-interval defaults and validation after environment overrides.
///
BOOST_AUTO_TEST_CASE(test_server_checkpoint_interval_defaults_and_precedence) {
    ECF_NAME_THIS_TEST();

    check_server_checkpoint_interval(std::nullopt, std::nullopt);
    check_server_checkpoint_interval(std::nullopt, 137);
    check_server_checkpoint_interval(-1, 1);
    check_server_checkpoint_interval(0, 120);
}

///
/// @brief Verifies that missing, empty and comments-only configuration files still apply defaults.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_optional_content_uses_defaults) {
    ECF_NAME_THIS_TEST();

    auto directory         = ecf::test::scaffold::MakeDirectory{}.create();
    const auto config_path = directory.path() / "server_environment.cfg";
    WithTestEnvironmentVariable home("ECF_HOME", fs::current_path().string());
    WithTestEnvironmentVariable port("ECF_PORT", "3144");
    WithoutTestEnvironmentVariable no_check_interval("ECF_CHECKINTERVAL");
    WithoutTestEnvironmentVariable no_prune("ECF_PRUNE_NODE_LOG");
    WithoutTestEnvironmentVariable no_threshold("ECF_TASK_THRESHOLD");
    const std::vector<std::optional<std::string>> contents = {std::nullopt, "", "# Comment only\n\n"};
    for (size_t i = 0; i < contents.size(); ++i) {
        BOOST_TEST_CONTEXT("Optional config case " << i) {
            std::optional<WithTestFile> config;
            if (contents[i]) {
                config.emplace(NamedTestFile{config_path.string()}, *contents[i]);
            }
            const auto log_path = directory.path() / ("server-" + std::to_string(i) + ".log");
            WithTestEnvironmentVariable log("ECF_LOG", log_path.string());
            const std::vector<std::string> args = {"ServerEnvironment"};
            ServerEnvironment serverEnv(args, config_path.string());

            BOOST_CHECK_EQUAL(serverEnv.checkPtInterval(), CheckPt::default_interval());
            BOOST_CHECK(serverEnv.checkMode() == CheckPt::ON_TIME);
            BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), 60);
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_JOB_CMD"), Ecf::JOB_CMD());
            BOOST_CHECK(fs::exists(log_path));
        }
    }
}

///
/// @brief Verifies discovery defaults when the startup configuration is absent.
///
BOOST_AUTO_TEST_CASE(test_server_config_discovery_missing) {
    ECF_NAME_THIS_TEST();
    check_server_config_discovery(std::nullopt, std::nullopt, false, 120, 60);
}

///
/// @brief Verifies discovery defaults for empty and comments-only startup configurations.
///
BOOST_AUTO_TEST_CASE(test_server_config_discovery_empty) {
    ECF_NAME_THIS_TEST();
    for (const auto& content : std::vector<std::string>{"", "# Comment only\n\n"}) {
        BOOST_TEST_CONTEXT("Startup configuration: '" << content << "'") {
            check_server_config_discovery(content, std::nullopt, false, 120, 60);
        }
    }
}

///
/// @brief Verifies that default discovery applies distinctive startup-directory settings.
///
BOOST_AUTO_TEST_CASE(test_server_config_discovery_startup_directory) {
    ECF_NAME_THIS_TEST();
    check_server_config_discovery("ECF_CHECKINTERVAL = 137\nECF_INTERVAL = 17\n", std::nullopt, false, 137, 17);
}

///
/// @brief Verifies that a configuration found only in server home is not discovered.
///
BOOST_AUTO_TEST_CASE(test_server_config_discovery_ignores_home_only) {
    ECF_NAME_THIS_TEST();
    check_server_config_discovery(std::nullopt, "ECF_CHECKINTERVAL = 251\nECF_INTERVAL = 23\n", false, 120, 60);
}

///
/// @brief Verifies that conflicting server-home settings cannot replace startup-directory settings.
///
BOOST_AUTO_TEST_CASE(test_server_config_discovery_startup_wins) {
    ECF_NAME_THIS_TEST();
    check_server_config_discovery(
        "ECF_CHECKINTERVAL = 137\nECF_INTERVAL = 17\n", "ECF_CHECKINTERVAL = 251\nECF_INTERVAL = 23\n", false, 137, 17);
}

///
/// @brief Verifies that the startup configuration is fully read before changing to its selected home.
///
BOOST_AUTO_TEST_CASE(test_server_config_discovery_config_selects_home) {
    ECF_NAME_THIS_TEST();
    check_server_config_discovery(
        "ECF_CHECKINTERVAL = 137\nECF_INTERVAL = 17\n", "ECF_CHECKINTERVAL = 251\nECF_INTERVAL = 23\n", true, 137, 17);
}

///
/// @brief Verifies all eight port-source combinations and the dependent default filenames.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_ports) {
    ECF_NAME_THIS_TEST();
    const std::vector<std::string> expected_ports = {"3141", "3142", "3143", "3143", "3144", "3144", "3144", "3144"};
    for (unsigned sources = 0; sources < expected_ports.size(); ++sources) {
        BOOST_TEST_CONTEXT("Port source mask: " << sources) {
            const auto content = sources & 1 ? "ECF_PORT = 3142\n" : "";
            std::vector<std::pair<std::string, std::string>> environment;
            if (sources & 2) {
                environment.emplace_back("ECF_PORT", "3143");
            }
            std::vector<std::string> args = {"ServerEnvironment"};
            if (sources & 4) {
                args.emplace_back("--port=3144");
            }
            check_server_precedence(content, environment, args, [&](const auto& serverEnv, const auto& startup) {
                const auto& port  = expected_ports[sources];
                const auto prefix = Host{}.name() + "." + port + ".";
                BOOST_CHECK_EQUAL(serverEnv.the_port(), port);
                BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_PORT"), port);
                for (const auto& [name, suffix] :
                     std::vector<std::pair<std::string, std::string>>{{"ECF_LOG", "ecf.log"},
                                                                      {"ECF_CHECK", "ecf.check"},
                                                                      {"ECF_CHECKOLD", "ecf.check.b"},
                                                                      {"ECF_PASSWD", "ecf.passwd"},
                                                                      {"ECF_CUSTOM_PASSWD", "ecf.custom_passwd"},
                                                                      {"ECF_LISTS", "ecf.lists"}}) {
                    const auto expected = name == "ECF_LOG" || name == "ECF_CHECK" || name == "ECF_CHECKOLD"
                                              ? (startup / (prefix + suffix)).string()
                                              : prefix + suffix;
                    BOOST_CHECK_EQUAL(server_variable(serverEnv, name), expected);
                }
                BOOST_CHECK_EQUAL(serverEnv.checkPtFilename(), (startup / (prefix + "ecf.check")).string());
                BOOST_CHECK_EQUAL(serverEnv.oldCheckPtFilename(), (startup / (prefix + "ecf.check.b")).string());
                BOOST_CHECK(fs::is_regular_file(startup / (prefix + "ecf.log")));
                const auto lines = last_startup_lines();
                BOOST_CHECK(std::find(lines.begin(), lines.end(), "  Port = " + port) != lines.end());
            });
        }
    }
}

///
/// @brief Verifies default, config-only, environment-only and conflicting path selections.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_paths) {
    ECF_NAME_THIS_TEST();
    const std::vector<std::pair<std::string, std::string>> settings = {{"ECF_HOME", ""},
                                                                       {"ECF_LOG", "ecf.log"},
                                                                       {"ECF_CHECK", "ecf.check"},
                                                                       {"ECF_CHECKOLD", "ecf.check.b"},
                                                                       {"ECF_PASSWD", "ecf.passwd"},
                                                                       {"ECF_CUSTOM_PASSWD", "ecf.custom_passwd"},
                                                                       {"ECF_LISTS", "ecf.lists"}};
    for (const auto& setting : settings) {
        const auto& name         = setting.first;
        const auto& default_name = setting.second;
        for (unsigned sources = 0; sources < 4; ++sources) {
            BOOST_TEST_CONTEXT(name << ", source mask: " << sources) {
                const auto config_value      = name == "ECF_HOME" ? "config home" : "config.file";
                const auto environment_value = name == "ECF_HOME" ? "environment home" : "environment.file";
                const auto content           = sources & 1 ? name + " = " + config_value + "\n" : "";
                std::vector<std::pair<std::string, std::string>> environment;
                if (sources & 2) {
                    environment.emplace_back(name, environment_value);
                }
                check_server_precedence(
                    content, environment, {"ServerEnvironment"}, [&](const auto& serverEnv, const auto& startup) {
                        const std::string selected = sources & 2 ? environment_value : config_value;
                        const auto home            = name == "ECF_HOME" && sources ? startup / selected : startup;
                        std::string expected;
                        if (name == "ECF_HOME") {
                            expected = home.string();
                        }
                        else {
                            const auto filename = sources ? selected : Host{}.name() + ".3141." + default_name;
                            expected            = name == "ECF_LOG" || name == "ECF_CHECK" || name == "ECF_CHECKOLD"
                                                      ? (home / filename).string()
                                                      : filename;
                        }
                        BOOST_CHECK_EQUAL(server_variable(serverEnv, name), expected);
                        BOOST_CHECK_EQUAL(serverEnv.ecf_home(), home.string());
                        BOOST_CHECK(fs::equivalent(fs::current_path(), home));
                        const auto log_name = name == "ECF_LOG" && sources ? selected : Host{}.name() + ".3141.ecf.log";
                        BOOST_CHECK(fs::is_regular_file(home / log_name));
                    });
            }
        }
    }
}

///
/// @brief Verifies config and environment precedence for the three numeric environment settings.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_numeric_settings) {
    ECF_NAME_THIS_TEST();
    for (const auto& setting : std::vector<std::pair<std::string, int>>{
             {"ECF_CHECKINTERVAL", 120}, {"ECF_PRUNE_NODE_LOG", 30}, {"ECF_TASK_THRESHOLD", 4000}}) {
        const auto& name        = setting.first;
        const int default_value = setting.second;
        for (unsigned sources = 0; sources < 4; ++sources) {
            BOOST_TEST_CONTEXT(name << ", source mask: " << sources) {
                const auto content = sources & 1 ? name + " = 137\n" : "";
                std::vector<std::pair<std::string, std::string>> environment;
                if (sources & 2) {
                    environment.emplace_back(name, "251");
                }
                const int expected = sources & 2 ? 251 : sources & 1 ? 137 : default_value;
                check_server_precedence(
                    content, environment, {"ServerEnvironment"}, [&](const auto& serverEnv, const auto&) {
                        const int actual = name == "ECF_CHECKINTERVAL"    ? serverEnv.checkPtInterval()
                                           : name == "ECF_PRUNE_NODE_LOG" ? serverEnv.ecf_prune_node_log()
                                                                          : JobProfiler::task_threshold();
                        BOOST_CHECK_EQUAL(actual, expected);
                        const auto lines = last_startup_lines();
                        BOOST_CHECK(std::find(lines.begin(),
                                              lines.end(),
                                              "  " + name + " = " + std::to_string(expected)) != lines.end());
                    });
            }
        }
    }
}

///
/// @brief Verifies scheduling defaults and CLI precedence over configuration.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_scheduling_interval) {
    ECF_NAME_THIS_TEST();
    for (unsigned sources = 0; sources < 4; ++sources) {
        BOOST_TEST_CONTEXT("Scheduling source mask: " << sources) {
            const auto content            = sources & 1 ? "ECF_INTERVAL = 17\n" : "";
            std::vector<std::string> args = {"ServerEnvironment"};
            if (sources & 2) {
                args.emplace_back("--ecfinterval=23");
            }
            check_server_precedence(content, {}, args, [&](const auto& serverEnv, const auto&) {
                const int expected = sources & 2 ? 23 : sources & 1 ? 17 : 60;
                BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), expected);
                BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_INTERVAL"), std::to_string(expected));
            });
        }
    }
}

///
/// @brief Verifies that independent selections from all three sources coexist in one initialisation.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_mixed_sources) {
    ECF_NAME_THIS_TEST();
    check_server_precedence(
        "ECF_HOME = config home\nECF_PORT = 3142\nECF_CHECKINTERVAL = 137\nECF_INTERVAL = 17\n"
        "ECF_CHECK = config.check\nECF_JOB_CMD = configured_submit\nECF_PRUNE_NODE_LOG = 19\n",
        {{"ECF_HOME", "environment home"},
         {"ECF_PORT", "3143"},
         {"ECF_CHECKINTERVAL", "251"},
         {"ECF_LOG", "environment.log"}},
        {"ServerEnvironment", "--port=3144", "--ecfinterval=23"},
        [](const auto& serverEnv, const auto& startup) {
            const auto home = startup / "environment home";
            BOOST_CHECK_EQUAL(serverEnv.the_port(), "3144");
            BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), 23);
            BOOST_CHECK_EQUAL(serverEnv.checkPtInterval(), 251);
            BOOST_CHECK_EQUAL(serverEnv.ecf_prune_node_log(), 19);
            BOOST_CHECK_EQUAL(serverEnv.ecf_home(), home.string());
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_LOG"), (home / "environment.log").string());
            BOOST_CHECK_EQUAL(serverEnv.checkPtFilename(), (home / "config.check").string());
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_JOB_CMD"), "configured_submit");
            BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_PASSWD"), Host{}.name() + ".3144.ecf.passwd");
            BOOST_CHECK(fs::is_regular_file(home / "environment.log"));
        });
}

///
/// @brief Verifies that config-only settings do not acquire unsupported environment overrides.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_config_only_settings) {
    ECF_NAME_THIS_TEST();
    const std::vector<std::pair<std::string, std::string>> commands = {{"ECF_JOB_CMD", "ECF_JOB_CMD"},
                                                                       {"ECF_KILL_CMD", "ECF_KILL_CMD"},
                                                                       {"ECF_STATUS_CMD", "ECF_STATUS_CMD"},
                                                                       {"ECF_CHECK_CMD", "ECF_CHECK_CMD"},
                                                                       {"ECF_URL_CMD", "ECF_URL_CMD"},
                                                                       {"ECF_URL_BASE", "ECF_URL_BASE"},
                                                                       {"ECF_URL", "ECF_URL"},
                                                                       {"ECF_MICRODEF", "ECF_MICRO"}};
    for (const bool configured : {false, true}) {
        BOOST_TEST_CONTEXT("Config-only settings present: " << configured) {
            std::string content = configured ? "ECF_INTERVAL = 17\nECF_CHECKMODE = CHECK_ALWAYS\n" : "";
            std::vector<std::pair<std::string, std::string>> environment = {
                {"ECF_INTERVAL", "23"}, {"ECF_CHECKMODE", "CHECK_NEVER"}, {"ECF_MICRO", "!"}};
            for (const auto& [key, exported] : commands) {
                if (configured) {
                    content += key + " = " + (key == "ECF_MICRODEF" ? "@" : "config_" + key) + "\n";
                }
                environment.emplace_back(key, key == "ECF_MICRODEF" ? "!" : "environment_" + key);
            }
            check_server_precedence(
                content, environment, {"ServerEnvironment"}, [&](const auto& serverEnv, const auto&) {
                    BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), configured ? 17 : 60);
                    BOOST_CHECK(serverEnv.checkMode() == (configured ? CheckPt::ALWAYS : CheckPt::ON_TIME));
                    const std::vector<std::string> defaults = {Ecf::JOB_CMD(),
                                                               Ecf::KILL_CMD(),
                                                               Ecf::STATUS_CMD(),
                                                               Ecf::CHECK_CMD(),
                                                               Ecf::URL_CMD(),
                                                               Ecf::URL_BASE(),
                                                               Ecf::URL(),
                                                               "%"};
                    for (size_t i = 0; i < commands.size(); ++i) {
                        const auto& [key, exported] = commands[i];
                        const auto expected =
                            configured ? (key == "ECF_MICRODEF" ? "@" : "config_" + key) : defaults[i];
                        BOOST_CHECK_EQUAL(server_variable(serverEnv, exported), expected);
                    }
                });
        }
    }
}

///
/// @brief Verifies that valid environment and CLI overrides cannot conceal a configuration parsing error.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_config_error_before_cli) {
    ECF_NAME_THIS_TEST();
    const auto original_path = fs::current_path();
    BOOST_CHECK_EXCEPTION(check_server_precedence("ECF_PORT = invalid_config_port\n",
                                                  {{"ECF_PORT", "3143"}},
                                                  {"ServerEnvironment", "--port=3144"},
                                                  [](const auto&, const auto&) {
                                                      BOOST_FAIL("Invalid configuration must not be accepted");
                                                  }),
                          ServerEnvironmentException,
                          [](const ServerEnvironmentException& error) {
                              const std::string message = error.what();
                              return message.find("server_environment.cfg") != std::string::npos &&
                                     message.find("invalid_config_port") != std::string::npos;
                          });
    BOOST_CHECK(fs::current_path() == original_path);
    BOOST_CHECK(!Log::instance());
}

BOOST_AUTO_TEST_CASE(test_server_environment_variables) {
    ECF_NAME_THIS_TEST();

    // Regression test to make sure the server environment variable do not get removed

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args);

    std::vector<std::string> expected_variables = ServerEnvironment::expected_variables();

    std::vector<std::pair<std::string, std::string>> server_vars;
    serverEnv.variables(server_vars);
    for (const std::string& expected_var : expected_variables) {

        bool found_var = false;
        using mpair    = std::pair<std::string, std::string>;
        for (const mpair& p : server_vars) {
            if (expected_var == p.first) {
                found_var = true;
                break;
            }
        }
        BOOST_CHECK_MESSAGE(found_var, "Failed to find server var " << expected_var);
    }

    // check other way, so that this test gets updated
    using mpair = std::pair<std::string, std::string>;
    for (const mpair& p : server_vars) {
        bool found_var = false;
        for (const std::string& expected_var : expected_variables) {
            if (expected_var == p.first) {
                found_var = true;
                break;
            }
        }
        BOOST_CHECK_MESSAGE(found_var, "Failed to update test for server var " << p.first);
    }

    // tear down remove the log file created by ServerEnvironment
    Host h;
    fs::remove(h.ecf_log_file(serverEnv.the_port()));

    /// Destroy Log singleton to avoid valgrind from complaining
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_profile_threshold_environment_variable) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment"};
    {
        auto* put = const_cast<char*>("ECF_TASK_THRESHOLD=9");
        BOOST_CHECK_MESSAGE(putenv(put) == 0, "putenv failed for " << put);
    }
    ServerEnvironment serverEnv(args);
    BOOST_CHECK_MESSAGE(JobProfiler::task_threshold() == 9,
                        "Expected task threshold of 9 but found " << JobProfiler::task_threshold());

    // ==================================================================================
    // Note test for errors
    std::vector<std::string> dodgy_thresholds;
    dodgy_thresholds.emplace_back("ECF_TASK_THRESHOLD=x");
    dodgy_thresholds.emplace_back("ECF_TASK_THRESHOLD=,");
    dodgy_thresholds.emplace_back("ECF_TASK_THRESHOLD=:");
    dodgy_thresholds.emplace_back("ECF_TASK_THRESHOLD=,,");

    for (auto& dodgy_threshold : dodgy_thresholds) {
        // cout << "check -------> " << dodgy_thresholds[i] << endl;
        BOOST_CHECK_MESSAGE(putenv(const_cast<char*>(dodgy_threshold.c_str())) == 0,
                            "putenv failed for " << dodgy_threshold);
        BOOST_CHECK_THROW(ServerEnvironment serverEnv(args), std::runtime_error);
    }

    unsetenv(const_cast<char*>(
        "ECF_TASK_THRESHOLD")); // remove from env, otherwise valgrind complains, *AND* affects other tests

    Host h;
    fs::remove(h.ecf_log_file(serverEnv.the_port()));

    /// Destroy Log singleton to avoid valgrind from complaining
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_protocol_is_plain_by_default) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args);

    BOOST_CHECK_MESSAGE(serverEnv.protocol() == ecf::Protocol::Plain,
                        "Expected protocol PLAIN but found " << ecf::to_ui_designation(serverEnv.protocol()));

    remove_log_file(serverEnv);
}

BOOST_AUTO_TEST_CASE(test_server_environment_protocol_is_http_when_requested) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--http"};
    ServerEnvironment serverEnv(args);

    BOOST_CHECK_MESSAGE(serverEnv.protocol() == ecf::Protocol::Http,
                        "Expected protocol HTTP but found " << ecf::to_ui_designation(serverEnv.protocol()));

    remove_log_file(serverEnv);
}

#ifdef ECF_OPENSSL

BOOST_AUTO_TEST_CASE(test_server_environment_logs_runtime_configuration_for_ssl_connection) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", "./");
    WithoutTestEnvironmentVariable no_ecf_ssl(ecf::environment::ECF_SSL);
    WithTestFile shared_crt(NamedTestFile{"server.crt"});

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--ssl"};
    ServerEnvironment serverEnv(args);
    BOOST_REQUIRE_MESSAGE(Log::instance(), "Log singleton not created");
    BOOST_REQUIRE_MESSAGE(serverEnv.ssl(), "Expected SSL to be enabled");

    check_runtime_configuration(
        serverEnv, "TCP/IP v4 with SSL", serverEnv.openssl().certificate()->crt_file().string() + " (shared)");

    remove_log_file(serverEnv);
    Log::destroy();
}

BOOST_AUTO_TEST_CASE(test_server_environment_protocol_is_promoted_to_ssl_when_ssl_is_enabled) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", "./");
    WithoutTestEnvironmentVariable no_ecf_ssl(ecf::environment::ECF_SSL);
    WithTestFile shared_crt(NamedTestFile{"server.crt"});

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--ssl"};
    ServerEnvironment serverEnv(args);

    BOOST_REQUIRE_MESSAGE(serverEnv.ssl(), "Expected SSL to be enabled");
    BOOST_CHECK_MESSAGE(serverEnv.protocol() == ecf::Protocol::Ssl,
                        "Expected the TCP/IP protocol to be promoted to SSL, but found "
                            << ecf::to_ui_designation(serverEnv.protocol()));

    remove_log_file(serverEnv);
}

BOOST_AUTO_TEST_CASE(test_server_environment_protocol_is_promoted_to_https_when_ssl_is_enabled) {
    ECF_NAME_THIS_TEST();

    // Notice that this also pins the order in which the options are handled: the promotion only yields HTTPS
    // because the HTTP option is processed before SSL is enabled. Were that order reversed, the protocol
    // reported by an encrypted HTTP server would silently degrade to HTTP.

    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", "./");
    WithoutTestEnvironmentVariable no_ecf_ssl(ecf::environment::ECF_SSL);
    WithTestFile shared_crt(NamedTestFile{"server.crt"});

    std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--http", "--ssl"};
    ServerEnvironment serverEnv(args);

    BOOST_REQUIRE_MESSAGE(serverEnv.ssl(), "Expected SSL to be enabled");
    BOOST_CHECK_MESSAGE(serverEnv.protocol() == ecf::Protocol::Https,
                        "Expected the HTTP protocol to be promoted to HTTPS, but found "
                            << ecf::to_ui_designation(serverEnv.protocol()));

    remove_log_file(serverEnv);
}

BOOST_AUTO_TEST_CASE(test_server_environment_protocol_is_unchanged_when_ssl_is_not_enabled) {
    ECF_NAME_THIS_TEST();

    // With ECF_SSL undefined, enabling SSL is a no-operation, and the protocol must not be promoted.
    // This is the outcome reached whenever SSL is requested but no certificate is found.

    WithoutTestEnvironmentVariable no_ecf_ssl(ecf::environment::ECF_SSL);

    {
        std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
        ServerEnvironment serverEnv(args);

        serverEnv.enable_ssl_if_defined();

        BOOST_REQUIRE_MESSAGE(!serverEnv.ssl(), "Expected SSL to remain disabled");
        BOOST_CHECK_MESSAGE(serverEnv.protocol() == ecf::Protocol::Plain,
                            "Expected protocol to remain PLAIN but found "
                                << ecf::to_ui_designation(serverEnv.protocol()));

        remove_log_file(serverEnv);
    }
    {
        std::vector<std::string> args = {"ServerEnvironment", "--port=3144", "--http"};
        ServerEnvironment serverEnv(args);

        serverEnv.enable_ssl_if_defined();

        BOOST_REQUIRE_MESSAGE(!serverEnv.ssl(), "Expected SSL to remain disabled");
        BOOST_CHECK_MESSAGE(serverEnv.protocol() == ecf::Protocol::Http,
                            "Expected protocol to remain HTTP but found "
                                << ecf::to_ui_designation(serverEnv.protocol()));

        remove_log_file(serverEnv);
    }
}

#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
