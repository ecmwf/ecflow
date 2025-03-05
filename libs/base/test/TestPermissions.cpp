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

#include "ecflow/server/AuthorisationService.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_Perms)

BOOST_AUTO_TEST_CASE(test_file_automatic_name_exists) {
    ECF_NAME_THIS_TEST();

    std::vector<fs::path> paths;
    for (int i = 0; i < 10; i++) {
        WithTestFile file;

        auto path = fs::absolute(file.path());
        paths.push_back(path);

        BOOST_CHECK(fs::exists(path));
    }
    for (const auto& path : paths) {
        BOOST_CHECK(!fs::exists(path));
    }
}

BOOST_AUTO_TEST_CASE(test_file_automatic_prefix_name_exists) {
    ECF_NAME_THIS_TEST();

    std::vector<fs::path> paths;
    for (int i = 0; i < 10; i++) {
        auto prefix = "test_file_" + std::to_string(i);
        WithTestFile file{AutomaticTestFile{prefix}};

        auto path = file.path();
        paths.push_back(path);

        BOOST_CHECK(fs::exists(path));
    }
    for (const auto& path : paths) {
        BOOST_CHECK(!fs::exists(path));
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
