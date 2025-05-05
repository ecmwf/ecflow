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

#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_VariableParsing)

BOOST_AUTO_TEST_CASE(test_single_defs) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_defs/edit/edit.def", "parser");

    Defs defs;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(defs.restore(path, errorMsg, warningMsg), errorMsg);

    // suite edit
    //     edit ECF_INCLUDE /home/ma/map/sms/example/x                  # comment line
    //     edit ECF_FILES   /home/ma/map/sms/example/x                  #comment line
    //     edit EXPVER 'f8na'                                          #
    //     edit USER 'ecgems'                                          #comment
    //     edit USER2 "ecgems"                                         # comment
    //     edit INT1 "10"                                             # comment
    //     edit INT2 '11'                                             # comment
    //     edit YMD  '20091012'                                        # comment
    //     family family
    //         edit var  "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%"  # comment line
    //         edit var2 'smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%'  #comment line
    //         task t2
    //     endfamily
    // endsuite
    suite_ptr editSuite = defs.findSuite("edit");
    BOOST_REQUIRE_MESSAGE(editSuite, "Could not find the edit suite");

    const Variable& int1 = editSuite->findVariable("INT1");
    BOOST_REQUIRE_MESSAGE(!int1.empty(), "Could not find variable INT1");
    BOOST_REQUIRE_MESSAGE(int1.value() == 10, "Expected INT1 to have a value of 10, but found " << int1.value());

    const Variable& int2 = editSuite->findVariable("INT2");
    BOOST_REQUIRE_MESSAGE(!int2.empty(), "Could not find variable INT2");
    BOOST_REQUIRE_MESSAGE(int2.value() == 11, "Expected INT2 to have a value of 11, but found " << int2.value());

    const Variable& ymd = editSuite->findVariable("YMD");
    BOOST_REQUIRE_MESSAGE(!ymd.empty(), "Could not find variable YMD");
    BOOST_REQUIRE_MESSAGE(ymd.value() == 20091012,
                          "Expected YMD to have a value of 20091012, but found " << ymd.value());

    const Variable& user = editSuite->findVariable("USER");
    BOOST_REQUIRE_MESSAGE(!user.empty(), "Could not find variable USER");
    BOOST_REQUIRE_MESSAGE(user.value() == 0, "Expected user to have a value of 0, but found " << user.value());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
