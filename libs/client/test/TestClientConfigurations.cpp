// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

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
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string() + "/");
    WithTestEnvironmentVariable ssl_env("ECF_SSL", "X");

    ClientInvoker client(true, somehost, someport);
    client.enable_ssl_prefer_specific();
    BOOST_CHECK(client.environment().ssl());
    BOOST_CHECK_EQUAL(client.get_certificate(), "server.crt");
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
    auto directory = MakeDirectory{}.create();
    WithTestEnvironmentVariable ssl_dir("ECF_SSL_DIR", directory.path().string() + "/");

    ClientInvoker client(true, somehost, someport);
    BOOST_CHECK_THROW(client.enable_ssl_prefer_specific(), std::runtime_error);
    BOOST_CHECK(!client.environment().ssl());
    BOOST_CHECK_EQUAL(client.get_certificate(), "");
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
