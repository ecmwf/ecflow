/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace ecf;

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
