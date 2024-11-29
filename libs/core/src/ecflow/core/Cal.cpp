/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Cal.hpp"

long Cal::julian_to_date(long jdate) {
    long x = 0, y = 0, d = 0, m = 0, e = 0;
    long day = 0, month = 0, year = 0;

    x = 4 * jdate - 6884477;
    y = (x / 146097) * 100;
    e = x % 146097;
    d = e / 4;

    x = 4 * d + 3;
    y = (x / 1461) + y;
    e = x % 1461;
    d = e / 4 + 1;

    x = 5 * d - 3;
    m = x / 153 + 1;
    e = x % 153;
    d = e / 5 + 1;

    if (m < 11)
        month = m + 2;
    else
        month = m - 10;

    day  = d;
    year = y + m / 11;

    return year * 10000 + month * 100 + day;
}

long Cal::date_to_julian(long ddate) {
    long m1 = 0, y1 = 0, a = 0, b = 0, c = 0, d = 0, j1 = 0;

    long month = 0, day = 0, year = 0;

    year = ddate / 10000;
    ddate %= 10000;
    month = ddate / 100;
    ddate %= 100;
    day = ddate;

    if (month > 2) {
        m1 = month - 3;
        y1 = year;
    }
    else {
        m1 = month + 9;
        y1 = year - 1;
    }
    a  = 146097 * (y1 / 100) / 4;
    d  = y1 % 100;
    b  = 1461 * d / 4;
    c  = (153 * m1 + 2) / 5 + day + 1721119;
    j1 = a + b + c;

    return j1;
}

// int main(int ac, char** av){ int a=Cal::date_to_julian(20170219) % 7; printf("%d", a); return 0;}
