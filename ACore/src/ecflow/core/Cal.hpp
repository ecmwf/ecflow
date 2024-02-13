/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_Cal_HPP
#define ecflow_core_Cal_HPP

class Cal {
public:
    // Disable default construction
    Cal() = delete;
    // Disable copy (and move) semantics
    Cal(const Cal&)                  = delete;
    const Cal& operator=(const Cal&) = delete;

    static long date_to_julian(long);
    static long julian_to_date(long);
};

#endif /* ecflow_core_Cal_HPP */
