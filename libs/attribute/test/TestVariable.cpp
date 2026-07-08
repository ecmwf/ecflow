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

BOOST_AUTO_TEST_CASE(test_variable_default_construction) {
    ECF_NAME_THIS_TEST();

    const Variable variable;
    BOOST_CHECK(variable.empty());
    BOOST_CHECK(variable.name().empty());
    BOOST_CHECK(variable.value().empty());
    BOOST_CHECK_EQUAL(variable.value<int>(), 0);
    BOOST_CHECK_EQUAL(variable.toString(), "edit  ''");
}

BOOST_AUTO_TEST_CASE(test_variable_parameterized_construction) {
    ECF_NAME_THIS_TEST();

    const Variable variable("name", "value");
    BOOST_CHECK(!variable.empty());
    BOOST_CHECK_EQUAL(variable.name(), "name");
    BOOST_CHECK_EQUAL(variable.value(), "value");
}

BOOST_AUTO_TEST_CASE(test_variable_new_variable_valid_names) {
    ECF_NAME_THIS_TEST();

    auto valid_names = std::vector<std::string>{"name", "_name", "name.with.dots", "name123", "Name_1"};

    for (const auto& name : valid_names) {
        auto variable = Variable::new_variable(name, "value");
        BOOST_CHECK_EQUAL(variable.name(), name);
        BOOST_CHECK_EQUAL(variable.value(), "value");
    }
}

BOOST_AUTO_TEST_CASE(test_variable_new_variable_invalid_names) {
    ECF_NAME_THIS_TEST();

    auto invalid_names = std::vector<std::string>{"", ".name", "name-with-dash", "name with space", "name!"};

    for (const auto& name : invalid_names) {
        BOOST_CHECK_THROW(Variable::new_variable(name, "value"), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_variable_set_name) {
    ECF_NAME_THIS_TEST();

    auto variable = Variable::new_variable("old_name", "value");
    variable.set_name("new_name");
    BOOST_CHECK_EQUAL(variable.name(), "new_name");
    BOOST_CHECK_EQUAL(variable.value(), "value");
}

BOOST_AUTO_TEST_CASE(test_variable_set_name_invalid_is_noop) {
    ECF_NAME_THIS_TEST();

    auto variable = Variable::new_variable("name", "value");
    BOOST_CHECK_THROW(variable.set_name(".name"), std::runtime_error);
    BOOST_CHECK_EQUAL(variable.name(), "name");
    BOOST_CHECK_EQUAL(variable.value(), "value");

    BOOST_CHECK_THROW(variable.set_name(""), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_variable_set_value) {
    ECF_NAME_THIS_TEST();

    auto variable = Variable::new_variable("name", "old_value");
    variable.set_value("new_value");
    BOOST_CHECK_EQUAL(variable.value(), "new_value");
    BOOST_CHECK_EQUAL(variable.name(), "name");
}

BOOST_AUTO_TEST_CASE(test_variable_value_by_ref) {
    ECF_NAME_THIS_TEST();

    auto variable           = Variable::new_variable("name", "value");
    variable.value_by_ref() = "modified";
    BOOST_CHECK_EQUAL(variable.value(), "modified");
}

BOOST_AUTO_TEST_CASE(test_variable_equality) {
    ECF_NAME_THIS_TEST();

    Variable a("name", "value");
    Variable b("name", "value");
    Variable c("other", "value");
    Variable d("name", "other");
    Variable e;
    Variable f;

    BOOST_CHECK(a == b);
    BOOST_CHECK(e == f);
    BOOST_CHECK(!(a == c));
    BOOST_CHECK(!(a == d));
    BOOST_CHECK(!(a == e));
}

BOOST_AUTO_TEST_CASE(test_variable_less_than) {
    ECF_NAME_THIS_TEST();

    Variable a("a", "x");
    Variable b("b", "y");
    Variable c("a", "z");

    BOOST_CHECK(a < b);
    BOOST_CHECK(!(b < a));
    BOOST_CHECK(!(a < c));
    BOOST_CHECK(!(a < a));
}

BOOST_AUTO_TEST_CASE(test_variable_dump) {
    ECF_NAME_THIS_TEST();

    auto variable = Variable::new_variable("name", "123");
    BOOST_CHECK_EQUAL(variable.dump(), "edit name '123' value(123)");
}

BOOST_AUTO_TEST_CASE(test_variable_write_appends) {
    ECF_NAME_THIS_TEST();

    auto variable      = Variable::new_variable("name", "value");
    std::string buffer = "prefix ";
    variable.write(buffer);
    BOOST_CHECK_EQUAL(buffer, "prefix edit name 'value'");
}

BOOST_AUTO_TEST_CASE(test_variable_is_valid_variable_name) {
    ECF_NAME_THIS_TEST();

    std::string msg;

    BOOST_CHECK(Variable::is_valid_variable_name("valid_name", msg));
    BOOST_CHECK(msg.empty());

    BOOST_CHECK(!Variable::is_valid_variable_name(".name", msg));
    BOOST_CHECK(!msg.empty());

    BOOST_CHECK(!Variable::is_valid_variable_name("", msg));
}

BOOST_AUTO_TEST_CASE(test_variable_empty_singleton) {
    ECF_NAME_THIS_TEST();

    const Variable& empty1 = Variable::EMPTY();
    const Variable& empty2 = Variable::EMPTY();

    BOOST_CHECK(empty1.empty());
    BOOST_CHECK(empty2.empty());
    BOOST_CHECK(&empty1 == &empty2);
    BOOST_CHECK_EQUAL(empty1.name(), "");
    BOOST_CHECK_EQUAL(empty1.value(), "");
}

BOOST_AUTO_TEST_CASE(test_variable_map) {
    ECF_NAME_THIS_TEST();

    VariableMap map(Variable("a", "1"), Variable("b", "2"));
    BOOST_CHECK(!map.empty());
    BOOST_CHECK_EQUAL(map.size(), 2);

    BOOST_CHECK_EQUAL(map["a"].name(), "a");
    BOOST_CHECK_EQUAL(map["a"].value(), "1");
    BOOST_CHECK_EQUAL(map["b"].name(), "b");
    BOOST_CHECK_EQUAL(map["b"].value(), "2");

    map.set_value("X");
    BOOST_CHECK_EQUAL(map["a"].value(), "X");
    BOOST_CHECK_EQUAL(map["b"].value(), "X");

    BOOST_CHECK_THROW(static_cast<void>(map["c"]), std::runtime_error);

    size_t count = 0;
    for (const auto& var : map) {
        BOOST_CHECK(!var.empty());
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 2);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
