/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <regex>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Filesystem)

BOOST_AUTO_TEST_CASE(test_unique_path_filename_pattern) {
    ECF_NAME_THIS_TEST();

    auto pattern = std::string{"ecflow_test_%%%%-%%%%-%%%%-%%"};
    auto path    = ecf::fsx::unique_path(pattern);

    std::regex re{"ecflow_test_[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{4}-[A-F0-9]{2}"};
    BOOST_CHECK(std::regex_match(path.native(), re));
}

BOOST_AUTO_TEST_CASE(test_unique_path_filepath_pattern) {
    ECF_NAME_THIS_TEST();

    auto pattern = std::string{"/path/to/ecflow_test_%%%%%%-%%%%%%"};
    auto path    = ecf::fsx::unique_path(pattern);

    std::regex re{"/path/to/ecflow_test_[A-F0-9]{6}-[A-F0-9]{6}"};
    BOOST_CHECK(std::regex_match(path.native(), re));
}

BOOST_AUTO_TEST_CASE(test_last_write_time) {
    ECF_NAME_THIS_TEST();

    std::string path = ecf::File::test_data("libs/core/test/data/test_write_time.txt", "libs/core");

    // Store something to set the last write time
    {
        std::ofstream content(path.c_str());
        content << "some content...";
    }

    auto expected = std::chrono::system_clock::now();
    auto actual   = ecf::fsx::last_write_time(path);

    using period = std::chrono::microseconds;
    auto diff    = std::chrono::duration_cast<period>(actual.time_since_epoch()) -
                std::chrono::duration_cast<period>(expected.time_since_epoch());

    // Check if within 0.2 seconds
    BOOST_CHECK(diff < std::chrono::milliseconds(200));

    fs::remove(path);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
