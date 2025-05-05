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

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

static std::string fileName = "test.txt";

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Serialisation)

BOOST_AUTO_TEST_CASE(test_calendar_serialisation) {
    ECF_NAME_THIS_TEST();

    Calendar cal;
    doSaveAndRestore(fileName, cal);
}

BOOST_AUTO_TEST_CASE(test_TimeSlot_serialisation) {
    ECF_NAME_THIS_TEST();

    { doSaveAndRestore<TimeSlot>(fileName); }

    {
        TimeSlot saved(1, 1);
        doSaveAndRestore(fileName, saved);
    }

    {
        TimeSlot saved(99, 59);
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_CASE(test_TimeSeries_serialisation) {
    ECF_NAME_THIS_TEST();

    { doSaveAndRestore<TimeSeries>(fileName); }
    {
        TimeSeries saved = TimeSeries(TimeSlot(10, 10));
        doSaveAndRestore(fileName, saved);
    }
    {
        TimeSeries saved = TimeSeries(TimeSlot(0, 0), TimeSlot(10, 10), TimeSlot(0, 10));
        doSaveAndRestore(fileName, saved);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
