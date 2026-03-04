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

        BOOST_REQUIRE(meter.get("pid").has_value());
        BOOST_CHECK(meter.get("pid").value() > ProcessMeter::pid_t{0});

        BOOST_REQUIRE(meter.get("maximum_memory_available").has_value());
        BOOST_CHECK(meter.get("maximum_memory_available").value() > ProcessMeter::memory_t{0});
        BOOST_CHECK(meter.get("maximum_memory_available").value().unit() == "MB");

        BOOST_REQUIRE(meter.get("virtual_memory_used").has_value());
        BOOST_CHECK(meter.get("virtual_memory_used").value() > ProcessMeter::memory_t{0});
        BOOST_CHECK(meter.get("virtual_memory_used").value().unit() == "MB");

        BOOST_REQUIRE(meter.get("resident_memory_used").has_value());
        BOOST_CHECK(meter.get("resident_memory_used").value() > ProcessMeter::memory_t{0});
        BOOST_CHECK(meter.get("resident_memory_used").value().unit() == "MB");

        BOOST_REQUIRE(meter.get("page_size").has_value());
        BOOST_CHECK(meter.get("page_size").value() > ProcessMeter::page_size_t{0});
        BOOST_CHECK(meter.get("page_size").value().unit() == "kB");

        BOOST_REQUIRE(meter.get("n_cpu_online").has_value());
        BOOST_CHECK(meter.get("n_cpu_online").value() > ProcessMeter::n_cpu_t{0});

        BOOST_REQUIRE(meter.get("n_cpu_maximum").has_value());
        BOOST_CHECK(meter.get("n_cpu_maximum").value() > ProcessMeter::n_cpu_t{0});

        BOOST_REQUIRE(meter.get("n_threads").has_value());
        BOOST_CHECK(meter.get("n_threads").value() > ProcessMeter::n_threads_t{0});

        BOOST_REQUIRE(meter.get("cpu_usage").has_value());
        BOOST_CHECK(meter.get("cpu_usage").has_value());
        BOOST_CHECK(meter.get("cpu_usage").value() >= double{0.0});
    }
    catch (std::exception& e) {
        BOOST_FAIL("Unexpected exception:" << e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
