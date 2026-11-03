// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/io/ios_state.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

// The configurations covered by this file are all concerned with the location and selection of SSL
// certificates, and are therefore only meaningful when ecFlow is built with SSL support.
#ifdef ECF_OPENSSL

class MockServerInvoker {
public:
    explicit MockServerInvoker(const std::string& commandline)
        : env_(CommandLine(commandline)) {}

    const ServerEnvironment& environment() const { return env_; };

private:
    ServerEnvironment env_;
};

BOOST_AUTO_TEST_SUITE(U_Server)

using namespace ecf::test::scaffold;

/*
 * The following exports an environment variable used for tests, which changes the location of the SSL certificates.
 * Instead of the default location (HOME/.ecflowrc/ssl), we use the current test directory.
 */
WithTestEnvironmentVariable ecf_ssl_dir("ECF_SSL_DIR", "./");

std::string somehost = ecf::Host().name();
std::string someport = "31415";
std::string someuser = "someuser";
std::string somepass = "somepass";

///
/// @brief Returns the ECF_SSL value that the server exports to jobs.
///
/// @param[in] env Server environment to inspect.
/// @return The exported value, or `<not exported>` when the server exports no ECF_SSL variable.
///
std::string exported_ecf_ssl(const ServerEnvironment& env) {
    std::vector<std::pair<std::string, std::string>> variables;
    env.variables(variables);
    for (const auto& [name, value] : variables) {
        if (name == "ECF_SSL") {
            return value;
        }
    }
    return "<not exported>";
}

BOOST_AUTO_TEST_SUITE(T_ServerConfiguration)

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_shared_and_options_none_and_certificates_shared_and_specific) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), "1");
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_specific_and_options_none_and_certificates_shared_and_specific) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), somehost + '.' + someport);

    // Note:
    //   The actual value of ECF_SSL is not used to find the specific certificate.
    //   Instead, the host name is resolved by the OS, and the selected port is used.
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_shared_and_options_none_and_certificates_shared_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), "1");
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_specific_and_options_none_and_certificates_shared_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), "1");
    BOOST_CHECK_EQUAL(env.openssl().selected_crt(), "server.crt");
    // Jobs follow the selected (shared) certificate, not the requested order.
    BOOST_CHECK_EQUAL(exported_ecf_ssl(env), "1");

    // Note:
    //   Even though ECF_SSL requests the specific certificate, the shared certificate is selected
    //   since it is the only one available.
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_shared_and_options_none_and_certificates_specific_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), somehost + '.' + someport);
    BOOST_CHECK_EQUAL(exported_ecf_ssl(env), somehost + '.' + someport);

    // Note:
    //   Even though ECF_SSL specifies the use of shared certificate, the specific certificate is selected
    //   since it is the only one kind available.
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_specific_and_options_none_and_certificates_specific_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), somehost + '.' + someport);
    BOOST_CHECK_EQUAL(exported_ecf_ssl(env), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(does_not_export_ecf_ssl_when_ssl_is_disabled) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");

    MockServerInvoker server("ecflow_server");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK(!env.ssl());
    BOOST_CHECK_EQUAL(exported_ecf_ssl(env), "<not exported>");
}

BOOST_AUTO_TEST_CASE(logs_selected_certificate_and_warns_on_fallback_at_start_up) {
    ECF_NAME_THIS_TEST();

    const std::string specific_name = somehost + '.' + someport + ".crt";

    // Returns the log line containing the given text, or an empty string when there is none
    auto line_with = [](const std::string& log, const std::string& text) {
        std::istringstream lines(log);
        for (std::string line; std::getline(lines, line);) {
            if (line.find(text) != std::string::npos) {
                return line;
            }
        }
        return std::string{};
    };

    // An unset ECF_SSL starts the server with --ssl alone
    auto check_log = [&](const std::optional<std::string>& ecf_ssl, bool has_shared, bool has_specific) {
        auto directory   = MakeDirectory{}.create();
        auto shared_name = has_shared ? std::string("server.crt") : std::string("unused-shared");
        auto shared      = MakeTestFile{}.with(SpecificFileLocation{shared_name, directory}).create();
        auto other_name  = has_specific ? specific_name : std::string("unused-specific");
        auto specific    = MakeTestFile{}.with(SpecificFileLocation{other_name, directory}).create();
        WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string());
        WithTestEnvironmentVariable ecf_log("ECF_LOG", (directory.path() / "server.log").string());
        WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
        std::optional<WithTestEnvironmentVariable> ssl_env;
        std::optional<WithoutTestEnvironmentVariable> no_ssl;
        if (ecf_ssl) {
            ssl_env.emplace("ECF_SSL", *ecf_ssl);
        }
        else {
            no_ssl.emplace("ECF_SSL");
        }

        MockServerInvoker server(ecf_ssl ? "ecflow_server" : "ecflow_server --ssl");
        BOOST_REQUIRE(server.environment().ssl());
        const auto log = ecf::Log::instance()->contents(-100);

        const bool shared_first    = ecf_ssl && (ecf_ssl->empty() || *ecf_ssl == "1");
        const bool shared_selected = has_shared && (shared_first || !has_specific);
        const std::string kind     = shared_selected ? "shared" : "server-specific";
        const auto selected        = (directory.path() / (shared_selected ? "server.crt" : specific_name)).string();
        const auto info            = line_with(log, "  SSL certificate = " + selected + " (" + kind + ")");
        BOOST_CHECK_MESSAGE(info.rfind("MSG:", 0) == 0, "Expected a MSG record of the selection in:\n" << log);

        const auto missing = (directory.path() / (shared_first ? "server.crt" : specific_name)).string();
        const auto warning = line_with(log, "not found; using the");
        if (shared_selected != shared_first) {
            const auto expected = "SSL certificate '" + missing + "' not found; using the " + kind + " certificate";
            BOOST_CHECK_MESSAGE(warning.rfind("WAR:", 0) == 0, "Expected a WAR record of the fallback in:\n" << log);
            BOOST_CHECK_MESSAGE(warning.find(expected) != std::string::npos, "Expected '" << expected << "'");
        }
        else {
            BOOST_CHECK_MESSAGE(warning.empty(), "Expected no fallback warning in:\n" << log);
        }
    };

    // Specific-first: falls back to the shared certificate only when the specific one is absent
    check_log(somehost + '.' + someport, true, false);
    check_log(somehost + '.' + someport, true, true);
    check_log("X", false, true);
    // Shared-first: falls back to the specific certificate only when the shared one is absent
    check_log("1", false, true);
    check_log("1", true, true);
    check_log("", true, false);
    // The option alone is specific-first
    check_log(std::nullopt, true, false);
    check_log(std::nullopt, true, true);
    check_log(std::nullopt, false, true);
}

BOOST_AUTO_TEST_CASE(selects_certificate_for_every_combination_of_ecf_ssl_and_ssl_option) {
    ECF_NAME_THIS_TEST();

    const std::string specific_stem  = somehost + '.' + someport;
    const std::string shared_first   = "shared certificate first, server-specific certificate as fallback";
    const std::string specific_first = "server-specific certificate first, shared certificate as fallback";
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    for (const bool has_shared : {false, true}) {
        for (const bool has_specific : {false, true}) {
            auto directory = MakeDirectory{}.create();
            WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string());
            auto shared = MakeTestFile{}
                              .with(SpecificFileLocation{has_shared ? "server.crt" : "unused-shared", directory})
                              .create();
            auto specific =
                MakeTestFile{}
                    .with(SpecificFileLocation{has_specific ? specific_stem + ".crt" : "unused-specific", directory})
                    .create();

            for (const auto& ecf_ssl : {std::optional<std::string>{},
                                        std::optional<std::string>{""},
                                        std::optional<std::string>{"1"},
                                        std::optional<std::string>{"X"},
                                        std::optional<std::string>{specific_stem}}) {
                std::optional<WithTestEnvironmentVariable> ssl_env;
                std::optional<WithoutTestEnvironmentVariable> no_ssl;
                if (ecf_ssl) {
                    ssl_env.emplace("ECF_SSL", *ecf_ssl);
                }
                else {
                    no_ssl.emplace("ECF_SSL");
                }

                for (const bool option : {false, true}) {
                    const auto command = std::string("ecflow_server") + (option ? " --ssl" : "");
                    const auto context = "ECF_SSL=" + (ecf_ssl ? "'" + *ecf_ssl + "'" : std::string("(unset)")) +
                                         (option ? " with --ssl" : "") + ", shared=" + std::to_string(has_shared) +
                                         ", specific=" + std::to_string(has_specific);
                    BOOST_TEST_CONTEXT(context) {
                        if (!ecf_ssl && !option) {
                            MockServerInvoker server(command);
                            BOOST_CHECK(!server.environment().ssl());
                            BOOST_CHECK_EQUAL(exported_ecf_ssl(server.environment()), "<not exported>");
                            continue;
                        }

                        // ECF_SSL, when set, decides the lookup order; --ssl alone selects specific-first
                        const bool prefer_shared = ecf_ssl && (ecf_ssl->empty() || *ecf_ssl == "1");
                        const auto& strategy     = prefer_shared ? shared_first : specific_first;
                        if (!has_shared && !has_specific) {
                            BOOST_CHECK_EXCEPTION(
                                MockServerInvoker{command}, std::runtime_error, [&](const std::runtime_error& error) {
                                    return std::string(error.what()).find("(using strategy: " + strategy + ")") !=
                                           std::string::npos;
                                });
                            continue;
                        }

                        // Jobs follow the selected certificate, not the requested order
                        const bool shared_selected = has_shared && (prefer_shared || !has_specific);
                        MockServerInvoker server(command);
                        const ServerEnvironment& env = server.environment();
                        BOOST_CHECK(env.ssl());
                        BOOST_CHECK_EQUAL(env.openssl().requested().shared(), prefer_shared);
                        BOOST_CHECK_EQUAL(env.openssl().selected_crt(),
                                          shared_selected ? std::string("server.crt") : specific_stem + ".crt");
                        BOOST_CHECK_EQUAL(exported_ecf_ssl(env), shared_selected ? std::string("1") : specific_stem);
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_none_and_options_ssl_and_certificates_shared_and_specific) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    MockServerInvoker server("ecflow_server -d --ssl");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), somehost + '.' + someport);
    // Jobs follow the selected (specific) certificate.
    BOOST_CHECK_EQUAL(exported_ecf_ssl(env), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_none_and_options_ssl_and_certificates_shared_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    MockServerInvoker server("ecflow_server -d --ssl");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), "1");
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_setup_environment_with_env_request_none_and_options_ssl_and_certificates_specific_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    MockServerInvoker server("ecflow_server -d --ssl");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_REQUIRE(env.openssl().certificate());
    BOOST_CHECK_EQUAL(env.openssl().certificate()->to_envvar_value(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(reports_ssl_enable_source_strategy_and_selected_certificate_in_debug_output) {
    ECF_NAME_THIS_TEST();

    auto directory = MakeDirectory{}.create();
    auto shared    = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string());
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");

    const std::string shared_first   = "shared certificate first, server-specific certificate as fallback";
    const std::string specific_first = "server-specific certificate first, shared certificate as fallback";
    // The leading newline requires each line to start at column 0, as the other server debug lines do
    const std::string selected =
        "\nSSL certificate selected: '" + (directory.path() / "server.crt").string() + "' (shared)\n";

    auto check_output = [&](bool flag, const std::string& source, const std::string& strategy) {
        for (const bool debug : {false, true}) {
            std::ostringstream output;
            {
                boost::io::ios_rdbuf_saver capture(std::cout, output.rdbuf());
                MockServerInvoker server(std::string("ecflow_server") + (flag ? " --ssl" : "") + (debug ? " -d" : ""));
                BOOST_CHECK_EQUAL(server.environment().openssl().selected_crt(), "server.crt");
            }
            if (debug) {
                // Prepended newline: the first line of the output also has to start at column 0
                const auto text     = "\n" + output.str();
                const auto expected = "\nSSL enabled: " + source + " (" + strategy + ")\n";
                BOOST_CHECK_NE(text.find(expected), std::string::npos);
                BOOST_CHECK_NE(text.find(selected), std::string::npos);
                BOOST_CHECK_LT(text.find(expected), text.find(selected));
            }
            else {
                BOOST_CHECK_EQUAL(output.str().find("SSL enabled:"), std::string::npos);
            }
        }
    };

    check_output(true, "via --ssl, ECF_SSL unset", specific_first);
    for (const auto* value : {"", "1", "X"}) {
        WithTestEnvironmentVariable ssl_env("ECF_SSL", value);
        const auto& strategy = std::string(value).empty() || std::string(value) == "1" ? shared_first : specific_first;
        check_output(false, "via ECF_SSL='" + std::string(value) + "'", strategy);
        check_output(true, "via --ssl with ECF_SSL='" + std::string(value) + "'", strategy);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#endif
