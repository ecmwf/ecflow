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

#include "ecflow/core/File.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_VariableParsing)

BOOST_AUTO_TEST_CASE(test_single_defs) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_defs/edit/edit.def", "parser");

    Defs defs;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(defs.restore(path, errorMsg, warningMsg), errorMsg);

    // suite edit
    //     edit ECF_INCLUDE /home/ma/map/sms/example/x              # comment line
    //     edit ECF_FILES   /home/ma/map/sms/example/x              #comment line
    //     edit EXPVER 'f8na'                                       #
    //     edit USER 'ecgems'                                       #comment
    //     edit USER2 "ecgems"                                      # comment
    //     edit INT1 "10"                                           # comment
    //     edit INT2 '11'                                           # comment
    //     edit YMD  '20091012'                                     # comment
    //     family family
    //         edit var  "smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%" # comment line
    //         edit var2 'smsfetch -F %ECF_FILES% -I %ECF_INCLUDE%' #comment line
    //         task t2
    //     endfamily
    // endsuite
    suite_ptr editSuite = defs.findSuite("edit");
    BOOST_REQUIRE_MESSAGE(editSuite, "Could not find the edit suite");

    const Variable& int1 = editSuite->findVariable("INT1");
    BOOST_REQUIRE_MESSAGE(!int1.empty(), "Could not find variable INT1");
    BOOST_REQUIRE_MESSAGE(int1.value() == 10, "Expected INT1 to have a value of 10, but found " << int1.value());

    const Variable& int2 = editSuite->findVariable("INT2");
    BOOST_REQUIRE_MESSAGE(!int2.empty(), "Could not find variable INT2");
    BOOST_REQUIRE_MESSAGE(int2.value() == 11, "Expected INT2 to have a value of 11, but found " << int2.value());

    const Variable& ymd = editSuite->findVariable("YMD");
    BOOST_REQUIRE_MESSAGE(!ymd.empty(), "Could not find variable YMD");
    BOOST_REQUIRE_MESSAGE(ymd.value() == 20091012,
                          "Expected YMD to have a value of 20091012, but found " << ymd.value());

    const Variable& user = editSuite->findVariable("USER");
    BOOST_REQUIRE_MESSAGE(!user.empty(), "Could not find variable USER");
    BOOST_REQUIRE_MESSAGE(user.value() == 0, "Expected user to have a value of 0, but found " << user.value());
}

struct Expected
{
    enum class VariableType { Server, User };

    std::string expected_name;
    std::string expected_value;
    VariableType variable_type = VariableType::User;
};

BOOST_AUTO_TEST_CASE(test_parsing_variable_empty_cases) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {{"a", ""}, {"b", ""}};

    auto content = R"--(
suite x
  edit a ''
  edit b ""
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_basic_cases) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "value"},
        {"b", "some value for b"},
        {"c", "some value for c"},
        {"d", "some value for d"},
        {"e", "some value for e"},
        {"f", "some value for f"},
        {"g", "some value for g"},
        {"h", "/path/to/x"},
        {"i", "no"},
    };

    auto content = R"--(
suite x
  edit a value
  edit b 'some value for b'
  edit c "some value for c"
  edit d 'some value for d' # with some comment
  edit e "some value for e" # with some comment
  edit f 'some value for f' # server
  edit g "some value for g" # server
  edit h /path/to/x # comment
  edit i no
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_cases_with_semicolon_in_value) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "a;a"},
        {"b", "b;b"},
        {"c", "'c;c'"},
        {"d", "\"d;d\""},
        {"e", "'e;e'"},
        {"f", "\"f;f\""},
        {"g", "g;g"},
    };

    auto content = R"--(
suite x
  edit a 'a;a'
  edit b "b;b"
  edit c ''c;c''
  edit d '"d;d"'
  edit e "'e;e'"
  edit f ""f;f""
  edit g g;g
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_tricky_characters_in_value_cases) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "single quote delimited value with '' single quotes"},
        {"b", "single quote delimited value with \"\" double quotes"},
        {"c", "double quote delimited value with '' single quotes"},
        {"d", "double quote delimited value with \"\" double quotes"},

        {"e", "single quote delimited value with '' single quotes, followed by comment"},
        {"f", "single quote delimited value with \"\" double quotes, followed by comment"},
        {"g", "double quote delimited value with '' single quotes, followed by comment"},
        {"h", "double quote delimited value with \"\" double quotes, followed by comment"},

        {"i", "single quote delimited value with # hash"},
        {"j", "double quote delimited value with # hash"},
        {"k", "single quote delimited value with # hash, followed by comment"},
        {"l", "double quote delimited value with # hash, followed by comment"},

        {"m", "'single quote delimited value including surrounding single quotes'"},
        {"n", "\"single quote delimited value including surrounding double quotes\""},
        {"o", "'single quote delimited value including surrounding single quotes, followed by comment'"},
        {"p", "\"single quote delimited value including surrounding double quotes, followed by comment\""},

        {"q", "'double quote delimited value including surrounding single quotes'"},
        {"r", "\"double quote delimited value including surrounding double quotes\""},
        {"s", "'double quote delimited value including surrounding single quotes, followed by comment'"},
        {"t", "\"double quote delimited value including surrounding double quotes, followed by comment\""}};

    auto content = R"--(
suite x
  edit a 'single quote delimited value with '' single quotes'
  edit b 'single quote delimited value with "" double quotes'
  edit c "double quote delimited value with '' single quotes"
  edit d "double quote delimited value with "" double quotes"

  edit e 'single quote delimited value with '' single quotes, followed by comment' # some comment
  edit f 'single quote delimited value with "" double quotes, followed by comment' # some comment
  edit g "double quote delimited value with '' single quotes, followed by comment" # some comment
  edit h "double quote delimited value with "" double quotes, followed by comment" # some comment

  edit i 'single quote delimited value with # hash'
  edit j "double quote delimited value with # hash"
  edit k 'single quote delimited value with # hash, followed by comment' # some comment
  edit l "double quote delimited value with # hash, followed by comment" # some comment

  edit m ''single quote delimited value including surrounding single quotes''
  edit n '"single quote delimited value including surrounding double quotes"'
  edit o ''single quote delimited value including surrounding single quotes, followed by comment'' # some comment
  edit p '"single quote delimited value including surrounding double quotes, followed by comment"' # some comment

  edit q "'double quote delimited value including surrounding single quotes'"
  edit r ""double quote delimited value including surrounding double quotes""
  edit s "'double quote delimited value including surrounding single quotes, followed by comment'" # some comment
  edit t ""double quote delimited value including surrounding double quotes, followed by comment"" # some comment
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_even_trickier_characters_in_value_cases) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "a'b'c\"d'e'#"},
        {"CMD", "${BROWSER:=firefox} -new-tab %SURL_BASE%/%URL_SUFFIX%"},
        {"VAR", "value !£$%^&*() ~@: ?><,."},
    };

    auto content = R"--(
suite x
  edit a 'a'b'c"d'e'#' # some " comment
  edit CMD '${BROWSER:=firefox} -new-tab %SURL_BASE%/%URL_SUFFIX%'
  edit VAR "value !£$%^&*() ~@: ?><,."
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_basic_comment_handling) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "a"},
        {"b", "b"},
        {"c", "c"},
        {"d", "d"},
        {"e", "e"},
    };

    auto content = R"--(
suite x
  edit a 'a' #
  edit b 'b' #comment
  edit c 'c' # comment
  edit d 'd' # some comment with multiple parts
  edit e 'e' #an attached comment with multiple parts
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_special_comment_handling) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "a' # no "},
        {"b", "'b'' # no "},
        {"c", "c' # no "},
        {"d", "'d'' # no "},
    };

    auto content = R"--(
suite x
  edit a 'a' # no ' # comment
  edit b ''b'' # no ' # comment
  edit c "c' # no "       # comment
  edit d "'d'' # no "     # comment
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_duplications) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"VAR1", "expected VAR1 value"},
        {"VAR2", "expected VAR2 value # no "},
    };

    auto content = R"--(
suite x
  edit VAR1 'first VAR1 value' # comment
  edit VAR2 'first VAR1 value' # no ' # comment
  edit VAR1 "expected VAR1 value"       # comment
  edit VAR2 "expected VAR2 value # no "     # comment
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_alias_format) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a:\"x\"", "'a'"},
        {"b:x", "b"},
        {"c:0", "0"},
        {"d:1", "1"},
        {"e:", "1"},
        {"f:", "1"},
    };

    auto content = R"--(
suite x
  family f
    task t
      alias alias1
        edit a:"x" ''a'' # some " comment
        edit b:x "b" # some ' comment
        edit c:0 0 # some ' comment
        edit d:1 '1'
        edit e: '1'
        edit f: '1'
      endalias
  endfamily
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("alias", "/x/f/t/alias1");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_whitespace_characters_in_value_cases) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {
        {"a", "   "},
        {"b", "   "},
        {"c", "\t "},
        {"d", " \t"},
        {"e", "a  b"},
        {"f", "a \t b"},
        {"g", "a\t \tb"},
        {"h", "a\t\tb"},
        {"i", "a  b\t\tc"},
        {"j", "a  b\t\tc"},
        {"k", "a\tb\tc"},
        {"l", "a b c"},
    };

    auto content = R"--(
suite x
	edit a '   '
	edit b "   "
	edit c '	 '
	edit d " 	"
	edit e 'a  b'
  edit f "a 	 b"
  edit g 'a	 	b' # some comment
  edit h "a		b" # some  comment
  edit i 'a  b		c' # some	comment
  edit j "a  b		c" # some		comment
  	edit k "a	b	c"	 # some		comment
  edit l "a b c"# some		comment
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_newline_characters_in_value_cases) {
    ECF_NAME_THIS_TEST();

    std::vector<Expected> expectations = {{"a", "a\\na"}};

    auto content = R"--(
suite x
  edit a 'a\na'
endsuite
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_at_server_level) {
    ECF_NAME_THIS_TEST();

    size_t nr_user_variables           = 8;
    std::vector<Expected> expectations = {
        {"a", "some long value for a", Expected::VariableType::User},
        {"b", "some long value for b", Expected::VariableType::User},
        {"c", "some long value for c", Expected::VariableType::Server},
        {"d", "some long value for d", Expected::VariableType::Server},
        {"e", "some long value for e", Expected::VariableType::User},
        {"f", "some long value for f", Expected::VariableType::User},
        {"g", "some long value for g", Expected::VariableType::User},
        {"h", "some long value for h", Expected::VariableType::User},
        {"i", "valuei", Expected::VariableType::Server},
        {"j", "valuej", Expected::VariableType::User},
        {"k", "valuek", Expected::VariableType::User},
    };

    auto content = R"--(
edit a 'some long value for a' # comment
edit b "some long value for b" # comment
edit c 'some long value for c' # server
edit d "some long value for d" # server
edit e 'some long value for e' # server but with more commen becomes a user variable
edit f "some long value for f" # server but with more commen becomes a user variable
edit g 'some long value for g' # not a server variable
edit h "some long value for h" # not a server variable
edit i valuei # server
edit j valuej # server but with more commen becomes a user variable
edit k valuek # not a server variable
)--";

    auto defs = Defs{};
    defs.restore_from_string(content);

    auto& state = defs.server_state();

    auto find_variable_by_type =
        [](const ServerState& state, std::string_view name, Expected::VariableType type) -> const Variable& {
        const std::vector<Variable>* variables = nullptr;
        if (type == Expected::VariableType::User) {
            variables = &state.user_variables();
        }
        else if (type == Expected::VariableType::Server) {
            variables = &state.server_variables();
        }
        else {
            throw std::runtime_error("Unknown variable type");
        }

        for (const auto& v : *variables) {
            if (v.name() == name) {
                return v;
            }
        }

        throw std::runtime_error("Variable not found");
    };

    BOOST_CHECK_EQUAL(state.user_variables().size(), nr_user_variables);
    // No check regarding the number of server variables, since there are uncountable automatically generatedvariables!

    for (const auto& e : expectations) {
        try {
            const auto& variable = find_variable_by_type(state, e.expected_name, e.variable_type);
            BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
            BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
        }
        catch (std::runtime_error& error) {
            BOOST_CHECK_MESSAGE(false,
                                "Failed to find variable '" << e.expected_name << "' with error: " << error.what());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_multiple_variables_from_single_line) {
    ECF_NAME_THIS_TEST();

    //
    // TODO: This is as a reminder that ecFlow currently does not correctly handle multiple statements on a single line.
    //
    // The current implementation performs the following sequence of simple steps on each line:
    //   (refer to DefsStructureParser::getNextLine() for details)
    //
    //   1) a (naive) search for comment character `#`, and removes all content after the first `#` character.
    //      this leads to incorrect parsing if the `#` character is part of a variable value.
    //
    //   2) a (naive) split of the line at each `;`
    //      again, this leads to incorrect split subexpressions whenever a `;` character is part of a variable value.
    //
    // n.b. The implementation supports a single variable definition per line, by disabling the subdivision of the line
    // into multiple statements when the line starts with `edit` and contains a `;`, and the entire line is treated as
    // a single variable definition.
    //
    // As a _soft_ mitigation to this issue, the documentation strongly suggests to separate multiple statements on
    // multiple lines.
    //

#if defined(MULTI_STATEMENTS_PER_LINE_CORRECTED)
    std::vector<Expected> expectations = {{"a", "a;b"}, {"B", "with # hash"}};

    auto content = R"--(
suite x
  edit A 'a;b'; edit B 'with # hash'
endsuite
)--";

    auto defs = Defs{};
    try {
        defs.restore_from_string(content);
    }
    catch (std::runtime_error& error) {
        BOOST_CHECK_MESSAGE(false,
                            "This is known to fail, due to the current implementation of handling multiple statements "
                            "on a single line. Currently produced the error: "
                                << error.what());
    }

    auto suite = defs.find_node("suite", "/x");

    BOOST_CHECK_EQUAL(suite->variables().size(), expectations.size());

    for (const auto& e : expectations) {
        auto variable = suite->findVariable(e.expected_name);
        BOOST_CHECK_EQUAL(variable.name(), e.expected_name);
        BOOST_CHECK_EQUAL(variable.theValue(), e.expected_value);
    }
#endif
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_incorrect_format_due_to_missing_comment_marker) {
    ECF_NAME_THIS_TEST();

    auto content = R"--(
suite x
  edit a 'value' some comment
endsuite
)--";
    auto defs    = Defs{};
    try {
        defs.restore_from_string(content);
        BOOST_CHECK_MESSAGE(false, "Expected parsing to fail due to incorrect variable format, but it succeeded");
    }
    catch (std::exception& e) {
        std::string_view msg = e.what();
        BOOST_CHECK_MESSAGE(msg.find("non-# found after value") != std::string::npos,
                            "Parsing failed as expected with error: " << msg);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_incorrect_format_due_to_comment_marker_included_in_value) {
    ECF_NAME_THIS_TEST();

    auto content = R"--(
suite x
  edit a 'value' # some ' comment
endsuite
)--";
    auto defs    = Defs{};
    try {
        defs.restore_from_string(content);
        BOOST_CHECK_MESSAGE(false, "Expected parsing to fail due to incorrect variable format, but it succeeded");
    }
    catch (std::exception& e) {
        std::string_view msg = e.what();
        BOOST_CHECK_MESSAGE(msg.find("non-# found after value") != std::string::npos,
                            "Parsing failed as expected with error: " << msg);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_incorrect_format_due_to_single_quotation_marker) {
    ECF_NAME_THIS_TEST();

    auto content = R"--(
suite x
  edit a ' some comment
endsuite
)--";

    auto defs = Defs{};
    try {
        defs.restore_from_string(content);
        BOOST_CHECK_MESSAGE(false, "Expected parsing to fail due to incorrect variable format, but it succeeded");
    }
    catch (std::exception& e) {
        std::string_view msg = e.what();
        BOOST_CHECK_MESSAGE(msg.find("Mismatched quote detected (only one quote found)") != std::string::npos,
                            "Parsing failed as expected with error: " << msg);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_incorrect_format_due_to_missing_comment_marker_with_multilevel_quotes) {
    ECF_NAME_THIS_TEST();

    auto content = R"--(
suite x
  edit a "'value'" # some "'" comment
endsuite
)--";
    auto defs    = Defs{};
    try {
        defs.restore_from_string(content);
        BOOST_CHECK_MESSAGE(false, "Expected parsing to fail due to incorrect variable format, but it succeeded");
    }
    catch (std::exception& e) {
        std::string_view msg = e.what();
        BOOST_CHECK_MESSAGE(msg.find("non-# found after value") != std::string::npos,
                            "Parsing failed as expected with error: " << msg);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_incorrect_format_due_to_invalid_variable_name) {
    ECF_NAME_THIS_TEST();

    auto content = R"--(
suite x
  edit a:out "value" # comment
endsuite
)--";
    auto defs    = Defs{};
    try {
        defs.restore_from_string(content);
        BOOST_CHECK_MESSAGE(false, "Expected parsing to fail due to incorrect variable format, but it succeeded");
    }
    catch (std::exception& e) {
        std::string_view msg = e.what();
        BOOST_CHECK_MESSAGE(msg.find("Invalid Variable name") != std::string::npos,
                            "Parsing failed as expected with error: " << msg);
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_variable_with_incorrect_format_due_to_multiline_value) {
    ECF_NAME_THIS_TEST();

    auto content = R"--(
suite x
  edit a 'some
value'
endsuite
)--";

    auto defs = Defs{};
    try {
        defs.restore_from_string(content);
        BOOST_CHECK_MESSAGE(false, "Expected parsing to fail due to incorrect variable format, but it succeeded");
    }
    catch (std::exception& e) {
        std::string_view msg = e.what();
        BOOST_CHECK_MESSAGE(msg.find("Mismatched quote detected (only one quote found) in line") != std::string::npos,
                            "Parsing failed as expected with error: " << msg);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
