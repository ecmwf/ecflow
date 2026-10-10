// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <boost/test/unit_test.hpp>

#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Memento.hpp"
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

BOOST_AUTO_TEST_CASE(test_ecf_owner_is_generated_from_the_owner) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s   = defs.add_suite("s");
    family_ptr f1 = s->add_family("f1");
    task_ptr t1   = f1->add_task("t1");
    task_ptr t2   = f1->add_task("t2");
    defs.beginAll();

    // Without an owner, the generated variable exists with an empty value; families have none
    check_gen_var(t1, "ECF_OWNER", "");
    BOOST_CHECK_MESSAGE(f1->findGenVariable("ECF_OWNER").empty(), "Expected no ECF_OWNER on a family");

    // An owner set on a container reaches every task below it
    f1->set_owner("xyza");
    BOOST_CHECK_MESSAGE(t1->owner() == "xyza", "Expected owner xyza, got " << t1->owner());
    BOOST_CHECK_MESSAGE(t2->owner() == "xyza", "Expected owner xyza, got " << t2->owner());
    check_gen_var(t1, "ECF_OWNER", "xyza");
    check_gen_var(t2, "ECF_OWNER", "xyza");

    // Set on one task, the others keep theirs
    t2->set_owner("xyzb");
    check_gen_var(t1, "ECF_OWNER", "xyza");
    check_gen_var(t2, "ECF_OWNER", "xyzb");
}

BOOST_AUTO_TEST_CASE(test_ecf_owner_is_not_overridden_in_job_generation) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    defs.beginAll();
    t->set_owner("xyza");

    // User variables of the same name, at any level, do not reach the command
    s->addVariable(Variable("ECF_OWNER", "xyzb"));
    t->addVariable(Variable("ECF_OWNER", "xyzc"));

    std::string cmd = "submit -c ~%ECF_OWNER%/config.yml %ECF_JOB%";
    NameValueMap user_edit_variables;
    BOOST_CHECK_MESSAGE(t->variable_substitution(cmd, user_edit_variables), "Substitution failed: " << cmd);
    BOOST_CHECK_MESSAGE(cmd.find("~xyza/config.yml") != std::string::npos, "Unexpected command: " << cmd);

    // The form with a default value takes the generated value too
    std::string with_default = "run as %ECF_OWNER:nobody%";
    BOOST_CHECK_MESSAGE(t->variable_substitution(with_default, user_edit_variables),
                        "Substitution failed: " << with_default);
    BOOST_CHECK_MESSAGE(with_default == "run as xyza", "Unexpected command: " << with_default);
}

BOOST_AUTO_TEST_CASE(test_ecf_owner_survives_the_state_and_the_memento) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    defs.beginAll();
    t->set_owner("xyza");

    // The owner is part of the state, not of the definition
    std::string state;
    defs.write_to_string(state, PrintStyle::STATE);
    BOOST_CHECK_MESSAGE(state.find("owner:xyza") != std::string::npos, "Expected the owner in the state:\n" << state);
    std::string migrate;
    defs.write_to_string(migrate, PrintStyle::MIGRATE);
    BOOST_CHECK_MESSAGE(migrate.find("owner:xyza") != std::string::npos,
                        "Expected the owner in the checkpoint form:\n"
                            << migrate);
    std::string plain;
    defs.write_to_string(plain, PrintStyle::DEFS);
    BOOST_CHECK_MESSAGE(plain.find("owner:") == std::string::npos, "Expected no owner in the definition:\n" << plain);

    // A restore of the state keeps it
    Defs restored;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(restored.restore_from_string(migrate, errorMsg, warningMsg), errorMsg);
    node_ptr rt = restored.findAbsNode("/s/t");
    BOOST_REQUIRE(rt && rt->isTask());
    BOOST_CHECK_MESSAGE(rt->isTask()->owner() == "xyza",
                        "Expected owner xyza after restore, got " << rt->isTask()->owner());
    BOOST_CHECK_MESSAGE(rt->findGenVariable("ECF_OWNER").value() == "xyza",
                        "Expected the generated ECF_OWNER after restore, got "
                            << rt->findGenVariable("ECF_OWNER").value());

    // A memento applied to a client copy sets it, and the generated variable follows
    Defs client;
    task_ptr ct = client.add_suite("s")->add_task("t");
    SubmittableMemento memento("", "", "", 0, "xyza");
    std::vector<ecf::Aspect::Type> aspects;
    ct->set_memento(&memento, aspects, false);
    BOOST_CHECK_MESSAGE(ct->owner() == "xyza", "Expected owner xyza from the memento, got " << ct->owner());
    ct->update_generated_variables();
    check_gen_var(ct, "ECF_OWNER", "xyza");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
