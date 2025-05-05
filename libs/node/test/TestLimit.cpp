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
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Limit)

BOOST_AUTO_TEST_CASE(test_limit_basics) {
    ECF_NAME_THIS_TEST();

    {
        Limit empty;
        Limit empty2;
        BOOST_CHECK_MESSAGE(empty == empty2, "Equality failed");

        Limit l1("name", 2);
        Limit l2("name", 2);
        BOOST_CHECK_MESSAGE(l1 == l2, "Equality failed");

        Limit l3("name", 2);
        Limit l4("name", 4);
        BOOST_CHECK_MESSAGE(!(l3 == l4), "Equality failed");
    }

    std::set<std::string> expected_empty_paths;
    std::set<std::string> expected_paths;

    Limit l1("name", 100);
    for (int i = 1; i < 10; i++) {
        std::string path = ecf::convert_to<std::string>(i);
        expected_paths.insert(path);
        l1.increment(1, path);
        BOOST_CHECK_MESSAGE(l1.paths() == expected_paths, "Expected paths not the same at " << i);
    }

    for (int i = 9; i >= 1; i--) {
        std::string path = ecf::convert_to<std::string>(i);
        l1.decrement(1, path);
        auto iter = expected_paths.find(path);
        expected_paths.erase(iter);

        if (l1.paths() != expected_paths) {
            std::cout << " Expected:";
            std::copy(expected_paths.begin(), expected_paths.end(), std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << "    Found:";
            std::copy(l1.paths().begin(), l1.paths().end(), std::ostream_iterator<std::string>(std::cout, " "));
        }
        BOOST_CHECK_MESSAGE(l1.paths() == expected_paths, "Expected paths not the same at " << i);
    }

    expected_paths.insert("/a/b/c");
    l1.increment(1, "/a/b/c");
    BOOST_CHECK_MESSAGE(l1.paths() == expected_paths, "Expected paths not the same");

    l1.decrement(1, "/a/b/c");
    BOOST_CHECK_MESSAGE(l1.paths() == expected_empty_paths, "Expected paths not the same");
    BOOST_CHECK_MESSAGE(l1.value() == 0, "Expected value to be 0");

    l1.increment(1, "/a/b/c");
    l1.increment(1, "/a/b/d");
    BOOST_CHECK_MESSAGE(l1.paths().size() == 2, "Expected 2 paths but found " << l1.paths().size());
    BOOST_CHECK_MESSAGE(l1.value() == 2, "Expected value to be 2 but found " << l1.value());
    l1.reset();
    BOOST_CHECK_MESSAGE(l1.value() == 0, "Expected value to be 0 after reset, but found " << l1.value());
    BOOST_CHECK_MESSAGE(l1.paths().size() == 0, "Expected 0 paths after reset but found " << l1.paths().size());

    l1.setValue(10);
    BOOST_CHECK_MESSAGE(l1.value() == 10, "Expected value to be 10, but found " << l1.value());
    BOOST_CHECK_MESSAGE(l1.paths().size() == 0, "Expected 0 paths after reset but found " << l1.paths().size());
}

BOOST_AUTO_TEST_CASE(test_limit_increment) {
    ECF_NAME_THIS_TEST();

    Limit limit("name", 10);    // Limit of 10
    limit.increment(1, "path"); // consume 1 token

    BOOST_CHECK_MESSAGE(limit.value() == 1,
                        "Expected increment to consume 1 token but it has consumed " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().size() == 1, "Expected 1 task path but found " << limit.paths().size());

    limit.increment(1, "path"); // Increment with same path, should *NOT* consume a token

    BOOST_CHECK_MESSAGE(limit.value() == 1, "Expected 1 token but it has consumed " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().size() == 1, "Expected 1 task path but found " << limit.paths().size());
}

BOOST_AUTO_TEST_CASE(test_limit_increment_2) {
    ECF_NAME_THIS_TEST();

    Limit limit("name", 10); // Limit of 10
    for (int i = 0; i < 20; i++) {
        // increment should keep increasing limit value, *EVEN* if over the limit. See ECFLOW-324
        std::string path = ecf::convert_to<std::string>(i);
        limit.increment(1, path);
        BOOST_CHECK_MESSAGE(limit.value() == i + 1,
                            "Expected limit value of " << i + 1 << " but found " << limit.value());
    }
}

BOOST_AUTO_TEST_CASE(test_limit_decrement) {
    ECF_NAME_THIS_TEST();

    Ecf::set_server(true); // needed to test state_change_numbers

    Limit limit("name", 10); // Limit of 10
    unsigned int expected_state_change_no = limit.state_change_no();

    limit.increment(1, "path");
    expected_state_change_no++; // consume 1 token, should increment state change no
    BOOST_CHECK_MESSAGE(limit.value() == 1, "Expected limit of value 1  but found " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().size() == 1, "Expected 1 task path but found " << limit.paths().size());
    BOOST_CHECK_MESSAGE(limit.state_change_no() == expected_state_change_no,
                        "Expected increment to increase state change no, expected "
                            << expected_state_change_no << " but found " << limit.state_change_no());

    // Since 'path_x' is NOT in the list of paths stored by the limit, there should be NO change to state_change_no
    limit.decrement(1, "path_x");
    BOOST_CHECK_MESSAGE(
        limit.value() == 1,
        " decrement of path that does not exist, should not affect the Limit: Expected limit of value 1  but found "
            << limit.value());
    BOOST_CHECK_MESSAGE(
        limit.paths().size() == 1,
        " decrement of path that does not exist, should not affect the Limit : Expected 1 task path but found "
            << limit.paths().size());
    BOOST_CHECK_MESSAGE(limit.state_change_no() == expected_state_change_no,
                        "Expected no change in state change no, expected " << expected_state_change_no << " but found "
                                                                           << limit.state_change_no());

    // Multiple decrements should leave Limit of value = 0, and not a negative number
    limit.decrement(1, "path");
    expected_state_change_no++;
    BOOST_CHECK_MESSAGE(limit.state_change_no() == expected_state_change_no,
                        "Expected decrement to increase state change no, expected "
                            << expected_state_change_no << " but found " << limit.state_change_no());

    // Since we have removed 'path' from the limit expect no further state changes
    limit.decrement(1, "path");
    limit.decrement(1, "path");
    limit.decrement(1, "path");
    BOOST_CHECK_MESSAGE(limit.value() == 0, "Expected limit of value 0  but found " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().size() == 0, "Expected no task paths but found " << limit.paths().size());
    BOOST_CHECK_MESSAGE(limit.state_change_no() == expected_state_change_no,
                        "Expected no change to state change no, expected " << expected_state_change_no << " but found "
                                                                           << limit.state_change_no());

    Ecf::set_server(false); // needed to test state_change_numbers
    // reset, to avoid effecting downstream tests
    Ecf::set_state_change_no(0);
    Ecf::set_modify_change_no(0);
}

BOOST_AUTO_TEST_CASE(test_limit_set_value) {
    ECF_NAME_THIS_TEST();

    Limit limit("name", 10);     // Limit of 10
    limit.increment(1, "path");  // consume 1 token
    limit.increment(1, "path2"); // consume 1 token

    BOOST_CHECK_MESSAGE(limit.value() == 2,
                        "Expected increment to consume 2 token but it has consumed " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().size() == 2, "Expected 2 task paths but found " << limit.paths().size());

    limit.setValue(4);
    BOOST_CHECK_MESSAGE(limit.value() == 4, "Expected setValue(4) but value found is: " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().size() == 2, "Expected 2 task paths but found " << limit.paths().size());

    limit.setValue(0);
    BOOST_CHECK_MESSAGE(limit.value() == 0, "Expected setValue(0) but found " << limit.value());
    BOOST_CHECK_MESSAGE(limit.paths().empty(),
                        "Setting value to zero should clear the paths, but found " << limit.paths().size());
}

// Globals used throughout the test
static std::string fileName = "testLimit.txt";
BOOST_AUTO_TEST_CASE(test_LimitDefaultConstructor_serialisation) {
    ECF_NAME_THIS_TEST();

    doSaveAndRestore<Limit>(fileName);
}

BOOST_AUTO_TEST_CASE(test_Limit_serialisation) {
    ECF_NAME_THIS_TEST();

    Limit saved1("limitName", 100);
    doSaveAndRestore(fileName, saved1);

    std::set<std::string> paths;
    paths.insert("path1");
    paths.insert("path2");
    Limit saved2("limitName", 100, 10, paths);
    doSaveAndRestore(fileName, saved2);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
