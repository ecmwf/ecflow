// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <future>
#include <limits>
#include <list>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Checks bounded executable rejection with the original diagnostic and no remaining listener.
///
/// @param[in] content Configuration settings to reject
/// @param[in] environment Environment overrides to reject
/// @param[in] fragments Required diagnostic fragments identifying the selected setting and cause
/// @param[in] before_log Whether rejection must precede log creation
/// @param[in] config_kind Regular, unreadable or directory configuration fixture
///
void check_rejected_initialisation(const std::string& content,
                                   const std::vector<std::pair<std::string, std::string>>& environment,
                                   const std::vector<std::string>& fragments,
                                   bool before_log                = true,
                                   const std::string& config_kind = "regular") {
    using namespace ecf::test::scaffold;

    auto startup           = MakeDirectory{}.create();
    const auto config_path = startup.path() / "server_environment.cfg";
    const auto log_path    = startup.path() / "server.log";

    std::list<WithoutTestEnvironmentVariable> cleared;
    for (const auto* name : {"ECF_HOME",
                             "ECF_PORT",
                             "ECF_LOG",
                             "ECF_CHECK",
                             "ECF_CHECKOLD",
                             "ECF_PASSWD",
                             "ECF_CUSTOM_PASSWD",
                             "ECF_LISTS",
                             "ECF_CHECKINTERVAL",
                             "ECF_PRUNE_NODE_LOG",
                             "ECF_TASK_THRESHOLD",
                             "ECF_SSL",
                             "ECF_DEBUG_SERVER",
                             "ECF_USER",
                             "ECF_HOST"}) {
        cleared.emplace_back(name);
    }

    auto port          = MakePort{}.with(AutomaticPortValue{}).create();
    std::string config = content;
    if (content.find("ECF_LOG =") == std::string::npos) {
        config += "ECF_LOG = server.log\n";
    }
    if (content.find("ECF_PORT =") == std::string::npos) {
        config += "ECF_PORT = " + std::to_string(port.value()) + "\n";
    }

    std::list<WithTestFile> files;
    BOOST_SCOPE_EXIT_ALL(&) {
        if (config_kind == "unreadable") {
            std::error_code error;
            fs::permissions(
                config_path, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace, error);
            BOOST_CHECK_MESSAGE(!error, "Unable to restore config permissions: " << error.message());
        }
    };

    if (config_kind == "directory") {
        fs::create_directories(config_path);
    }
    else {
        files.emplace_back(NamedTestFile{config_path.string()}, config);
        if (config_kind == "unreadable") {
            fs::permissions(config_path, fs::perms::none, fs::perm_options::replace);
            if (::access(config_path.c_str(), R_OK) == 0) {
                ecf::test::scaffold::skip_unenforced_permissions("the unreadable config case");
                return;
            }
        }
    }

    std::vector<std::string> args = {"--port=" + std::to_string(port.value())};
    std::list<WithTestEnvironmentVariable> overrides;
    for (const auto& [name, value] : environment) {
        overrides.emplace_back(name, value);
        if (name == "ECF_PORT") {
            // Do not turn the unresolved invalid-environment/valid-CLI policy into a contract.
            args.clear();
        }
    }

    Process process(fs::path(CMAKE_ECFLOW_BUILD_DIR()) / "bin" / "ecflow_server", args, startup.path());
    auto completion   = std::async(std::launch::async, [&] { return process.wait(); });
    const bool exited = completion.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
    if (!exited) {
        process.terminate();
    }

    const int status  = completion.get();
    const auto output = process.read_stdout() + process.read_stderr();
    BOOST_REQUIRE_MESSAGE(exited, "Invalid settings did not stop startup within five seconds: " << output);
    BOOST_CHECK_EQUAL(status, 1);
    for (const auto& fragment : fragments) {
        BOOST_CHECK_MESSAGE(output.find(fragment) != std::string::npos, output);
    }
    BOOST_CHECK_EQUAL(fs::exists(log_path), !before_log);

    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    boost::system::error_code error;
    socket.connect({boost::asio::ip::address_v4::loopback(), static_cast<unsigned short>(port.value())}, error);
    BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                        "Expected connection refusal after failed startup, found: " << error.message());
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerValidation)

///
/// @brief Verifies clean rejection of explicitly empty required paths and command strings.
///
BOOST_AUTO_TEST_CASE(test_server_validation_empty_required_settings) {
    ECF_NAME_THIS_TEST();

    for (const std::string name : {"ECF_HOME",
                                   "ECF_LOG",
                                   "ECF_CHECK",
                                   "ECF_CHECKOLD",
                                   "ECF_PASSWD",
                                   "ECF_CUSTOM_PASSWD",
                                   "ECF_LISTS",
                                   "ECF_JOB_CMD",
                                   "ECF_KILL_CMD",
                                   "ECF_STATUS_CMD",
                                   "ECF_CHECK_CMD",
                                   "ECF_URL_CMD",
                                   "ECF_URL_BASE",
                                   "ECF_URL",
                                   "ECF_MICRODEF"}) {
        BOOST_TEST_CONTEXT("Empty config setting: " << name) {
            check_rejected_initialisation(name + " =\n", {}, {name, "must not be empty"});
        }
    }

    for (const std::string name :
         {"ECF_HOME", "ECF_LOG", "ECF_CHECK", "ECF_CHECKOLD", "ECF_PASSWD", "ECF_CUSTOM_PASSWD", "ECF_LISTS"}) {
        BOOST_TEST_CONTEXT("Empty environment setting: " << name) {
            check_rejected_initialisation("", {{name, ""}}, {name, "must not be empty"});
        }
    }
}

///
/// @brief Verifies executable rejection of invalid integer text and values outside the integer range.
///
BOOST_AUTO_TEST_CASE(test_server_validation_integer_inputs) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::string> values = {
        "not_an_integer",
        std::to_string(static_cast<long long>(std::numeric_limits<int>::max()) + 1),
        std::to_string(static_cast<long long>(std::numeric_limits<int>::min()) - 1)};
    for (const std::string name :
         {"ECF_PORT", "ECF_CHECKINTERVAL", "ECF_INTERVAL", "ECF_PRUNE_NODE_LOG", "ECF_TASK_THRESHOLD"}) {
        for (const auto& value : values) {
            BOOST_TEST_CONTEXT(name << " = " << value) {
                check_rejected_initialisation(name + " = " + value + "\n", {}, {"server_environment.cfg", name, value});
                if (name != "ECF_INTERVAL") {
                    check_rejected_initialisation("", {{name, value}}, {name, value});
                }
            }
        }
    }
}

///
/// @brief Verifies failure diagnostics for numeric ranges after parsing succeeds.
///
BOOST_AUTO_TEST_CASE(test_server_validation_numeric_ranges) {
    ECF_NAME_THIS_TEST();

    for (const int interval : {0, 61}) {
        BOOST_TEST_CONTEXT("Scheduling interval: " << interval) {
            check_rejected_initialisation("ECF_INTERVAL = " + std::to_string(interval) + "\n",
                                          {},
                                          {"ECF_INTERVAL", std::to_string(interval)},
                                          false);
        }
    }

    check_rejected_initialisation("ECF_PRUNE_NODE_LOG = -1\n", {}, {"ECF_PRUNE_NODE_LOG", "-1"}, false);

    for (const int threshold : {-1, 0}) {
        BOOST_TEST_CONTEXT("Task threshold: " << threshold) {
            check_rejected_initialisation("ECF_TASK_THRESHOLD = " + std::to_string(threshold) + "\n",
                                          {},
                                          {"ECF_TASK_THRESHOLD", std::to_string(threshold), "positive integer"});
            check_rejected_initialisation("",
                                          {{"ECF_TASK_THRESHOLD", std::to_string(threshold)}},
                                          {"ECF_TASK_THRESHOLD", std::to_string(threshold), "positive integer"});
        }
    }

    for (const int interval : {-1, 0}) {
        BOOST_TEST_CONTEXT("Checkpoint interval: " << interval) {
            check_rejected_initialisation("ECF_CHECKINTERVAL = " + std::to_string(interval) + "\n",
                                          {},
                                          {"ECF_CHECKINTERVAL", std::to_string(interval), "positive integer"},
                                          false);
            check_rejected_initialisation("",
                                          {{"ECF_CHECKINTERVAL", std::to_string(interval)}},
                                          {"ECF_CHECKINTERVAL", std::to_string(interval), "positive integer"},
                                          false);
        }
    }
}

///
/// @brief Verifies that an unsupported checkpoint mode stops startup, naming the file, the value and the modes.
///
BOOST_AUTO_TEST_CASE(test_server_validation_check_mode) {
    ECF_NAME_THIS_TEST();

    for (const std::string mode : {"unsupported", "check_on_time", ""}) {
        BOOST_TEST_CONTEXT("Checkpoint mode: '" << mode << "'") {
            check_rejected_initialisation("ECF_CHECKMODE = " + mode + "\n",
                                          {},
                                          {"server_environment.cfg",
                                           "ECF_CHECKMODE",
                                           "'" + mode + "'",
                                           "CHECK_NEVER, CHECK_ON_TIME or CHECK_ALWAYS"});
        }
    }
}

///
/// @brief Verifies that an existing unreadable config is not treated as an absent optional file.
///
BOOST_AUTO_TEST_CASE(test_server_validation_unreadable_config) {
    ECF_NAME_THIS_TEST();

    check_rejected_initialisation("",
                                  {{"ECF_PORT", "invalid_environment_port"}},
                                  {"server_environment.cfg", "Cannot open", "reading"},
                                  true,
                                  "unreadable");
}

///
/// @brief Verifies that a directory cannot stand in for the optional configuration file.
///
BOOST_AUTO_TEST_CASE(test_server_validation_directory_config) {
    ECF_NAME_THIS_TEST();

    check_rejected_initialisation("",
                                  {{"ECF_PORT", "invalid_environment_port"}},
                                  {"server_environment.cfg", "not a regular file"},
                                  true,
                                  "directory");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
