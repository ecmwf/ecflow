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

#include "ecflow/base/cts/task/CompleteCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientRetry)

const ClientToServerCmd& ping = CtsCmd(CtsCmd::Api::PING);
const ClientToServerCmd& task = CompleteCmd("/path/to/task", "password", "rid", 1);
const ClientToServerCmd& user = CtsCmd(CtsCmd::Api::RESTART_SERVER);

BOOST_AUTO_TEST_CASE(test_client_does_not_retry_ping_command) {
    ECF_NAME_THIS_TEST();

    ClientInvoker client;
    BOOST_CHECK(client.is_not_retrying(ping));
}

BOOST_AUTO_TEST_CASE(test_client_does_not_retry_user_command_by_default) {
    ECF_NAME_THIS_TEST();

    ClientInvoker client;
    BOOST_CHECK(client.is_not_retrying(user));
}

BOOST_AUTO_TEST_CASE(test_client_does_not_retry_when_denied_is_set) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable env_var(ecf::environment::ECF_DENIED, "true");
    ClientInvoker client;
    BOOST_CHECK(client.is_not_retrying(ping));
    BOOST_CHECK(client.is_not_retrying(task));
    BOOST_CHECK(client.is_not_retrying(user));
}

BOOST_AUTO_TEST_CASE(test_client_does_not_retry_when_testing_is_set) {
    ECF_NAME_THIS_TEST();

    ClientInvoker client;
    client.taskPath("/path/to/task"); // This effectively sets the testing mode
    BOOST_CHECK(client.is_not_retrying(ping));
    BOOST_CHECK(client.is_not_retrying(task));
    BOOST_CHECK(client.is_not_retrying(user));
}

BOOST_AUTO_TEST_CASE(test_client_retries_task_command_by_default) {
    ECF_NAME_THIS_TEST();

    ClientInvoker client;
    BOOST_CHECK(!client.is_not_retrying(task));
}

BOOST_AUTO_TEST_CASE(test_client_follows_host_file_policy) {
    ECF_NAME_THIS_TEST();

    {
        WithTestEnvironmentVariable env_var(ecf::environment::ECF_HOSTFILE_POLICY, "task");

        ClientInvoker client;
        BOOST_CHECK(client.is_not_retrying(ping));
        BOOST_CHECK(client.is_not_retrying(user));
        BOOST_CHECK(!client.is_not_retrying(task));
    }

    {
        WithTestEnvironmentVariable env_var(ecf::environment::ECF_HOSTFILE_POLICY, "all");

        ClientInvoker client;
        BOOST_CHECK(!client.is_not_retrying(ping));
        BOOST_CHECK(!client.is_not_retrying(user));
        BOOST_CHECK(!client.is_not_retrying(task));
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
