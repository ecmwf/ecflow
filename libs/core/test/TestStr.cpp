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
#include <functional>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/StringSplitter.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Str)

BOOST_AUTO_TEST_CASE(test_algorithm_join) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::vector<std::string> input;
        std::string delimiter;
        std::string expected;
    };

    std::vector<tc> test_cases = {
        {{}, ",", ""},
        {{"a", "b", "c"}, "", "abc"},
        {{"a"}, ",", "a"},
        {{"a", "b"}, ",", "a,b"},
        {{"a", "b", "c"}, ",", "a,b,c"},
        {{"a", "b", "c"}, ";;", "a;;b;;c"},
    };

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::join(tc.input, tc.delimiter);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_replace_first) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string search;
        std::string replace;
        std::string expected;
    };

    std::vector<tc> test_cases = {
        {"abc def abc def abc", " ", "", "abcdef abc def abc"},
        {"abc def abc def abc", "a", "x", "xbc def abc def abc"},
        {"abc def abc def abc", "b", "x", "axc def abc def abc"},
        {"abc def abc def abc", "c", "x", "abx def abc def abc"},
        {"abc def abc def abc", "d", "x", "abc xef abc def abc"},
        {"abc def abc def abc", "abc", "xxx", "xxx def abc def abc"},
    };

    for (const auto& tc : test_cases) {
        auto actual = tc.input;
        ecf::algorithm::replace_first(actual, tc.search, tc.replace);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_split_at) {
    ECF_NAME_THIS_TEST();
    struct tc
    {
        std::string input;
        std::vector<std::string> expected;
    };

    std::vector<tc> test_cases = {
        {"", {}},
        {"  ", {}},
        {"a", {"a"}},
        {" a", {"a"}},
        {"a ", {"a"}},
        {" a ", {"a"}},
        {"\ta", {"a"}},
        {"a\t", {"a"}},
        {"\ta\t", {"a"}},
        {"\n", {"\n"}},
        {" \t a\t \t", {"a"}},
        {"\t\t\ta\t\t\t", {"a"}},
        {"\n", {"\n"}},
        {"        a     b     c       d        ", {"a", "b", "c", "d"}},
        {" - !   $ % ^ & * ( ) - + ?", {"-", "!", "$", "%", "^", "&", "*", "(", ")", "-", "+", "?"}},
        {"This is a string", {"This", "is", "a", "string"}},
        {"		 verify complete:8	       # 4 sundays in october hence expect 8 task completions",
         {"verify",
          "complete:8",
          "#",
          "4",
          "sundays",
          "in",
          "october",
          "hence",
          "expect",
          "8",
          "task",
          "completions"}}};

    for (const auto& tc : test_cases) {
        std::vector<std::string> result;
        ecf::algorithm::split_at(result, tc.input);
        BOOST_CHECK_MESSAGE(result == tc.expected,
                            "Expected:\n"
                                << ecf::algorithm::as_string(tc.expected) << "\nbut found:\n"
                                << ecf::algorithm::as_string(result));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_split_at_with_separators) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string separator;
        std::vector<std::string> expected;
    };

    std::vector<tc> test_cases = {{"", ",", {}},
                                  {"abc,def,ghi", ",", {"abc", "def", "ghi"}},
                                  {"abc,,def,ghi", ",", {"abc", "def", "ghi"}},
                                  {",abc,def,ghi", ",", {"abc", "def", "ghi"}},
                                  {"abc,def,ghi,", ",", {"abc", "def", "ghi"}},
                                  {"xxx;,;yyy;,;zzz", ";,", {"xxx", "yyy", "zzz"}},
                                  {"xxx;a,b;yyy;c,d;zzz", ";,", {"xxx", "a", "b", "yyy", "c", "d", "zzz"}},
                                  {"aeiou,12345 12345,aeiou", " ", {"aeiou,12345", "12345,aeiou"}},
                                  {"a,,b,c;d; ;e;;;f;", ",,;", {"a", "b", "c", "d", " ", "e", "f"}}};

    for (const auto& tc : test_cases) {
        std::vector<std::string> splits{"this", "is", "garbage"};

        const auto& actual = ecf::algorithm::split_at(splits, tc.input, tc.separator);

        BOOST_CHECK_MESSAGE(actual == splits, "Expected the returned value and the input vector to be the same object");
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Splitting >>>" << tc.input << "<<<, Expected " << ecf::algorithm::as_string(tc.expected)
                                            << ", found " << ecf::algorithm::as_string(actual));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_split_by_with_separators) {

    struct TestCase
    {
        std::string input;
        std::string pattern;
        std::vector<std::string> expected;
    };

    std::vector<TestCase> testCases = {
        {"This is a string", "...", {"This is a string"}},
        {"This...is...a...string", "...", {"This", "is", "a", "string"}},
        {"This...is...a...string...", "...", {"This", "is", "a", "string"}},
        {"...This...is...a...string", "...", {"This", "is", "a", "string"}},
        {"...This...is...a...string...", "...", {"This", "is", "a", "string"}},
        {"......This......is......a......string......", "...", {"This", "is", "a", "string"}},
        {"..This.....is.....a.....string.....", "...", {"..This", "..is", "..a", "..string", ".."}},
        {"expression 1==expression 2", "==", {"expression 1", "expression 2"}},
        {"expression 1==expression 2==expression 3", "==", {"expression 1", "expression 2", "expression 3"}},
        {"expression 1 == expression 2", " == ", {"expression 1", "expression 2"}},
        {"expression 1 == expression 2 == expression 3", " == ", {"expression 1", "expression 2", "expression 3"}},
        {"expression 1 eq expression 2", " eq ", {"expression 1", "expression 2"}},
        {"expression 1eqexpression 2", " eq ", {"expression 1eqexpression 2"}}};

    for (const auto& testCase : testCases) {
        std::vector<std::string> result;
        ecf::algorithm::split_by(result, testCase.input, testCase.pattern);
        BOOST_CHECK_MESSAGE(result == testCase.expected,
                            "Failed for input: '" << testCase.input << "' pattern: '" << testCase.pattern
                                                  << "'. Expected: " << ecf::algorithm::as_string(testCase.expected)
                                                  << " but found: " << ecf::algorithm::as_string(result));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_split_within_quotes) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string quotes;
        std::vector<std::string_view> expected;
    };

    std::vector<tc> test_cases = {{"", "'", {}},
                                  {"''", "'", {"''"}},
                                  {"''  ' '", "'", {"''", "' '"}},
                                  {"'a'  'b' ''", "'", {"'a'", "'b'", "''"}},
                                  {"'abc'  'def' 'ghi'", "'", {"'abc'", "'def'", "'ghi'"}},
                                  {"'a'  'b' '", "'", {"'a'", "'b'", "'"}},
                                  {"'a'  b' '", "'", {"'a'", "' '"}},
                                  {"'a'  x  'b' y 'c'  zz  ", "'", {"'a'", "x", "'b'", "y", "'c'", "zz"}},
                                  {"\"a\" x \"b\" x \"c\"", "\"", {"\"a\"", "x", "\"b\"", "x", "\"c\""}},
                                  {"'a \" b \" c' \"d ' e ' f\"", "\"'", {"'a \" b \" c'", "\"d ' e ' f\""}}};

    for (const auto& tc : test_cases) {
        const auto& actual = ecf::algorithm::split_within_quotes(tc.input, tc.quotes);

        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Splitting >>>" << tc.input << "<<<, Expected " << ecf::algorithm::as_string(tc.expected)
                                            << ", found " << ecf::algorithm::as_string(actual));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_starts_with) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string prefix;
        bool expected;
    };

    std::vector<tc> test_cases = {{"", "", true},
                                  {"abcdef", "", true},
                                  {"abcdef", "a", true},
                                  {"abcdef", "ab", true},
                                  {"abcdef", "abc", true},
                                  {"abcdef", "abcx", false},
                                  {"abcdef", "abcdefx", false},
                                  {"zabcdef", "abcdef", false}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::starts_with(tc.input, tc.prefix);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Expected " << ecf::algorithm::as_string(tc.expected) << ", found "
                                        << ecf::algorithm::as_string(actual));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_ends_with) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string prefix;
        bool expected;
    };

    std::vector<tc> test_cases = {{"", "", true},
                                  {"abcdef", "", true},
                                  {"abcdef", "f", true},
                                  {"abcdef", "ef", true},
                                  {"abcdef", "def", true},
                                  {"abcdef", "xdef", false},
                                  {"abcdef", "xabcdef", false},
                                  {"abcdefz", "abcdef", false}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::ends_with(tc.input, tc.prefix);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Expected " << ecf::algorithm::as_string(tc.expected) << ", found "
                                        << ecf::algorithm::as_string(actual));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_trim) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string expected_trim_both;
        std::string expected_trim_leading;
        std::string expected_trim_trailing;
    };

    std::vector<tc> test_cases = {
        {"", "", "", ""},
        {" ", "", "", ""},
        {"  ", "", "", ""},
        {"abc", "abc", "abc", "abc"},
        {" abc", "abc", "abc", " abc"},
        {"abc ", "abc", "abc ", "abc"},
        {" abc ", "abc", "abc ", " abc"},
        {"\tabc\t", "abc", "abc\t", "\tabc"},
        {"\nabc\n", "abc", "abc\n", "\nabc"},
        {" \t\nabc \t\n", "abc", "abc \t\n", " \t\nabc"},
        {" \t\n abc \t\n ", "abc", "abc \t\n ", " \t\n abc"},
    };

    for (const auto& tc : test_cases) {
        auto actual = tc.input;
        ecf::algorithm::trim(actual);
        auto expected = tc.expected_trim_both;
        BOOST_CHECK_MESSAGE(actual == expected, "Expected '" << expected << "', found '" << actual << "'");
    }

    for (const auto& tc : test_cases) {
        auto actual = tc.input;
        ecf::algorithm::trim_leading(actual);
        auto expected = tc.expected_trim_leading;
        BOOST_CHECK_MESSAGE(actual == expected, "Expected '" << expected << "', found '" << actual << "'");
    }

    for (const auto& tc : test_cases) {
        auto actual   = ecf::algorithm::trim_leading_copy(tc.input);
        auto expected = tc.expected_trim_leading;
        BOOST_CHECK_MESSAGE(actual == expected, "Expected '" << expected << "', found '" << actual << "'");
    }

    for (const auto& tc : test_cases) {
        auto actual = tc.input;
        ecf::algorithm::trim_trailing(actual);
        auto expected = tc.expected_trim_trailing;
        BOOST_CHECK_MESSAGE(actual == expected, "Expected '" << expected << "', found '" << actual << "'");
    }

    for (const auto& tc : test_cases) {
        auto actual   = ecf::algorithm::trim_trailing_copy(tc.input);
        auto expected = tc.expected_trim_trailing;
        BOOST_CHECK_MESSAGE(actual == expected, "Expected '" << expected << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_contains) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string pattern;
        bool expected;
    };

    std::vector<tc> test_cases = {{"", "", true},
                                  {"abcdef", "", true},
                                  {"abcdef", "a", true},
                                  {"abcdef", "ab", true},
                                  {"abcdef", "abc", true},
                                  {"abcdef", "abcx", false},
                                  {"abcdef", "abcdefx", false},
                                  {"zabcdef", "abcdef", true}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::contains(tc.input, tc.pattern);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Expected " << ecf::algorithm::as_string(tc.expected) << ", found "
                                        << ecf::algorithm::as_string(actual));
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_remove_all) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string pattern;
        std::string expected;
    };

    std::vector<tc> test_cases = {{"", "", ""},
                                  {"abcdef", "", "abcdef"},
                                  {"abcdef", "a", "bcdef"},
                                  {"abcdef", "ab", "cdef"},
                                  {"abcdef", "abc", "def"},
                                  {"abcdef", "abcd", "ef"},
                                  {"abcdef", "abcx", "abcdef"},
                                  {"abcdef", "abcdefx", "abcdef"},
                                  {"zabcdef", "abcdef", "z"},
                                  {"abcxyzdef", "xyz", "abcdef"},
                                  {"xyzabcdef", "xyz", "abcdef"},
                                  {"abcxyzdef", "xyz", "abcdef"},
                                  {"xaxbxcxdxexfx", "x", "abcdef"},
                                  {"xyzaxyzbxyzcxyzdxyzexyzfxyz", "xyz", "abcdef"}};

    for (const auto& tc : test_cases) {
        auto actual = tc.input;
        ecf::algorithm::remove_all(actual, tc.pattern);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected " << tc.expected << ", found " << actual);
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_transform_to_vector) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::algorithm;

    {
        std::vector<int> input    = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
        std::vector<int> expected = {1, 4, 9, 16, 25, 36, 49, 64, 81, 0};

        auto actual = ecf::algorithm::transform_to_vector(input, [](int v) { return v * v; });
        BOOST_CHECK_MESSAGE(actual == expected,
                            "Expected '" << ecf::algorithm::as_string(expected) << "', found '"
                                         << ecf::algorithm::as_string(actual) << "'");
    }

    {
        std::vector<int> input            = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
        std::vector<std::string> expected = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

        auto actual = ecf::algorithm::transform_to_vector(input, [](int v) { return std::to_string(v); });
        BOOST_CHECK_MESSAGE(actual == expected,
                            "Expected '" << ecf::algorithm::as_string(expected) << "', found '"
                                         << ecf::algorithm::as_string(actual) << "'");
    }

    {
        std::vector<std::string> input = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
        std::vector<int> expected      = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};

        auto actual = ecf::algorithm::transform_to_vector(input, [](const std::string& v) { return std::stoi(v); });
        BOOST_CHECK_MESSAGE(actual == expected,
                            "Expected '" << ecf::algorithm::as_string(expected) << "', found '"
                                         << ecf::algorithm::as_string(actual) << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_tolower) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string expected;
    };

    std::vector<tc> test_cases = {{"", ""},
                                  {"a", "a"},
                                  {"A", "a"},
                                  {"a1", "a1"},
                                  {"A1", "a1"},
                                  {"abc", "abc"},
                                  {"ABC", "abc"},
                                  {"AbC", "abc"},
                                  {"aBc", "abc"},
                                  {"aBC", "abc"},
                                  {"Abc", "abc"},
                                  {"abC", "abc"},
                                  {"123", "123"},
                                  {"!@#$%^&*()_+", "!@#$%^&*()_+"},
                                  {"a1B2c3D4e5F6g7H8i9J0", "a1b2c3d4e5f6g7h8i9j0"}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::tolower(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_is_valid_name) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        bool expected;
    };

    std::vector<tc> test_cases = {
        {"", false},
        {"a", true},
        {"abc", true},
        {"1abc", true},
        {"abc1", true},
        {"a1b2c3", true},
        {"a1c", true},
        {"_abc", true},     // '_' is an allowed character as the first character of a name
        {"a_b_c", true},    // '_' is an allowed character as the first character of a name
        {"-abc", false},    // '-' is not an allowed character
        {"a-b-c", false},   // '-' is not an allowed character
        {".abc", false},    // '.' is an allowed character but not as the first character of a name'
        {"a.b.c", true},    // '.' is an allowed character but not as the first character of a name'
        {"a_b-c.d", false}, // '-' is not an allowed character
    };

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::is_valid_name(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Validating '" << tc.input << "', Expected '" << tc.expected << "', found '" << actual
                                           << "'");
    }

    for (const auto& tc : test_cases) {
        std::string error;
        auto actual = ecf::algorithm::is_valid_name(tc.input, error);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
        if (tc.expected) {
            BOOST_CHECK_MESSAGE(error.empty(), "Expected empty error, found '" << error << "'");
        }
        else {
            BOOST_CHECK_MESSAGE(!error.empty(), "Expected non-empty error, found '" << error << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_remove_double_quotes) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string expected;
    };

    const std::vector<tc> test_cases = {
        {"", ""},
        {"\"\"", ""},
        {"\"", "\""},
        {"''", "''"},
        {"'", "'"},
        {"abc", "abc"},
        {"\"abc\"", "abc"},
        {"\"abc", "\"abc"},
        {"abc\"", "abc\""},
        {"'abc", "'abc"},
        {"abc'", "abc'"},
        {"'abc'", "'abc'"},
    };

    for (const auto& tc : test_cases) {
        {
            std::string actual = tc.input;
            ecf::algorithm::remove_double_quotes(actual);
            BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
        }
        {
            std::string actual = ecf::algorithm::remove_double_quotes_copy(tc.input);
            BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_remove_single_quotes) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string expected;
    };

    std::vector<tc> test_cases = {
        {"", ""},
        {"\"\"", "\"\""},
        {"\"", "\""},
        {"''", ""},
        {"'", "'"},
        {"abc", "abc"},
        {"\"abc\"", "\"abc\""},
        {"\"abc", "\"abc"},
        {"abc\"", "abc\""},
        {"'abc", "'abc"},
        {"abc'", "abc'"},
        {"'abc'", "abc"},
    };

    for (const auto& tc : test_cases) {
        {
            std::string actual = tc.input;
            ecf::algorithm::remove_single_quotes(actual);
            BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
        }
        {
            std::string actual = ecf::algorithm::remove_single_quotes_copy(tc.input);
            BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_case_insensitive_compare) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string lhs;
        std::string rhs;
        bool expected;
    };

    std::vector<tc> test_cases = {{"", "", true},
                                  {"a", "a", true},
                                  {"a", "A", true},
                                  {"A", "a", true},
                                  {"abc", "abc", true},
                                  {"abc", "ABC", true},
                                  {"Abc", "aBC", true},
                                  {"Str", "Str", true},
                                  {"Str", "Str1", false},
                                  {"", "Str1", false},
                                  {"Str", "STR", true},
                                  {"Case", "CaSE", true}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::case_insensitive_compare(tc.lhs, tc.rhs);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_case_insensitive_less) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string lhs;
        std::string rhs;
        bool expected;
    };

    std::vector<tc> test_cases = {{"", "", false},
                                  {"a", "a", false},
                                  {"a", "A", true},
                                  {"A", "a", false},
                                  {"abc", "abc", false},
                                  {"abc", "ABC", true},
                                  {"Abc", "aBC", false},
                                  {"Str", "Str", false},
                                  {"Str", "Str1", true},
                                  {"", "Str1", true},
                                  {"Str", "STR", true},
                                  {"Case", "CaSE", true},
                                  {"abc", "abd", true},
                                  {"abd", "abc", false}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::case_insensitive_less(tc.lhs, tc.rhs);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Comparing '" << tc.lhs << "' < '" << tc.rhs << "' Expected '" << tc.expected
                                          << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_case_insensitive_greater) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string lhs;
        std::string rhs;
        bool expected;
    };

    std::vector<tc> test_cases = {{"", "", false},
                                  {"a", "a", false},
                                  {"a", "A", false},
                                  {"A", "a", true},
                                  {"abc", "abc", false},
                                  {"abc", "ABC", false},
                                  {"Abc", "aBC", true},
                                  {"Str", "Str", false},
                                  {"Str", "Str1", true},
                                  {"", "Str1", true},
                                  {"Str", "STR", false},
                                  {"Case", "CaSE", false},
                                  {"abc", "abd", false},
                                  {"abd", "abc", true}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::case_insensitive_greater(tc.lhs, tc.rhs);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Comparing '" << tc.lhs << "' > '" << tc.rhs << "' Expected '" << tc.expected
                                          << "', found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_is_int) {
    ECF_NAME_THIS_TEST();

    struct test_case
    {
        std::string input;
        bool expected;
    };

    std::vector<test_case> test_cases = {{"0", true},
                                         {"1", true},
                                         {"-0", true},
                                         {"-1", true},
                                         {"", false},
                                         {"-", false},
                                         {" ", false},
                                         {"q", false},
                                         {"q22", false},
                                         {"99 99", false},
                                         {"99\t99", false}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::is_int(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Checking '" << tc.input << "', Expected " << tc.expected << ", found " << actual);
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_to_int) {
    ECF_NAME_THIS_TEST();

    struct test_case
    {
        std::string input;
        int expected;
    };

    std::vector<test_case> test_cases = {{"0", 0},
                                         {"1", 1},
                                         {"-0", 0},
                                         {"-1", -1},
                                         {"", std::numeric_limits<int>::max()},
                                         {"-", std::numeric_limits<int>::max()},
                                         {" ", std::numeric_limits<int>::max()},
                                         {"q", std::numeric_limits<int>::max()},
                                         {"q22", std::numeric_limits<int>::max()},
                                         {"99 99", std::numeric_limits<int>::max()}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::to_int(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected,
                            "Converting '" << tc.input << "' to int, Expected " << tc.expected << ", found " << actual);
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_tail) {
    ECF_NAME_THIS_TEST();
    std::string test;
    BOOST_CHECK_MESSAGE(!ecf::algorithm::tail(test, 7), "Empty sring should return false");

    test                 = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    std::string expected = "line";
    BOOST_CHECK_MESSAGE(ecf::algorithm::tail(test, 1) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);

    test     = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    expected = "a\nstring\nwith\nlots\nof\nnew\nline";
    BOOST_CHECK_MESSAGE(ecf::algorithm::tail(test, 7) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);

    test     = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    expected = test;
    BOOST_CHECK_MESSAGE(!ecf::algorithm::tail(test, 9) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);
}

BOOST_AUTO_TEST_CASE(test_algorithm_head) {
    ECF_NAME_THIS_TEST();
    std::string test;
    BOOST_CHECK_MESSAGE(!ecf::algorithm::head(test, 7), "Empty string should return false");

    test                 = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    std::string expected = "this\n";
    BOOST_CHECK_MESSAGE(ecf::algorithm::head(test, 1) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);

    test     = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    expected = "this\nis\n";
    BOOST_CHECK_MESSAGE(ecf::algorithm::head(test, 2) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);

    test     = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    expected = "this\nis\na\nstring\nwith\nlots\nof\n";
    BOOST_CHECK_MESSAGE(ecf::algorithm::head(test, 7) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);

    test     = "this\nis\na\nstring\nwith\nlots\nof\nnew\nline";
    expected = test;
    BOOST_CHECK_MESSAGE(!ecf::algorithm::head(test, 9) && test == expected,
                        "Expected:\n"
                            << expected << "\nbut found:\n"
                            << test);
}

BOOST_AUTO_TEST_CASE(test_algorithm_as_string_for_bool) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        bool input;
        std::string expected;
    };

    std::vector<tc> test_cases = {{true, "true"}, {false, "false"}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::as_string(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    };
}

BOOST_AUTO_TEST_CASE(test_algorithm_as_string_for_vector_of_arithmetic) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::vector<int> input;
        std::string expected;
    };

    std::vector<tc> test_cases = {
        {{}, "[  ]"}, {{1}, "[ 1 ]"}, {{1, 2}, "[ 1, 2 ]"}, {{123, 456, 789}, "[ 123, 456, 789 ]"}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::as_string(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    };
}

BOOST_AUTO_TEST_CASE(test_algorithm_as_string_for_vector_of_strings) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::vector<std::string> input;
        std::string expected;
    };

    std::vector<tc> test_cases = {
        {{}, "[  ]"}, {{"a"}, "[ a ]"}, {{"a", "b"}, "[ a, b ]"}, {{"abc", "def", "ghi"}, "[ abc, def, ghi ]"}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::as_string(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    };
}

BOOST_AUTO_TEST_CASE(test_algorithm_as_string_for_vector_of_string_views) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::vector<std::string_view> input;
        std::string expected;
    };

    std::vector<tc> test_cases = {
        {{}, "[  ]"}, {{"a"}, "[ a ]"}, {{"a", "b"}, "[ a, b ]"}, {{"abc", "def", "ghi"}, "[ abc, def, ghi ]"}};

    for (const auto& tc : test_cases) {
        auto actual = ecf::algorithm::as_string(tc.input);
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "', found '" << actual << "'");
    };
}

BOOST_AUTO_TEST_CASE(test_algortihm_replace) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string find;
        std::string replace;
        std::string expected;
        bool result;
    };

    std::vector<tc> test_cases = {{"This is a string", "", "Replacement", "This is a string", false},
                                  {"This is a string", "Pattern", "Replacment", "This is a string", false},
                                  {"This is a string", "This", "That", "That is a string", true},
                                  {"This is a string", "This is a string", "", "", true},
                                  {"This is a string", "is a", "was a", "This was a string", true},
                                  {"This\n is a string", "\n", "\\n", "This\\n is a string", true}};

    for (const auto& tc : test_cases) {
        std::string actual = tc.input;

        auto result = ecf::algorithm::replace(actual, tc.find, tc.replace);

        BOOST_CHECK_MESSAGE(result == tc.result,
                            "Replace failed for " << actual << " find(" << tc.find << ") replace(" << tc.replace
                                                  << ")");
        BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "' but found '" << actual << "'");
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_replace_all) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string input;
        std::string find;
        std::string replace;
        std::string expected;
        bool result;
    };

    std::vector<tc> test_cases = {
        {"This is a string", "", "Replacement", "This is a string", false},
        {"This is a string", "Pattern", "Replacement", "This is a string", false},
        {"This is a string", "This", "That", "That is a string", true},
        {"This is a string", "This is a string", "", "", true},
        {"This is a string", "is a", "was a", "This was a string", true},
        {"This\n is a string", "\n", "\\n", "This\\n is a string", true},
        {"This\n is\n a\n string\n", "\n", "\\n", R"(This\n is\n a\n string\n)", true},
        {"This\n is\n a\n string\n", "\n", "", "This is a string", true},
    };

    for (const auto& tc : test_cases) {
        auto actual = tc.input;

        { // Check that replacement happens as expected
            bool result = ecf::algorithm::replace_all(actual, tc.find, tc.replace);

            BOOST_CHECK_MESSAGE(result == tc.result,
                                "Replace successful for " << actual << " find(" << tc.find << ") replace(" << tc.replace
                                                          << ")");
            BOOST_CHECK_MESSAGE(actual == tc.expected, "Expected '" << tc.expected << "' but found '" << actual << "'");
        }

        auto copy = actual;

        { // Ensure that attempting replacement again returns false and does not change the input string
            bool result = ecf::algorithm::replace_all(copy, tc.find, tc.replace);

            BOOST_CHECK_MESSAGE(!result,
                                "Replace unsuccessful for " << copy << " find(" << tc.find << ") replace(" << tc.replace
                                                            << "), since no occurrences were left");
            BOOST_CHECK_MESSAGE(actual == copy, "Expected '" << actual << "' but found '" << copy << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_sort_using_case_insensitive_order) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::vector<std::string> input;
        std::vector<std::string> expected_less;
        std::vector<std::string> expected_greater;
    };

    std::vector<tc> test_cases = {
        // clang-format off
        {
            {"c", "A2", "a1", "b1", "B2"},
            {"a1", "A2", "b1", "B2", "c"},
            {"c", "B2", "b1", "A2", "a1"}
        },
        {
            {"c", "B", "A", "b", "a"},
            {"a", "A", "b", "B", "c"},
            {"c", "B", "b", "A", "a"}
        },
        {
            {"suite", "SUITE", "baSE", "Base", "case", "CaSe", "1234"},
            {"1234", "baSE", "Base", "case", "CaSe", "suite", "SUITE"},
            {"SUITE", "suite", "CaSe", "case", "Base", "baSE", "1234"}
        }
        // clang-format on
    };

    for (const auto& tc : test_cases) {

        {
            auto actual = tc.input;
            std::sort(actual.begin(), actual.end(), ecf::algorithm::case_insensitive_less);
            BOOST_REQUIRE_MESSAGE(actual == tc.expected_less,
                                  "expected " << ecf::algorithm::as_string(tc.expected_less) << " but found "
                                              << ecf::algorithm::as_string(actual));
        }

        {
            auto actual = tc.input;
            std::sort(actual.begin(), actual.end(), ecf::algorithm::case_insensitive_greater);
            BOOST_REQUIRE_MESSAGE(actual == tc.expected_greater,
                                  "expected " << ecf::algorithm::as_string(tc.expected_greater) << " but found "
                                              << ecf::algorithm::as_string(actual));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_algorithm_get_token) {

    struct TestCase
    {
        std::string input;
        size_t index;
        std::string delimiters;
        bool expected_outcome;
        std::string expected_token;
    };

    std::vector<TestCase> testCases = {
        // all tokens can be accessed
        {"0,1,2,3,4,5,6,7,8,9,10", 0, ",", true, "0"},
        {"0,1,2,3,4,5,6,7,8,9,10", 1, ",", true, "1"},
        {"0,1,2,3,4,5,6,7,8,9,10", 2, ",", true, "2"},
        {"0,1,2,3,4,5,6,7,8,9,10", 3, ",", true, "3"},
        {"0,1,2,3,4,5,6,7,8,9,10", 4, ",", true, "4"},
        {"0,1,2,3,4,5,6,7,8,9,10", 5, ",", true, "5"},
        {"0,1,2,3,4,5,6,7,8,9,10", 6, ",", true, "6"},
        {"0,1,2,3,4,5,6,7,8,9,10", 7, ",", true, "7"},
        {"0,1,2,3,4,5,6,7,8,9,10", 8, ",", true, "8"},
        {"0,1,2,3,4,5,6,7,8,9,10", 9, ",", true, "9"},
        {"0,1,2,3,4,5,6,7,8,9,10", 10, ",", true, "10"},

        // out-of-range tokens are correctly handled
        {"0,1,2,3,4,5,6,7,8,9,10", 11, ",", false, ""},
        {"0,1,2,3,4,5,6,7,8,9,10", 12, ",", false, ""},

        // now using another delimiter
        {"0 1 2 3 4 5 6 7 8 9 10", 0, " ", true, "0"},
        {"0 1 2 3 4 5 6 7 8 9 10", 5, " ", true, "5"},
        {"0 1 2 3 4 5 6 7 8 9 10", 10, " ", true, "10"},
        {"0 1 2 3 4 5 6 7 8 9 10", 42, " ", false, ""},

        // now using multiple delimiters
        {"0 1\t2 3\t4 5\t6 7\t8 9\t10", 0, " \t", true, "0"},
        {"0 1\t2 3\t4 5\t6 7\t8 9\t10", 5, " \t", true, "5"},
        {"0 1\t2 3\t4 5\t6 7\t8 9\t10", 10, " \t", true, "10"},
        {"0 1\t2 3\t4 5\t6 7\t8 9\t10", 42, " \t", false, ""},
    };

    for (const auto& testCase : testCases) {
        std::string actual_token;
        auto actual_outcome =
            ecf::algorithm::get_token(testCase.input, testCase.index, actual_token, testCase.delimiters);

        BOOST_REQUIRE_MESSAGE(actual_outcome == testCase.expected_outcome,
                              "Correct output getting token from '" << testCase.input << "' at index " << testCase.index
                                                                    << " with delimiters '" << testCase.delimiters
                                                                    << "'");

        if (testCase.expected_outcome) {
            BOOST_CHECK_MESSAGE(actual_token == testCase.expected_token,
                                "Found correct token with input: '"
                                    << testCase.input << "' at index " << testCase.index << " with delimiters '"
                                    << testCase.delimiters << "'. Expected '" << testCase.expected_token << "' found '"
                                    << actual_token << "'");
        }
    }
}

BOOST_AUTO_TEST_CASE(test_performance_loop, *boost::unit_test::disabled()) {
    ECF_NAME_THIS_TEST();

    //
    // The goal of this test is to investigate the fastest looping mechanism over an std::vector, by comparing:
    //  - raw for loop
    //  - std::for_each
    //  - std::vector<T>::iterator-based iteration
    //  - index-based iteration
    //

    class Fred {
    public:
        explicit Fred(int i = 0)
            : i_(i) {
            // Do nothing...
        }
        Fred(const Fred& rhs)            = default;
        Fred& operator=(const Fred& rhs) = default;
        ~Fred()                          = default;

        void inc() { i_++; }

    private:
        int i_;
    };

    const size_t size = 200000000;
    std::vector<Fred> vec;
    vec.reserve(size);
    for (size_t i = 0; i < size; i++) {
        vec.push_back(Fred(i));
    }

    {
        ecf::PerformanceTimer timer;
        for (auto& fred : vec) {
            fred.inc();
        }
        ECF_TEST_DBG(<< "Time: for(auto &fred : vec) { fred.inc(); }                                                "
                     << timer);
    }

    {
        ecf::PerformanceTimer timer;
        std::for_each(vec.begin(), vec.end(), [](Fred& fred) { fred.inc(); });
        ECF_TEST_DBG(<< "Time: std::for_each(vec.begin(),vec.end(),[](Fred& fred) { fred.inc();} );                 "
                     << timer);
    }

    {
        ecf::PerformanceTimer timer;
        auto theEnd = vec.end();
        for (auto i = vec.begin(); i < theEnd; i++) {
            (*i).inc();
        }
        ECF_TEST_DBG(<< "Time: for (std::vector<Fred>::iterator  i = vec.begin(); i < theEnd ; i++) { (*i).inc(); } "
                     << timer);
    }

    {
        ecf::PerformanceTimer timer;
        size_t theSize = vec.size();
        for (size_t i = 0; i < theSize; i++) {
            vec[i].inc();
        }
        ECF_TEST_DBG(<< "Time: for (size_t i = 0; i < theSize ; i++) { vec[i].inc(); }                              "
                     << timer);
    }
}

BOOST_AUTO_TEST_CASE(test_performance_convert_string_to_int, *boost::unit_test::disabled()) {
    ECF_NAME_THIS_TEST();

    //
    // The goal of this test is to investigate the fastest mechanism to convert from string to int, by comparing:
    //  - methodX, using istringstream marshalling
    //  - method1, using ecf::convert_to with try/catch
    //  - method2, using ecf::convert_to with try/catch but only if the first character of the string is a numeric char
    //  - method3, using atoi (which we known to be fast but does not handle errors)
    //

    auto methodX = [](const std::string& str, std::vector<std::string>& stringRes, std::vector<int>& numberRes) {
        // 0.81
        // for bad conversion istringstream seems to return 0, hence add guard
        if (str.find_first_of(ecf::string_constants::numeric_chars, 0) == 0) {
            int number = 0;
            std::istringstream(str) >> number;
            numberRes.push_back(number);
        }
        else {
            stringRes.push_back(str);
        }
    };

    auto method1 = [](const std::string& str, std::vector<std::string>& stringRes, std::vector<int>& numberRes) {
        // 12.2
        try {
            int number = ecf::convert_to<int>(str);
            numberRes.push_back(number);
        }
        catch (const ecf::bad_conversion&) {
            stringRes.push_back(str);
        }
    };

    auto method2 = [](const std::string& str, std::vector<std::string>& stringRes, std::vector<int>& numberRes) {
        // 0.6
        if (str.find_first_of(ecf::string_constants::numeric_chars, 0) == 0) {
            try {
                int number = ecf::convert_to<int>(str);
                numberRes.push_back(number);
            }
            catch (const ecf::bad_conversion&) {
                stringRes.push_back(str);
            }
        }
        else {
            stringRes.push_back(str);
        }
    };

    auto method3 = [](const std::string& str, std::vector<std::string>& stringRes, std::vector<int>& numberRes) {
        // 0.14
        // atoi return 0 for errors,
        int number = atoi(str.c_str()); // does not handle errors
        if (number == 0 && str.size() != 1) {
            stringRes.push_back(str);
        }
        else {
            numberRes.push_back(number);
        }
    };

    size_t the_size = 1000000;
    std::vector<std::string> stringTokens;
    std::vector<std::string> numberTokens;
    std::vector<int> expectedNumberRes;
    for (size_t i = 0; i < the_size; i++) {
        stringTokens.push_back("astring");
    }
    for (size_t i = 0; i < the_size; i++) {
        numberTokens.push_back(ecf::convert_to<std::string>(i));
        expectedNumberRes.push_back(i);
    }

    std::vector<std::string> stringRes;
    stringTokens.reserve(stringTokens.size());
    std::vector<int> numberRes;
    numberRes.reserve(expectedNumberRes.size());

    {
        ecf::PerformanceTimer timer;
        for (size_t i = 0; i < stringTokens.size(); i++) {
            method1(stringTokens[i], stringRes, numberRes);
        }
        for (size_t i = 0; i < numberTokens.size(); i++) {
            method1(numberTokens[i], stringRes, numberRes);
        }
        ECF_TEST_DBG(<< "Time for method1  elapsed time = " << timer);
        BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes, " method 1 wrong");
        BOOST_CHECK_MESSAGE(stringTokens == stringRes, "method 1 wrong");
        numberRes.clear();
        stringRes.clear();
    }

    {
        ecf::PerformanceTimer timer;
        for (size_t i = 0; i < stringTokens.size(); i++) {
            methodX(stringTokens[i], stringRes, numberRes);
        }
        for (size_t i = 0; i < numberTokens.size(); i++) {
            methodX(numberTokens[i], stringRes, numberRes);
        }
        ECF_TEST_DBG(<< "Time for methodX  elapsed time = " << timer);
        BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes, " method X wrong");
        BOOST_CHECK_MESSAGE(stringTokens == stringRes, "method X wrong");
        numberRes.clear();
        stringRes.clear();
    }

    {
        ecf::PerformanceTimer timer;
        for (size_t i = 0; i < stringTokens.size(); i++) {
            method2(stringTokens[i], stringRes, numberRes);
        }
        for (size_t i = 0; i < numberTokens.size(); i++) {
            method2(numberTokens[i], stringRes, numberRes);
        }
        ECF_TEST_DBG("Time for method2  elapsed time = " << timer);
        BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes, "method 2 wrong");
        BOOST_CHECK_MESSAGE(stringTokens == stringRes, "method 2 wrong");
        numberRes.clear();
        stringRes.clear();
    }

    {
        ecf::PerformanceTimer timer;
        for (size_t i = 0; i < stringTokens.size(); i++) {
            method3(stringTokens[i], stringRes, numberRes);
        }
        for (size_t i = 0; i < numberTokens.size(); i++) {
            method3(numberTokens[i], stringRes, numberRes);
        }
        ECF_TEST_DBG(<< "Time for method3  elapsed time = " << timer);
        BOOST_CHECK_MESSAGE(numberRes == expectedNumberRes,
                            " method3 wrong numberRes.size()=" << numberRes.size()
                                                               << " expected size = " << expectedNumberRes.size());
        BOOST_CHECK_MESSAGE(stringTokens == stringRes,
                            " method3 wrong stringRes.size()=" << stringRes.size()
                                                               << " expected size = " << stringTokens.size());
        numberRes.clear();
        stringRes.clear();
    }
}

BOOST_AUTO_TEST_CASE(test_performance_convert_int_to_string, *boost::unit_test::disabled()) {
    ECF_NAME_THIS_TEST();

    //
    // The goal of this test is to investigate the fastest mechanism to convert from string to int, by comparing:
    //  - ostream = 0.97
    //  - lexical_cast = 0.45
    //

    const int the_size = 1000000;

    {
        ecf::PerformanceTimer timer;
        for (size_t i = 0; i < the_size; i++) {
            std::ostringstream st;
            st << i;
            [[maybe_unused]] std::string s = st.str();
        }
        ECF_TEST_DBG("Time for int to string using ostringstream  elapsed time = " << timer);
    }

    {
        ecf::PerformanceTimer timer;
        for (size_t i = 0; i < the_size; i++) {
            [[maybe_unused]] std::string s = ecf::convert_to<std::string>(i);
        }
        ECF_TEST_DBG("Time for int to string using ecf::convert_to elapsed time = " << timer);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
