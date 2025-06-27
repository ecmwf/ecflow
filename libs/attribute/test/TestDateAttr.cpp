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

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/DateAttr.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;
using namespace boost::gregorian;
using namespace boost::posix_time;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_DateAttr)

BOOST_AUTO_TEST_CASE(test_date) {
    ECF_NAME_THIS_TEST();

    {
        DateAttr empty;
        DateAttr empty2;
        BOOST_CHECK_MESSAGE(empty == empty2, "Equality failed");
        BOOST_CHECK_MESSAGE(empty.day() == 0 && empty.month() == 0 && empty.year() == 0, "");
    }
    {
        for (int day = 0; day < 28; day++) {
            for (int month = 0; month < 13; month++) {
                int year = 2017;
                if (month == 0)
                    year = 0;
                std::stringstream ss;
                if (day == 0)
                    ss << "*";
                else
                    ss << day;
                ss << ".";
                if (month == 0)
                    ss << "*";
                else
                    ss << month;
                ss << ".";
                if (year == 0)
                    ss << "*";
                else
                    ss << year;

                DateAttr date1(day, month, year);
                DateAttr date2(ss.str());
                BOOST_CHECK_MESSAGE(date1 == date2, "Equality failed");
                BOOST_CHECK_MESSAGE(date1.name() == date2.name(), "name failed");
            }
        }
    }
}

static DateAttr print_and_parse_attr(DateAttr& date) {

    std::string output;
    ecf::write_t(output, date, PrintStyle::MIGRATE);
    output.erase(output.begin() + output.size() - 1); // remove trailing newline

    std::vector<std::string> tokens;
    Str::split_orig(output, tokens);

    return DateAttr::create(tokens, true /*read state*/);
}

BOOST_AUTO_TEST_CASE(test_date_parsing) {
    ECF_NAME_THIS_TEST();

    {
        DateAttr date("12.12.2019");
        date.setFree();
        DateAttr parsed_date = print_and_parse_attr(date);

        BOOST_CHECK_MESSAGE(date == parsed_date,
                            "Parse failed expected " << date.dump() << " but found " << parsed_date.dump());
    }
    {
        DateAttr date("12.12.2019");
        date.setFree();
        DateAttr parsed_date = print_and_parse_attr(date);

        BOOST_CHECK_MESSAGE(date == parsed_date,
                            "Parse failed expected " << date.dump() << " but found " << parsed_date.dump());
    }
    {
        DateAttr date("12.12.2019");
        DateAttr parsed_date = print_and_parse_attr(date);

        BOOST_CHECK_MESSAGE(date == parsed_date,
                            "Parse failed expected " << date.dump() << " but found " << parsed_date.dump());
    }
}

BOOST_AUTO_TEST_CASE(test_date_errors) {
    ECF_NAME_THIS_TEST();

    {
        BOOST_REQUIRE_THROW(DateAttr("-1.2.*"), std::runtime_error);
        BOOST_REQUIRE_THROW(DateAttr("32.2.*"), std::runtime_error);
        BOOST_REQUIRE_THROW(DateAttr("1.-1.*"), std::runtime_error);
        BOOST_REQUIRE_THROW(DateAttr("1.13.*"), std::runtime_error);
        BOOST_REQUIRE_THROW(DateAttr("1.13.-1"), std::runtime_error);
        BOOST_REQUIRE_THROW(DateAttr("1.13.99999999"), std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
