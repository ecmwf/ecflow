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
#include <string>

#include <boost/test/unit_test.hpp>

#include "TestNaming.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_EnvironmentSubstitution)

BOOST_AUTO_TEST_CASE(test_environment_substitution) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    Suite* s = nullptr;
    {
        suite_ptr suite = defs.add_suite("suite");
        s               = suite.get();
        suite->addVariable(Variable("AVI", "avi"));

        std::vector<std::pair<std::string, std::string>> env;
        env.emplace_back(ecf::environment::ECF_HOME, string("/home/smshome"));
        env.emplace_back(string("FRED"), string("/home/fred"));
        env.emplace_back(string("BILL"), string("/home/bill"));
        env.emplace_back(string("JANE"), string("/home/jane"));
        env.emplace_back(string("REP"), string("$REP/bill"));
        defs.set_server().add_or_update_user_variables(env);
    }

    // Check for recursive, in which case we only substitute once
    string expected = "$REP/bill";
    std::string cmd = "$REP";
    BOOST_REQUIRE_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    // See page 31, section 5.1 variable inheritance, of SMS users guide
    cmd      = "$ECF_HOME";
    expected = "/home/smshome";
    BOOST_REQUIRE_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "$ECF_HOME/include";
    expected = "/home/smshome/include";
    BOOST_REQUIRE_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "$ECF_HOME$FRED$BILL$JANE";
    expected = "/home/smshome/home/fred/home/bill/home/jane";
    BOOST_REQUIRE_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "$ECF_HOME/$FRED/$BILL/$JANE";
    expected = "/home/smshome//home/fred//home/bill//home/jane";
    BOOST_CHECK_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH";
    expected = "%PATH";
    BOOST_CHECK_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "$$";
    expected = "$$";
    BOOST_CHECK_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "$ERROR$";
    expected = "$ERROR$";
    BOOST_CHECK_MESSAGE(!s->variable_dollar_substitution(cmd),
                        " substitution expected to fail since ERROR does not exist");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variable_dollar_substitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
