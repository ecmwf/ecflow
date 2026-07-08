/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <array>
#include <cstdio>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Variable.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Variable)

BOOST_AUTO_TEST_CASE(test_multi_line_variable_values) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string name;
        std::string value;
        std::string expected;
    };

    auto values = std::vector<tc>{{"name", "value", "edit name 'value'"},
                                  {"name", "", "edit name ''"},
                                  {"name", "value\n", "edit name 'value\\n'"},
                                  {"name", "val1\nxxx\nval2", "edit name 'val1\\nxxx\\nval2'"}};

    for (const auto& [name, value, expected] : values) {
        auto variable = Variable::new_variable(name, value);

        {
            auto actual = variable.name();
            BOOST_CHECK_MESSAGE(actual == name, "expected name '" << name << "', but found '" << actual << "'");
        }
        {
            auto actual = variable.value();
            BOOST_CHECK_MESSAGE(actual == value, "expected value '" << value << "', but found '" << actual << "'");
        }
        {
            auto actual = variable.toString();
            BOOST_CHECK_MESSAGE(actual == expected, "expected '" << expected << "', but found '" << actual << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_variable_integer_value) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string value;
        int expected;
    };

    auto values = std::vector<tc>{{"sdsd", 0},
                                  {"0fred0", 0},
                                  {"fted", 0},
                                  {"%value%", 0},
                                  {"a", 0},
                                  {"", 0},
                                  {"0", 0},
                                  {"00", 0},
                                  {"000", 0},
                                  {"0000", 0},
                                  {"0000000000000", 0},
                                  {"0100", 100},
                                  {"0001", 1},
                                  {"2359", 2359}};

    for (const auto& [value, expected] : values) {
        auto variable = Variable::new_variable("name", value);
        auto actual   = variable.value<int>();
        BOOST_CHECK_MESSAGE(actual == expected, "expected '0' but found " << actual << " for " << value);
    }
}

BOOST_AUTO_TEST_CASE(test_variable_time_value) {
    ECF_NAME_THIS_TEST();

    std::array<char, 255> smstime;
    for (int h = 0; h < 24; h++) {
        for (int m = 0; m < 60; m++) {
            int output_written = snprintf(smstime.data(), smstime.size(), "%02d%02d", h, m);
            BOOST_CHECK_MESSAGE(output_written == 4, " expected size 4 but found " << output_written);
            auto variable = Variable::new_variable("name", smstime.data());

            int actual   = variable.value<int>();
            int expected = stoi(std::string(smstime.data(), smstime.size()));

            BOOST_CHECK_MESSAGE(actual == expected, "expected '" << expected << "' but found '" << actual << "'");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
