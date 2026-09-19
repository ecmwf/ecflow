/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Label)

BOOST_AUTO_TEST_CASE(test_label_parsing) {
    ECF_NAME_THIS_TEST();

    {
        std::string line = "label name \"value\"";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = R"(label name "value\nvalue")";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value\nvalue");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = "label name \"value that is multiple token !!!! 23445 !^ & * ( )\"";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value that is multiple token !!!! 23445 !^ & * ( )");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = R"(label name "value\n that\n is\n multiple\n token\n and\n new\n \nlines")";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value\n that\n is\n multiple\n token\n and\n new\n \nlines");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
}

namespace {

// Builds the line that the definition writer produces for a label with the given values
std::string write_line(const std::string& value, const std::string& new_value, const std::string& name = "L") {
    auto escape = [](std::string s) {
        ecf::algorithm::replace_all(s, "\n", "\\n");
        return s;
    };
    std::string line = "label " + name + " \"" + escape(value) + "\"";
    if (!new_value.empty()) {
        line += " # \"" + escape(new_value) + "\"";
    }
    return line;
}

void check_parse(const std::string& line,
                 bool parse_state,
                 const std::string& expected_value,
                 const std::string& expected_new_value) {
    std::vector<std::string> tokens;
    ecf::algorithm::split_at(tokens, line);

    Label label;
    label.parse(line, tokens, parse_state);

    BOOST_CHECK_MESSAGE(label.value() == expected_value,
                        "Parsing '" << line << "' (state=" << parse_state << "): expected default value '"
                                    << expected_value << "' but found '" << label.value() << "'");
    BOOST_CHECK_MESSAGE(label.new_value() == expected_new_value,
                        "Parsing '" << line << "' (state=" << parse_state << "): expected current value '"
                                    << expected_new_value << "' but found '" << label.new_value() << "'");
}

// True when the value contains a double quote followed by blanks and '#', the one shape that a definition
// line cannot represent without ambiguity (the quote would be read as closing the value before a comment)
bool contains_quote_blanks_hash(const std::string& value) {
    for (size_t pos = value.find('"'); pos != std::string::npos; pos = value.find('"', pos + 1)) {
        size_t i = pos + 1;
        while (i < value.size() && (value[i] == ' ' || value[i] == '\t')) {
            ++i;
        }
        if (i > pos + 1 && i < value.size() && value[i] == '#') {
            return true;
        }
    }
    return false;
}

// Writes the label as the definition writer would, parses the line back, and expects the original values
void check_round_trip(const std::string& value, const std::string& new_value) {
    check_parse(write_line(value, new_value), true, value, new_value);
    if (!contains_quote_blanks_hash(value)) {
        // Without state, the same line is a definition, where anything after the closing quote is a comment
        check_parse(write_line(value, ""), false, value, "");
    }
}

struct ValuePair
{
    const char* value;
    const char* new_value;
};

} // namespace

BOOST_AUTO_TEST_CASE(test_label_round_trip_hash_in_values) {
    ECF_NAME_THIS_TEST();

    const ValuePair cases[] = {
        {"abc", "some value"},
        {"abc", "some#value"},
        {"x", "current"},
        {"x", "cur#rent"},
        {"", "#0 30/50 or 0.048 ~> 0.050"},
        {"", "#0 96/96"},
        {"", "#1 28/50 or 0.043 ~> 0.050"},
        {"",
         "### min for /mc/main/1/fc10d/fc/pf/3/modeleps_nemo: 12\n### max for "
         "/mc/main/1/fc10d/fc/pf/3/modeleps_nemo:15"},
        {"abc", "#leading"},
        {"abc", "trailing#"},
        {"abc", "#"},
        {"abc", "x # y"},
        {"abc", "x #\"y"},
        {"a#b", ""},
        {"a#b", "x"},
        {"a#b", "x#y"},
        {"a#b c", "x"},
        {"a #b", ""},
        {"a #b", "x"},
        {"a # b", "x"},
        {"#40fd83", ""},
        {"#af412e-dirty", ""},
        {"abc", "multi\nline#x"},
    };
    for (const auto& c : cases) {
        check_round_trip(c.value, c.new_value);
    }
}

BOOST_AUTO_TEST_CASE(test_label_round_trip_quotes_in_values) {
    ECF_NAME_THIS_TEST();

    const ValuePair cases[] = {
        {"abc", "say \"hi\""},
        {"say \"hi\"", "x"},
        {"say \"hi\"", ""},
        {"\"note\"", ""},
        {"\"note\"", "It is now possible to archive as eccams. See https://jira.ecmwf.int/browse/SD-72748"},
        {"\"", "x"},
        {"\"", ""},
        {"x\" ", "y"},
        {"\" x", "y"},
        {"a\" # b", "x"},
        {"a # \"b", "x"},
        {"", "\"q\" x"},
        {"abc", "\""},
        {"abc", "x\" # \"y"},
        {"abc", "x \"#\" y"},
        {"abc", "x \" y"},
        {"abc", " # \"x"},
        {"abc", "\" # "},
        {"abc", "a'b"},
        {"Global Fire Assimilation System suite \"GFAS\" (experiment 0001)", ""},
        {"ONLY CAMS \"NON-CRITICAL\" SUITES DEPENDING ON COMPO OR ECGEMS EXPERIMENTS",
         "ONLY CAMS \"NON-CRITICAL\" SUITES DEPENDING ON COMPO OR RD EXPERIMENTS"},
        {"",
         "Launch failed: Could not find script branch: REAL_IFS_SCRIPTS_GIT_TAG=\"fra5996_CY50R1_OBECOR.NEMO-627\" in "
         "REAL_IFS_SCRIPTS_GIT_REPO=\"ssh:///~fra5996/ifs-scripts.git\" [Wed-07/22/26-10:59:20 BST]"},
        {"{end: !!timestamp '2001-12-31 23:59:59', start: !!timestamp '1994-01-01 00:00:00'}", ""},
        {"additions: true\ngroup_by: monthly\nremapping: {param_level: '{param}'}\nuse_grib_paramid: "
         "true\nvariable_naming: "
         "param",
         ""},
    };
    for (const auto& c : cases) {
        check_round_trip(c.value, c.new_value);
    }
}

BOOST_AUTO_TEST_CASE(test_label_round_trip_blanks_in_values) {
    ECF_NAME_THIS_TEST();

    const ValuePair cases[] = {
        {"", "x"},
        {"abc", "  padded  "},
        {"abc", "   "},
        {"a  b", ""},
        {"a  b", "c   d"},
        {" a ", " b "},
        {"\t", "\t"},
        {"", "20K\t/ec/ws1/tc/emos/test/fdb:/ec/ws1/tc/emos/test/fdb"},
        {"ERA6 HRES Tco799 - spinup 2016 v2", "ERA6 HRES Tco799 - 2016 "},
        {"49R1 COMPO o-suite (0079) control ", "49R1 COMPO o-suite  control "},
        {"CI fc double precision CY49R1_develop",
         "CI fc double precision CY49R1_develop\nPURPOSE: to replicate failure of multio 2.7.0 with satimsim\nSTATUS: "
         "for "
         "Dom/Mirco to investigate"},
        {"5cfa2a40fa214167847fb1d9b5161812 - febr 2024 - 10 members - o96->o320",
         "5cfa2a40fa214167847fb1d9b5161812 - febr 2024 - 10 members - o96->o320 - 9e5 steps\n"},
        {"value\n that\n is\n multiple\n token\n and\n new\n \nlines", ""},
        {"CY50R1.0 Tco319L137 SP eORCA025_Z75 - S2S control - like j4db but ifs-nemo scalars \xe2\x86\x92 fields, test "
         "reproducible",
         ""},
    };
    for (const auto& c : cases) {
        check_round_trip(c.value, c.new_value);
    }
}

BOOST_AUTO_TEST_CASE(test_label_round_trip_exhaustive_short_values) {
    ECF_NAME_THIS_TEST();

    // Every default value of up to four characters and every current value of up to three characters, drawn
    // from an alphabet made of the significant characters, must survive a state round trip, unless the default
    // value contains a double quote followed by blanks and '#', which is the documented limitation.
    // Each eligible default value must also survive a definition-only round trip.
    const std::string alphabet = "a\"# \t";

    std::vector<std::string> values{""};
    for (size_t length = 1; length <= 4; ++length) {
        std::vector<std::string> longer;
        for (const auto& value : values) {
            if (value.size() == length - 1) {
                for (char c : alphabet) {
                    longer.push_back(value + c);
                }
            }
        }
        values.insert(values.end(), longer.begin(), longer.end());
    }

    size_t definitions_checked = 0;
    size_t state_checked       = 0;
    size_t ambiguous           = 0;
    for (const auto& value : values) {
        const bool value_is_ambiguous = contains_quote_blanks_hash(value);
        if (!value_is_ambiguous) {
            check_parse(write_line(value, ""), false, value, "");
            ++definitions_checked;
        }
        for (const auto& new_value : values) {
            if (new_value.size() > 3) {
                continue;
            }
            if (value_is_ambiguous) {
                ++ambiguous;
                continue;
            }
            ++state_checked;

            std::string line = write_line(value, new_value);
            std::vector<std::string> tokens;
            ecf::algorithm::split_at(tokens, line);
            Label label;
            label.parse(line, tokens, true);
            if (label.value() != value || label.new_value() != new_value) {
                BOOST_ERROR("Round trip failed for default '" << value << "' and current '" << new_value << "': line '"
                                                              << line << "' read as default '" << label.value()
                                                              << "' and current '" << label.new_value() << "'");
            }
        }
    }
    BOOST_CHECK_EQUAL(definitions_checked, 755);
    BOOST_CHECK_MESSAGE(state_checked > 100000,
                        "Expected more than 100000 state pairs to be checked, found " << state_checked);
    BOOST_CHECK_MESSAGE(ambiguous > 0, "Expected some pairs to be excluded as ambiguous");
}

BOOST_AUTO_TEST_CASE(test_label_parsing_definition_forms) {
    ECF_NAME_THIS_TEST();

    // Hand-written definition lines: a trailing '#' after the closing quote starts a comment
    check_parse("label L \"x\" # comment", false, "x", "");

    // A quote immediately followed by '#' is part of the value; a comment needs a blank before '#'
    check_parse("label L \"a\"#b\"", false, "a\"#b", "");
    check_parse("label L \"a\"#b\" # comment", false, "a\"#b", "");
    check_parse("label L 'a'#b'", false, "a'#b", "");
    check_parse("label L \"a\"#\"", false, "a\"#", "");
    check_parse("label L \"x # y\" # comment", false, "x # y", "");
    check_parse("label L \"say \"hi\"\" # comment", false, "say \"hi\"", "");
    check_parse("label L 'say \"hi\" # ok' # comment", false, "say \"hi\" # ok", "");
    check_parse("label L 'it's' # comment", false, "it's", "");
    check_parse("label L 'it's'", false, "it's", "");
    check_parse("label L \"\"note\"\" # c", false, "\"note\"", "");
    check_parse("label file \"\" # for empty label", false, "", "");
    check_parse("label L \"a  b\"", false, "a  b", "");
    check_parse("label L \"x\"  #  comment", false, "x", "");
    check_parse("label L \"x\"\t# comment", false, "x", "");

    // Unquoted values
    check_parse("label OBS 0", false, "0", "");
    check_parse("label name string", false, "string", "");
    check_parse("label name string # comment", false, "string", "");
    check_parse("label OBS 0", true, "0", "");

    // With state, an unquoted default may be followed by the current value; hashes and quotes inside it are kept
    check_parse("label L 0 # \"running\"", true, "0", "running");
    check_parse("label L 0 # \"run#1\"", true, "0", "run#1");
    check_parse("label L 0 # \"say \"hi\"\"", true, "0", "say \"hi\"");
    check_parse("label L a#b c # \"x # y\"", true, "a#b c", "x # y");
    check_parse("label L 0  #  \"padded\"  ", true, "0", "padded");
    check_parse("label L 0 # \"multi\\nline\"", true, "0", "multi\nline");
    check_parse("label L 0 # \"running\"", false, "0", "");
    check_parse("label L 0 # comment", true, "0", "");
    check_parse("label L 0 #\"x\"", true, "0", "");

    // A single unquoted token starting with '#' is a value, not a comment; more tokens still end at a comment
    check_parse("label rev #40fd83", false, "#40fd83", "");
    check_parse("label rev #40fd83", true, "#40fd83", "");
    check_parse("label rev #", false, "#", "");
    check_parse("label rev #40fd83 # comment", false, "", "");

    // Single quoted values
    check_parse("label simple_label 'ecgems'", false, "ecgems", "");
    check_parse("label L 'a \"b\" c' # \"x\"", true, "a \"b\" c", "x");
}

BOOST_AUTO_TEST_CASE(test_label_parsing_state_forms) {
    ECF_NAME_THIS_TEST();

    // Explicit empty current value
    check_parse("label L \"abc\" # \"\"", true, "abc", "");

    // Blanks around the separator are tolerated
    check_parse("label L \"abc\"  #  \"x\"", true, "abc", "x");
    check_parse("label L \"abc\"\t#\t\"x\"", true, "abc", "x");

    // A trailing comment after the current value is ignored
    check_parse("label L \"x\" # \"y\" # note", true, "x", "y");

    // A definition line with a comment is still a definition line
    check_parse("label L \"x\" # comment", true, "x", "");
    check_parse("label file \"\" # for empty label", true, "", "");

    // A comment that itself holds a double quote is not recognised in state mode; the last quote closes the value
    check_parse("label L \"x\" # see \"docs\"", true, "x\" # see \"docs", "");

    // A default value that was truncated by an earlier version keeps its stray quote, nothing more is lost
    check_parse("label a \"\"x\"", true, "\"x", "");
}

BOOST_AUTO_TEST_CASE(test_label_parsing_tolerates_unterminated_lines) {
    ECF_NAME_THIS_TEST();

    // A state line cut after the separator, or inside the current value, keeps what is present
    check_parse("label L \"abc\" # \"", true, "abc", "");
    check_parse("label L \"abc\" # \"x", true, "abc", "x");
    check_parse("label L \"abc\" # \"x y", true, "abc", "x y");

    // A default value with no closing quote extends to the end of the line, in both modes
    check_parse("label L \"abc", false, "abc", "");
    check_parse("label L \"abc", true, "abc", "");
    check_parse("label L 'abc", false, "abc", "");
    check_parse("label L \"a b c", false, "a b c", "");

    // Mismatched quote characters are not a separator; the last double quote closes the value
    check_parse("label L \"abc' # \"x\"", true, "abc' # \"x", "");
}

BOOST_AUTO_TEST_CASE(test_label_parsing_ambiguous_default_value) {
    ECF_NAME_THIS_TEST();

    ecf::test::scaffold::TestLog test_log("test_label_parsing_ambiguous_default_value.log");

    // A default value containing its quote followed by blanks and '#' cannot be told apart from the state
    // separator; the first separator wins. These lines document the limitation.
    check_parse("label L \"a\" # \"b\" # \"x\"", true, "a", "b\" # \"x");
    check_parse("label L \"\" # \" # \"a\"", true, "", " # \"a");
    check_parse("label L \"say \"hi\" # ok\" # comment", false, "say \"hi", "");

    // Each ambiguous line, in state or definition form, is reported in the log with the values that were read
    std::string log_contents = Log::instance()->contents(100);
    BOOST_CHECK_MESSAGE(log_contents.find("WAR:") != std::string::npos, "Expected a warning in log:\n" << log_contents);
    BOOST_CHECK_MESSAGE(log_contents.find("the value may be truncated; label 'L' read with default value 'a' "
                                          "and current value 'b\" # \"x' from: label L \"a\" # \"b\" # \"x\"") !=
                            std::string::npos,
                        "Expected the first ambiguous line to be reported in log:\n"
                            << log_contents);
    BOOST_CHECK_MESSAGE(
        log_contents.find("label 'L' read with default value '' and current value ' # \"a' from: label L "
                          "\"\" # \" # \"a\"") != std::string::npos,
        "Expected the second ambiguous line to be reported in log:\n"
            << log_contents);
    BOOST_CHECK_MESSAGE(log_contents.find("label 'L' read with default value 'say \"hi' and current value '' from: "
                                          "label L \"say \"hi\" # ok\" # comment") != std::string::npos,
                        "Expected the definition line to be reported in log:\n"
                            << log_contents);
}

BOOST_AUTO_TEST_CASE(test_label_parsing_unambiguous_lines_do_not_warn) {
    ECF_NAME_THIS_TEST();

    ecf::test::scaffold::TestLog test_log("test_label_parsing_unambiguous_lines_do_not_warn.log");

    // A single separator, hash and quote characters in either value, or a comment, never trigger the warning
    check_parse("label L \"a#b\" # \"x#y\"", true, "a#b", "x#y");
    check_parse("label L \"a # b\" # \"x # y\"", true, "a # b", "x # y");
    check_parse("label L \"say \"hi\"\" # \"say \"hi\"\"", true, "say \"hi\"", "say \"hi\"");
    check_parse("label L \"abc\" # \"x \"#\" y\"", true, "abc", "x \"#\" y");
    check_parse("label L \"x\" # \"y\" # note", true, "x", "y");
    check_parse("label L \"x\" # comment", false, "x", "");

    std::string log_contents = Log::instance()->contents(100);
    BOOST_CHECK_MESSAGE(log_contents.find("WAR:") == std::string::npos,
                        "Expected no warning in log:\n"
                            << log_contents);
}

BOOST_AUTO_TEST_CASE(test_label_parsing_rejects_short_lines) {
    ECF_NAME_THIS_TEST();

    std::vector<std::string> tokens;
    std::string line = "label name";
    ecf::algorithm::split_at(tokens, line);
    Label label;
    BOOST_CHECK_THROW(label.parse(line, tokens, false), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_label_equality) {
    ECF_NAME_THIS_TEST();

    Label a("name", "value", "new_value");
    Label same("name", "value", "new_value");
    Label different_name("other", "value", "new_value");
    Label different_value("name", "other", "new_value");
    Label different_new_value("name", "value", "other");

    BOOST_CHECK(a == same);
    BOOST_CHECK(!(a != same));

    BOOST_CHECK(a != different_name);
    BOOST_CHECK(!(a == different_name));

    BOOST_CHECK(a != different_value);
    BOOST_CHECK(!(a == different_value));

    BOOST_CHECK(a != different_new_value);
    BOOST_CHECK(!(a == different_new_value));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
