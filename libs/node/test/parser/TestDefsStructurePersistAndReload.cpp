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

#include <boost/test/unit_test.hpp>

#include "MyDefsFixture.hpp"
#include "PersistHelper.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/NodeAlgorithms.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_DefsStructurePersisteAndReload)

//=============================================================================
// This test case will save the defs file in old style format
// and the parse it back in. As we add different types to our defs fixture
// we can automatically check that what we save can be parsed back in.
// Specifically written to test the parser.
// Note: Aliases are *NOT* written in the defs file BUT are when in MIGRATE
BOOST_AUTO_TEST_CASE(test_defs_structure_persistence_and_reload) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture theDefsFixture;
    PersistHelper helper;
    BOOST_CHECK_MESSAGE(helper.test_cereal_checkpt_and_reload(theDefsFixture.defsfile_), helper.errorMsg());

    // Note: Aliases are *NOT* written in PrintStyle::DEFS file
    // Hence in order for this test to pass, we must delete the alias first & reset task alias_no
    auto aliases = ecf::get_all_aliases(theDefsFixture.defsfile_);
    for (auto alias : aliases) {
        alias->parent()->isTask()->reset_alias_number();
        alias->remove();
    }
    BOOST_CHECK_MESSAGE(helper.test_persist_and_reload(theDefsFixture.defsfile_, PrintStyle::DEFS), helper.errorMsg());
}

BOOST_AUTO_TEST_CASE(test_defs_checkpt_persistence_and_reload) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture theDefsFixture;
    PersistHelper helper;
    BOOST_CHECK_MESSAGE(helper.test_cereal_checkpt_and_reload(theDefsFixture.defsfile_), helper.errorMsg());
    BOOST_CHECK_MESSAGE(helper.test_defs_checkpt_and_reload(theDefsFixture.defsfile_), helper.errorMsg());
}

// This test is used to find a task given a path of the form:
//   suite/family/task
//    suite/family/family/task
//
void test_find_task_using_path(NodeContainer* f, const Defs& defs) {
    BOOST_CHECK_MESSAGE(f == defs.findAbsNode(f->absNodePath()).get(),
                        "Could not find path " << f->absNodePath() << "\n");

    for (auto node : f->children()) {
        BOOST_CHECK_MESSAGE(node.get() == defs.findAbsNode(node->absNodePath()).get(),
                            "Could not find path " << node->absNodePath() << "\n");
        Family* family = node->isFamily();
        if (family) {
            test_find_task_using_path(family, defs);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_find_task_using_paths) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture theDefsFixture;

    const auto& suites = theDefsFixture.defsfile_.suites();
    for (suite_ptr suite : suites) {
        test_find_task_using_path(suite.get(), theDefsFixture.defsfile_);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
