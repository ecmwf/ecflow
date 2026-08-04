/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm>
#include <deque>
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
        auto ctx = ecf::FormatContext::make_for(PrintStyle::DEFS);
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
        auto ctx = ecf::FormatContext::make_for(PrintStyle::DEFS);
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
        auto ctx = ecf::FormatContext::make_for(PrintStyle::DEFS);
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
        auto ctx = ecf::FormatContext::make_for(PrintStyle::DEFS);
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

BOOST_AUTO_TEST_CASE(test_format_indentation_does_not_overflow) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    // The indentation counter must grow monotonically, and remain bounded, well beyond 128 levels.
    // The critical iteration is the 128th: an 8-bit signed counter wraps to -128 at that point, and
    // the negative product is converted to the unsigned return type, yielding roughly 4 billion.

    constexpr int levels = 1000;

    auto ctx = ecf::FormatContext::make_for(PrintStyle::DEFS);

    // Indent is a scope guard, and decreases the indentation when destroyed. The elements are held in a
    // std::deque because, unlike std::vector, growing it neither relocates nor destroys the existing
    // elements; a std::vector would fire the destructor of every element it relocates while reallocating.
    std::deque<Indent> indents;
    for (int level = 1; level <= levels; ++level) {
        indents.emplace_back(ctx);
        BOOST_REQUIRE_EQUAL(ctx.format.indentation_spaces(), static_cast<uint32_t>(2 * level));
    }

    // Unwinding the indentation restores the initial level.
    indents.clear();
    BOOST_CHECK_EQUAL(ctx.format.indentation_spaces(), static_cast<uint32_t>(0));
}

BOOST_AUTO_TEST_CASE(test_expression_ast_rendering_with_deep_tree) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    // A chain of 'and' terms parses into a chain of AstAnd nodes, one nesting level per term.
    // Rendering such a tree used to overflow the 8-bit indentation counter beyond depth 127,
    // and request an allocation of approximately 4 GB for the leading whitespace of a single line.

    constexpr size_t terms = 200;

    Family f("f");

    std::string expression;
    for (size_t i = 0; i < terms; ++i) {
        auto name = "t" + std::to_string(i);
        f.add_task(name);
        if (i > 0) {
            expression += " and ";
        }
        expression += name + " == complete";
    }

    auto t = f.add_task("deep");
    t->add_trigger(expression);

    auto ast = t->triggerAst();
    BOOST_REQUIRE(ast != nullptr);

    auto ctx = ecf::FormatContext::make_for(PrintStyle::DEFS);

    std::string actual;
    BOOST_REQUIRE_NO_THROW(ecf::write_t(actual, *ast, ctx));

    // The tree is rendered in full: a header line, one line per operator, and one line per operand.
    // A chain of N terms contributes N-1 'and' operators, N comparison operators, and 2N operands.
    BOOST_CHECK_EQUAL(std::count(std::begin(actual), std::end(actual), '\n'), static_cast<long>(4 * terms));

    // No line is indented beyond the depth of the tree. The deepest operands sit two levels below
    // the innermost 'and', which itself sits one level below the header.
    size_t deepest = 0;
    for (size_t begin = 0; begin < actual.size();) {
        auto end    = actual.find('\n', begin);
        auto indent = actual.find_first_not_of(' ', begin) - begin;
        deepest     = std::max(deepest, indent);
        begin       = (end == std::string::npos) ? actual.size() : end + 1;
    }
    BOOST_CHECK_LE(deepest, 2 * (terms + 3));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
