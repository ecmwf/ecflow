/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Host.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

struct MockClientInvoker
{
    explicit MockClientInvoker(const std::string& commandline) : client_() {
        client_.set_cli(true);
        // Process the command line arguments to trigger the environment initialization
        auto b = client_.get_cmd_from_args(CommandLine(commandline));
    }

    const ClientEnvironment& environment() const { return client_.environment(); };

private:
    ClientInvoker client_;
};

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientConfiguration)

/*
 * The following exports an environment variable used for tests, which changes the location of the SSL certificates.
 * Instead of the default location (HOME/.ecflowrc/ssl), we use the current test directory.
 */
WithTestEnvironmentVariable ecf_ssl_dir("ECF_SSL_DIR", "./");

std::string somehost = "somehost";
std::string someport = "31415";
std::string someuser = "someuser";
std::string somepass = "somepass";

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_none__certificates_shared_and_specific) {
    WithTestFile shared_crt("server.crt");
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_specific__options_none__certificates_shared_and_specific) {
    WithTestFile shared_crt("server.crt");
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_none__certificates_specific_only) {
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");

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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_none__certificates_shared_only) {
    WithTestFile shared_crt("server.crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_ssl__certificates_shared_and_specific) {
    WithTestFile shared_crt("server.crt");
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_ssl__certificates_shared_and_specific) {
    WithTestFile shared_crt("server.crt");
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_ssl__certificates_specific_only) {
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_ssl__certificates_shared_only) {
    WithTestFile shared_crt("server.crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_host_ssl__certificates_shared_and_specific) {
    WithTestFile shared_crt("server.crt");
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_host_ssl__certificates_specific_only) {
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_host_ssl__certificates_shared_only) {
    WithTestFile shared_crt("server.crt");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_host_request_none__options_host_ssl__certificates_shared_only) {
    WithTestFile shared_crt("server.crt");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overriden");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_host_request_none__options_host_ssl__certificates_specific_only) {
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overriden");
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

BOOST_AUTO_TEST_CASE(can_setup_environment__env_host_request_none__options_host_ssl__certificates_shared_and_specific) {
    WithTestFile shared_crt("server.crt");
    WithTestFile specific_crt(somehost + '.' + someport + ".crt");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overriden");
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

BOOST_AUTO_TEST_CASE(can_setup_environment_without_ssl) {
    WithTestFile shared_crt("server.crt");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "to_be_overriden");
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_user("ECF_USER", someuser);

    MockClientInvoker client("ecflow_client -d --host " + somehost + " --ping");
    const ClientEnvironment& env = client.environment();

    BOOST_CHECK_EQUAL(env.host(), somehost);
    BOOST_CHECK_EQUAL(env.port(), someport);
    BOOST_CHECK_EQUAL(env.get_user_name(), someuser);
    BOOST_CHECK(!env.ssl());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
