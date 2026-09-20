// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_LabelParsing)

namespace {

// Writes a definition holding one label in the given style, restores it, and expects the same values back
void check_label_round_trip(const std::string& value, const std::string& new_value, PrintStyle::Type_t style) {
    Defs defs;
    suite_ptr suite = defs.add_suite("s");
    task_ptr task   = suite->add_task("t");
    task->add_label("L", value, "");
    if (!new_value.empty()) {
        task->changeLabel("L", new_value);
    }

    std::string text;
    defs.write_to_string(text, style);

    Defs restored;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(restored.restore_from_string(text, errorMsg, warningMsg),
                          "Failed to restore definition:\n"
                              << text << "\n"
                              << errorMsg);

    node_ptr node = restored.findAbsNode("/s/t");
    BOOST_REQUIRE_MESSAGE(node, "Could not find task /s/t in restored definition");
    const Label& label = node->find_label("L");

    BOOST_CHECK_MESSAGE(label.value() == value,
                        "Style " << PrintStyle::to_string(style) << ": expected default value '" << value
                                 << "' but found '" << label.value() << "' when restoring:\n"
                                 << text);

    // A definition without state carries no current value
    const std::string& expected_new_value = (style == PrintStyle::DEFS) ? std::string() : new_value;
    BOOST_CHECK_MESSAGE(label.new_value() == expected_new_value,
                        "Style " << PrintStyle::to_string(style) << ": expected current value '" << expected_new_value
                                 << "' but found '" << label.new_value() << "' when restoring:\n"
                                 << text);
}

struct ValuePair
{
    const char* value;
    const char* new_value;
};

} // namespace

BOOST_AUTO_TEST_CASE(test_label_values_survive_write_and_restore) {
    ECF_NAME_THIS_TEST();

    const ValuePair cases[] = {
        {"abc", "some value"},
        {"abc", "some#value"},
        {"", "#0 30/50 or 0.048 ~> 0.050"},
        {"", "#0 96/96"},
        {"abc", "#leading"},
        {"abc", "trailing#"},
        {"abc", "#"},
        {"abc", "x # y"},
        {"a#b", "x#y"},
        {"a\"#b", "x"},
        {"a #b", "x"},
        {"a # b", "x"},
        {"#40fd83", ""},
        {"abc", "say \"hi\""},
        {"say \"hi\"", "x"},
        {"\"note\"", "x"},
        {"abc", "x \"#\" y"},
        {"abc", "x\" # \"y"},
        {"abc", "\""},
        {"ONLY CAMS \"NON-CRITICAL\" SUITES", "ONLY CAMS \"NON-CRITICAL\" SUITES TOO"},
        {"abc", "a'b"},
        {"{end: !!timestamp '2001-12-31 23:59:59'}", ""},
        {"abc", "  padded  "},
        {"  a  ", "  b  "},
        {"a  b", "c   d"},
        {"", "20K\t/ec/ws1/tc/emos/test/fdb"},
        {"CI fc", "CI fc\nPURPOSE: replicate\nSTATUS: open"},
        {"steps", "9e5 steps\n"},
        {"abc", "multi\nline#x"},
        {"scalars \xe2\x86\x92 fields", ""},
    };
    for (const auto& c : cases) {
        for (auto style : {PrintStyle::DEFS, PrintStyle::STATE, PrintStyle::MIGRATE, PrintStyle::NET}) {
            check_label_round_trip(c.value, c.new_value, style);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_label_definition_file_with_hash_and_quotes) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_defs/label/hash.def", "parser");

    Defs defs;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(defs.restore(path, errorMsg, warningMsg), errorMsg);

    node_ptr task = defs.findAbsNode("/label_hash/f/t");
    BOOST_REQUIRE_MESSAGE(task, "Could not find task /label_hash/f/t");

    auto check = [&](const std::string& name, const std::string& expected) {
        const Label& label = task->find_label(name);
        BOOST_CHECK_MESSAGE(label.value() == expected,
                            "Label " << name << ": expected '" << expected << "' but found '" << label.value() << "'");
        BOOST_CHECK_MESSAGE(label.new_value().empty(),
                            "Label " << name << ": expected no current value but found '" << label.new_value() << "'");
    };
    check("version", "#40fd83");
    check("dirty", "#af412e-dirty");
    check("with_hash", "x#y");
    check("with_blank_hash", "x #y");
    check("with_blank_hash_blank", "x # y");
    check("with_trailing_hash", "x#");
    check("only_hash", "#");
    check("quoted_word", "\"note\"");
    check("quoted_inside", "say \"hi\" to all");
    check("trailing_quote", "say \"hi\"");
    check("leading_quote", "\"quoted\" start");
    check("single_quotes", "it's");
    check("yaml", "{end: !!timestamp '2001-12-31 23:59:59', start: !!timestamp '1994-01-01 00:00:00'}");
    check("blanks", "  padded  ");
    check("commented", "x # y");
    check("empty_commented", "");
    check("unquoted", "0");
}

BOOST_AUTO_TEST_CASE(test_label_state_file_with_hash_and_quotes) {
    ECF_NAME_THIS_TEST();

    std::string path = File::test_data("libs/node/test/parser/data/good_defs_state/label/hash.def", "parser");

    Defs defs;
    std::string errorMsg, warningMsg;
    BOOST_REQUIRE_MESSAGE(defs.restore(path, errorMsg, warningMsg), errorMsg);

    node_ptr task = defs.findAbsNode("/label_hash_state/f/t");
    BOOST_REQUIRE_MESSAGE(task, "Could not find task /label_hash_state/f/t");

    auto check = [&](const std::string& name, const std::string& expected, const std::string& expected_new) {
        const Label& label = task->find_label(name);
        BOOST_CHECK_MESSAGE(label.value() == expected,
                            "Label " << name << ": expected default '" << expected << "' but found '" << label.value()
                                     << "'");
        BOOST_CHECK_MESSAGE(label.new_value() == expected_new,
                            "Label " << name << ": expected current '" << expected_new << "' but found '"
                                     << label.new_value() << "'");
    };
    check("Traj", "", "#0 96/96");
    check("Min", "", "#1 28/50 or 0.043 ~> 0.050");
    check("info",
          "",
          "### min for /mc/main/1/fc10d/fc/pf/3/modeleps_nemo: 12\n### max for "
          "/mc/main/1/fc10d/fc/pf/3/modeleps_nemo:15");
    check("hash_both", "a#b", "x#y");
    check("blank_hash_both", "a # b", "x # y");
    check("leading", "abc", "#leading");
    check("trailing", "abc", "trailing#");
    check("only_hash", "abc", "#");
    check("quoted_hash", "abc", "x \"#\" y");
    check("separator_like", "abc", "x\" # \"y");
    check("quotes_both", "ONLY CAMS \"NON-CRITICAL\" SUITES", "ONLY CAMS \"NON-CRITICAL\" SUITES TOO");
    check("quoted_word", "\"note\"", "It is now possible to archive as eccams");
    check("trailing_quote", "abc", "say \"hi\"");
    check("lone_quote", "abc", "\"");
    check("tab", "", "20K\t/ec/ws1/tc/emos/test/fdb:/ec/ws1/tc/emos/test/fdb");
    check("blanks", "  a  ", "  b  ");
    check("trailing_blank", "spinup 2016 v2", "2016 ");
    check("multiline", "CI fc", "CI fc\nPURPOSE: replicate\nSTATUS: open");
    check("ends_newline", "steps", "9e5 steps\n");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
