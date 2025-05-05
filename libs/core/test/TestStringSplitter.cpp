/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Str.hpp"
#include "ecflow/core/StringSplitter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;
using namespace boost;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_StringSplitter)

static void
check(const std::string& line, const StringSplitter& string_splitter, const std::vector<std::string>& expected) {
    std::vector<std::string> result;
    while (!string_splitter.finished()) {
        std::string_view ref = string_splitter.next();
        result.emplace_back(ref.begin(), ref.end());
    }
    BOOST_CHECK_MESSAGE(result.size() == expected.size(),
                        "expected size " << expected.size() << " but found " << result.size() << " for '" << line
                                         << "'");
    BOOST_CHECK_MESSAGE(result == expected, "failed for '" << line << "'");
    if (result != expected) {
        ECF_TEST_DBG(<< "Line    :'" << line);
        ECF_TEST_DBG(<< "Actual  :");
        for (const std::string& t : result) {
            ECF_TEST_DBG(<< "   '" << t << "'");
        }
        ECF_TEST_DBG(<< "Expected:");
        for (const std::string& t : expected) {
            ECF_TEST_DBG(<< "'" << t << "'");
        }
    }
}

static void check(const std::string& line, const std::vector<std::string>& expected, const char* delims = " \t") {
    std::vector<std::string> result;
    std::vector<std::string_view> result2;
    StringSplitter::split2(line, result2, delims);
    std::transform(result2.begin(), result2.end(), std::back_inserter(result), [](const std::string_view& sv) {
        return std::string(sv.begin(), sv.end());
    });

    BOOST_CHECK_MESSAGE(result.size() == expected.size(),
                        "expected size " << expected.size() << " but found " << result.size() << " for '" << line
                                         << "'");
    BOOST_CHECK_MESSAGE(result == expected, "failed for '" << line << "'");
    if (result != expected) {
        ECF_TEST_DBG(<< "Line    :'" << line);
        ECF_TEST_DBG(<< "Actual  :");
        for (const std::string& t : result) {
            ECF_TEST_DBG(<< "   '" << t << "'");
        }
        ECF_TEST_DBG(<< "Expected:");
        for (const std::string& t : expected) {
            ECF_TEST_DBG(<< "'" << t << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_StringSplitter) {
    ECF_NAME_THIS_TEST();

    std::string line = "This is a string please split me";
    std::vector<std::string> expected;

    expected.emplace_back("This");
    expected.emplace_back("is");
    expected.emplace_back("a");
    expected.emplace_back("string");
    expected.emplace_back("please");
    expected.emplace_back("split");
    expected.emplace_back("me");
    StringSplitter string_splitter(line);
    check(line, string_splitter, expected);
    check(line, expected);

    // reset
    string_splitter.reset();
    check(line, string_splitter, expected);
}

BOOST_AUTO_TEST_CASE(test_str_split_StringSplitter) {
    ECF_NAME_THIS_TEST();

    // If end is delimeter, then preserved as empty token

    std::string line = "This is a string";
    std::vector<std::string> expected;

    expected.emplace_back("This");
    expected.emplace_back("is");
    expected.emplace_back("a");
    expected.emplace_back("string");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter("This is a string"), expected);
    check(line, expected);

    expected.clear();
    line = "";
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter(""), expected);
    check(line, expected);

    expected.clear();
    line = "  ";
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter("  "), expected);
    check(line, expected);

    expected.clear();
    line = "a";
    expected.emplace_back("a");
    check(line, StringSplitter(line), expected);
    check(line, expected);

    // Some implementation fail this test
    expected.clear();
    line = "\n";
    expected.emplace_back("\n");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter("\n"), expected);
    check(line, expected);

    expected.clear();
    line = "a ";
    expected.emplace_back("a");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter("a "), expected);
    check(line, expected);

    expected.clear();
    expected.emplace_back("a");
    line = " a";
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter(" a"), expected);
    check(line, expected);

    expected.clear();
    line = "  a  "; // check sequential tabs
    expected.emplace_back("a");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter("  a  "), expected);
    check(line, expected);

    expected.clear();
    line = " a ";
    expected.emplace_back("a");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter(" a "), expected);
    check(line, expected);

    expected.clear();
    line = "        a     b     c       d        ";
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("c");
    expected.emplace_back("d");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter("        a     b     c       d        "), expected);
    check(line, expected);

    expected.clear();
    line = " - !   $ % ^  & * ( ) - + ? ";
    expected.emplace_back("-");
    expected.emplace_back("!");
    expected.emplace_back("$");
    expected.emplace_back("%");
    expected.emplace_back("^");
    expected.emplace_back("&");
    expected.emplace_back("*");
    expected.emplace_back("(");
    expected.emplace_back(")");
    expected.emplace_back("-");
    expected.emplace_back("+");
    expected.emplace_back("?");
    check(line, StringSplitter(line), expected);
    check(line, StringSplitter(" - !   $ % ^  & * ( ) - + ? "), expected);
    check(line, expected);

    // Check tabs
    expected.clear();
    line = "     verify complete:8                      # 4 sundays in october hence expect 8 task completions";
    expected.emplace_back("verify");
    expected.emplace_back("complete:8");
    expected.emplace_back("#");
    expected.emplace_back("4");
    expected.emplace_back("sundays");
    expected.emplace_back("in");
    expected.emplace_back("october");
    expected.emplace_back("hence");
    expected.emplace_back("expect");
    expected.emplace_back("8");
    expected.emplace_back("task");
    expected.emplace_back("completions");
    check(line, StringSplitter(line), expected);
    check(line,
          StringSplitter(
              "     verify complete:8                      # 4 sundays in october hence expect 8 task completions"),
          expected);
    check(line, expected);

    // Check paths
    expected.clear();
    expected.emplace_back("a");
    line = "/a";
    check(line, StringSplitter(line, "/"), expected);
    check(line, StringSplitter("/a", "/"), expected);
    check(line, expected, "/");

    expected.clear();
    line = "";
    check(line, StringSplitter(line, "/"), expected);
    check(line, StringSplitter("", "/"), expected);
    check(line, expected, "/");

    expected.clear();
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("c");
    expected.emplace_back("c");
    expected.emplace_back("e");
    line = "/a/b/c/c//e";
    check(line, StringSplitter(line, "/"), expected);
    check(line, expected, "/");

    expected.clear();
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("c");
    expected.emplace_back("c");
    expected.emplace_back("e");
    line = "///a/b/c/c//e";
    check(line, StringSplitter(line, "/"), expected);
    check(line, StringSplitter("///a/b/c/c//e", "/"), expected);
    check(line, expected, "/");

    expected.clear();
    expected.emplace_back("a");
    expected.emplace_back("b");
    expected.emplace_back("c");
    expected.emplace_back("c");
    expected.emplace_back("e");
    line = "//a/b/c/c//e/";
    check(line, StringSplitter(line, "/"), expected);
    check(line, StringSplitter("//a/b/c/c//e/", "/"), expected);
    check(line, expected, "/");

    expected.clear();
    expected.emplace_back("a ");
    expected.emplace_back("b");
    expected.emplace_back("c");
    expected.emplace_back("c e");
    line = "/a /b/c/c e";
    check(line, StringSplitter(line, "/"), expected);
    check(line, StringSplitter("/a /b/c/c e", "/"), expected);
    check(line, expected, "/");
}

static void test_get_token(const std::string& line, const char* delims = " \t") {
    std::vector<std::string> tokens;
    Str::split_orig(line, tokens, delims);
    for (size_t i = 0; i < tokens.size(); i++) {
        std::string token;
        BOOST_CHECK_MESSAGE(StringSplitter::get_token(line, i, token, delims) && token == tokens[i],
                            "Expected to find " << tokens[i] << " but found " << token);
    }
    std::string token;
    BOOST_CHECK_MESSAGE(!StringSplitter::get_token(line, tokens.size(), token), "Expected to fail");
}

BOOST_AUTO_TEST_CASE(test_StringSplitter_get_token) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> test_data = {"This is a string",
                                          "a",
                                          " a",
                                          "a ",
                                          " a ",
                                          "        a     b     c       d        ",
                                          " - !   $ % ^  & * ( ) - + ? ",
                                          "\n"};

    std::vector<std::string> test_data1 = {
        "/a",
        "///a/b/c/c//e",
        "//a/b/c/c//e/",
    };

    for (const auto& s : test_data) {
        test_get_token(s);
    }
    for (const auto& s : test_data1) {
        test_get_token(s, "/");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
