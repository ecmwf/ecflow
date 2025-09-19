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

#include "MyDefsFixture.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_FIXTURE_TEST_SUITE(U_Node, MyDefsFixture)

BOOST_AUTO_TEST_SUITE(T_Persistence)

// Allow for multiple archives
static void testPersistence(const Defs& fixtureDefs) {
    std::string check_pt_file = "fixture_defs.check";
    fixtureDefs.cereal_save_as_checkpt(check_pt_file);
    std::string error_msg;
    BOOST_CHECK_MESSAGE(fixtureDefs.checkInvariants(error_msg), error_msg);

    Defs restoredDefs;
    restoredDefs.cereal_restore_from_checkpt(check_pt_file);
    error_msg.clear();
    BOOST_CHECK_MESSAGE(restoredDefs.checkInvariants(error_msg), error_msg);

    bool theyCompare = (restoredDefs == fixtureDefs);
    if (!theyCompare) {

        std::cout << "Dump restored defs\n" << ecf::as_string(restoredDefs, PrintStyle::DEFS) << "\n";
        std::cout << "Dump fixture defs\n" << ecf::as_string(fixtureDefs, PrintStyle::DEFS) << "\n";

        BOOST_CHECK_MESSAGE(theyCompare, "restored defs file is not same as fixtureDefs defs file");
    }

    cout << " check pt file_size: " << fs::file_size(check_pt_file) << "\n";

    // Uncomment if you want see what this file looks like
    fs::remove(check_pt_file);
}

BOOST_AUTO_TEST_CASE(test_node_tree_persistence_text) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_MESSAGE(true, ""); // stop boost complaining about no assertions
    testPersistence(fixtureDefsFile());
}

BOOST_AUTO_TEST_CASE(test_node_defs_persistence) {
    ECF_NAME_THIS_TEST();

    const Defs& defs = fixtureDefsFile();
    auto nodes       = ecf::get_all_nodes(defs);
    BOOST_REQUIRE_MESSAGE(nodes.size() > 0, "Expected nodes");
    for (auto& node : nodes) {
        std::string node_as_defs_string = ecf::as_string(node, PrintStyle::MIGRATE);
        node_ptr the_copy               = Node::create(node_as_defs_string);
        BOOST_REQUIRE_MESSAGE(the_copy,
                              "Failed to create node " << node->absNodePath() << " from string:\n"
                                                       << node_as_defs_string);
        BOOST_REQUIRE_MESSAGE(*the_copy == *node, "Nodes not the same");
    }
    for (auto& node : nodes) {
        std::string node_as_defs_string = ecf::as_string(node, PrintStyle::NET);
        node_ptr the_copy               = Node::create(node_as_defs_string);
        BOOST_REQUIRE_MESSAGE(the_copy,
                              "Failed to create node " << node->absNodePath() << " from string:\n"
                                                       << node_as_defs_string);
        BOOST_REQUIRE_MESSAGE(*the_copy == *node, "Nodes not the same");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
