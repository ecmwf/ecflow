// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <chrono>

#include <boost/test/unit_test.hpp>

#include "ecflow/http/DefsStorage.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using std::chrono::seconds;

BOOST_AUTO_TEST_SUITE(U_Http)

BOOST_AUTO_TEST_SUITE(T_DefsStorage)

BOOST_AUTO_TEST_CASE(test_polling_interval_without_drift_stays_at_the_base) {
    ECF_NAME_THIS_TEST();

    // The drift is disabled when the maximum is not above the base interval
    BOOST_CHECK(ecf::http::polling_interval(10, 10, 0.0) == seconds(10));
    BOOST_CHECK(ecf::http::polling_interval(10, 5, 3600.0) == seconds(10));
}

BOOST_AUTO_TEST_CASE(test_polling_interval_grows_by_one_second_for_each_idle_minute) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::http::polling_interval(10, 300, 0.0) == seconds(10));
    BOOST_CHECK(ecf::http::polling_interval(10, 300, 59.0) == seconds(10));
    BOOST_CHECK(ecf::http::polling_interval(10, 300, 60.0) == seconds(11));
    BOOST_CHECK(ecf::http::polling_interval(10, 300, 600.0) == seconds(20));
}

BOOST_AUTO_TEST_CASE(test_polling_interval_is_capped_at_the_maximum_but_never_below_thirty_seconds) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::http::polling_interval(10, 300, 1.0e6) == seconds(300));
    BOOST_CHECK(ecf::http::polling_interval(10, 20, 1.0e6) == seconds(30));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
