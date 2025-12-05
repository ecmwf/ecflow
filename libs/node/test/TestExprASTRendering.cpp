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
#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/Defs.hpp"
#include "ecflow/node/ExprAst.hpp"
#include "ecflow/node/ExprParser.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_ExprAST)

BOOST_AUTO_TEST_CASE(test_expression_ast_rendering) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace std::string_literals;

    Family f("f");
    auto t1 = f.add_task("t1");
    t1->add_meter("meter", 0, 100, 50, 0);
    t1->add_variable("YMD", "20260101");

    auto t2 = f.add_task("t2");
    t2->add_trigger("t1 eq complete");

    {
        auto ctx = ecf::Context::make_for(PrintStyle::DEFS);
        auto ast = t2->triggerAst();
        std::string actual;
        ecf::write_t(actual, *ast, ctx);

        std::string expected = R"(  # Trigger Evaluation Tree
    # EQUAL (false)
      # NODE node(?not-found?) t1 unknown(0) # check suite filter
      # NODE_STATE complete(1)
)"s;

        BOOST_CHECK_MESSAGE(actual == expected,
                            "AST rendering of expression match expected output.\nExpected:\n<"
                                << expected << ">\nActual:\n<" << actual << ">");
        BOOST_CHECK_MESSAGE(actual.size() == expected.size(),
                            "AST rendering of expression match expected size.\nExpected size: "
                                << expected.size() << "\nActual size: " << actual.size());
    }

    auto t3 = f.add_task("t3");
    t3->add_trigger("t1:meter > 51");

    {
        auto ctx = ecf::Context::make_for(PrintStyle::DEFS);
        auto ast = t3->triggerAst();
        std::string actual;
        ecf::write_t(actual, *ast, ctx);

        std::string expected = R"(  # Trigger Evaluation Tree
    # GREATER_THAN (false)
      # t1:meter node(?not-found?) t1 value(0) # check suite filter
      # INTEGER 51
)"s;

        BOOST_CHECK_MESSAGE(actual == expected,
                            "AST rendering of expression match expected output.\nExpected:\n"
                                << expected << "<END>\nActual:\n"
                                << actual << "<END>");
        BOOST_CHECK_MESSAGE(actual.size() == expected.size(),
                            "AST rendering of expression match expected size.\nExpected size: "
                                << expected.size() << "\nActual size: " << actual.size());
    }

    auto t4 = f.add_task("t4");
    t4->add_trigger("t1:meter > 51 AND (t1 eq complete OR t2 eq complete) AND (t3 eq complete)");

    {
        auto ctx = ecf::Context::make_for(PrintStyle::DEFS);
        auto ast = t4->triggerAst();
        std::string actual;
        ecf::write_t(actual, *ast, ctx);

        std::string expected = R"(  # Trigger Evaluation Tree
    # AND (false)
      # GREATER_THAN (false)
        # t1:meter node(?not-found?) t1 value(0) # check suite filter
        # INTEGER 51
      # AND (false)
        # OR (false)
          # EQUAL (false)
            # NODE node(?not-found?) t1 unknown(0) # check suite filter
            # NODE_STATE complete(1)
          # EQUAL (false)
            # NODE node(?not-found?) t2 unknown(0) # check suite filter
            # NODE_STATE complete(1)
        # EQUAL (false)
          # NODE node(?not-found?) t3 unknown(0) # check suite filter
          # NODE_STATE complete(1)
)"s;

        BOOST_CHECK_MESSAGE(actual == expected,
                            "AST rendering of expression did not match expected output.\nExpected:\n"
                                << expected << "<END>\nActual:\n"
                                << actual << "<END>");
        BOOST_CHECK_MESSAGE(actual.size() == expected.size(),
                            "AST rendering of expression did not match expected size.\nExpected size: "
                                << expected.size() << "\nActual size: " << actual.size());
    }

    auto t5 = f.add_task("t5");
    t5->add_variable("YMD", "20260101");
    t5->add_trigger("t2 eq complete AND :YMD le t1:YMD");

    {
        auto ctx = ecf::Context::make_for(PrintStyle::DEFS);
        auto ast = t5->triggerAst();
        std::string actual;
        ecf::write_t(actual, *ast, ctx);

        std::string expected = R"(  # Trigger Evaluation Tree
    # AND (false)
      # EQUAL (false)
        # NODE node(?not-found?) t2 unknown(0) # check suite filter
        # NODE_STATE complete(1)
      # LESS_EQUAL (false)
        # :YMD node(t5) USER-VARIABLE value(20260101)
        # t1:YMD node(?not-found?) t1 value(0) # check suite filter
)"s;

        BOOST_CHECK_MESSAGE(actual == expected,
                            "AST rendering of expression did not match expected output.\nExpected:\n"
                                << expected << "<END>\nActual:\n"
                                << actual << "<END>");
        BOOST_CHECK_MESSAGE(actual.size() == expected.size(),
                            "AST rendering of expression did not match expected size.\nExpected size: "
                                << expected.size() << "\nActual size: " << actual.size());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
