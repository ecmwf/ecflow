/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdio>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Variable)

BOOST_AUTO_TEST_CASE(test_multi_line_variable_values) {
    ECF_NAME_THIS_TEST();

    {
        Variable var("name", "value");
        BOOST_CHECK_MESSAGE(var.name() == "name", "name not as expected");
        BOOST_CHECK_MESSAGE(var.theValue() == "value", "value not as expected");

        std::string expected = "edit name 'value'";
        BOOST_CHECK_MESSAGE(var.toString() == expected, "expected " << expected << " but found " << var.toString());
    }
    {
        Variable var("name", "");
        std::string expected = "edit name ''";
        BOOST_CHECK_MESSAGE(var.toString() == expected, "expected " << expected << " but found " << var.toString());
    }
    {
        Variable var("name", "value\n");
        std::string expected = "edit name 'value\\n'";
        BOOST_CHECK_MESSAGE(var.toString() == expected, "expected " << expected << " but found " << var.toString());
    }
    {
        Variable var("name", "val1\nxxx\nval2");
        std::string expected = "edit name 'val1\\nxxx\\nval2'";
        BOOST_CHECK_MESSAGE(var.toString() == expected, "expected " << expected << " but found " << var.toString());
    }
}

BOOST_AUTO_TEST_CASE(test_variable_value) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> values;
    values.emplace_back("sdsd");
    values.emplace_back("0fred0");
    values.emplace_back("fted");
    values.emplace_back("%value%");
    values.emplace_back("a");
    values.emplace_back("");
    values.emplace_back("0");
    values.emplace_back("00");
    values.emplace_back("000");
    values.emplace_back("0000");
    values.emplace_back("0000000000000");
    for (const auto& value : values) {
        Variable var("name", "");
        var.set_value(value);
        BOOST_CHECK_MESSAGE(var.value() == 0, "expected 0 but found " << var.value() << " for " << value);
    }

    {
        Variable var("name", "0100");
        BOOST_CHECK_MESSAGE(var.value() == 100, "expected 100 but found " << var.value());
    }
    {
        Variable var("name", "0001");
        BOOST_CHECK_MESSAGE(var.value() == 1, "expected 1 but found " << var.value());
    }
    {
        Variable var("name", "2359");
        BOOST_CHECK_MESSAGE(var.value() == 2359, "expected 2359 but found " << var.value());
    }

    // make sure time is convertible to an integer
    constexpr int buff_size = 255;
    char smstime[buff_size];
    for (int h = 0; h < 24; h++) {
        for (int m = 1; m < 60; m++) {
            int output_written = snprintf(smstime, buff_size, "%02d%02d", h, m);
            BOOST_CHECK_MESSAGE(output_written == 4, " expected size 4 but found " << output_written);
            Variable var("name", "");
            var.set_value(smstime);
            int value = stoi(string(smstime));
            BOOST_CHECK_MESSAGE(var.value() == value, "expected " << value << " but found " << var.value());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
