// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_VariableSubstitution)

BOOST_AUTO_TEST_CASE(test_variable_substitution) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    {
        s->addVariable(Variable::new_variable("AVI", "avi"));
        s->addVariable(Variable::new_variable("BAHRA", "bahra"));
        s->addVariable(Variable::new_variable("LOWER", "10"));
        s->addVariable(Variable::new_variable("PATH", "/fred/bill/joe"));
        s->addVariable(Variable::new_variable("EMPTY_VARIABLE", ""));
        s->addVariable(Variable::new_variable("fred", "%bill%"));
        s->addVariable(Variable::new_variable("bill", "%fred%"));
        s->addVariable(Variable::new_variable("hello", "%hello%"));
        s->addVariable(Variable::new_variable("mary", "%jane%"));
        s->addVariable(Variable::new_variable("jane", "10"));
    }

    // See page 31, section 5.1 variable inheritance, of SMS users guide
    std::string cmd      = "%AVI%-%BAHRA%-%LOWER%-%AVI%";
    std::string expected = "avi-bahra-10-avi";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), "substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%ECF_VERSION%";
    expected = Version::full();
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), "substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH%";
    expected = "/fred/bill/joe";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "/%AVI%/%BAHRA%/%LOWER%%PATH%";
    expected = "/avi/bahra/10/fred/bill/joe";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%EMPTY_VARIABLE%";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%mary%";
    expected = "10"; // double substitution
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%fred%";
    expected = "%fred%"; // infinite substitution
    BOOST_CHECK_MESSAGE(!s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%hello%";
    expected = "%hello%"; // infinite substitution
    BOOST_CHECK_MESSAGE(!s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = Ecf::MICRO();
    expected = Ecf::MICRO();
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH";
    expected = "%PATH";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%%";
    expected = "%";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%ERROR%";
    expected = "%ERROR%";
    BOOST_CHECK_MESSAGE(!s->variableSubstitution(cmd), " substitution expected to fail since ERROR does not exist");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    // new rules
    // %<VAR>:substitute %
    // If we find VAR, then use it, else use substitute
    cmd      = "%AVI:goblly gook%";
    expected = "avi";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH:goblly::: gook%";
    expected = "/fred/bill/joe";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%LOWER:fred% %AVI:fred2%";
    expected = "10 avi";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%LOWER:fred% %AVI:fred2";
    expected = "10 %AVI:fred2";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%EMPTY_VARIABLE::goblly gook%";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%NULL:goblly gook%";
    expected = "goblly gook";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%NULL::goblly gook%";
    expected = ":goblly gook";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%NULL:%";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%:%";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_double_micro) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");

    std::string cmd      = "%%";
    std::string expected = "%";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%%%";
    expected = "%%";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%%%%";
    expected = "%%";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%%%%%";
    expected = "%%%";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "date +%%Y.%%m.%%d";
    expected = "date +%Y.%m.%d";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), "substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "printf %%02d %HOUR:00%";
    expected = "printf %02d 00";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), "substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    s->addVariable(Variable::new_variable("HOUR", "hammer time"));
    cmd      = "printf %%02d %HOUR:00%";
    expected = "printf %02d hammer time";
    BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), "substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_CASE(test_user_variable_substitution) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    {
        std::vector<std::pair<std::string, std::string>> pairs = {{"AVI", "avi"},
                                                                  {"BAHRA", "bahra"},
                                                                  {"LOWER", "10"},
                                                                  {"PATH", "/fred/bill/joe"},
                                                                  {"EMPTY_VARIABLE", ""},
                                                                  {"fred", "%bill%"},
                                                                  {"bill", "%fred%"},
                                                                  {"hello", "%hello%"},
                                                                  {"mary", "%jane%"},
                                                                  {"jane", "10"}};

        for (auto& [name, value] : pairs) {
            s->addVariable(Variable::new_variable(name, value));
        }
    }

    NameValueMap user_variables = {{"AVI", "_avi"},
                                   {"BAHRA", "_bahra"},
                                   {"LOWER", "_10"},
                                   {"PATH", "_/fred/bill/joe"},
                                   {"EMPTY_VARIABLE", "_"},
                                   {"fred", "%bill%"},
                                   {"bill", "%fred%"},
                                   {"hello", "%hello%"},
                                   {"mary", "%jane%"},
                                   {"jane", "_10"}};

    // See page 31, section 5.1 variable inheritance, of SMS users guide
    std::string cmd      = "%AVI%-%BAHRA%-%LOWER%-%AVI%";
    std::string expected = "_avi-_bahra-_10-_avi";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), "substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH%";
    expected = "_/fred/bill/joe";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "/%AVI%/%BAHRA%/%LOWER%%PATH%";
    expected = "/_avi/_bahra/_10_/fred/bill/joe";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%EMPTY_VARIABLE%";
    expected = "_";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%mary%";
    expected = "_10"; // double substitution
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%fred%";
    expected = "%fred%"; // infinite substitution
    BOOST_CHECK_MESSAGE(!s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%hello%";
    expected = "%hello%"; // infinite substitution
    BOOST_CHECK_MESSAGE(!s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = Ecf::MICRO();
    expected = Ecf::MICRO();
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH";
    expected = "%PATH";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%%";
    expected = "%";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%ERROR%";
    expected = "%ERROR%";
    BOOST_CHECK_MESSAGE(!s->variable_substitution(cmd, user_variables),
                        " substitution expected to fail since ERROR does not exist");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    // new rules
    // %<VAR>:substitute %
    // If we find VAR, then use it, else use substitute
    cmd      = "%AVI:goblly gook%";
    expected = "_avi";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%PATH:goblly::: gook%";
    expected = "_/fred/bill/joe";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%LOWER:fred% %AVI:fred2%";
    expected = "_10 _avi";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%LOWER:fred% %AVI:fred2";
    expected = "_10 %AVI:fred2";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%EMPTY_VARIABLE::goblly gook%";
    expected = "_";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%NULL:goblly gook%";
    expected = "goblly gook";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%NULL::goblly gook%";
    expected = ":goblly gook";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%NULL:%";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%:%";
    expected = "";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

BOOST_AUTO_TEST_CASE(test_user_variable_substitution_1) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("AVI", "avi"));

    NameValueMap user_variables;
    user_variables.insert(std::make_pair(std::string("AVI:goblly gook"), std::string("avtar")));

    // new rules
    // %<VAR>:substitute %
    // If we find VAR, then use it, else use substitute
    // However when we have user_variables if we find the complete string
    // in the user variable list, we use user veriable value:
    // cmd = %FRED:BILL%"   and   user_variable = "FRED:BILL","Joe90"   ===> cmd = "Joe90"
    std::string cmd      = "%AVI:goblly gook%";
    std::string expected = "avtar";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%AVI:goblly%";
    expected = "avi";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");

    cmd      = "%FRED:goblly%";
    expected = "goblly";
    BOOST_CHECK_MESSAGE(s->variable_substitution(cmd, user_variables), " substitution failed ");
    BOOST_CHECK_MESSAGE(cmd == expected, "expected '" << expected << "' but found '" << cmd << "'");
}

static std::vector<std::string> required_server_variables() {
    std::vector<std::string> required_server_variables;
    required_server_variables.push_back(ecf::environment::ECF_PORT);
    required_server_variables.push_back(ecf::environment::ECF_HOST);

    required_server_variables.push_back(ecf::environment::ECF_HOME);
    required_server_variables.emplace_back("ECF_LOG");
    required_server_variables.emplace_back("ECF_CHECK");
    required_server_variables.emplace_back("ECF_CHECKOLD");

    // These variable are read in from the environment, but are not exposed
    // since they only affect the server
    // ECF_CHECKINTERVAL
    // ECF_LISTS

    // variables that can be overridden, in the suite definition
    required_server_variables.emplace_back("ECF_JOB_CMD");
    required_server_variables.emplace_back("ECF_KILL_CMD");
    required_server_variables.emplace_back("ECF_STATUS_CMD");
    required_server_variables.emplace_back("ECF_URL_CMD");
    required_server_variables.emplace_back("ECF_URL_BASE");
    required_server_variables.emplace_back("ECF_URL");
    required_server_variables.emplace_back("ECF_MICRO");

    // Reference variable, these should be read only
    required_server_variables.emplace_back("ECF_PID");     // server PID
    required_server_variables.emplace_back("ECF_VERSION"); // server version
    return required_server_variables;
}

BOOST_AUTO_TEST_CASE(test_server_variable_substitution) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");

    std::vector<std::string> vec = required_server_variables();
    for (const auto& i : vec) {
        if (i == "ECF_PID") {
            continue; // CANT test since, this is process ID of server
        }
        std::string value;
        BOOST_CHECK_MESSAGE(s->findParentVariableValue(i, value), "Could not find Server variable " << i);
        BOOST_CHECK_MESSAGE(!value.empty(), "Empty server variable value for " << i);
    }

    for (const auto& i : vec) {
        if (i == "ECF_JOB_CMD") {
            continue; // CANT test since it requires %ECF_JOB% and %ECF_JOBOUT%
        }
        if (i == "ECF_KILL_CMD") {
            continue; // CANT test since it requires %ECF_PID%
        }
        if (i == "ECF_STATUS_CMD") {
            continue; // CANT test since it requires %ECF_RID%
        }
        if (i == "ECF_PID") {
            continue; // CANT test since, this is process ID of server
        }
        std::string cmd = "%";
        cmd += i;
        cmd += "%";
        BOOST_CHECK_MESSAGE(s->variableSubstitution(cmd), " substitution failed for " << i << " : " << cmd);
        if (i == "ECF_VERSION") {
            BOOST_CHECK_MESSAGE(cmd == Version::full(),
                                "expected '" << Version::full() << "' but found '" << cmd << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_generated_variable_substitution) {
    ECF_NAME_THIS_TEST();

    // test that if ECF_OUT is defined using %, then we perform variable substitution
    // test that if ECF_JOBOUT or ECF_JOB are specified, they take priority over the generated variables

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("PATH", "/fred/bill/joe"));
    s->addVariable(Variable::new_variable("ECF_HOME", "/ecf_home"));
    family_ptr f = s->add_family("f");
    task_ptr t   = f->add_task("t");
    t->addVariable(Variable::new_variable("ECF_OUT", "%PATH%"));
    family_ptr f1 = s->add_family("f1");
    f1->addVariable(Variable::new_variable("PATH2", "/fred/bill/joe2"));
    task_ptr t1 = f1->add_task("t1");
    t1->addVariable(Variable::new_variable("ECF_OUT", "%PATH2%"));
    task_ptr t2 = f1->add_task("t2");
    t2->addVariable(Variable::new_variable("ECF_JOBOUT", "ECFLOW-999"));
    t2->addVariable(Variable::new_variable("ECF_JOB", "ECFLOW-999"));

    // begin_all
    defs.beginAll();
    t->update_generated_variables();
    t1->update_generated_variables();
    t2->update_generated_variables();

    // cout << defs;

    std::string value;
    value.clear();
    t->findParentVariableValue(ecf::environment::ECF_JOBOUT, value);
    BOOST_CHECK_MESSAGE(value == "/fred/bill/joe/suite/f/t.0",
                        "ECF_JOBOUT expected /fred/bill/joe/suite/f/t.0, but found " << value);

    value.clear();
    t1->findParentVariableValue(ecf::environment::ECF_JOBOUT, value);
    BOOST_CHECK_MESSAGE(value == "/fred/bill/joe2/suite/f1/t1.0",
                        "ECF_JOBOUT expected /fred/bill/joe/suite/f/t.0, but found " << value);

    // ECFLOW-999  make sure that if ECF_JOBOUT or ECF_JOB are overridden, they take priority over
    // the generated variables of the same name
    std::string cmd = Ecf::JOB_CMD();
    BOOST_CHECK_MESSAGE(t2->variableSubstitution(cmd), " variableSubstitution failed for " << Ecf::JOB_CMD());

    std::string expected = "ECFLOW-999 1> ECFLOW-999 2>&1";
    BOOST_CHECK_MESSAGE(cmd == expected, "variable substitution failed expected " << expected << " but found " << cmd);
}

namespace {

using Variables = std::vector<std::pair<std::string, std::string>>;

// Substitute cmd with and without a record; check that both give the same command, and return the record
Node::SubstitutionRecord substitute_with_record(const Node& node, const std::string& cmd, bool expected_ok = true) {
    std::string without = cmd;
    BOOST_CHECK_EQUAL(node.variableSubstitution(without), expected_ok);

    std::string with = cmd;
    Node::SubstitutionRecord record;
    BOOST_CHECK_EQUAL(node.variableSubstitution(with, &record), expected_ok);
    BOOST_CHECK_EQUAL(with, without);
    return record;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_variable_substitution_records_direct_and_nested_references) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("HOST", "hpc"));
    s->addVariable(Variable::new_variable("OPTS", "-q %QUEUE%"));
    s->addVariable(Variable::new_variable("QUEUE", "nf"));

    auto record = substitute_with_record(*s, "submit %OPTS% %HOST% %HOST%");
    BOOST_CHECK(record.variables == (Variables{{"OPTS", "-q %QUEUE%"}, {"QUEUE", "nf"}, {"HOST", "hpc"}}));
    BOOST_CHECK_EQUAL(record.masked_command, "submit -q nf hpc hpc");
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_records_the_value_used_of_a_default_reference) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("QUEUE", "nf"));

    auto record = substitute_with_record(*s, "submit %QUEUE:np% %HOST:localhost%");
    BOOST_CHECK(record.variables == (Variables{{"QUEUE", "nf"}, {"HOST", "localhost"}}));
    BOOST_CHECK_EQUAL(record.masked_command, "submit nf localhost");
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_records_generated_and_alias_variables) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("ECF_HOME", "/home"));
    task_ptr t = s->add_task("t");
    defs.beginAll();
    t->update_generated_variables();

    auto record = substitute_with_record(*t, "submit %ECF_NAME% %ECF_PASS%");
    BOOST_REQUIRE_EQUAL(record.variables.size(), 2u);
    BOOST_CHECK_EQUAL(record.variables[0].first, "ECF_NAME");
    BOOST_CHECK_EQUAL(record.variables[0].second, "/suite/t");
    BOOST_CHECK_EQUAL(record.variables[1].first, "ECF_PASS");
    BOOST_CHECK_NE(record.variables[1].second, "***");

    alias_ptr a = t->add_alias_only();
    a->add_alias_variable("A:0", "zero");
    record = substitute_with_record(*a, "run %A:0%");
    BOOST_CHECK(record.variables == (Variables{{"A:0", "zero"}}));
    BOOST_CHECK_EQUAL(record.masked_command, "run zero");

    // The name of an alias variable may hold a ':', which separates the parts of a credential name as well
    a->add_alias_variable("API_TOKEN:0", "abc123");
    std::string cmd = "run %API_TOKEN:0%";
    BOOST_CHECK(a->variableSubstitution(cmd));
    BOOST_CHECK_EQUAL(cmd, "run abc123");
    record = substitute_with_record(*a, "run %API_TOKEN:0%");
    BOOST_CHECK(record.variables == (Variables{{"API_TOKEN:0", "***"}}));
    BOOST_CHECK_EQUAL(record.masked_command, "run ***");
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_records_with_double_micro) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("API_TOKEN", "abc123"));

    auto record = substitute_with_record(*s, "date +%%Y%%m && curl -H 'Bearer %API_TOKEN%'");
    BOOST_CHECK_EQUAL(record.masked_command, "date +%Y%m && curl -H 'Bearer ***'");
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_records_a_partial_substitution) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("API_TOKEN", "abc123"));

    auto record = substitute_with_record(*s, "submit %API_TOKEN% %UNDEFINED%", false);
    BOOST_CHECK_EQUAL(record.masked_command, "submit *** %UNDEFINED%");
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_masks_a_credential_variable) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("API_TOKEN", "abc123"));
    s->addVariable(Variable::new_variable("AUTH", "Bearer %API_TOKEN%"));
    s->addVariable(Variable::new_variable("DB_PASSWORD", "s3cr3t"));

    // Directly, through another variable, and as %NAME:default% with NAME defined and undefined
    auto record = substitute_with_record(*s, "submit %API_TOKEN% -H '%AUTH%' %DB_PASSWORD:x% %MY_SECRET:fallback%");
    BOOST_CHECK_EQUAL(record.masked_command, "submit *** -H 'Bearer ***' *** ***");
    BOOST_CHECK(
        record.variables ==
        (Variables{
            {"API_TOKEN", "***"}, {"AUTH", "Bearer %API_TOKEN%"}, {"DB_PASSWORD", "***"}, {"MY_SECRET", "***"}}));
}

BOOST_AUTO_TEST_CASE(test_variable_substitution_masks_only_the_substituted_credential) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("suite");
    s->addVariable(Variable::new_variable("DB_PASSWORD", "1"));
    s->addVariable(Variable::new_variable("USER", "alice"));
    s->addVariable(Variable::new_variable("JOB", "/x/t.job1"));

    // Text equal to the value of a credential is kept; the values of ECF_PASS and USER are not credentials
    auto record = substitute_with_record(*s, "submit -u %USER% --pw %DB_PASSWORD% %JOB%");
    BOOST_CHECK_EQUAL(record.masked_command, "submit -u alice --pw *** /x/t.job1");
    BOOST_CHECK(record.variables == (Variables{{"USER", "alice"}, {"DB_PASSWORD", "***"}, {"JOB", "/x/t.job1"}}));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
