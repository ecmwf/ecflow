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

#include "scaffold/Naming.hpp"
#include "scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_NodeExtern)

BOOST_AUTO_TEST_CASE(test_e2e_use_extern_to_local_node_in_partial_suite) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the declaration of extern 'local' nodes,
     * and ensure the correct handling of these extern objects belonging to partial suite definitions.
     *
     *
     * Requirements
     *
     * - ecFlow allows to replace a family from a partial suite definition, with triggers to an extern local node.
     *
     */

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto full_defs = MakeTestFile{}
                         .with(SpecificFileLocation{"suite.full.def", cwd})
                         .with(R"--(
suite user
  family experiment1
    task task
  endfamily
  family experiment2
    task task
  endfamily
endsuite;
)--"

                               )
                         .create();

    { // load the full suite definition
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{full_defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    auto partial_defs = MakeTestFile{}
                            .with(SpecificFileLocation{"suite.partial.def", cwd})
                            .with(R"--(
extern /user/experiment1/task
suite user
  family experiment2
    task task
      trigger /user/experiment1/task == complete
  endfamily
endsuite;
)--"

                                  )
                            .create();

    { // replace a specific family, from a partial suite definition using extern to refer to local node
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandReplace{partial_defs.path(), "/user/experiment2"});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_use_extern_to_local_attribute_in_partial_suite) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the declaration of extern attributes (e.g. variable) attached to 'local' nodes,
     * and ensure the correct handling of these extern objects belonging to partial suite definitions.
     *
     *
     * Requirements
     *
     * - ecFlow allows to replace a family from a partial suite definition, with triggers to an extern local node.
     *
     */

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto full_defs = MakeTestFile{}
                         .with(SpecificFileLocation{"suite.full.def", cwd})
                         .with(R"--(
suite user
  family experiment1
    task task
      edit VARIABLE 3145
  endfamily
  family experiment2
    task task
  endfamily
endsuite;
)--"

                               )
                         .create();

    { // load the full suite definition
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{full_defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    auto partial_defs = MakeTestFile{}
                            .with(SpecificFileLocation{"suite.partial.def", cwd})
                            .with(R"--(
extern /user/experiment1/task:VARIABLE
suite user
  family experiment2
    task task
      trigger /user/experiment1/task:VARIABLE == 3145
  endfamily
endsuite;
)--"

                                  )
                            .create();

    { // replace a specific family, from a partial suite definition using extern to refer to local node
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(
            RunClient::CommandReplace{partial_defs.path(), "/user/experiment2"});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
