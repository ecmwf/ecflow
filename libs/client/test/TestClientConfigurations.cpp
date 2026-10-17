// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <sstream>

#include <boost/io/ios_state.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

// The configurations covered by this file are all concerned with the location and selection of SSL
// certificates, and are therefore only meaningful when ecFlow is built with SSL support.
#ifdef ECF_OPENSSL

struct MockClientInvoker
{
    explicit MockClientInvoker(const std::string& commandline)
        : client_() {
        client_.set_cli(true);
        // Process the command line arguments to trigger the environment initialization
        auto b = client_.get_cmd_from_args(CommandLine(commandline));
    }

    ClientEnvironment& environment() { return client_.environment(); };
    const ClientEnvironment& environment() const { return client_.environment(); };

private:
    ClientInvoker client_;
};

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientConfiguration)

using namespace ecf::test::scaffold;

/*
 * The following exports an environment variable used for tests, which changes the location of the SSL certificates.
 * Instead of the default location (HOME/.ecflowrc/ssl), we use the current test directory.
 */
WithTestEnvironmentVariable ecf_ssl_dir("ECF_SSL_DIR", "./");

std::string somehost = "somehost";
std::string someport = "31415";
std::string someuser = "someuser";
std::string somepass = "somepass";

BOOST_AUTO_TEST_CASE(reports_ssl_enable_source_and_requested_strategy) {
    ECF_NAME_THIS_TEST();
    auto directory = MakeDirectory{}.create();
    auto shared    = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string());
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");

    const std::string shared_first   = "shared certificate first, server-specific certificate as fallback";
    const std::string specific_first = "server-specific certificate first, shared certificate as fallback";

    auto check_output = [&](bool flag, const std::string& source, const std::string& strategy) {
        for (const bool debug : {false, true}) {
            std::ostringstream output;
            {
                boost::io::ios_rdbuf_saver capture(std::cout, output.rdbuf());
                MockClientInvoker cli("ecflow_client --host " + somehost + " --port " + someport + " --ping" +
                                      (flag ? " --ssl" : "") + (debug ? " -d" : ""));
                BOOST_CHECK_EQUAL(cli.environment().openssl().selected_crt(), "server.crt");
            }
            if (debug) {
                const auto expected = "SSL enable: " + source + " (" + strategy + ")";
                BOOST_CHECK_NE(output.str().find(expected), std::string::npos);
                BOOST_CHECK_LT(output.str().find(expected), output.str().find("SSL certificate selected:"));
            }
            else {
                BOOST_CHECK_EQUAL(output.str().find("SSL enable:"), std::string::npos);
            }
        }
    };

    check_output(true, "via --ssl, ECF_SSL unset", shared_first);
    for (const auto* value : {"", "1", "X", "?"}) {
        WithTestEnvironmentVariable ssl_env("ECF_SSL", value);
        const auto& strategy = std::string(value).empty() || std::string(value) == "1" ? shared_first : specific_first;
        check_output(false, "via ECF_SSL='" + std::string(value) + "'", strategy);
        check_output(true, "via --ssl with ECF_SSL='" + std::string(value) + "'", strategy);
    }
}

BOOST_AUTO_TEST_CASE(reports_selected_certificate_in_debug_output) {
    ECF_NAME_THIS_TEST();
    auto directory = MakeDirectory{}.create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", fs::relative(directory.path()).string());
    const auto specific_name = somehost + '.' + someport + ".crt";

    auto check_output = [&](const std::string& shared_first, const std::string& specific_first) {
        for (const auto* mode : {"", "1", "X", "?"}) {
            WithTestEnvironmentVariable ssl_env("ECF_SSL", mode);
            const auto selected = std::string(mode).empty() || std::string(mode) == "1" ? shared_first : specific_first;
            const auto path     = (directory.path() / selected).string();
            const auto kind     = selected == "server.crt" ? "shared" : "server-specific";
            const auto message  = "SSL certificate selected: '" + path + "' (" + kind + ")";

            for (const bool debug : {false, true}) {
                std::ostringstream output;
                {
                    boost::io::ios_rdbuf_saver capture(std::cout, output.rdbuf());
                    MockClientInvoker cli("ecflow_client --host " + somehost + " --port " + someport + " --ping" +
                                          (debug ? " -d" : ""));
                    BOOST_CHECK_EQUAL(cli.environment().openssl().selected_crt(), selected);
                }
                if (debug) {
                    BOOST_CHECK_NE(output.str().find(message), std::string::npos);
                }
                else {
                    BOOST_CHECK_EQUAL(output.str().find("SSL certificate selected:"), std::string::npos);
                }
            }
        }
    };

    {
        auto shared = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
        check_output("server.crt", "server.crt");
        auto specific = MakeTestFile{}.with(SpecificFileLocation{specific_name, directory}).create();
        check_output("server.crt", specific_name);
    }
    {
        auto specific = MakeTestFile{}.with(SpecificFileLocation{specific_name, directory}).create();
        check_output(specific_name, specific_name);
    }
}

BOOST_AUTO_TEST_CASE(accepts_certificate_directories_with_optional_trailing_separator) {
    ECF_NAME_THIS_TEST();
    auto directory          = MakeDirectory{}.create();
    const auto certificates = directory.path() / "ssl certificates";
    fs::create_directory(certificates);
    auto shared = MakeTestFile{}.with(SpecificFileLocation{"server.crt", certificates}).create();
    auto specific =
        MakeTestFile{}.with(SpecificFileLocation{somehost + '.' + someport + ".crt", certificates}).create();

    for (const auto& path : {certificates.string(), certificates.string() + "/", fs::relative(certificates).string()}) {
        WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", path);
        for (const auto* mode : {"1", "X"}) {
            WithTestEnvironmentVariable ssl_env("ECF_SSL", mode);
            MockClientInvoker cli("ecflow_client --host " + somehost + " --port " + someport + " --ssl --ping");
            BOOST_CHECK_EQUAL(cli.environment().openssl().selected_crt(),
                              std::string(mode) == "1" ? "server.crt" : somehost + '.' + someport + ".crt");
        }

        ClientInvoker ui(true, somehost, someport);
        ui.enable_ssl_prefer_specific();
        BOOST_CHECK_EQUAL(ui.get_certificate(), somehost + '.' + someport + ".crt");
        BOOST_CHECK_EQUAL(ui.environment().openssl().info(),
                          "SSL certificate selected: '" + specific.path().string() + "' (server-specific)");

        ClientInvoker shared_ui(true, "sharedhost", someport);
        shared_ui.enable_ssl_prefer_specific();
        BOOST_CHECK_EQUAL(shared_ui.get_certificate(), "server.crt");
        BOOST_CHECK_EQUAL(shared_ui.environment().openssl().info(),
                          "SSL certificate selected: '" + shared.path().string() + "' (shared)");
    }
}

BOOST_AUTO_TEST_CASE(uses_default_certificate_directory_when_override_is_unset) {
    ECF_NAME_THIS_TEST();
    auto directory          = MakeDirectory{}.create();
    const auto certificates = directory.path() / ".ecflowrc" / "ssl";
    fs::create_directories(certificates);
    auto shared = MakeTestFile{}.with(SpecificFileLocation{"server.crt", certificates}).create();
    auto specific =
        MakeTestFile{}.with(SpecificFileLocation{somehost + '.' + someport + ".crt", certificates}).create();
    WithTestEnvironmentVariable home("HOME", directory.path().string());
    WithoutTestEnvironmentVariable ssl_dir("ECF_SSL_DIR");
    WithTestEnvironmentVariable ssl_env("ECF_SSL", "X");

    MockClientInvoker cli("ecflow_client --host " + somehost + " --port " + someport + " --ssl --ping");
    BOOST_CHECK_EQUAL(cli.environment().openssl().selected_crt(), somehost + '.' + someport + ".crt");

    ClientInvoker ui(true, somehost, someport);
    ui.enable_ssl_prefer_specific();
    BOOST_CHECK_EQUAL(ui.get_certificate(), somehost + '.' + someport + ".crt");
    BOOST_CHECK_EQUAL(ui.environment().openssl().info(),
                      "SSL certificate selected: '" + specific.path().string() + "' (server-specific)");
}

BOOST_AUTO_TEST_CASE(finds_server_tls_files_with_optional_trailing_separator) {
    ECF_NAME_THIS_TEST();
    auto directory   = MakeDirectory{}.create();
    auto certificate = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();

    for (const auto& path : {directory.path().string(), directory.path().string() + "/"}) {
        WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", path);
        ecf::Openssl server;
        server.enable_prefer_shared(somehost, someport);

        const auto key_path = (directory.path() / "server.key").string();
        BOOST_CHECK_EXCEPTION(server.init_for_server(), std::runtime_error, [&](const std::runtime_error& error) {
            return std::string(error.what()).find("'" + key_path + "'") != std::string::npos;
        });

        auto key            = MakeTestFile{}.with(SpecificFileLocation{"server.key", directory}).create();
        const auto pem_path = (directory.path() / "dh1024.pem").string();
        BOOST_CHECK_EXCEPTION(server.init_for_server(), std::runtime_error, [&](const std::runtime_error& error) {
            return std::string(error.what()).find("'" + pem_path + "'") != std::string::npos;
        });

        auto parameters = MakeTestFile{}.with(SpecificFileLocation{"dh2048.pem", directory}).create();
        // All files are found; loading the empty certificate must now fail.
        BOOST_CHECK_THROW(server.init_for_server(), boost::system::system_error);
    }
}

BOOST_AUTO_TEST_CASE(prefers_specific_certificate_independently_of_ssl_environment) {
    ECF_NAME_THIS_TEST();
    auto directory = MakeDirectory{}.create();
    auto shared    = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
    auto specific  = MakeTestFile{}.with(SpecificFileLocation{somehost + '.' + someport + ".crt", directory}).create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string() + "/");
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");

    auto check_selection = [] {
        ClientInvoker client(true, somehost, someport);
        client.enable_ssl_prefer_specific();
        BOOST_CHECK(client.environment().ssl());
        BOOST_CHECK(client.environment().protocol() == ecf::Protocol::Ssl);
        BOOST_CHECK_EQUAL(client.get_certificate(), somehost + '.' + someport + ".crt");
    };

    check_selection();
    for (const auto* value : {"", "1", "X"}) {
        WithTestEnvironmentVariable ssl_env("ECF_SSL", value);
        check_selection();
    }
}

BOOST_AUTO_TEST_CASE(falls_back_to_shared_certificate_when_specific_is_absent) {
    ECF_NAME_THIS_TEST();
    auto directory = MakeDirectory{}.create();
    auto shared    = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string());
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");

    auto check_selection = [] {
        ClientInvoker client(true, somehost, someport);
        client.enable_ssl_prefer_specific();
        BOOST_CHECK(client.environment().ssl());
        BOOST_CHECK_EQUAL(client.get_certificate(), "server.crt");
    };

    check_selection();
    for (const auto* value : {"", "1", "X", "other"}) {
        WithTestEnvironmentVariable ssl_env("ECF_SSL", value);
        check_selection();

        MockClientInvoker cli("ecflow_client --host " + somehost + " --port " + someport + " --ssl --ping");
        BOOST_CHECK(cli.environment().ssl());
        BOOST_CHECK_EQUAL(cli.environment().openssl().selected_crt(), "server.crt");
    }
}

BOOST_AUTO_TEST_CASE(selects_certificates_independently_for_multiple_servers) {
    ECF_NAME_THIS_TEST();
    auto directory = MakeDirectory{}.create();
    auto shared    = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
    auto first     = MakeTestFile{}.with(SpecificFileLocation{somehost + '.' + someport + ".crt", directory}).create();
    auto second    = MakeTestFile{}.with(SpecificFileLocation{somehost + ".31416.crt", directory}).create();
    auto third     = MakeTestFile{}.with(SpecificFileLocation{"otherhost." + someport + ".crt", directory}).create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string() + "/");

    ClientInvoker first_client(true, somehost, someport);
    ClientInvoker second_client(true, somehost, "31416");
    ClientInvoker third_client(true, "otherhost", someport);
    ClientInvoker shared_client(true, "sharedhost", someport);
    first_client.enable_ssl_prefer_specific();
    second_client.enable_ssl_prefer_specific();
    third_client.enable_ssl_prefer_specific();
    shared_client.enable_ssl_prefer_specific();

    BOOST_CHECK_EQUAL(first_client.get_certificate(), somehost + '.' + someport + ".crt");
    BOOST_CHECK_EQUAL(second_client.get_certificate(), somehost + ".31416.crt");
    BOOST_CHECK_EQUAL(third_client.get_certificate(), "otherhost." + someport + ".crt");
    BOOST_CHECK_EQUAL(shared_client.get_certificate(), "server.crt");
}

BOOST_AUTO_TEST_CASE(reports_missing_certificates_without_enabling_ssl) {
    ECF_NAME_THIS_TEST();
    auto directory          = MakeDirectory{}.create();
    const auto certificates = directory.path() / "ssl certificates";
    fs::create_directory(certificates);
    WithoutTestEnvironmentVariable no_ssl("ECF_SSL");

    auto check_errors = [&](const fs::path& certificate_directory) {
        for (const auto& host : {somehost, std::string("localhost")}) {
            const auto resolved_host = host == "localhost" ? ecf::Host().name() : host;
            const auto specific      = (certificate_directory / (resolved_host + '.' + someport + ".crt")).string();
            const auto shared        = (certificate_directory / "server.crt").string();
            const auto shared_error =
                "Error: Unable to find certificate file: '" + shared + "' or '" + specific +
                "' (using strategy: shared certificate first, server-specific certificate as fallback)";
            const auto specific_error =
                "Error: Unable to find certificate file: '" + specific + "' or '" + shared +
                "' (using strategy: server-specific certificate first, shared certificate as fallback)";
            auto matches_shared     = [&](const std::runtime_error& error) { return error.what() == shared_error; };
            auto matches_specific   = [&](const std::runtime_error& error) { return error.what() == specific_error; };
            auto matches_cli_shared = [&](const std::runtime_error& error) {
                return std::string(error.what()).find("\n" + shared_error + "\n") != std::string::npos;
            };

            ClientInvoker ui(true, host, someport);
            BOOST_CHECK_EXCEPTION(ui.enable_ssl_prefer_specific(), std::runtime_error, matches_specific);
            BOOST_CHECK(!ui.environment().ssl());
            BOOST_CHECK_EQUAL(ui.get_certificate(), "");
            BOOST_CHECK_EQUAL(ui.environment().openssl().info(), "SSL disabled");

            ecf::Openssl ssl;
            BOOST_CHECK_EXCEPTION(ssl.enable_prefer_shared(host, someport), std::runtime_error, matches_shared);
            BOOST_CHECK(!ssl.enabled());
            BOOST_CHECK_EQUAL(ssl.selected_crt(), "");
            BOOST_CHECK_EXCEPTION(
                MockClientInvoker{"ecflow_client --host " + host + " --port " + someport + " --ssl --ping"},
                std::runtime_error,
                matches_cli_shared);

            for (const auto* mode : {"", "1", "X", "?"}) {
                WithTestEnvironmentVariable ssl_env("ECF_SSL", mode);
                const auto& expected =
                    std::string(mode).empty() || std::string(mode) == "1" ? shared_error : specific_error;
                auto matches_expected     = [&](const std::runtime_error& error) { return error.what() == expected; };
                auto matches_cli_expected = [&](const std::runtime_error& error) {
                    return std::string(error.what()).find("\n" + expected + "\n") != std::string::npos;
                };
                for (const bool flag : {false, true}) {
                    BOOST_CHECK_EXCEPTION(MockClientInvoker{"ecflow_client --host " + host + " --port " + someport +
                                                            " --ping" + (flag ? " --ssl" : "")},
                                          std::runtime_error,
                                          matches_cli_expected);
                }
                BOOST_CHECK_EXCEPTION(ssl.enable_based_on_envvar(host, someport), std::runtime_error, matches_expected);
                BOOST_CHECK(!ssl.try_enable_from_envvar(host, someport, mode));
                BOOST_CHECK(!ssl.enabled());
                BOOST_CHECK_EQUAL(ssl.selected_crt(), "");
            }
        }
    };

    for (const auto& path : {certificates.string(), certificates.string() + "/", fs::relative(certificates).string()}) {
        WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", path);
        check_errors(certificates);
    }
    {
        WithTestEnvironmentVariable home("HOME", directory.path().string());
        WithoutTestEnvironmentVariable ssl_dir("ECF_SSL_DIR");
        check_errors(directory.path() / ".ecflowrc" / "ssl");
    }
}

BOOST_AUTO_TEST_CASE(does_not_fall_back_when_specific_certificate_cannot_be_loaded) {
    ECF_NAME_THIS_TEST();
    auto directory = MakeDirectory{}.create();
    auto shared    = MakeTestFile{}.with(SpecificFileLocation{"server.crt", directory}).create();
    auto specific  = MakeTestFile{}
                        .with(SpecificFileLocation{somehost + '.' + someport + ".crt", directory})
                        .with("invalid certificate")
                        .create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string() + "/");

    ClientInvoker client(true, somehost, someport);
    client.enable_ssl_prefer_specific();
    BOOST_CHECK_THROW(client.environment().openssl().init_for_client(), boost::system::system_error);
    BOOST_CHECK_EQUAL(client.get_certificate(), somehost + '.' + someport + ".crt");

    WithTestEnvironmentVariable ssl_env("ECF_SSL", "X");
    MockClientInvoker cli("ecflow_client --host " + somehost + " --port " + someport + " --ssl --ping");
    BOOST_CHECK_THROW(cli.environment().openssl().init_for_client(), boost::system::system_error);
    BOOST_CHECK_EQUAL(cli.environment().openssl().selected_crt(), somehost + '.' + someport + ".crt");
}

BOOST_AUTO_TEST_CASE(selects_specific_certificate_for_localhost_without_shared_certificate) {
    ECF_NAME_THIS_TEST();
    auto directory         = MakeDirectory{}.create();
    const auto certificate = ecf::Host().name() + '.' + someport + ".crt";
    auto specific          = MakeTestFile{}.with(SpecificFileLocation{certificate, directory}).create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string() + "/");

    ClientInvoker client(true, "localhost", someport);
    client.enable_ssl_prefer_specific();
    BOOST_CHECK_EQUAL(client.get_certificate(), certificate);
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_shared_and_options_none_and_certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockClientInvoker client("ecflow_client -d --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_specific_and_options_none_and_certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    MockClientInvoker client("ecflow_client -d --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);

    // Note:
    //   The actual value of ECF_SSL is not used find the specific certificate.
    //   Instead, the host name is resolved by the OS, and the selected port is used.
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_shared_and_options_none_and_certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});

    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockClientInvoker client("ecflow_client -d --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_shared_and_options_none_and_certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockClientInvoker client("ecflow_client -d --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_shared_and_options_ssl_and_certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockClientInvoker client("ecflow_client -d --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_none_and_options_ssl_and_certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");

    // Note:
    //   When only using the command line option, if both shared and specific certificates are available,
    //   there is no way to select the specific certificate.
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_none_and_options_ssl_and_certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_none_and_options_ssl_and_certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_none_and_options_host_ssl_and_certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_none_and_options_host_ssl_and_certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_request_none_and_options_host_ssl_and_certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_host_request_none_and_options_host_ssl_and_certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overridden");
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_env_host_request_none_and_options_host_ssl_and_certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overridden");
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(
    is_able_to_handle_env_host_request_none_and_options_host_ssl_and_certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overridden");
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ssl --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_environment_without_ssl) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overridden");
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(!env.ssl());
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_environment_using_host_port_and_hostfile_with_no_env_vars) {
    auto content = fs::path{"test_hostsfile.txt"};
    WithTestFile hostsfile_content(NamedTestFile{content.c_str()}, R"(
# Some comment
host1    # Another comment
  host2:5678
)");
    WithTestEnvironmentVariable ecf_hostfile("ECF_HOSTFILE", content.string());

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --port " + someport + " --ping");
    ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.host_file(), content);
    BOOST_CHECK(env.host_file_policy_is_task());
    BOOST_CHECK(!env.host_file_policy_is_all());

    {
        std::string error;
        auto next = env.get_next_host(error);
        BOOST_CHECK(next);

        BOOST_CHECK_EQUAL(env.host(), "host1");
        BOOST_CHECK_EQUAL(env.port(), someport);
        // Surprise!
        //
        // I was expecting ecf::Str::DEFAULT_PORT_NUMBER() here, but no...
        // Because ECF_PORT / --port are used to define a port, that port is then used
        // by default when no port is defined in the hostfile.
        //
    }

    {
        std::string error;
        auto next = env.get_next_host(error);
        BOOST_CHECK(next);

        BOOST_CHECK_EQUAL(env.host(), "host2");
        BOOST_CHECK_EQUAL(env.port(), "5678");
    }
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_environment_using_host_port_and_hostfile_with_overriden_host_port_env_vars) {
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overridden");
    WithTestEnvironmentVariable ecf_port("ECF_PORT", "12345");

    auto content = fs::path{"test_hostsfile.txt"};
    WithTestFile hostsfile_content(NamedTestFile{content.c_str()}, R"(
# Some comment
host1    # Another comment
  host2:5678
)");
    WithTestEnvironmentVariable ecf_hostfile("ECF_HOSTFILE", content.string());

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --port " + someport + " --ping");
    ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.host_file(), content);
    BOOST_CHECK(env.host_file_policy_is_task());
    BOOST_CHECK(!env.host_file_policy_is_all());

    {
        std::string error;
        auto next = env.get_next_host(error);
        BOOST_CHECK(next);

        BOOST_CHECK_EQUAL(env.host(), "host1");
        BOOST_CHECK_EQUAL(env.port(), someport);
        // Surprise!
        //
        // I was expecting ecf::Str::DEFAULT_PORT_NUMBER() here, but no...
        // Because ECF_PORT / --port are used to define a port, that port is then used
        // by default when no port is defined in the hostfile.
        //
    }

    {
        std::string error;
        auto next = env.get_next_host(error);
        BOOST_CHECK(next);

        BOOST_CHECK_EQUAL(env.host(), "host2");
        BOOST_CHECK_EQUAL(env.port(), "5678");
    }
}

BOOST_AUTO_TEST_CASE(is_able_to_handle_environment_using_host_port_and_hostfile_with_non_overriden_host_port_env_vars) {
    WithTestEnvironmentVariable ecf_host("ECF_HOST", somehost);
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    auto content = fs::path{"test_hostsfile.txt"};
    WithTestFile hostsfile_content(NamedTestFile{content.c_str()}, R"(
# Some comment
host1    # Another comment
  host2:5678
)");
    WithTestEnvironmentVariable ecf_hostfile("ECF_HOSTFILE", content.string());

    MockClientInvoker client("ecflow_client -d --ping");
    ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.host_file(), content);
    BOOST_CHECK(env.host_file_policy_is_task());
    BOOST_CHECK(!env.host_file_policy_is_all());

    {
        std::string error;
        auto next = env.get_next_host(error);
        BOOST_CHECK(next);

        BOOST_CHECK_EQUAL(env.host(), "host1");
        BOOST_CHECK_EQUAL(env.port(), someport);
        // Surprise!
        //
        // I was expecting ecf::Str::DEFAULT_PORT_NUMBER() here, but no...
        // Because ECF_PORT / --port are used to define a port, that port is then used
        // by default when no port is defined in the hostfile.
        //
    }

    {
        std::string error;
        auto next = env.get_next_host(error);
        BOOST_CHECK(next);

        BOOST_CHECK_EQUAL(env.host(), "host2");
        BOOST_CHECK_EQUAL(env.port(), "5678");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#endif
