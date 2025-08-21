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
#include "ecflow/server/ServerEnvironment.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

class MockServerInvoker {
public:
    explicit MockServerInvoker(const std::string& commandline) : env_(CommandLine(commandline)) {}

    const ServerEnvironment& environment() const { return env_; };

private:
    ServerEnvironment env_;
};

BOOST_AUTO_TEST_SUITE(U_Server)

/*
 * The following exports an environment variable used for tests, which changes the location of the SSL certificates.
 * Instead of the default location (HOME/.ecflowrc/ssl), we use the current test directory.
 */
WithTestEnvironmentVariable ecf_ssl_dir("ECF_SSL_DIR", "./");

std::string somehost = ecf::Host().name();
std::string someport = "31415";
std::string someuser = "someuser";
std::string somepass = "somepass";

BOOST_AUTO_TEST_SUITE(T_ServerConfiguration)

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_none__certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_specific__options_none__certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);

    // Note:
    //   The actual value of ECF_SSL is not used find the specific certificate.
    //   Instead, the host name is resolved by the OS, and the selected port is used.
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_none__certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_specific__options_none__certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    BOOST_CHECK_THROW(MockServerInvoker server("ecflow_server -d"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_shared__options_none__certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", "1");

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);

    // Note:
    //   Even though ECF_SSL specifies the use of shared certificate, the specific certificate is selected
    //   since it is the only one kind available.
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_specific__options_none__certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);
    WithTestEnvironmentVariable ecf_ssl("ECF_SSL", somehost + '.' + someport);

    MockServerInvoker server("ecflow_server -d");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_ssl__certificates_shared_and_specific) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    MockServerInvoker server("ecflow_server -d --ssl");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");

    // Note:
    //   When only using the command line option, if both shared and specific certificates are available,
    //   there is no way to select the specific certificate.
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_ssl__certificates_shared_only) {
    WithTestFile shared_crt(NamedTestFile{"server.crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    MockServerInvoker server("ecflow_server -d --ssl");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), "1");
}

BOOST_AUTO_TEST_CASE(can_setup_environment__env_request_none__options_ssl__certificates_specific_only) {
    WithTestFile specific_crt(NamedTestFile{somehost + '.' + someport + ".crt"});
    WithTestEnvironmentVariable ecf_port("ECF_PORT", someport);

    MockServerInvoker server("ecflow_server -d --ssl");
    const ServerEnvironment& env = server.environment();

    BOOST_CHECK_EQUAL(std::to_string(env.port()), someport);
    BOOST_CHECK(env.ssl());
    BOOST_CHECK_EQUAL(env.openssl().ssl(), somehost + '.' + someport);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
