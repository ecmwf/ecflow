/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_CalendarUpdateParams_HPP
#define ecflow_core_CalendarUpdateParams_HPP

///
/// \brief Collate list of arguments used to update calendar
///

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace ecf {

class CalendarUpdateParams {
public:
    // For use in the server
    CalendarUpdateParams(const boost::posix_time::ptime& time_now,
                         const boost::posix_time::time_duration& serverPollPeriod,
                         bool serverRunning)
        : timeNow_(time_now),
          serverPollPeriod_(serverPollPeriod),
          serverRunning_(serverRunning),
          forTest_(false) {}

    // For use in the simulator/ unit tests
    explicit CalendarUpdateParams(const boost::posix_time::time_duration& serverPollPeriod)
        : timeNow_(boost::date_time::not_a_date_time),
          serverPollPeriod_(serverPollPeriod),
          serverRunning_(true),
          forTest_(true) {}

    // For use in test
    CalendarUpdateParams(const boost::posix_time::ptime& time_now,
                         const boost::posix_time::time_duration& serverPollPeriod,
                         bool serverRunning,
                         bool forTest)
        : timeNow_(time_now),
          serverPollPeriod_(serverPollPeriod),
          serverRunning_(serverRunning),
          forTest_(forTest) {}

    // Disable copy (and move) semantics
    CalendarUpdateParams(const CalendarUpdateParams&)                  = delete;
    const CalendarUpdateParams& operator=(const CalendarUpdateParams&) = delete;

    const boost::posix_time::ptime& timeNow() const { return timeNow_; }
    const boost::posix_time::time_duration& serverPollPeriod() const { return serverPollPeriod_; }
    bool serverRunning() const { return serverRunning_; }
    bool forTest() const { return forTest_; }

private:
    boost::posix_time::ptime timeNow_;                  // Current time and date, not used in simulator
    boost::posix_time::time_duration serverPollPeriod_; // equivalent to calendar increment
    bool serverRunning_;                                // Is the server running or stopped
    bool forTest_;                                      // Used with Simulator
};

} // namespace ecf

#endif /* ecflow_core_CalendarUpdateParams_HPP */
