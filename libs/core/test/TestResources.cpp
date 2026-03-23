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

BOOST_AUTO_TEST_CASE(can_create_named_values) {
    ECF_NAME_THIS_TEST();
    using namespace ecf::resources;

    auto display = [](const NamedValue& v) {
        std::ostringstream ss;

        ss << std::fixed << std::setprecision(2) << v;
        return ss.str();
    };

    {
        NamedValue empty;
        BOOST_CHECK(empty.name() == "");
        BOOST_CHECK(empty.unit() == "");
        std::cout << empty << std::endl;
        BOOST_CHECK(display(empty) == "0");
    }

    {
        NamedValue v("a", int32_t{-42}, "X");
        BOOST_CHECK(v.name() == "a");
        BOOST_CHECK(v.unit() == "X");
        BOOST_CHECK(display(v) == "-42");
    }

    {
        NamedValue v("b", uint32_t{42}, "Y");
        BOOST_CHECK(v.name() == "b");
        BOOST_CHECK(v.unit() == "Y");
        BOOST_CHECK(display(v) == "42");
    }

    {
        NamedValue v("c", int64_t{-42});
        BOOST_CHECK(v.name() == "c");
        BOOST_CHECK(v.unit() == "");
        BOOST_CHECK(display(v) == "-42");
    }

    {
        NamedValue v("d", uint64_t{42});
        BOOST_CHECK(v.name() == "d");
        BOOST_CHECK(v.unit() == "");
        BOOST_CHECK(display(v) == "42");
    }

    {
        NamedValue v("e", double{3.14}, "Z");
        BOOST_CHECK(v.name() == "e");
        BOOST_CHECK(v.unit() == "Z");
        BOOST_CHECK(display(v) == "3.14");
    }
}

BOOST_AUTO_TEST_CASE(can_create_process_meter) {
    ECF_NAME_THIS_TEST();
    using namespace ecf::resources;

    auto meter = ProcessMeter::make()
                     .with_pid(ProcessMeter::pid_t{12345})
                     .with_maximum_memory(ProcessMeter::memory_t{1024})
                     .with_virtual_memory(ProcessMeter::memory_t{1024 * 1024})
                     .with_resident_memory(ProcessMeter::memory_t{1024 * 1024 * 1024})
                     .with_page_size(ProcessMeter::page_size_t{4096})
                     .with_arena_memory(ProcessMeter::page_size_t{2048})
                     .with_tracked_memory(ProcessMeter::page_size_t{2049})
                     .with_freed_memory(ProcessMeter::page_size_t{2050})
                     .with_n_cpu_online(ProcessMeter::n_cpu_t{1})
                     .with_n_cpu_maximum(ProcessMeter::n_cpu_t{2})
                     .with_n_threads(ProcessMeter::n_threads_t{16});

    BOOST_REQUIRE(meter.get("pid").has_value());
    BOOST_CHECK(meter.get("pid").value() == ProcessMeter::pid_t{12345});

    BOOST_REQUIRE(meter.get("maximum_memory_available").has_value());
    BOOST_CHECK(meter.get("maximum_memory_available").value() == ProcessMeter::memory_t{1024});

    BOOST_REQUIRE(meter.get("virtual_memory_used").has_value());
    BOOST_CHECK(meter.get("virtual_memory_used").value() == ProcessMeter::memory_t{1024 * 1024});

    BOOST_REQUIRE(meter.get("resident_memory_used").has_value());
    BOOST_CHECK(meter.get("resident_memory_used").value() == ProcessMeter::memory_t{1024 * 1024 * 1024});

    BOOST_REQUIRE(meter.get("page_size").has_value());
    BOOST_CHECK(meter.get("page_size").value() == ProcessMeter::page_size_t{4096});

    BOOST_REQUIRE(meter.get("arena_memory").has_value());
    BOOST_CHECK(meter.get("arena_memory").value() == ProcessMeter::page_size_t{2048});

    BOOST_REQUIRE(meter.get("tracked_memory").has_value());
    BOOST_CHECK(meter.get("tracked_memory").value() == ProcessMeter::page_size_t{2049});

    BOOST_REQUIRE(meter.get("freed_memory").has_value());
    BOOST_CHECK(meter.get("freed_memory").value() == ProcessMeter::page_size_t{2050});

    BOOST_REQUIRE(meter.get("n_cpu_online").has_value());
    BOOST_CHECK(meter.get("n_cpu_online").value() == ProcessMeter::n_cpu_t{1});

    BOOST_REQUIRE(meter.get("n_cpu_maximum").has_value());
    BOOST_CHECK(meter.get("n_cpu_maximum").value() == ProcessMeter::n_cpu_t{2});

    BOOST_REQUIRE(meter.get("n_threads").has_value());
    BOOST_CHECK(meter.get("n_threads").value() == ProcessMeter::n_threads_t{16});
}

BOOST_AUTO_TEST_CASE(can_measure_process_resources) {
    ECF_NAME_THIS_TEST();
    using namespace ecf::resources;

    try {
        auto m = Machine::make();

        auto meter = m.get_process_meter();

        BOOST_REQUIRE(!meter.get("unknown").has_value());

        BOOST_REQUIRE(meter.get("pid").has_value());
        BOOST_CHECK(meter.get("pid").value() > ProcessMeter::pid_t{0});

        BOOST_REQUIRE(meter.get("maximum_memory_available").has_value());
        BOOST_CHECK(meter.get("maximum_memory_available").value() > ProcessMeter::memory_t{0});
        BOOST_CHECK(meter.get("maximum_memory_available").value().unit() == "MB");

        BOOST_REQUIRE(meter.get("virtual_memory_used").has_value());
        BOOST_CHECK(meter.get("virtual_memory_used").value() > ProcessMeter::memory_t{0});
        BOOST_CHECK(meter.get("virtual_memory_used").value().unit() == "MB");

        BOOST_REQUIRE(meter.get("resident_memory_used").has_value());
        // Note:
        // The following is only checked to ensure it is non-negative. This is necessary as, unfortunately,
        // in some scopes (e.g. AG complex), the value is 0 even though the process is of course using memory.
        BOOST_CHECK(meter.get("resident_memory_used").value() >= ProcessMeter::memory_t{0});
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

#if defined(HAVE_MALLINFO) || defined(HAVE_MALLINFO2)

        BOOST_REQUIRE(meter.get("arena_memory").has_value());
        BOOST_CHECK(meter.get("arena_memory").value() > ProcessMeter::memory_t{0});

        BOOST_REQUIRE(meter.get("tracked_memory").has_value());
        BOOST_CHECK(meter.get("tracked_memory").value() > ProcessMeter::memory_t{0});

        BOOST_REQUIRE(meter.get("freed_memory").has_value());
        BOOST_CHECK(meter.get("freed_memory").value() > ProcessMeter::memory_t{0});

#else

        BOOST_REQUIRE(!meter.get("arena_memory").has_value());
        BOOST_REQUIRE(!meter.get("tracked_memory").has_value());
        BOOST_REQUIRE(!meter.get("freed_memory").has_value());

#endif
    }
    catch (std::exception& e) {
        BOOST_FAIL("Unexpected exception:" << e.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
