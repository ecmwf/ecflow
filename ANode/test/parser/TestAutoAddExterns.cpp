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

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(ParserTestSuite)

// Test that automatic add of externs
BOOST_AUTO_TEST_CASE(test_auto_add_externs) {
    std::string path = File::test_data("ANode/test/parser/data/single_defs/test_auto_add_extern.def", "parser");

    size_t mega_file_size = fs::file_size(path);
    cout << "AParser:: ...test_auto_add_externs " << path << " file_size(" << mega_file_size << ")\n";

    Defs defs;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(defs.restore(path, errorMsg, warningMsg), "Expected no errors, but found " << errorMsg);
    BOOST_REQUIRE_MESSAGE(warningMsg.empty(), "Expected no warnings but found:\n" << warningMsg);

    // Check number of extrens read in: Duplicate should be ignore
    BOOST_REQUIRE_MESSAGE(defs.externs().size() == 11,
                          "Expected 11 externs as starting point but found " << defs.externs().size() << "\n"
                                                                             << defs << "\n");

    // Test auto extern generation. Don't remove existing extern's
    defs.auto_add_externs(false /* remove_existing_externs_first*/);
    BOOST_REQUIRE_MESSAGE(defs.externs().size() == 11,
                          "Expected 11, auto_add_extern(false) gave: " << defs.externs().size() << "\n"
                                                                       << defs << "\n");

    // By removing the externs read, in we can determine the real number of extern;s from
    // parsing all the trigger expressions, and inlimit references
    defs.auto_add_externs(true /* remove_existing_externs_first*/);
    BOOST_REQUIRE_MESSAGE(defs.externs().size() == 10,
                          "Expected 10 externs, since redundant externs removed, auto_add_extern(true) gave: "
                              << defs.externs().size() << "\n"
                              << defs << "\n");
}

BOOST_AUTO_TEST_SUITE_END()
