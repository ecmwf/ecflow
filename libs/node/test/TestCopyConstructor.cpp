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

#include "MyDefsFixture.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_CopyConstructor)

BOOST_AUTO_TEST_CASE(test_copy_constructors) {
    ECF_NAME_THIS_TEST();
    MyDefsFixture theDefsFixture;

    DebugEquality debug_equality; // only as affect in DEBUG build
    Defs copy = Defs(theDefsFixture.defsfile_);
    BOOST_CHECK_MESSAGE(copy == theDefsFixture.defsfile_, "copy constructor failed");
}

BOOST_AUTO_TEST_CASE(test_default_constructors) {
    ECF_NAME_THIS_TEST();

    Task t;
    BOOST_CHECK_MESSAGE(t.check_defaults(), "constructor test for defaults failed");

    Task t2("fred");
    BOOST_CHECK_MESSAGE(t2.check_defaults(), "constructor test for defaults failed");

    Suite s;
    BOOST_CHECK_MESSAGE(s.check_defaults(), "constructor test for defaults failed");

    Suite s2("fred");
    BOOST_CHECK_MESSAGE(s2.check_defaults(), "constructor test for defaults failed");

    Family f;
    BOOST_CHECK_MESSAGE(f.check_defaults(), "constructor test for defaults failed");

    Family f2("fred");
    BOOST_CHECK_MESSAGE(f2.check_defaults(), "constructor test for defaults failed");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
