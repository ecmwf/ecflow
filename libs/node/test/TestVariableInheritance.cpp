/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_VariableInheritance)

static void findParentVariableValue(task_ptr t, const std::string& name, const std::string& expected) {
    std::string value;
    BOOST_CHECK_MESSAGE(t->findParentVariableValue(name, value),
                        "Task " << t->debugNodePath() << " could not find variable of name " << name);
    BOOST_CHECK_MESSAGE(value == expected,
                        "From task " << t->debugNodePath() << " for variable " << name << " expected value " << expected
                                     << " but found " << value);
}

BOOST_AUTO_TEST_CASE(test_variable_inheritance) {
    ECF_NAME_THIS_TEST();

    // See page 31, section 5.1 variable inheritance, of SMS users guide
    task_ptr t;
    task_ptr t2;
    task_ptr z;

    Defs defs;
    {
        suite_ptr suite = defs.add_suite("suite");
        suite->addVariable(Variable::new_variable("TOPLEVEL", "10"));
        suite->addVariable(Variable::new_variable("MIDDLE", "10"));
        suite->addVariable(Variable::new_variable("LOWER", "10"));

        family_ptr fam = suite->add_family("f");
        fam->addVariable(Variable::new_variable("MIDDLE", "20"));
        t = fam->add_task("t");
        t->addVariable(Variable::new_variable("LOWER", "abc"));
        t2 = fam->add_task("t2");

        family_ptr fam2 = suite->add_family("f2");
        fam2->addVariable(Variable::new_variable("TOPLEVEL", "40"));
        z = fam2->add_task("z");
    }

    // Generate variables, needed since,findParentVariableValue also serach's the generated variables
    defs.beginAll();

    // See page 31, section 5.1 variable inheritance, of SMS users guide
    findParentVariableValue(t, "TOPLEVEL", "10");
    findParentVariableValue(t2, "TOPLEVEL", "10");
    findParentVariableValue(z, "TOPLEVEL", "40");

    findParentVariableValue(t, "MIDDLE", "20");
    findParentVariableValue(t2, "MIDDLE", "20");
    findParentVariableValue(z, "MIDDLE", "10");

    findParentVariableValue(t, "LOWER", "abc");
    findParentVariableValue(t2, "LOWER", "10");
    findParentVariableValue(z, "LOWER", "10");
}

BOOST_AUTO_TEST_CASE(test_variable_inheritance_from_server_state) {
    ECF_NAME_THIS_TEST();

    //
    // Test: when a variable is not found on the node or any of its parents, the lookup falls back to the
    // server variables. A server variable is found by presence, so that an empty value is still reported as
    // found (with an empty value), while a variable that does not exist anywhere is reported as not found.
    //

    Defs defs;
    task_ptr t = defs.add_suite("suite")->add_family("f")->add_task("t");
    defs.server_state().add_or_update_user_variables("SERVER_VAR", "server_value");
    defs.server_state().add_or_update_user_variables("EMPTY_SERVER_VAR", "");
    defs.beginAll();

    findParentVariableValue(t, "SERVER_VAR", "server_value");
    findParentVariableValue(t, "EMPTY_SERVER_VAR", "");

    std::string value = "<not set>";
    BOOST_CHECK_MESSAGE(t->findParentUserVariableValue("EMPTY_SERVER_VAR", value),
                        "expected findParentUserVariableValue to find the empty server variable");
    BOOST_CHECK_MESSAGE(value.empty(), "expected an empty value but found: " << value);

    value = "<not set>";
    BOOST_CHECK_MESSAGE(!t->findParentVariableValue("UNDEFINED_VAR", value),
                        "expected findParentVariableValue to not find an undefined variable");
    BOOST_CHECK_MESSAGE(!t->findParentUserVariableValue("UNDEFINED_VAR", value),
                        "expected findParentUserVariableValue to not find an undefined variable");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
