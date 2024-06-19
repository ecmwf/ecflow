/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/Variable.hpp"

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_VariableMap)

BOOST_AUTO_TEST_CASE(test_variablemap_is_able_to_create_empty_variable_map) {
    VariableMap empty;

    BOOST_CHECK(empty.empty());
    BOOST_CHECK(empty.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_variablemap_is_able_to_create_variable_map) {
    VariableMap variables{Variable("n1", "v1"), Variable("n2", "v2"), Variable("n3", "v3")};

    BOOST_CHECK(!variables.empty());
    BOOST_CHECK(variables.size() == 3);
}

BOOST_AUTO_TEST_CASE(test_variablemap_is_able_set_value_to_all_variables_in_variable_map) {
    VariableMap variables{Variable("n1", "v1"), Variable("n2", "v2"), Variable("n3", "v3")};

    std::string value = "some large value just for precaution!";
    // set value of all variables
    variables.set_value(value);

    for (const auto& variable : variables) {
        BOOST_CHECK(variable.theValue() == value);
    }
}

BOOST_AUTO_TEST_CASE(test_variablemap_is_able_to_access_variable_in_variable_map) {
    VariableMap variables{Variable("n1", "v1"), Variable("n2", "v2"), Variable("n3", "v3")};

    const Variable& variable = variables["n1"];

    BOOST_CHECK(variable.name() == "n1");
    BOOST_CHECK(variable.theValue() == "v1");
}

BOOST_AUTO_TEST_CASE(test_variablemap_throws_when_accessing_inexistent_variable_in_variable_map) {
    VariableMap variables{Variable("n1", "v1"), Variable("n2", "v2"), Variable("n3", "v3")};

    Variable found;
    BOOST_CHECK_EXCEPTION(
        found = variables["nX"], std::runtime_error, [](const std::runtime_error& e) { return true; });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
