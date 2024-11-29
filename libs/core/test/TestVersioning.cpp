/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "TestVersioning.hpp"

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace boost;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Versioning)

// This class will create a Class X. This is serialised.
// This class is then reloaded, with different kinds of version changes, to base version 0;
// Note: we simulate different release of class X, by using name spaces
//       This is possible since the name space is not written.
BOOST_AUTO_TEST_CASE(test_versioning) {
    ECF_NAME_THIS_TEST();

    {
        // write out version 0; This will be reloaded with different version of X
        const version0::X t = version0::X(10);
        ecf::save("version0", t);
    }

    {
        // Version 1 adds a new data member: i.e min_
        version_new_data_member::X t;
        ecf::restore("version0", t);
        BOOST_CHECK_MESSAGE(t == version_new_data_member::X(10, 0), "Should be the same");
    }

    {
        // Version 1 change data member name: from hour_ -> hours_:
        // This required no change at all. Since serialisation relies on order.
        version_change_dm_name::X t;
        ecf::restore("version0", t);
        BOOST_CHECK_MESSAGE(t == version_change_dm_name::X(10), "Should be the same");
    }

    {
        // Version 1 change data member type: from int -> string :
        version_change_dm_type::X t;
        ecf::restore("version0", t);
        BOOST_CHECK_MESSAGE(t == version_change_dm_type::X("10"),
                            "Reading integer as string expected '10', but found string " << t.str());
    }

    // remove the generate file
    fs::remove("version0");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
