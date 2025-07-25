/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientEnvironment)

BOOST_AUTO_TEST_CASE(test_client_environment_host_file_parsing) {
    ECF_NAME_THIS_TEST();

    std::string good_host_file = File::test_data("libs/client/test/data/good_hostfile", "libs/client");

    // local host should be implicitly added to internal host list
    std::string the_host = ClientEnvironment::hostSpecified();
    if (the_host.empty())
        the_host = Str::LOCALHOST();

    std::vector<std::string> expectedHost;
    expectedHost.push_back(the_host);
    expectedHost.emplace_back("host1");
    expectedHost.emplace_back("host2");
    expectedHost.emplace_back("host3");
    expectedHost.emplace_back("host4");
    expectedHost.emplace_back("host5");
    expectedHost.emplace_back("host6");

    ClientEnvironment client_env(good_host_file);
    std::string home_host = client_env.host();
    std::string host;
    size_t count = 0;
    BOOST_CHECK_MESSAGE(home_host == expectedHost[count],
                        "Expected home host " << expectedHost[count] << " but found " << home_host);

    bool home_host_fnd = false;
    while (home_host != host) {
        // Cycle through the host until we reach the home host
        std::string errorMsg;
        BOOST_CHECK_MESSAGE(client_env.get_next_host(errorMsg), errorMsg);
        host = client_env.host();
        count++;
        if (host == home_host) {
            home_host_fnd = true;
            count         = 0;
        }
        BOOST_REQUIRE_MESSAGE(count < expectedHost.size(), "Test file out of date");
        BOOST_CHECK_MESSAGE(host == expectedHost[count],
                            "Expected host " << expectedHost[count] << " but found " << host);
    }
    BOOST_CHECK_MESSAGE(home_host_fnd, "Cycling through host file, should lead to home host");
}

BOOST_AUTO_TEST_CASE(test_client_environment_host_file_defaults) {
    ECF_NAME_THIS_TEST();

    // When the HOST file does *NOT* indicate the port, it should be taken
    // from the config/environment.
    // In file good_hostfile, host3 and host5 do not specify a port, hence
    // this port is assumed to be the job supplied port, that was read from
    // config or environment. To test this correctly we need to specify a port
    // other than the default

    std::string good_host_file = File::test_data("libs/client/test/data/good_hostfile", "libs/client");

    // local host should be implicitly added to internal host list
    std::vector<std::pair<std::string, std::string>> expectedHost;
    expectedHost.emplace_back(Str::LOCALHOST(), string("5111")); // here 5111 is job supplied port
    expectedHost.emplace_back(string("host1"), string("3142"));
    expectedHost.emplace_back(string("host2"), string("3141"));
    expectedHost.emplace_back(string("host3"), string("5111")); // not specified in host file, hence expect 5111
    expectedHost.emplace_back(string("host4"), string("4001"));
    expectedHost.emplace_back(string("host5"), string("5111")); // not specified in host file, hence expect 5111
    expectedHost.emplace_back(string("host6"), string("4081"));

    // Create the ClientEnvironment overriding the config & environment. To specify host and port
    ClientEnvironment client_env(good_host_file, Str::LOCALHOST(), "5111");
    std::string home_host = client_env.host();
    std::string home_port = client_env.port();
    BOOST_CHECK_MESSAGE(Str::LOCALHOST() == home_host && "5111" == home_port, "host host & port not as expected");

    std::string host;
    size_t count       = 0;
    bool home_host_fnd = false;
    while (home_host != host) {
        // Cycle through the host until we reach the home host
        std::string errorMsg;
        BOOST_CHECK_MESSAGE(client_env.get_next_host(errorMsg), errorMsg);
        host             = client_env.host();
        std::string port = client_env.port();
        count++;
        if (host == home_host) {
            BOOST_CHECK_MESSAGE(count - 1 == 6,
                                "Expected 6 hosts in host file"); // ignore last increment of count, hence -1
            home_host_fnd = true;
            count         = 0;
        }
        BOOST_REQUIRE_MESSAGE(count < expectedHost.size(), "Test file out of date");
        BOOST_CHECK_MESSAGE(host == expectedHost[count].first,
                            "Expected host " << expectedHost[count].first << " but found " << host);
        BOOST_CHECK_MESSAGE(port == expectedHost[count].second,
                            "Expected port " << expectedHost[count].second << " but found " << port);
    }
    BOOST_CHECK_MESSAGE(home_host_fnd, "Cycling through host file, should lead to home host");
}

BOOST_AUTO_TEST_CASE(test_client_environment_empty_host_file) {
    ECF_NAME_THIS_TEST();

    std::string empty_host_file = File::test_data("libs/client/test/data/empty_hostfile", "libs/client");

    std::string errormsg;
    BOOST_CHECK_MESSAGE(File::create(empty_host_file, "", errormsg), "Failed to create empty host file " << errormsg);

    ClientEnvironment client_env(empty_host_file);
    std::string errorMsg;
    BOOST_CHECK_MESSAGE(client_env.get_next_host(errorMsg), errorMsg);
    BOOST_CHECK_MESSAGE(client_env.get_next_host(errorMsg), errorMsg);

    fs::remove(empty_host_file);
}

BOOST_AUTO_TEST_CASE(test_client_environment_host_file_policy) {
    ECF_NAME_THIS_TEST();

    {
        // When no host file policy is specified...

        ClientEnvironment env(false);
        BOOST_CHECK(!env.host_file_policy_is_all());
        BOOST_CHECK(env.host_file_policy_is_task());
    }

    {
        WithTestEnvironmentVariable policy("ECF_HOSTFILE_POLICY", "");

        ClientEnvironment env(false);
        BOOST_CHECK(!env.host_file_policy_is_all());
        BOOST_CHECK(env.host_file_policy_is_task());
    }

    {
        WithTestEnvironmentVariable policy("ECF_HOSTFILE_POLICY", "task");

        ClientEnvironment env(false);
        BOOST_CHECK(!env.host_file_policy_is_all());
        BOOST_CHECK(env.host_file_policy_is_task());
    }

    {
        WithTestEnvironmentVariable policy("ECF_HOSTFILE_POLICY", "all");

        ClientEnvironment env(false);
        BOOST_CHECK(env.host_file_policy_is_all());
        BOOST_CHECK(!env.host_file_policy_is_task());
    }

    {
        WithTestEnvironmentVariable policy("ECF_HOSTFILE_POLICY", "anything");

        ClientEnvironment env(false);
        BOOST_CHECK(!env.host_file_policy_is_all());
        BOOST_CHECK(env.host_file_policy_is_task());
    }
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
