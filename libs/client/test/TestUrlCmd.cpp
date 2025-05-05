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
#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/client/UrlCmd.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_UrlCmd)

//=============================================================================
// This will test the UrlCmd.
BOOST_AUTO_TEST_CASE(test_url_cmd) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/client/test/data/lifecycle.txt", "libs/client");

    defs_ptr defs = Defs::create();

    std::string errorMsg, warningMsg;
    bool parse = defs->restore(path, errorMsg, warningMsg);
    BOOST_CHECK_MESSAGE(parse, errorMsg);

    // Check error conditions
    BOOST_REQUIRE_THROW(UrlCmd(defs, "a made up name"), std::runtime_error);
    BOOST_REQUIRE_THROW(UrlCmd(defs_ptr(), "/suite1/family1/a"), std::runtime_error);

    // The Url command relies on variable substitution, hence we must ensure that
    // generated variables are created.
    defs->beginAll();

    UrlCmd urlCmd(defs, "/suite1/family1/a");
    std::string expected = "${BROWSER:=firefox} -new-tab https://confluence.ecmwf.int/display/ECFLOW/ecflow+home";
    std::string actual   = urlCmd.getUrl();
    BOOST_CHECK_MESSAGE(expected == actual, "Expected '" << expected << "' but found " << actual);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
