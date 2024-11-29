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

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Converter.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Defs)

BOOST_AUTO_TEST_CASE(test_defs_absorb) {
    ECF_NAME_THIS_TEST();

    // Create a defs file corresponding to:
    // suite suite1
    //  family family
    //   	task t1
    //  endfamily
    // endsuite
    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("suite1");
        family_ptr fam  = suite->add_family("family");
        fam->add_task("t1");
    }
    // Then ABSORB a new def
    // suite suite1-n
    //   family family
    //   		task suite1_task1
    //   endfamily
    // endsuite
    Defs otherDefs;
    {
        for (int i = 0; i < 14; ++i) {
            suite_ptr suite1 = otherDefs.add_suite("suite" + ecf::convert_to<std::string>(i));
            family_ptr fam   = suite1->add_family("family");
            fam->add_task("suite1_task1");
        }
    }

    theDefs.absorb(&otherDefs, true);
    BOOST_CHECK_MESSAGE(otherDefs.suiteVec().empty(), "absorb failed");
}

BOOST_AUTO_TEST_CASE(test_defs_absorb_server_user_variables) {
    ECF_NAME_THIS_TEST();

    Defs theDefs;
    Defs otherDefs;
    {
        otherDefs.set_server().add_or_update_user_variables("VAR1", "VAL");
        otherDefs.set_server().add_or_update_user_variables("VAR2", "VAL");
        otherDefs.set_server().add_or_update_user_variables("VAR3", "VAL");
        otherDefs.add_suite("suite");
    }

    BOOST_CHECK_MESSAGE(theDefs.server().user_variables().empty(), "Expected no server user variables");

    theDefs.absorb(&otherDefs, true);

    BOOST_CHECK_MESSAGE(otherDefs.suiteVec().empty(), "absorb failed");
    BOOST_CHECK_MESSAGE(theDefs.suiteVec().size() == 1, "absorb failed");
    BOOST_CHECK_MESSAGE(theDefs.server().user_variables().size() == 3, "Expected 3 server user variables");
}

BOOST_AUTO_TEST_CASE(test_ECFLOW_1684) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    defs.add_edit_history("/", "request");
    defs.add_edit_history("/a/made/up/path1", "request");
    defs.add_edit_history("/a/made/up/path1", "request2");
    defs.add_edit_history("/a/made/up/path2", "request2");
    defs.add_edit_history("/a/made/up/path3", "request2");

    BOOST_CHECK_MESSAGE(defs.get_edit_history().size() == 4,
                        "Expected edit history of size 4 but found " << defs.get_edit_history().size());

    // This should remove all edit history, where the history paths do not exist as nodes in a defs, BUT need to leave
    // ROOT as is though.
    defs.handle_migration();
    BOOST_CHECK_MESSAGE(defs.get_edit_history().size() == 1,
                        "Expected edit history of size 1 but found " << defs.get_edit_history().size());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
