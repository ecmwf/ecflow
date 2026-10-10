// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <vector>

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
#include "ecflow/core/Str.hpp"
#include "ecflow/node/JobProfiler.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace ecf;

// Imported individually, rather than the whole namespace, since the scaffold also declares a Host.
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

} // namespace

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_ServerEnvironment)

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

BOOST_AUTO_TEST_CASE(test_server_config_file) {
    ECF_NAME_THIS_TEST();

    // Regression test to make sure the server environment variable do not get removed

    std::vector<std::string> args = {"ServerEnvironment"};
    ServerEnvironment serverEnv(args, File::test_data("Server/server_environment.cfg", "Server"));

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

namespace {

///
/// @brief Provides a copy of the default server environment file in the current directory.
///
/// The server configuration file is looked for beside the server environment file; a copy in the
/// current directory lets each test place its own server.cfg next to it, without touching the sources.
///
struct WithServerEnvironmentFile
{
    WithServerEnvironmentFile()
        : file(NamedTestFile{"server_environment.cfg"}, default_content()) {}
    static std::string default_content() {
        std::string content;
        File::open(File::test_data("Server/server_environment.cfg", "Server"), content);
        return content;
    }
    WithTestFile file;
    std::string path() const { return "server_environment.cfg"; }
};

///
/// @brief Constructs a server environment and returns the outcome of its validation.
///
/// @param[in] env_file The server environment file to read
/// @param[out] error   The reason the environment is not valid, empty when it is
/// @return A pair of the spawn-as-owner switch and the file that set it
///
std::pair<bool, std::string> read_server_config(const WithServerEnvironmentFile& env_file, std::string& error) {
    std::vector<std::string> args = {"ServerEnvironment"};
    ServerEnvironment serverEnv(args, env_file.path());
    error.clear();
    serverEnv.valid(error);
    std::pair<bool, std::string> result{serverEnv.spawn_as_owner(), serverEnv.server_config_file()};
    remove_log_file(serverEnv);
    return result;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_server_config_file_absent_means_no_switch) {
    ECF_NAME_THIS_TEST();

    WithServerEnvironmentFile env_file;
    std::string error;
    auto [spawn_as_owner, config_file] = read_server_config(env_file, error);
    BOOST_CHECK_MESSAGE(!spawn_as_owner, "Expected no switch without a server configuration file");
    BOOST_CHECK_MESSAGE(config_file.empty(), "Expected no server configuration file, found " << config_file);
    BOOST_CHECK_MESSAGE(error.empty(), "Expected a valid environment, got: " << error);
}

BOOST_AUTO_TEST_CASE(test_server_config_file_flag_off_or_missing_means_no_switch) {
    ECF_NAME_THIS_TEST();

    WithServerEnvironmentFile env_file;
    for (const std::string content : {"{}", "{\"spawn_as_owner\": false}"}) {
        WithTestFile config(NamedTestFile{"server.cfg"}, content);
        std::string error;
        auto [spawn_as_owner, config_file] = read_server_config(env_file, error);
        BOOST_CHECK_MESSAGE(!spawn_as_owner, "Expected no switch for " << content);
        BOOST_CHECK_MESSAGE(ecf::algorithm::ends_with(config_file, "server.cfg"),
                            "Expected the file to be read, found " << config_file);
        BOOST_CHECK_MESSAGE(error.empty(), "Expected a valid environment for " << content << ", got: " << error);
    }
}

BOOST_AUTO_TEST_CASE(test_server_config_file_flag_on_needs_root) {
    ECF_NAME_THIS_TEST();

    WithServerEnvironmentFile env_file;
    WithTestFile config(NamedTestFile{"server.cfg"}, "{\"spawn_as_owner\": true}");
    std::string error;
    auto [spawn_as_owner, config_file] = read_server_config(env_file, error);
    BOOST_CHECK_MESSAGE(spawn_as_owner, "Expected the switch to be enabled");
    if (geteuid() == 0) {
        BOOST_CHECK_MESSAGE(error.empty(), "Expected a valid environment as root, got: " << error);
    }
    else {
        BOOST_CHECK_MESSAGE(error.find("spawn_as_owner") != std::string::npos &&
                                error.find("root") != std::string::npos,
                            "Expected the refusal to name the flag and root, got: " << error);
    }
}

BOOST_AUTO_TEST_CASE(test_server_config_file_faults_prevent_the_start) {
    ECF_NAME_THIS_TEST();

    WithServerEnvironmentFile env_file;
    struct Fault
    {
        std::string content;
        std::string expected;
    };
    for (const Fault& fault : {Fault{"", "not valid JSON"},
                               Fault{"{ not json", "not valid JSON"},
                               Fault{"[true]", "JSON object"},
                               Fault{"{\"spawn_as_owner\": \"yes\"}", "true or false"},
                               Fault{"{\"spawn_as_ownr\": true}", "unknown setting 'spawn_as_ownr'"}}) {
        WithTestFile config(NamedTestFile{"server.cfg"}, fault.content);
        std::string error;
        auto [spawn_as_owner, config_file] = read_server_config(env_file, error);
        BOOST_CHECK_MESSAGE(!spawn_as_owner, "Expected no switch for " << fault.content);
        BOOST_CHECK_MESSAGE(
            error.find(fault.expected) != std::string::npos && error.find("server.cfg") != std::string::npos,
            "Expected '" << fault.expected << "' naming the file for " << fault.content << ", got: " << error);
    }
}

BOOST_AUTO_TEST_CASE(test_server_config_file_is_reported_at_start_up) {
    ECF_NAME_THIS_TEST();

    // The log records which file was read and whether jobs are spawned as their owner
    WithServerEnvironmentFile env_file;
    auto log_of = [&](const std::string& content) {
        WithTestFile config(NamedTestFile{"server.cfg"}, content);
        std::vector<std::string> args = {"ServerEnvironment"};
        ServerEnvironment serverEnv(args, env_file.path());
        Log::instance()->flush();
        std::string log;
        File::open(Log::instance()->path(), log);
        remove_log_file(serverEnv);
        std::string dump = serverEnv.dump();
        return std::make_pair(log, dump);
    };

    auto [log_off, dump_off] = log_of("{}");
    BOOST_CHECK_MESSAGE(log_off.find("Server configuration ") != std::string::npos &&
                            log_off.find("server.cfg") != std::string::npos,
                        "Expected the log to name the configuration file:\n"
                            << log_off);
    BOOST_CHECK_MESSAGE(log_off.find("Jobs are spawned as the server account") != std::string::npos,
                        "Expected the log to report the server account:\n"
                            << log_off);
    BOOST_CHECK_MESSAGE(dump_off.find("spawn_as_owner = 'false'") != std::string::npos,
                        "Unexpected dump:\n"
                            << dump_off);

    auto [log_on, dump_on] = log_of("{\"spawn_as_owner\": true}");
    BOOST_CHECK_MESSAGE(log_on.find("Jobs are spawned as their owner") != std::string::npos,
                        "Expected the log to report the switch:\n"
                            << log_on);
    BOOST_CHECK_MESSAGE(dump_on.find("spawn_as_owner = 'true'") != std::string::npos, "Unexpected dump:\n" << dump_on);
}

BOOST_AUTO_TEST_CASE(test_server_config_file_is_looked_for_beside_the_environment_file) {
    ECF_NAME_THIS_TEST();

    // The environment file lives in a sub-directory: the server.cfg beside it is read, the one in the
    // current directory is not
    fs::create_directories("cfgdir");
    WithTestFile env(NamedTestFile{"cfgdir/server_environment.cfg"}, WithServerEnvironmentFile::default_content());
    WithTestFile beside(NamedTestFile{"cfgdir/server.cfg"}, "{\"spawn_as_owner\": true}");
    WithTestFile elsewhere(NamedTestFile{"server.cfg"}, "{\"spawn_as_owner\": false}");
    {
        std::vector<std::string> args = {"ServerEnvironment"};
        ServerEnvironment serverEnv(args, "cfgdir/server_environment.cfg");
        BOOST_CHECK_MESSAGE(serverEnv.spawn_as_owner(), "Expected the file beside the environment file to be read");
        BOOST_CHECK_MESSAGE(ecf::algorithm::ends_with(serverEnv.server_config_file(), "cfgdir/server.cfg"),
                            "Unexpected file: " << serverEnv.server_config_file());
        remove_log_file(serverEnv);
    }
    fs::remove_all("cfgdir");
}

BOOST_AUTO_TEST_CASE(test_server_config_file_per_server_faults_are_reported_even_when_the_shared_file_is_valid) {
    ECF_NAME_THIS_TEST();

    WithServerEnvironmentFile env_file;
    std::string per_server_name;
    {
        std::vector<std::string> args = {"ServerEnvironment"};
        ServerEnvironment serverEnv(args, env_file.path());
        Host host;
        per_server_name = host.prefix_host_and_port(serverEnv.the_port(), "server.cfg");
        remove_log_file(serverEnv);
    }

    WithTestFile shared(NamedTestFile{"server.cfg"}, "{}");
    WithTestFile per_server(NamedTestFile{per_server_name}, "{ broken");
    std::string error;
    auto [spawn_as_owner, config_file] = read_server_config(env_file, error);
    BOOST_CHECK_MESSAGE(!spawn_as_owner, "Expected no switch");
    BOOST_CHECK_MESSAGE(error.find(per_server_name) != std::string::npos &&
                            error.find("not valid JSON") != std::string::npos,
                        "Expected the per-server file to be the one reported, got: " << error);
}

BOOST_AUTO_TEST_CASE(test_server_config_file_per_server_takes_precedence) {
    ECF_NAME_THIS_TEST();

    WithServerEnvironmentFile env_file;
    std::string per_server_name;
    {
        std::vector<std::string> args = {"ServerEnvironment"};
        ServerEnvironment serverEnv(args, env_file.path());
        Host host;
        per_server_name = host.prefix_host_and_port(serverEnv.the_port(), "server.cfg");
        remove_log_file(serverEnv);
    }

    WithTestFile shared(NamedTestFile{"server.cfg"}, "{\"spawn_as_owner\": true}");
    WithTestFile per_server(NamedTestFile{per_server_name}, "{\"spawn_as_owner\": false}");
    std::string error;
    auto [spawn_as_owner, config_file] = read_server_config(env_file, error);
    BOOST_CHECK_MESSAGE(!spawn_as_owner, "Expected the per-server file to win");
    BOOST_CHECK_MESSAGE(ecf::algorithm::ends_with(config_file, per_server_name),
                        "Expected " << per_server_name << " to be read, found " << config_file);
    BOOST_CHECK_MESSAGE(error.empty(), "Expected a valid environment, got: " << error);
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
