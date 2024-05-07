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

#include "ecflow/service/executor/PeriodicTaskExecutor.hpp"

BOOST_AUTO_TEST_SUITE(U_Aviso)

BOOST_AUTO_TEST_SUITE(T_PeriodicTaskExecutor)

BOOST_AUTO_TEST_CASE(test_with_invalid_expiry) {
    ecf::service::executor::PeriodicTaskExecutor executor([](const std::chrono::system_clock::time_point& now) {});
    BOOST_CHECK_THROW(executor.start(std::chrono::milliseconds(100)), ecf::service::executor::InvalidExecutorArgument);
}

BOOST_AUTO_TEST_CASE(test_start_stop) {
    int counter = 0;
    ecf::service::executor::PeriodicTaskExecutor executor(
        [&counter](const std::chrono::system_clock::time_point& now) { ++counter; });
    executor.start(std::chrono::seconds(2));
    std::this_thread::sleep_for(std::chrono::seconds(7));
    executor.stop();
    BOOST_CHECK_EQUAL(counter, 4);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
