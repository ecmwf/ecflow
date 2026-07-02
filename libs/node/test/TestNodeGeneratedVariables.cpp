/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_NodeGeneratedVariables)

template <typename NodePtr>
static void check_gen_var(const NodePtr& node, const std::string& name, const std::string& expected) {
    std::string value;
    BOOST_CHECK_MESSAGE(node->findParentVariableValue(name, value),
                        "Node " << node->debugNodePath() << " could not find variable '" << name << "'");
    BOOST_CHECK_MESSAGE(value == expected,
                        "Node " << node->debugNodePath() << " variable '" << name << "' expected '" << expected
                                << "' but got '" << value << "'");
}

template <typename NodePtr>
static void check_composite_absolute_path(const NodePtr& node, const std::string& expected) {
    std::string dirname;
    node->findParentVariableValue("ECF_DIRNAME", dirname);
    std::string basename;
    node->findParentVariableValue("ECF_BASENAME", basename);
    std::string absolute = dirname + "/" + basename;
    BOOST_CHECK_MESSAGE(absolute == expected,
                        "Node " << node->debugNodePath() << " expected composite absolute path '" << expected
                                << "' but got '" << absolute << "'");
}

BOOST_AUTO_TEST_CASE(test_ecf_dirname_and_basename) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s   = defs.add_suite("s");
    family_ptr f1 = s->add_family("f1");
    family_ptr f2 = f1->add_family("f2");
    task_ptr t    = f2->add_task("t");

    defs.beginAll();

    // Suite: no parent => ECF_DIRNAME is empty string
    check_gen_var(s, "ECF_DIRNAME", "");
    check_gen_var(s, "ECF_BASENAME", "s");
    check_composite_absolute_path(s, "/s");

    // Family f1: parent is suite /s
    check_gen_var(f1, "ECF_DIRNAME", "/s");
    check_gen_var(f1, "ECF_BASENAME", "f1");
    check_composite_absolute_path(f1, "/s/f1");

    // Family f2: parent is family /s/f1
    check_gen_var(f2, "ECF_DIRNAME", "/s/f1");
    check_gen_var(f2, "ECF_BASENAME", "f2");
    check_composite_absolute_path(f2, "/s/f1/f2");

    // Task t: parent is family /s/f1/f2
    check_gen_var(t, "ECF_DIRNAME", "/s/f1/f2");
    check_gen_var(t, "ECF_BASENAME", "t");
    check_composite_absolute_path(t, "/s/f1/f2/t");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
