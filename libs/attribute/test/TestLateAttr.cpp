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
#include <stdexcept>
#include <string>

#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/core/Calendar.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_LateAttr)

BOOST_AUTO_TEST_CASE(test_late_attr_submitted) {
    ECF_NAME_THIS_TEST();

    // REF: ECFLOW-322
    Calendar calendar;
    calendar.init(ptime(date(2013, 7, 9), minutes(0)), Calendar::REAL); // tuesday

    /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
    ///               relative, so + is simple ignored, if present. If the node stays submitted
    ///               longer than the time specified, the late flag is set
    /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
    ///               is still queued or submitted, the late flag is set
    /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
    ///               taken from the time the node became active, otherwise node must be complete by
    ///               the time given.
    ecf::LateAttr lateAttr;
    lateAttr.addSubmitted(ecf::TimeSlot(0, 4));

    calendar.update(time_duration(minutes(1)));
    calendar.update(time_duration(minutes(1)));
    calendar.update(time_duration(minutes(1)));
    calendar.update(time_duration(minutes(1)));
    calendar.update(time_duration(minutes(1)));

    // set submitted state at 00:05:00
    std::pair<NState, boost::posix_time::time_duration> state =
        std::make_pair(NState(NState::SUBMITTED), calendar.duration());

    // after four minutes in submitted state, we should be late
    for (int m = 1; m < 10; m++) {
        calendar.update(time_duration(minutes(1)));

        lateAttr.checkForLateness(state, calendar);

        if (m >= 4) {
            BOOST_CHECK_MESSAGE(lateAttr.isLate(),
                                " expected to be late at " << to_simple_string(calendar.suiteTime()));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_late_attr_active) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    calendar.init(ptime(date(2013, 7, 9), minutes(0)), Calendar::REAL); // tuesday

    /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
    ///               relative, so + is simple ignored, if present. If the node stays submitted
    ///               longer than the time specified, the late flag is set
    /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
    ///               is still queued or submitted, the late flag is set
    /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
    ///               taken from the time the node became active, otherwise node must be complete by
    ///               the time given.

    ecf::LateAttr lateAttr;
    lateAttr.addActive(ecf::TimeSlot(10, 0));

    // set submitted state at 00:00:00
    std::pair<NState, boost::posix_time::time_duration> state =
        std::make_pair(NState(NState::SUBMITTED), calendar.duration());

    // after 10 hours we, if we are not active, we should be late
    for (int m = 1; m < 23; m++) {
        calendar.update(time_duration(hours(1)));

        lateAttr.checkForLateness(state, calendar);

        if (m >= 10) {
            BOOST_CHECK_MESSAGE(lateAttr.isLate(),
                                " expected to be late at " << to_simple_string(calendar.suiteTime()));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_late_attr_complete_relative) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    calendar.init(ptime(date(2013, 7, 9), minutes(0)), Calendar::REAL); // tuesday

    /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
    ///               relative, so + is simple ignored, if present. If the node stays submitted
    ///               longer than the time specified, the late flag is set
    /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
    ///               is still queued or submitted, the late flag is set
    /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
    ///               taken from the time the node became active, otherwise node must be complete by
    ///               the time given.

    ecf::LateAttr lateAttr;
    lateAttr.addComplete(ecf::TimeSlot(0, 15), true);

    // set active state at 00:00:00
    std::pair<NState, boost::posix_time::time_duration> state =
        std::make_pair(NState(NState::ACTIVE), calendar.duration());

    // after 15 minutes relative, if we are not complete, we should be late
    for (int m = 1; m < 23; m++) {
        calendar.update(time_duration(minutes(1)));

        lateAttr.checkForLateness(state, calendar);

        if (m >= 15) {
            BOOST_CHECK_MESSAGE(lateAttr.isLate(),
                                " expected to be late at " << to_simple_string(calendar.suiteTime()));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_late_attr_complete_real) {
    ECF_NAME_THIS_TEST();

    Calendar calendar;
    calendar.init(ptime(date(2013, 7, 9), minutes(0)), Calendar::REAL); // tuesday

    /// -s submitted: The time node can stay submitted (format [+]hh:mm). submitted is always
    ///               relative, so + is simple ignored, if present. If the node stays submitted
    ///               longer than the time specified, the late flag is set
    /// -a Active   : The time of day the node must have become active (format hh:mm). If the node
    ///               is still queued or submitted, the late flag is set
    /// -c Complete : The time node must become complete (format {+}hh:mm). If relative, time is
    ///               taken from the time the node became active, otherwise node must be complete by
    ///               the time given.

    ecf::LateAttr lateAttr;
    lateAttr.addComplete(ecf::TimeSlot(3, 0), false);

    // set active state at 00:00:00
    std::pair<NState, boost::posix_time::time_duration> state =
        std::make_pair(NState(NState::ACTIVE), calendar.duration());

    // after 3 hours we, if we are not complete, we should be late
    for (int m = 1; m < 7; m++) {

        calendar.update(time_duration(hours(1)));

        lateAttr.checkForLateness(state, calendar);

        if (m >= 3) {
            BOOST_CHECK_MESSAGE(lateAttr.isLate(),
                                " expected to be late at " << to_simple_string(calendar.suiteTime()));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_late_parsing) {
    ECF_NAME_THIS_TEST();

    TimeSlot start(10, 10);
    TimeSlot finish(23, 10);
    {
        LateAttr late;
        LateAttr parsedlate;
        BOOST_CHECK_MESSAGE(late == parsedlate,
                            "Expected " << late.toString() << " but found " << parsedlate.toString());
    }
    {
        LateAttr late;
        late.addSubmitted(start);
        LateAttr parsedLate = LateAttr::create("late -s 10:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-s 10:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
    {
        LateAttr late;
        late.addActive(finish);
        LateAttr parsedLate = LateAttr::create("late -a 23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-a 23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
    {
        LateAttr late;
        late.addComplete(finish, false);
        LateAttr parsedLate = LateAttr::create("late -c 23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-c 23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
    {
        LateAttr late;
        late.addComplete(finish, true);
        LateAttr parsedLate = LateAttr::create("late -c +23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-c +23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
    {
        LateAttr late;
        late.addComplete(finish, true);
        LateAttr parsedLate = LateAttr::create("late -c +23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-c +23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
    {
        LateAttr late;
        late.addSubmitted(start);
        late.addActive(finish);
        late.addComplete(finish, true);
        LateAttr parsedLate = LateAttr::create("late -s 10:10 -a 23:10 -c +23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-s 10:10 -a 23:10 -c +23:10");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
    {
        LateAttr late;
        late.addSubmitted(start);
        late.addActive(finish);
        late.addComplete(finish, true);
        LateAttr parsedLate = LateAttr::create("late -c +23:10 -s 10:10 -a 23:10 ");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
        parsedLate = LateAttr::create("-c +23:10 -s 10:10 -a 23:10 ");
        BOOST_CHECK_MESSAGE(late == parsedLate,
                            "Expected " << late.toString() << " but found " << parsedLate.toString());
    }
}

BOOST_AUTO_TEST_CASE(test_late_parsing_errors) {
    ECF_NAME_THIS_TEST();

    BOOST_REQUIRE_THROW((void)LateAttr::create(""), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late 10:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -s 100:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -s 10:107"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("10:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -a"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("-a"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("-c"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("-s"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c -s 10:10 -a 23:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c +23:10 -s -a 23:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c +23:105 -s 10:10 -a 23:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c +23:10 -c +23:10 -s 10:10 -a 23:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c +23:10 -s 10:10 -a 23:10 -a 23:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c +23:10 -s 10:10  -s 10:10 -a 23:10"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late -c -s -a"), std::runtime_error);
    BOOST_REQUIRE_THROW((void)LateAttr::create("late +23:10 10:10 23:10"), std::runtime_error);

    BOOST_CHECK_THROW((void)LateAttr::create("late  -a 23:11 -c"), std::runtime_error);
    BOOST_CHECK_THROW((void)LateAttr::create("late -c +23:10 -s 10:10 -a"), std::runtime_error);
    BOOST_CHECK_THROW((void)LateAttr::create("late -c +23:10 -s 10:10 -a 11:11 -s"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
