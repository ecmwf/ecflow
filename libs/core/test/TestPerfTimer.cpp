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

#include "TestNaming.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/perf_timer.hpp"

using namespace boost;
using namespace std;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_PerfTimer)

static void func(int const count = 100000) {
    for (int i = 0; i < count; i++) {
        std::string s = std::string("fred");
        s += ecf::convert_to<std::string>(i);
    }
}

BOOST_AUTO_TEST_CASE(test_perf_with_chrono) {
    ECF_NAME_THIS_TEST();

    // using namespace std::chrono_literals;

    [[maybe_unused]] auto t = perf_timer<>::duration(func, 100000);

    // This does not work, ie calling func with no arguments ?? with our hacked invoke, wait till c++ 17
    // auto t0= perf_timer<std::chrono::nanoseconds>::duration(func);

    [[maybe_unused]] auto t1 = perf_timer<std::chrono::nanoseconds>::duration(func, 10);
    [[maybe_unused]] auto t2 = perf_timer<std::chrono::microseconds>::duration(func, 100);
    [[maybe_unused]] auto t3 = perf_timer<std::chrono::milliseconds>::duration(func, 100000);

#if PRINT_TIMING_RESULTS
    ECF_TEST_DBG(<< "   " << std::chrono::duration<double, std::micro>(t).count() << " micro");
    ECF_TEST_DBG(<< "   " << std::chrono::duration<double, std::milli>(t).count() << " milli");
    ECF_TEST_DBG(<< "   " << std::chrono::duration<double, std::nano>(t).count() << " nano");
    ECF_TEST_DBG(<< "   " << std::chrono::duration<double, std::milli>(t1 + t2 + t3).count() << " milli");
#endif

    BOOST_CHECK_MESSAGE(true, "dummy to keep unit test happy");
}

BOOST_AUTO_TEST_CASE(test_chrono_timer) {
    ECF_NAME_THIS_TEST();

    {
        Timer<std::chrono::milliseconds> timer;
        func();
        timer.elapsed("   func with default args , milliseconds");

#if PRINT_TIMING_RESULTS
        using namespace std::chrono;
        ECF_TEST_DBG(<< "    " << duration<double, std::micro>(timer.elapsed()).count() << " micro");
        ECF_TEST_DBG(<< "    " << duration<double, std::milli>(timer.elapsed()).count() << " milli");
        ECF_TEST_DBG(<< "    " << duration<double, std::nano>(timer.elapsed()).count() << " nano");
#endif
    }
    {
        Timer<std::chrono::microseconds> timer;
        func();
        timer.elapsed("   func with default args , microseconds");
    }
    {
        Timer<std::chrono::nanoseconds> timer;
        func();
        timer.elapsed("   func with default args , nanoseconds");
    }
    {
        Timer<std::chrono::seconds> timer;
        func();
        timer.elapsed("   func with default args , seconds");
    }
    BOOST_CHECK_MESSAGE(true, "dummy to keep unit test happy");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
