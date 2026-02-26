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

#include "ecflow/core/resources/Machine.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Resources)

BOOST_AUTO_TEST_CASE(can_measure_process_resources) {
    ECF_NAME_THIS_TEST();
    using namespace ecf::resources;

    try {
        auto m = Machine::make();

        auto meter = m.get_process_meter();

        BOOST_CHECK(meter.pid > 0);
        BOOST_CHECK(meter.maximum_memory > 0);
        BOOST_CHECK(meter.virtual_memory > 0);
        BOOST_CHECK(meter.resident_memory > 0);
        BOOST_CHECK(meter.page_size > 0);
        BOOST_CHECK(meter.n_cpu_online > 0);
        BOOST_CHECK(meter.n_cpu_maximum > 0);
        BOOST_CHECK(meter.n_threads > 0);
    }
    catch (std::exception& e) {
        BOOST_FAIL("Unexpected exception:" << e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
