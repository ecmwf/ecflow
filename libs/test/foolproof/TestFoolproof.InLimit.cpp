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

#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_InLimit)

namespace {

// Suite definition used by tests that reference a limit in a dedicated family (/s/limits).
// The task under test lives at /s/f/t.
constexpr std::string_view k_suite_with_external_limit = R"--(
suite s
  family limits
    limit cpu 10
  endfamily
  family f
    task t
  endfamily
endsuite
)--";

// Suite definition for the test that references a limit local to the task's own family.
// Both the limit and the task live inside /s/f.
constexpr std::string_view k_suite_with_local_limit = R"--(
suite s
  family f
    limit cpu 10
    task t
  endfamily
endsuite
)--";

/// Assert that the server definition contains the expected node structure and that
/// no inlimit attribute is present on any node.
void check_initial_state(const ecf::test::scaffold::Host& host,
                         const ecf::test::scaffold::Port& port,
                         const ecf::test::scaffold::Directory& cwd) {
    using namespace ecf::test::scaffold;

    auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
    BOOST_REQUIRE(client.ok());
    const auto& c = client.value();

    ECF_TEST_DBG("Output of --get (initial state):\n" << c.stdout_buffer);

    BOOST_CHECK(c.stdout_contains("suite s"));
    BOOST_CHECK(c.stdout_contains("limit cpu 10"));
    BOOST_CHECK(c.stdout_contains("task t"));
    BOOST_CHECK(!c.stdout_contains("inlimit"));
}

} // namespace

BOOST_AUTO_TEST_CASE(test_alter_add_inlimit_normal) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd  = MakeDirectory{}.create();
    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    BOOST_REQUIRE(server.ok());

    auto defs =
        MakeTestFile{}.with(SpecificFileLocation{"suite.inlimits.def", cwd}).with(k_suite_with_external_limit).create();
    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --load:\n" << client.value().stdout_buffer);
    }
    check_initial_state(host, port, cwd);

    { // Add the inlimit: ecflow_client --alter add inlimit /s/limits:cpu "1" /s/f/t
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandAlterAddInlimit{"/s/limits:cpu", "1", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter add inlimit:\n" << client.value().stdout_buffer);
    }
    { // Verify the inlimit is present
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get:\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("inlimit /s/limits:cpu"));
    }
}

BOOST_AUTO_TEST_CASE(test_alter_add_inlimit_submission) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd  = MakeDirectory{}.create();
    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    BOOST_REQUIRE(server.ok());

    auto defs =
        MakeTestFile{}.with(SpecificFileLocation{"suite.inlimits.def", cwd}).with(k_suite_with_external_limit).create();
    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --load:\n" << client.value().stdout_buffer);
    }
    check_initial_state(host, port, cwd);

    { // Add the submission-limited inlimit: ecflow_client --alter add inlimit /s/limits:cpu "2 -s" /s/f/t
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandAlterAddInlimit{"/s/limits:cpu", "2 -s", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter add inlimit (submission):\n" << client.value().stdout_buffer);
    }
    { // Verify the submission-limited inlimit is present
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get:\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("inlimit -s /s/limits:cpu 2"));
    }
}

BOOST_AUTO_TEST_CASE(test_alter_add_inlimit_node_only) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd  = MakeDirectory{}.create();
    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    BOOST_REQUIRE(server.ok());

    auto defs =
        MakeTestFile{}.with(SpecificFileLocation{"suite.inlimits.def", cwd}).with(k_suite_with_external_limit).create();
    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --load:\n" << client.value().stdout_buffer);
    }
    check_initial_state(host, port, cwd);

    { // Add the node-only inlimit: ecflow_client --alter add inlimit /s/limits:cpu "2 -n" /s/f/t
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandAlterAddInlimit{"/s/limits:cpu", "2 -n", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter add inlimit (node only):\n" << client.value().stdout_buffer);
    }
    { // Verify the node-only inlimit is present
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get:\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("inlimit -n /s/limits:cpu 2"));
    }
}

BOOST_AUTO_TEST_CASE(test_alter_add_then_delete_inlimit) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd  = MakeDirectory{}.create();
    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    BOOST_REQUIRE(server.ok());

    auto defs =
        MakeTestFile{}.with(SpecificFileLocation{"suite.inlimits.def", cwd}).with(k_suite_with_external_limit).create();
    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --load:\n" << client.value().stdout_buffer);
    }
    check_initial_state(host, port, cwd);

    { // Add the inlimit: ecflow_client --alter add inlimit /s/limits:cpu "5" /s/f/t
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandAlterAddInlimit{"/s/limits:cpu", "5", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter add inlimit:\n" << client.value().stdout_buffer);
    }
    { // Verify the inlimit is present
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get (after add):\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("inlimit /s/limits:cpu 5"));
    }
    { // Remove the inlimit: ecflow_client --alter delete inlimit /s/f/t
        auto client =
            RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandAlterDeleteInlimit{"/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter delete inlimit:\n" << client.value().stdout_buffer);
    }
    { // Verify the inlimit has been removed
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get (after delete):\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("task t"));
        BOOST_CHECK(!c.stdout_contains("inlimit"));
    }
}

BOOST_AUTO_TEST_CASE(test_alter_add_then_delete_inlimit_without_path_to_associated_limit) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd  = MakeDirectory{}.create();
    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    BOOST_REQUIRE(server.ok());

    auto defs =
        MakeTestFile{}.with(SpecificFileLocation{"suite.inlimits.def", cwd}).with(k_suite_with_local_limit).create();
    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --load:\n" << client.value().stdout_buffer);
    }
    check_initial_state(host, port, cwd);

    { // Add the inlimit: ecflow_client --alter add inlimit cpu "-s 5" /s/f/t
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandAlterAddInlimit{"cpu", "-s 5", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter add inlimit:\n" << client.value().stdout_buffer);
    }
    { // Verify the inlimit is present
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get (after add):\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("inlimit -s cpu 5"));
    }
    { // Remove the inlimit: ecflow_client --alter delete inlimit /s/f/t
        auto client =
            RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandAlterDeleteInlimit{"/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter delete inlimit:\n" << client.value().stdout_buffer);
    }
    { // Verify the inlimit has been removed
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get (after delete):\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("task t"));
        BOOST_CHECK(!c.stdout_contains("inlimit"));
    }
}

BOOST_AUTO_TEST_CASE(test_alter_delete_inlimit_by_name_removes_inlimit) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto cwd  = MakeDirectory{}.create();
    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    BOOST_REQUIRE(server.ok());

    auto defs =
        MakeTestFile{}.with(SpecificFileLocation{"suite.inlimits.def", cwd}).with(k_suite_with_local_limit).create();
    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
    }
    check_initial_state(host, port, cwd);

    { // Add the inlimit (no path prefix — limit lives in the same family)
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandAlterAddInlimit{"cpu", "3", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter add inlimit:\n" << client.value().stdout_buffer);
    }
    { // Verify add worked
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        BOOST_REQUIRE(client.value().stdout_contains("inlimit cpu 3"));

        ECF_TEST_DBG("Output of --get (after add):\n" << client.value().stdout_buffer);
    }
    { // Delete by name: ecflow_client --alter delete inlimit cpu /s/f/t
        auto client =
            RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandAlterDeleteInlimit{"cpu", "/s/f/t"});
        BOOST_REQUIRE(client.ok());

        ECF_TEST_DBG("Output of --alter delete inlimit cpu:\n" << client.value().stdout_buffer);
    }
    { // Verify the inlimit has been removed
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        const auto& c = client.value();

        ECF_TEST_DBG("Output of --get (after delete):\n" << c.stdout_buffer);

        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("limit cpu 10"));
        BOOST_CHECK(c.stdout_contains("task t"));
        BOOST_CHECK(!c.stdout_contains("inlimit"));
    }
}

BOOST_AUTO_TEST_SUITE_END() // T_InLimit

BOOST_AUTO_TEST_SUITE_END() // S_Foolproof
