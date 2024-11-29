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
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
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
    std::vector<alias_ptr> alias_vec;
    theDefsFixture.defsfile_.get_all_aliases(alias_vec);
    for (alias_ptr al : alias_vec) {
        al->parent()->isTask()->reset_alias_number();
        al->remove();
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
// 	  suite/family/task
//    suite/family/family/task
//
void test_find_task_using_path(NodeContainer* f, const Defs& defs) {
    BOOST_CHECK_MESSAGE(f == defs.findAbsNode(f->absNodePath()).get(),
                        "Could not find path " << f->absNodePath() << "\n");

    for (node_ptr t : f->nodeVec()) {
        BOOST_CHECK_MESSAGE(t.get() == defs.findAbsNode(t->absNodePath()).get(),
                            "Could not find path " << t->absNodePath() << "\n");
        Family* family = t->isFamily();
        if (family) {
            test_find_task_using_path(family, defs);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_find_task_using_paths) {
    ECF_NAME_THIS_TEST();

    MyDefsFixture theDefsFixture;

    const std::vector<suite_ptr>& suiteVec = theDefsFixture.defsfile_.suiteVec();
    for (suite_ptr s : suiteVec) {
        test_find_task_using_path(s.get(), theDefsFixture.defsfile_);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
