/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
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
#include "ecflow/core/Pid.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/JobProfiler.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

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

#ifdef ECF_OPENSSL

///
/// @brief Sets or clears an environment variable, restoring its previous value on destruction.
///
/// Environment variables are process-wide, and the test binary runs every case in a single process. A
/// variable left modified therefore leaks into whichever case happens to run next, making that case
/// depend on the order in which the suites were registered. Restoring the previous value confines the
/// modification to the scope that made it.
///
class ScopedEnvironmentVariable {
public:
    ///
    /// @brief Applies the given value to the given variable, for the lifetime of the instance.
    ///
    /// @param[in] name The environment variable to modify
    /// @param[in] value The value to apply; when empty, the variable is cleared instead
    ///
    ScopedEnvironmentVariable(std::string name, std::optional<std::string> value)
        : name_(std::move(name)),
          previous_(ecf::environment::fetch<std::string>(name_.c_str())) {
        if (value) {
            setenv(name_.c_str(), value.value().c_str(), 1);
        }
        else {
            unsetenv(name_.c_str());
        }
    }

    ScopedEnvironmentVariable(const ScopedEnvironmentVariable&)            = delete;
    ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;

    ~ScopedEnvironmentVariable() {
        if (previous_) {
            setenv(name_.c_str(), previous_.value().c_str(), 1);
        }
        else {
            unsetenv(name_.c_str());
        }
    }

private:
    std::string name_;
    std::optional<std::string> previous_;
};

///
/// @brief Provides a temporary certificate directory, for the lifetime of the instance.
///
/// The directory holds a placeholder `server.crt`, and is registered in ECF_SSL_DIR so that it is
/// searched in place of the certificate directory of the user running the tests. ECF_SSL is cleared, so
/// that the outcome does not depend on the environment the tests are launched from. Both variables are
/// restored on destruction, and the directory is removed.
///
/// Only the existence of the certificate file is relevant here: `Openssl` checks for the file when SSL
/// is enabled, and validates its content later, when the server creates the SSL context, which these
/// tests never reach. A placeholder therefore avoids generating a real certificate.
///
/// @invariant The registered directory always ends with a separator, since the certificate paths are
/// built by direct concatenation.
///
class TemporaryCertificate {
public:
    explicit TemporaryCertificate(const std::string& name)
        : directory_((fs::temp_directory_path() / ("ecflow_test_ssl_" + name + "_" + Pid::getpid())).string() + "/"),
          ecf_ssl_(ecf::environment::ECF_SSL, std::nullopt),
          ecf_ssl_dir_("ECF_SSL_DIR", directory_) {
        fs::create_directories(directory_);

        std::ofstream certificate(directory_ + "server.crt");
        certificate << "placeholder\n";
        certificate.close();
    }

    TemporaryCertificate(const TemporaryCertificate&)            = delete;
    TemporaryCertificate& operator=(const TemporaryCertificate&) = delete;

    ~TemporaryCertificate() { fs::remove_all(directory_); }

private:
    std::string directory_;
    ScopedEnvironmentVariable ecf_ssl_;
    ScopedEnvironmentVariable ecf_ssl_dir_;
};

#endif

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

    TemporaryCertificate certificate("promoted_to_ssl");

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

    TemporaryCertificate certificate("promoted_to_https");

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

    ScopedEnvironmentVariable no_ecf_ssl(ecf::environment::ECF_SSL, std::nullopt);

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
