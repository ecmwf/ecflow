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

#include "ecflow/client/Rtt.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_Rtt)

BOOST_AUTO_TEST_CASE(test_client_invoker_round_trip_times) {
    ECF_NAME_THIS_TEST();

    std::string root_path = File::test_data("libs/client/test/data/", "libs/client");

    /// Open file rtt.dat and compute average round trip times
    std::string result = Rtt::analyis(root_path + "rtt.dat");
    // cout << result << "\n";

    /// generated a file with results
    std::string errorMsg;
    string generated_file = root_path + "rtt_analysis.dat";
    BOOST_CHECK_MESSAGE(File::create(generated_file, result, errorMsg), errorMsg);

    /// Compare with a reference file
    std::vector<std::string> ignoreVec;
    errorMsg.clear();
    std::string diffs = File::diff(generated_file, root_path + "ref_analysis.dat", ignoreVec, errorMsg);
    BOOST_CHECK_MESSAGE(diffs.empty(), diffs << "\n" << errorMsg);

    if (diffs.empty())
        fs::remove(generated_file);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
