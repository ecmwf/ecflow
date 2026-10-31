// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
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
#include "ecflow/node/Defs.hpp"
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
/// @brief Isolates a server start-up from the ambient environment and from the global state of other tests.
///
/// Clears every environment variable read by the server, and restores the working directory, the profiling
/// threshold and the server flag on destruction, after destroying the log.
///
class IsolatedServerStartup {
public:
    IsolatedServerStartup()
        : original_path_{fs::current_path()},
          original_threshold_{JobProfiler::task_threshold()},
          original_server_{Ecf::server()} {
        for (const auto* name :
             {"ECF_HOME",           "ECF_PORT",          "ECF_LOG",          "ECF_CHECK",         "ECF_CHECKOLD",
              "ECF_PASSWD",         "ECF_CUSTOM_PASSWD", "ECF_LISTS",        "ECF_CHECKINTERVAL", "ECF_PRUNE_NODE_LOG",
              "ECF_TASK_THRESHOLD", "ECF_SSL",           "ECF_DEBUG_SERVER", "ECF_INTERVAL",      "ECF_CHECKMODE",
              "ECF_JOB_CMD",        "ECF_KILL_CMD",      "ECF_STATUS_CMD",   "ECF_CHECK_CMD",     "ECF_URL_CMD",
              "ECF_URL_BASE",       "ECF_URL",           "ECF_MICRODEF",     "ECF_MICRO"}) {
            cleared_.emplace_back(name);
        }
    }

    IsolatedServerStartup(const IsolatedServerStartup&)            = delete;
    IsolatedServerStartup& operator=(const IsolatedServerStartup&) = delete;

    ~IsolatedServerStartup() {
        Log::destroy();
        JobProfiler::set_task_threshold(original_threshold_);
        Ecf::set_server(original_server_);
        std::error_code error;
        fs::current_path(original_path_, error);
        BOOST_CHECK_MESSAGE(!error, "Unable to restore test working directory: " << error.message());
    }

private:
    fs::path original_path_;
    size_t original_threshold_;
    bool original_server_;
    std::list<WithoutTestEnvironmentVariable> cleared_;
};

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
    auto startup = ecf::test::scaffold::MakeDirectory{}.create();
    IsolatedServerStartup isolation;
    const auto absolute_home   = startup.path() / "absolute workspace";
    const auto absolute_backup = startup.path() / "backup.check";
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
            // Several forms name the same directory, so a log left by a previous form must not satisfy the check
            fs::remove(expected_home / "server.log");
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
    auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    IsolatedServerStartup isolation;
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
    auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    IsolatedServerStartup isolation;
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
    auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    IsolatedServerStartup isolation;
    const auto home_path     = directory.path() / "server home";
    const auto expected_home = home_content || config_selects_home ? home_path : directory.path();
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
/// @brief Runs startup assertions with isolated inputs and scoped cleanup.
///
/// Cleanup:
/// - Restores the working directory and environment inputs.
/// - Restores the server-process flag and profiling threshold.
/// - Destroys the log created during initialisation.
///
/// @tparam Check Callable accepting the server environment and the startup directory
/// @param[in] content Configuration-file content
/// @param[in] environment Explicit environment overrides
/// @param[in] args Command-line tokens including the executable name
/// @param[in] check Assertions to run after construction and the expected validation outcome
/// @param[in] expected_error Diagnostic fragment required when validation is expected to fail
///
template <typename Check>
void check_server_precedence(const std::string& content,
                             const std::vector<std::pair<std::string, std::string>>& environment,
                             const std::vector<std::string>& args,
                             Check check,
                             const std::string& expected_error = {}) {
    auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    IsolatedServerStartup isolation;
    std::list<WithTestEnvironmentVariable> overrides;
    for (const auto& [name, value] : environment) {
        overrides.emplace_back(name, value);
    }
    // A sentinel distinct from the default, so that a default left unapplied is detected
    JobProfiler::set_task_threshold(987);
    fs::create_directories(directory.path() / "config home");
    fs::create_directories(directory.path() / "environment home");
    WithTestFile config(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
    fs::current_path(directory.path());

    ServerEnvironment serverEnv(args);
    std::string error;
    const bool valid = serverEnv.valid(error);
    if (expected_error.empty()) {
        BOOST_REQUIRE_MESSAGE(valid, error);
    }
    else {
        BOOST_CHECK(!valid);
        BOOST_CHECK_MESSAGE(error.find(expected_error) != std::string::npos, error);
    }
    check(serverEnv, directory.path());
}

///
/// @brief Checks rejection of malformed or out-of-range integer text through every supported source.
///
/// @param[in] value Invalid integer text that must remain visible in the diagnostic
///
void check_server_invalid_integer(const std::string& value) {
    for (const std::string name :
         {"ECF_PORT", "ECF_CHECKINTERVAL", "ECF_INTERVAL", "ECF_PRUNE_NODE_LOG", "ECF_TASK_THRESHOLD"}) {
        for (const std::string source : {"config", "environment", "cli"}) {
            if ((source == "environment" && name == "ECF_INTERVAL") ||
                (source == "cli" && name != "ECF_PORT" && name != "ECF_INTERVAL")) {
                continue;
            }
            BOOST_TEST_CONTEXT(name << " from " << source << ": " << value) {
                std::string content;
                std::vector<std::pair<std::string, std::string>> environment;
                std::vector<std::string> args = {"ServerEnvironment"};
                std::string diagnostic_key    = name;
                if (source == "config") {
                    content = name + " = " + value + "\n";
                }
                else if (source == "environment") {
                    environment.emplace_back(name, value);
                }
                else {
                    diagnostic_key = name == "ECF_PORT" ? "--port" : "--ecfinterval";
                    args.emplace_back(diagnostic_key + "=" + value);
                }

                BOOST_CHECK_EXCEPTION(
                    check_server_precedence(content, environment, args, [](const auto&, const auto&) {}),
                    ServerEnvironmentException,
                    [&](const ServerEnvironmentException& error) {
                        const std::string message = error.what();
                        return message.find(diagnostic_key) != std::string::npos &&
                               message.find(value) != std::string::npos &&
                               (source != "config" || message.find("server_environment.cfg") != std::string::npos) &&
                               (source != "cli" || message.find("Invalid server command line") != std::string::npos);
                    });
                BOOST_CHECK(!Log::instance());
            }
        }
    }
}

///
/// @brief Checks all six resolved filename variables for both supported input sources.
///
/// @param[in] form Path form: unset, default, custom, dot, nested, absolute or spaces
///
void check_server_filename_resolution(const std::string& form) {
    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Path form: " << form << ", environment source: " << from_environment) {
            auto directory = ecf::test::scaffold::MakeDirectory{}.create();
            IsolatedServerStartup isolation;
            const auto home    = directory.path() / "server home";
            const auto outside = directory.path() / "external files";
            fs::create_directories(home / "runtime");
            fs::create_directories(home / "runtime with spaces");
            fs::create_directories(outside);
            std::string content = "ECF_HOME = server home\n";
            std::list<WithTestEnvironmentVariable> overrides;
            std::vector<std::pair<std::string, std::string>> expected;
            const auto prefix = Host{}.name() + ".3144.";
            for (const auto& [name, basename] :
                 std::vector<std::pair<std::string, std::string>>{{"ECF_LOG", "ecf.log"},
                                                                  {"ECF_CHECK", "ecf.check"},
                                                                  {"ECF_CHECKOLD", "ecf.check.b"},
                                                                  {"ECF_PASSWD", "ecf.passwd"},
                                                                  {"ECF_CUSTOM_PASSWD", "ecf.custom_passwd"},
                                                                  {"ECF_LISTS", "ecf.lists"}}) {
                std::string input = basename;
                if (form == "custom") {
                    input = "custom." + basename;
                }
                else if (form == "dot") {
                    input = "./" + basename;
                }
                else if (form == "nested") {
                    input = "runtime/" + basename;
                }
                else if (form == "absolute") {
                    input = (outside / basename).string();
                }
                else if (form == "spaces") {
                    input = "runtime with spaces/custom " + basename;
                }
                if (form != "unset") {
                    if (from_environment) {
                        overrides.emplace_back(name, input);
                    }
                    else {
                        content += name + " = " + input + "\n";
                    }
                }
                const auto resolved = form == "unset" || form == "default" ? prefix + basename : input;
                const bool anchored = name == "ECF_LOG" || name == "ECF_CHECK" || name == "ECF_CHECKOLD";
                expected.emplace_back(name, anchored ? (home / resolved).string() : resolved);
            }
            WithTestFile config(NamedTestFile{(directory.path() / "server_environment.cfg").string()}, content);
            fs::current_path(directory.path());
            ServerEnvironment serverEnv({"ServerEnvironment", "--port=3144"});
            std::string error;
            BOOST_REQUIRE_MESSAGE(serverEnv.valid(error), error);
            BOOST_CHECK_EQUAL(serverEnv.ecf_home(), home.string());
            BOOST_CHECK(fs::equivalent(fs::current_path(), home));
            for (const auto& [name, value] : expected) {
                BOOST_CHECK_EQUAL(server_variable(serverEnv, name), value);
            }
            BOOST_CHECK_EQUAL(serverEnv.checkPtFilename(), expected[1].second);
            BOOST_CHECK_EQUAL(serverEnv.oldCheckPtFilename(), expected[2].second);
            BOOST_CHECK(fs::is_regular_file(expected[0].second));
            if (form != "unset" && form != "default") {
                BOOST_CHECK(!fs::exists(home / (prefix + "ecf.log")));
            }
        }
    }
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
    BOOST_REQUIRE_MESSAGE(fs::is_regular_file(config_path), "Shipped server configuration not found: " << config_path);

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

    IsolatedServerStartup isolation;
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
/// @brief Verifies that every line of the shipped configuration is parsed and applied.
///
/// The shipped values equal the built-in defaults, so a copy with two changed values is loaded instead; values
/// placed near the start and the end of the file show that the whole file is read.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_shipped_values_are_applied) {
    ECF_NAME_THIS_TEST();

    const auto shipped_path = File::test_data("libs/server/server_environment.cfg", "libs/server");
    std::ifstream shipped(shipped_path);
    BOOST_REQUIRE_MESSAGE(shipped.is_open(), "Shipped server configuration not found: " << shipped_path);
    std::string content{std::istreambuf_iterator<char>(shipped), std::istreambuf_iterator<char>()};

    for (const auto& [from, to] :
         std::vector<std::pair<std::string, std::string>>{{"ECF_CHECKINTERVAL = 120\n", "ECF_CHECKINTERVAL = 121\n"},
                                                          {"ECF_PRUNE_NODE_LOG = 30\n", "ECF_PRUNE_NODE_LOG = 31\n"}}) {
        const auto position = content.find(from);
        BOOST_REQUIRE_MESSAGE(position != std::string::npos, "Shipped configuration has no line: " << from);
        content.replace(position, from.size(), to);
    }

    WithTestFile config(AutomaticTestFile{"server_environment.cfg"}, content);
    WithTestFile log_file(AutomaticTestFile{"server-config.log"}, "");
    WithTestEnvironmentVariable ecf_log("ECF_LOG", log_file.path().string());
    WithoutTestEnvironmentVariable no_check_interval("ECF_CHECKINTERVAL");
    WithoutTestEnvironmentVariable no_prune("ECF_PRUNE_NODE_LOG");

    const std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args, config.path().string());

    BOOST_CHECK_EQUAL(serverEnv.checkPtInterval(), 121);
    BOOST_CHECK_EQUAL(serverEnv.ecf_prune_node_log(), 31);
}

///
/// @brief Verifies that help and version requests read no configuration, environment setting or runtime file.
///
BOOST_AUTO_TEST_CASE(test_server_information_options_bypass_settings) {
    ECF_NAME_THIS_TEST();

    // A directory as configuration file and an invalid port would both stop a normal start
    auto directory = ecf::test::scaffold::MakeDirectory{}.create();
    WithTestEnvironmentVariable invalid_port("ECF_PORT", "not_a_port");
    WithTestEnvironmentVariable invalid_threshold("ECF_TASK_THRESHOLD", "0");
    Log::destroy();

    for (const std::string option : {"--help", "-h", "--version", "-v"}) {
        BOOST_TEST_CONTEXT("Option " << option) {
            const auto original_path            = fs::current_path();
            const std::vector<std::string> args = {"ServerEnvironment", option};
            std::optional<ServerEnvironment> serverEnv;
            BOOST_REQUIRE_NO_THROW(serverEnv.emplace(args, directory.path().string()));
            const bool help = option == "--help" || option == "-h";
            BOOST_CHECK_EQUAL(serverEnv->help_option(), help);
            BOOST_CHECK_EQUAL(serverEnv->version_option(), !help);
            BOOST_CHECK(!Log::instance());
            BOOST_CHECK_EQUAL(fs::current_path(), original_path);
        }
    }
}

///
/// @brief Verifies that a failed whitelist reload reports the selected file and its cause, and that a corrected
/// file can be reloaded afterwards.
///
/// Permissions kept in force after a failed reload are verified end to end by s_foolproof.server_startup
/// (T_ServerReload).
///
BOOST_AUTO_TEST_CASE(test_server_whitelist_failed_reload_reports_cause) {
    ECF_NAME_THIS_TEST();

    auto directory                  = ecf::test::scaffold::MakeDirectory{}.create();
    const auto lists                = directory.path() / "selected.lists";
    const std::string valid_content = "4.5.0\nalice\n";
    {
        std::ofstream output(lists);
        output << valid_content;
    }
    IsolatedServerStartup isolation;
    WithTestEnvironmentVariable ecf_lists("ECF_LISTS", lists.string());
    WithTestEnvironmentVariable ecf_log("ECF_LOG", (directory.path() / "server.log").string());

    const std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    ServerEnvironment serverEnv(args, (directory.path() / "missing.cfg").string());
    std::string error;
    BOOST_REQUIRE_MESSAGE(serverEnv.valid(error), error);

    for (const std::string replacement : {"malformed", "directory", "missing"}) {
        BOOST_TEST_CONTEXT("Replacement " << replacement) {
            fs::remove_all(lists);
            if (replacement == "malformed") {
                std::ofstream output(lists);
                output << "not_a_version\n";
            }
            else if (replacement == "directory") {
                fs::create_directory(lists);
            }

            error = "prefix: ";
            BOOST_CHECK(!serverEnv.reloadWhiteListFile(error));
            BOOST_CHECK_EQUAL(error.rfind("prefix: ", 0), 0u);
            BOOST_CHECK_MESSAGE(error.find(lists.string()) != std::string::npos, error);
            const std::string cause = replacement == "malformed"   ? "Could not load ECF_LISTS file"
                                      : replacement == "directory" ? "is not a regular file"
                                                                   : "does not exist";
            BOOST_CHECK_MESSAGE(error.find(cause) != std::string::npos, error);

            fs::remove_all(lists);
            {
                std::ofstream output(lists);
                output << valid_content;
            }
            error.clear();
            BOOST_CHECK_MESSAGE(serverEnv.reloadWhiteListFile(error), error);
        }
    }
}

///
/// @brief Verifies that a configuration path that cannot be inspected stops initialisation, unlike a missing file.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_cannot_be_inspected) {
    ECF_NAME_THIS_TEST();

    auto directory      = ecf::test::scaffold::MakeDirectory{}.create();
    const auto locked   = directory.path() / "locked";
    const auto config   = locked / "server_environment.cfg";
    const auto log_path = directory.path() / "server.log";
    fs::create_directories(locked);
    BOOST_SCOPE_EXIT_ALL(&) {
        std::error_code error;
        fs::permissions(locked, fs::perms::owner_all, fs::perm_options::replace, error);
        BOOST_CHECK_MESSAGE(!error, "Unable to restore directory permissions: " << error.message());
        Log::destroy();
    };
    fs::permissions(locked, fs::perms::none, fs::perm_options::replace);

    std::error_code status_error;
    [[maybe_unused]] const auto status = fs::status(config, status_error);
    if (status_error != std::errc::permission_denied) {
        ecf::test::scaffold::skip_unenforced_permissions("the uninspectable configuration case");
        return;
    }

    WithTestEnvironmentVariable ecf_log("ECF_LOG", log_path.string());
    const std::vector<std::string> args = {"ServerEnvironment", "--port=3144"};
    BOOST_CHECK_EXCEPTION(
        ServerEnvironment(args, config.string()), ServerEnvironmentException, [&](const ServerEnvironmentException& e) {
            const std::string what = e.what();
            return what.find("Cannot inspect configuration file") != std::string::npos &&
                   what.find(config.string()) != std::string::npos;
        });
    BOOST_CHECK(!fs::exists(log_path));
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
    IsolatedServerStartup isolation;
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

///
/// @brief Verifies default filename selection when all six settings are absent.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_unset) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("unset");
}

///
/// @brief Verifies host and port prefixing of explicitly supplied bare defaults.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_explicit_defaults) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("default");
}

///
/// @brief Verifies that non-default basenames are not prefixed.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_custom_basenames) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("custom");
}

///
/// @brief Verifies that dot-prefixed defaults are treated as explicit paths.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_dot_prefixed) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("dot");
}

///
/// @brief Verifies relative directory paths against a separate server home.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_relative_directories) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("nested");
}

///
/// @brief Verifies that absolute filenames outside server home are unchanged.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_absolute) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("absolute");
}

///
/// @brief Verifies that spaces in directory and file names survive both input sources.
///
BOOST_AUTO_TEST_CASE(test_server_filenames_spaces) {
    ECF_NAME_THIS_TEST();
    check_server_filename_resolution("spaces");
}

///
/// @brief Verifies controlled rejection of nonnumeric and partially numeric integer inputs.
///
BOOST_AUTO_TEST_CASE(test_server_validation_integer_text) {
    ECF_NAME_THIS_TEST();

    check_server_invalid_integer("not_an_integer");
    check_server_invalid_integer("12seconds");
}

///
/// @brief Verifies rejection of integers outside the destination range without truncation or wraparound.
///
BOOST_AUTO_TEST_CASE(test_server_validation_integer_overflow_and_underflow) {
    ECF_NAME_THIS_TEST();

    check_server_invalid_integer(std::to_string(static_cast<long long>(std::numeric_limits<int>::max()) + 1));
    check_server_invalid_integer(std::to_string(static_cast<long long>(std::numeric_limits<int>::min()) - 1));
}

///
/// @brief Verifies port-range boundaries from configuration, environment and command-line inputs without listeners.
///
BOOST_AUTO_TEST_CASE(test_server_validation_port_boundaries) {
    ECF_NAME_THIS_TEST();

    for (const int port : {1023, 1024, 49150, 49151}) {
        for (const std::string source : {"config", "environment", "cli"}) {
            BOOST_TEST_CONTEXT("Port " << port << " from " << source) {
                std::string content;
                std::vector<std::pair<std::string, std::string>> environment;
                std::vector<std::string> args = {"ServerEnvironment"};
                if (source == "config") {
                    content = "ECF_PORT = " + std::to_string(port) + "\n";
                }
                else if (source == "environment") {
                    environment.emplace_back("ECF_PORT", std::to_string(port));
                }
                else {
                    args.emplace_back("--port=" + std::to_string(port));
                }

                check_server_precedence(
                    content,
                    environment,
                    args,
                    [&](const auto& serverEnv, const auto&) {
                        BOOST_CHECK_EQUAL(serverEnv.port(), port);
                        BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_PORT"), std::to_string(port));
                    },
                    port == 1023 || port == 49151 ? "Server port " + std::to_string(port) + " not set correctly" : "");
            }
        }
    }
}

///
/// @brief Verifies scheduling-interval boundaries from configuration and command-line inputs.
///
BOOST_AUTO_TEST_CASE(test_server_validation_scheduling_boundaries) {
    ECF_NAME_THIS_TEST();

    for (const int interval : {0, 1, 60, 61}) {
        for (const bool from_cli : {false, true}) {
            BOOST_TEST_CONTEXT("Scheduling interval " << interval << ", CLI source: " << from_cli) {
                const auto content            = from_cli ? "" : "ECF_INTERVAL = " + std::to_string(interval) + "\n";
                std::vector<std::string> args = {"ServerEnvironment"};
                if (from_cli) {
                    args.emplace_back("--ecfinterval=" + std::to_string(interval));
                }

                check_server_precedence(
                    content,
                    {},
                    args,
                    [&](const auto& serverEnv, const auto&) {
                        BOOST_CHECK_EQUAL(serverEnv.submitJobsInterval(), interval);
                        BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_INTERVAL"), std::to_string(interval));
                    },
                    interval == 0 || interval == 61
                        ? "ECF_INTERVAL (job scheduling interval) must be in the range [1-60] seconds, but found " +
                              std::to_string(interval)
                        : "");
            }
        }
    }
}

///
/// @brief Verifies pruning boundaries and preservation of historical checkpoint entries when pruning is zero.
///
BOOST_AUTO_TEST_CASE(test_server_validation_pruning_boundaries) {
    ECF_NAME_THIS_TEST();

    for (const int days : {-1, 0, 1, std::numeric_limits<int>::max()}) {
        for (const bool from_environment : {false, true}) {
            BOOST_TEST_CONTEXT("Pruning days " << days << ", environment source: " << from_environment) {
                const auto content = from_environment ? "" : "ECF_PRUNE_NODE_LOG = " + std::to_string(days) + "\n";
                std::vector<std::pair<std::string, std::string>> environment;
                if (from_environment) {
                    environment.emplace_back("ECF_PRUNE_NODE_LOG", std::to_string(days));
                }

                check_server_precedence(
                    content,
                    environment,
                    {"ServerEnvironment"},
                    [&](const auto& serverEnv, const auto& startup) {
                        BOOST_CHECK_EQUAL(serverEnv.ecf_prune_node_log(), days);
                        // An entry from 2016 survives only when pruning is disabled (zero days)
                        if (days == 0 || days == 1) {
                            const std::string history = "MSG:[07:36:05 10.3.2016] --requeue force /retained :tester";
                            Defs original;
                            original.add_suite("retained");
                            original.add_edit_history("/retained", history);
                            const auto checkpoint = (startup / "history.check").string();
                            original.write_to_checkpt_file(checkpoint);

                            Defs restored;
                            restored.ecf_prune_node_log(serverEnv.ecf_prune_node_log());
                            std::string error, warning;
                            BOOST_REQUIRE_MESSAGE(restored.restore(checkpoint, error, warning), error);
                            const auto& entries = restored.get_edit_history();
                            if (days == 0) {
                                BOOST_REQUIRE_EQUAL(entries.count("/retained"), 1);
                                BOOST_REQUIRE_EQUAL(entries.at("/retained").size(), 1);
                                BOOST_CHECK_EQUAL(entries.at("/retained").front(), history);
                            }
                            else {
                                BOOST_CHECK_EQUAL(entries.count("/retained"), 0);
                            }
                        }
                    },
                    days < 0 ? "ECF_PRUNE_NODE_LOG must be a non-negative integer" : "");
            }
        }
    }
}

///
/// @brief Verifies positive profiling thresholds and rejects zero and negative values before unsigned conversion.
///
BOOST_AUTO_TEST_CASE(test_server_validation_task_threshold_boundaries) {
    ECF_NAME_THIS_TEST();

    for (const int threshold : {std::numeric_limits<int>::min(), -1, 0, 1, std::numeric_limits<int>::max()}) {
        for (const bool from_environment : {false, true}) {
            BOOST_TEST_CONTEXT("Task threshold " << threshold << ", environment source: " << from_environment) {
                const auto content = from_environment ? "" : "ECF_TASK_THRESHOLD = " + std::to_string(threshold) + "\n";
                std::vector<std::pair<std::string, std::string>> environment;
                if (from_environment) {
                    environment.emplace_back("ECF_TASK_THRESHOLD", std::to_string(threshold));
                }

                const auto run = [&] {
                    check_server_precedence(content, environment, {"ServerEnvironment"}, [&](const auto&, const auto&) {
                        BOOST_CHECK_EQUAL(JobProfiler::task_threshold(), static_cast<size_t>(threshold));
                    });
                };
                if (threshold > 0) {
                    run();
                }
                else {
                    BOOST_CHECK_EXCEPTION(run(), ServerEnvironmentException, [&](const auto& error) {
                        const std::string message = error.what();
                        return message.find("ECF_TASK_THRESHOLD") != std::string::npos &&
                               message.find(std::to_string(threshold)) != std::string::npos &&
                               message.find("positive integer") != std::string::npos;
                    });
                }
            }
        }
    }
}

///
/// @brief Verifies that positive environment thresholds replace non-positive config values before validation.
///
BOOST_AUTO_TEST_CASE(test_server_validation_task_threshold_effective_value) {
    ECF_NAME_THIS_TEST();

    for (const int configured : {-1, 0}) {
        BOOST_TEST_CONTEXT("Configured threshold: " << configured) {
            check_server_precedence(
                "ECF_TASK_THRESHOLD = " + std::to_string(configured) + "\n",
                {{"ECF_TASK_THRESHOLD", "137"}},
                {"ServerEnvironment"},
                [](const auto&, const auto&) { BOOST_CHECK_EQUAL(JobProfiler::task_threshold(), 137); });
        }
    }
}

///
/// @brief Verifies that supported non-empty environment values replace empty configuration paths before validation.
///
BOOST_AUTO_TEST_CASE(test_server_validation_empty_config_with_environment_override) {
    ECF_NAME_THIS_TEST();

    for (const std::string name :
         {"ECF_HOME", "ECF_LOG", "ECF_CHECK", "ECF_CHECKOLD", "ECF_PASSWD", "ECF_CUSTOM_PASSWD", "ECF_LISTS"}) {
        BOOST_TEST_CONTEXT("Overridden empty config setting: " << name) {
            const auto selected = name == "ECF_HOME" ? "environment home" : "selected.file";
            check_server_precedence(name + " =\n",
                                    {{name, selected}},
                                    {"ServerEnvironment"},
                                    [&](const auto& serverEnv, const auto& startup) {
                                        const bool anchored = name == "ECF_HOME" || name == "ECF_LOG" ||
                                                              name == "ECF_CHECK" || name == "ECF_CHECKOLD";
                                        const auto expected = anchored ? (startup / selected).string() : selected;
                                        BOOST_CHECK_EQUAL(server_variable(serverEnv, name), expected);
                                    });
        }
    }
}

///
/// @brief Verifies filename resolution leaves exact configuration bytes and environment inputs unchanged.
///
BOOST_AUTO_TEST_CASE(test_server_observability_inputs_are_not_mutated) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable unrelated("ECF_OBSERVABILITY_SENTINEL", "unchanged unrelated input");
    const std::vector<std::pair<std::string, std::string>> settings = {{"ECF_HOME", "environment home"},
                                                                       {"ECF_PORT", "3143"},
                                                                       {"ECF_LOG", "ecf.log"},
                                                                       {"ECF_CHECK", "ecf.check"},
                                                                       {"ECF_CHECKOLD", "ecf.check.b"},
                                                                       {"ECF_PASSWD", "ecf.passwd"},
                                                                       {"ECF_CUSTOM_PASSWD", "ecf.custom_passwd"},
                                                                       {"ECF_LISTS", "ecf.lists"}};
    const std::string content =
        "# Preserve whitespace and comments exactly.\n\nECF_HOME = config home\nECF_PORT = 3142\n"
        "ECF_LOG = ecf.log\nECF_CHECK = ecf.check\nECF_CHECKOLD = ecf.check.b\n"
        "ECF_PASSWD = ecf.passwd\nECF_CUSTOM_PASSWD = ecf.custom_passwd\nECF_LISTS = ecf.lists\n";

    for (const bool from_environment : {false, true}) {
        BOOST_TEST_CONTEXT("Environment inputs present: " << from_environment) {
            const auto environment = from_environment ? settings : std::vector<std::pair<std::string, std::string>>{};
            check_server_precedence(
                content,
                environment,
                {"ServerEnvironment", "--port=3144"},
                [&](const auto& serverEnv, const auto& startup) {
                    std::ifstream input(startup / "server_environment.cfg", std::ios::binary);
                    BOOST_REQUIRE(input.is_open());
                    const std::string actual{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
                    BOOST_CHECK_EQUAL(actual, content);
                    for (const auto& [name, value] : settings) {
                        const auto* selected = std::getenv(name.c_str());
                        if (from_environment) {
                            BOOST_REQUIRE(selected);
                            BOOST_CHECK_EQUAL(selected, value);
                        }
                        else {
                            BOOST_CHECK(selected == nullptr);
                        }
                    }
                    BOOST_CHECK_EQUAL(std::getenv("ECF_OBSERVABILITY_SENTINEL"), "unchanged unrelated input");
                    BOOST_CHECK_EQUAL(serverEnv.the_port(), "3144");
                    BOOST_CHECK_EQUAL(serverEnv.ecf_home(),
                                      (startup / (from_environment ? "environment home" : "config home")).string());
                    const auto home = startup / (from_environment ? "environment home" : "config home");
                    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_CHECK"),
                                      (home / Host{}.ecf_checkpt_file("3144")).string());
                    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_LOG"),
                                      (home / Host{}.ecf_log_file("3144")).string());
                });
        }
    }
}

///
/// @brief Verifies isolated startup fixtures restore inputs and global state on success and exception unwinding.
///
BOOST_AUTO_TEST_CASE(test_server_observability_fixture_restores_state) {
    ECF_NAME_THIS_TEST();

    const auto original_path      = fs::current_path();
    const auto original_threshold = JobProfiler::task_threshold();
    const bool original_server    = Ecf::server();
    BOOST_SCOPE_EXIT_ALL(&) {
        JobProfiler::set_task_threshold(original_threshold);
        Ecf::set_server(original_server);
    };

    const std::vector<std::pair<std::string, std::string>> ambient = {{"ECF_HOME", "/nonexistent/outer-home"},
                                                                      {"ECF_PORT", "invalid_outer_port"},
                                                                      {"ECF_LOG", "/nonexistent/outer.log"},
                                                                      {"ECF_CHECKINTERVAL", "invalid_outer_interval"},
                                                                      {"ECF_TASK_THRESHOLD", "invalid_outer_threshold"},
                                                                      {"ECF_SSL", "unsupported_outer_ssl"},
                                                                      {"ECF_LISTS", ""},
                                                                      {"ECF_OBSERVABILITY_SENTINEL", "outer sentinel"}};
    std::list<WithTestEnvironmentVariable> inputs;
    for (const auto& [name, value] : ambient) {
        inputs.emplace_back(name, value);
    }
    WithoutTestEnvironmentVariable absent("ECF_CUSTOM_PASSWD");

    for (const bool fail_after_startup : {false, true}) {
        BOOST_TEST_CONTEXT("Exception after startup: " << fail_after_startup) {
            JobProfiler::set_task_threshold(987);
            Ecf::set_server(false);
            const auto run = [&] {
                check_server_precedence(
                    "ECF_HOME = config home\nECF_TASK_THRESHOLD = 137\n",
                    {{"ECF_HOME", "environment home"},
                     {"ECF_PORT", "3143"},
                     {"ECF_LOG", "selected.log"},
                     {"ECF_TASK_THRESHOLD", "251"}},
                    {"ServerEnvironment", "--port=3144"},
                    [&](const auto& serverEnv, const auto& startup) {
                        BOOST_CHECK_EQUAL(serverEnv.the_port(), "3144");
                        BOOST_CHECK_EQUAL(JobProfiler::task_threshold(), 251);
                        BOOST_CHECK(Ecf::server());
                        BOOST_CHECK(fs::equivalent(fs::current_path(), startup / "environment home"));
                        BOOST_CHECK(std::getenv("ECF_CHECKINTERVAL") == nullptr);
                        BOOST_CHECK(std::getenv("ECF_SSL") == nullptr);
                        BOOST_CHECK_EQUAL(std::getenv("ECF_OBSERVABILITY_SENTINEL"), "outer sentinel");
                        if (fail_after_startup) {
                            throw std::runtime_error("intentional observation failure");
                        }
                    });
            };
            if (fail_after_startup) {
                BOOST_CHECK_EXCEPTION(run(), std::runtime_error, [](const auto& error) {
                    return std::string(error.what()) == "intentional observation failure";
                });
            }
            else {
                run();
            }

            BOOST_CHECK(fs::current_path() == original_path);
            BOOST_CHECK_EQUAL(JobProfiler::task_threshold(), 987);
            BOOST_CHECK(!Ecf::server());
            BOOST_CHECK(!Log::instance());
            for (const auto& [name, value] : ambient) {
                const auto* restored = std::getenv(name.c_str());
                BOOST_REQUIRE(restored);
                BOOST_CHECK_EQUAL(restored, value);
            }
            BOOST_CHECK(std::getenv("ECF_CUSTOM_PASSWD") == nullptr);
        }
    }
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

    // Restores the environment, the profiling threshold and the working directory, and destroys the log
    IsolatedServerStartup isolation;

    std::vector<std::string> args = {"ServerEnvironment"};
    {
        WithTestEnvironmentVariable threshold("ECF_TASK_THRESHOLD", "9");
        ServerEnvironment serverEnv(args);
        BOOST_CHECK_MESSAGE(JobProfiler::task_threshold() == 9,
                            "Expected task threshold of 9 but found " << JobProfiler::task_threshold());

        Host h;
        fs::remove(h.ecf_log_file(serverEnv.the_port()));
    }

    for (const auto* dodgy_threshold : {"x", ",", ":", ",,"}) {
        BOOST_TEST_CONTEXT("ECF_TASK_THRESHOLD=" << dodgy_threshold) {
            WithTestEnvironmentVariable threshold("ECF_TASK_THRESHOLD", dodgy_threshold);
            BOOST_CHECK_THROW(ServerEnvironment serverEnv(args), std::runtime_error);
        }
    }
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

//
// Inputs not accepted in server_environment.cfg: environment variables and command-line options only
//

///
/// @brief Verifies that server_environment.cfg refuses the inputs that only other sources provide.
///
BOOST_AUTO_TEST_CASE(test_server_config_file_rejects_inputs_of_other_sources) {
    ECF_NAME_THIS_TEST();
    for (const std::string name : {"ECF_HOST", "ECF_SSL", "ECF_DEBUG_SERVER"}) {
        BOOST_TEST_CONTEXT("Setting: " << name) {
            check_server_config_parse_error(name + " = 1\n", name);
        }
    }
}

///
/// @brief Verifies that debug output is enabled by `--debug`, or by the presence of ECF_DEBUG_SERVER.
///
/// The value of ECF_DEBUG_SERVER is not interpreted: defining the variable, even as `0` or empty, enables debug.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_debug) {
    ECF_NAME_THIS_TEST();
    struct Case
    {
        std::optional<std::string> environment;
        bool option;
        bool expected;
    };
    for (const auto& c : std::vector<Case>{{std::nullopt, false, false},
                                           {"1", false, true},
                                           {"0", false, true},
                                           {"", false, true},
                                           {std::nullopt, true, true},
                                           {"0", true, true}}) {
        BOOST_TEST_CONTEXT("ECF_DEBUG_SERVER: " << c.environment.value_or("(unset)") << ", --debug: " << c.option) {
            std::vector<std::pair<std::string, std::string>> environment;
            if (c.environment) {
                environment.emplace_back("ECF_DEBUG_SERVER", *c.environment);
            }
            std::vector<std::string> args = {"ServerEnvironment"};
            if (c.option) {
                args.emplace_back("--debug");
            }
            check_server_precedence("", environment, args, [&](const auto& serverEnv, const auto&) {
                BOOST_CHECK_EQUAL(serverEnv.debug(), c.expected);
                const auto lines = last_startup_lines();
                const auto entry = std::string("  ECF_DEBUG_SERVER = ") + (c.expected ? "enabled" : "disabled");
                BOOST_CHECK(std::find(lines.begin(), lines.end(), entry) != lines.end());
            });
        }
    }
}

///
/// @brief Verifies that job generation is enabled by default, and disabled only by `--dis_job_gen`.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_job_generation) {
    ECF_NAME_THIS_TEST();
    for (const bool option : {false, true}) {
        BOOST_TEST_CONTEXT("--dis_job_gen: " << option) {
            std::vector<std::string> args = {"ServerEnvironment"};
            if (option) {
                args.emplace_back("--dis_job_gen");
            }
            check_server_precedence("", {}, args, [&](const auto& serverEnv, const auto&) {
                BOOST_CHECK_EQUAL(serverEnv.jobGeneration(), !option);
            });
        }
    }
}

///
/// @brief Verifies that the server uses TCP over IPv4 by default, and IPv6 only with `--v6`, with or without HTTP.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_tcp_protocol) {
    ECF_NAME_THIS_TEST();
    struct Case
    {
        bool v6;
        bool http;
        ecf::Protocol expected_protocol;
    };
    for (const auto& c : std::vector<Case>{{false, false, ecf::Protocol::Plain},
                                           {true, false, ecf::Protocol::Plain},
                                           {false, true, ecf::Protocol::Http},
                                           {true, true, ecf::Protocol::Http}}) {
        BOOST_TEST_CONTEXT("--v6: " << c.v6 << ", --http: " << c.http) {
            std::vector<std::string> args = {"ServerEnvironment"};
            if (c.v6) {
                args.emplace_back("--v6");
            }
            if (c.http) {
                args.emplace_back("--http");
            }
            check_server_precedence("", {}, args, [&](const auto& serverEnv, const auto&) {
                const auto expected_tcp = c.v6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4();
                BOOST_CHECK(serverEnv.tcp_protocol() == expected_tcp);
                BOOST_CHECK_MESSAGE(serverEnv.protocol() == c.expected_protocol,
                                    "Expected protocol " << ecf::to_ui_designation(c.expected_protocol) << " but found "
                                                         << ecf::to_ui_designation(serverEnv.protocol()));
            });
        }
    }
}

///
/// @brief Verifies that the host is always the name of the machine, whatever ECF_HOST holds in the environment.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_host_ignores_environment) {
    ECF_NAME_THIS_TEST();
    const auto machine = Host{}.name();
    for (const std::optional<std::string>& value :
         {std::optional<std::string>{}, std::optional<std::string>{"other.host"}}) {
        BOOST_TEST_CONTEXT("ECF_HOST: " << value.value_or("(unset)")) {
            std::optional<WithoutTestEnvironmentVariable> unset;
            std::vector<std::pair<std::string, std::string>> environment;
            if (value) {
                environment.emplace_back("ECF_HOST", *value);
            }
            else {
                unset.emplace("ECF_HOST");
            }
            check_server_precedence(
                "", environment, {"ServerEnvironment"}, [&](const auto& serverEnv, const auto& startup) {
                    BOOST_CHECK_EQUAL(serverEnv.hostPort().first, machine);
                    BOOST_CHECK_EQUAL(server_variable(serverEnv, "ECF_HOST"), machine);
                    BOOST_CHECK(fs::is_regular_file(startup / (machine + ".3141.ecf.log")));
                });
        }
    }
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

///
/// @brief Verifies how ECF_SSL and `--ssl` select the certificate, and how SSL promotes the protocol.
///
/// Notes:
///
///   - ECF_SSL, when defined, decides the lookup order, also when `--ssl` is given:
///     - `1` or empty: shared certificate first, server-specific certificate as fallback;
///     - any other value: server-specific certificate first, shared certificate as fallback.
///   - `--ssl` alone looks for the shared certificate first.
///   - SSL promotes TCP/IP to SSL, and HTTP to HTTPS.
///
BOOST_AUTO_TEST_CASE(test_server_precedence_ssl) {
    ECF_NAME_THIS_TEST();
    const auto specific_name = Host{}.name() + ".3141";
    struct Case
    {
        std::optional<std::string> ecf_ssl;
        bool ssl_option;
        bool http_option;
        bool shared_crt;
        bool specific_crt;
        bool expected_shared;
        ecf::Protocol expected_protocol;
    };
    const std::vector<Case> cases = {// ECF_SSL alone
                                     {"1", false, false, true, true, true, ecf::Protocol::Ssl},
                                     {"", false, false, true, true, true, ecf::Protocol::Ssl},
                                     {"1", false, false, false, true, false, ecf::Protocol::Ssl},
                                     {specific_name, false, false, true, true, false, ecf::Protocol::Ssl},
                                     {specific_name, false, false, true, false, true, ecf::Protocol::Ssl},
                                     {"1", false, true, true, true, true, ecf::Protocol::Https},
                                     // --ssl alone
                                     {std::nullopt, true, false, true, true, true, ecf::Protocol::Ssl},
                                     {std::nullopt, true, false, false, true, false, ecf::Protocol::Ssl},
                                     {std::nullopt, true, true, true, true, true, ecf::Protocol::Https},
                                     // ECF_SSL with --ssl
                                     {specific_name, true, false, true, true, false, ecf::Protocol::Ssl},
                                     {"1", true, true, true, true, true, ecf::Protocol::Https}};

    for (const auto& c : cases) {
        BOOST_TEST_CONTEXT("ECF_SSL: " << c.ecf_ssl.value_or("(unset)") << ", --ssl: " << c.ssl_option
                                       << ", --http: " << c.http_option << ", shared: " << c.shared_crt
                                       << ", specific: " << c.specific_crt) {
            auto certificates = ecf::test::scaffold::MakeDirectory{}.create();
            std::list<WithTestFile> crts;
            if (c.shared_crt) {
                crts.emplace_back(NamedTestFile{(certificates.path() / "server.crt").string()});
            }
            if (c.specific_crt) {
                crts.emplace_back(NamedTestFile{(certificates.path() / (specific_name + ".crt")).string()});
            }
            std::vector<std::pair<std::string, std::string>> environment = {
                {"ECF_SSL_DIR", certificates.path().string()}};
            if (c.ecf_ssl) {
                environment.emplace_back("ECF_SSL", *c.ecf_ssl);
            }
            std::vector<std::string> args = {"ServerEnvironment"};
            if (c.ssl_option) {
                args.emplace_back("--ssl");
            }
            if (c.http_option) {
                args.emplace_back("--http");
            }
            check_server_precedence("", environment, args, [&](const auto& serverEnv, const auto&) {
                BOOST_REQUIRE(serverEnv.ssl());
                BOOST_REQUIRE(serverEnv.openssl().certificate());
                BOOST_CHECK_EQUAL(serverEnv.openssl().certificate()->shared(), c.expected_shared);
                BOOST_CHECK_MESSAGE(serverEnv.protocol() == c.expected_protocol,
                                    "Expected protocol " << ecf::to_ui_designation(c.expected_protocol) << " but found "
                                                         << ecf::to_ui_designation(serverEnv.protocol()));
            });
        }
    }
}

///
/// @brief Verifies that start-up stops when SSL is requested, by ECF_SSL or `--ssl`, and no certificate exists.
///
BOOST_AUTO_TEST_CASE(test_server_ssl_without_certificate_stops_startup) {
    ECF_NAME_THIS_TEST();
    struct Case
    {
        std::optional<std::string> ecf_ssl;
        bool ssl_option;
    };
    for (const auto& [ecf_ssl, ssl_option] : std::vector<Case>{{"1", false}, {std::nullopt, true}, {"1", true}}) {
        BOOST_TEST_CONTEXT("ECF_SSL: " << ecf_ssl.value_or("(unset)") << ", --ssl: " << ssl_option) {
            auto certificates = ecf::test::scaffold::MakeDirectory{}.create();
            std::vector<std::pair<std::string, std::string>> environment = {
                {"ECF_SSL_DIR", certificates.path().string()}};
            if (ecf_ssl) {
                environment.emplace_back("ECF_SSL", *ecf_ssl);
            }
            std::vector<std::string> args = {"ServerEnvironment"};
            if (ssl_option) {
                args.emplace_back("--ssl");
            }
            BOOST_CHECK_EXCEPTION(
                check_server_precedence("",
                                        environment,
                                        args,
                                        [](const auto&, const auto&) {
                                            BOOST_FAIL("Start-up without a certificate must not be accepted");
                                        }),
                std::runtime_error,
                [](const std::runtime_error& error) {
                    return std::string(error.what()).find("Unable to find certificate file") != std::string::npos;
                });
        }
    }
}

#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
