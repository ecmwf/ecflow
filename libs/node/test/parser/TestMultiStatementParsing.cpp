// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "TemporaryFile.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Parser)

BOOST_AUTO_TEST_SUITE(T_MultiStatementParsing)

namespace {

///
/// @brief Holds the definition of a suite whose statements are all on a single, ';' joined, line.
///
const std::string single_line_suite =
    "suite s2; clock real +01:00; endclock +02:00; task ta; family f1; task fa; endfamily; task tb; endsuite";

///
/// @brief Builds a definition made of a regular suite followed by a ';' joined one.
///
/// @return The definition, without a final newline
///
std::string two_suites() {
    return "suite s1\n task t1\nendsuite\n" + single_line_suite;
}

///
/// @brief Writes the given content verbatim, in particular without appending a final newline.
///
/// @param[in] path Path to the file to create or overwrite
/// @param[in] content Content to write, taken as is
///
void write_verbatim(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    BOOST_REQUIRE_MESSAGE(file.good(), "Could not open temporary file " << path);
    file << content;
}

///
/// @brief Parses the given content as a definition file.
///
/// @param[in] content Content of the definition file
/// @param[out] defs Definition restored from the content
/// @param[out] errorMsg Reason for the failure, when the parsing does not succeed
/// @return true when the parsing succeeds, false otherwise
///
bool parse_as_file(const std::string& content, Defs& defs, std::string& errorMsg) {
    TemporaryFile temporary("tmp_%%%%-%%%%-%%%%-%%%%.def");
    write_verbatim(temporary.path(), content);

    std::string warningMsg;
    return defs.restore(temporary.path(), errorMsg, warningMsg);
}

///
/// @brief Parses the given content as a definition string.
///
/// @param[in] content Content of the definition
/// @param[out] defs Definition restored from the content
/// @param[out] errorMsg Reason for the failure, when the parsing does not succeed
/// @return true when the parsing succeeds, false otherwise
///
bool parse_as_string(const std::string& content, Defs& defs, std::string& errorMsg) {
    std::string warningMsg;
    return defs.restore_from_string(content, errorMsg, warningMsg);
}

///
/// @brief Checks that the ';' joined suite is parsed in full, and not truncated at the first
/// statement.
///
/// @param[in] defs Definition expected to contain the suite
///
void check_single_line_suite(const Defs& defs) {
    auto suite = defs.findAbsNode("/s2");
    BOOST_REQUIRE_MESSAGE(suite, "Expected to find suite /s2");

    auto as_suite = std::dynamic_pointer_cast<Suite>(suite);
    BOOST_REQUIRE_MESSAGE(as_suite, "Expected /s2 to be a suite");
    BOOST_CHECK_MESSAGE(as_suite->clockAttr(), "Expected a clock on /s2");
    BOOST_CHECK_MESSAGE(as_suite->clock_end_attr(), "Expected an end clock on /s2");

    for (const auto& path : {"/s2/ta", "/s2/f1", "/s2/f1/fa", "/s2/tb"}) {
        BOOST_CHECK_MESSAGE(defs.findAbsNode(path), "Expected to find node " << path);
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(test_parsing_last_line_with_multiple_statements) {
    ECF_NAME_THIS_TEST();

    // The final newline determines whether the underlying stream is still good once the last line has been
    // read; the statements queued from that line must be parsed in either case.
    for (const auto& ending : {"", "\n"}) {
        std::string content = two_suites() + ending;

        Defs from_file;
        std::string errorMsg;
        BOOST_REQUIRE_MESSAGE(parse_as_file(content, from_file, errorMsg), errorMsg);
        check_single_line_suite(from_file);

        Defs from_string;
        BOOST_REQUIRE_MESSAGE(parse_as_string(content, from_string, errorMsg), errorMsg);
        check_single_line_suite(from_string);

        BOOST_CHECK_MESSAGE(from_file == from_string, "Parse by string != parse by filename");
    }
}

BOOST_AUTO_TEST_CASE(test_parsing_last_line_with_multiple_statements_and_comment) {
    ECF_NAME_THIS_TEST();

    std::string content = two_suites() + " # a trailing comment";

    Defs defs;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(parse_as_file(content, defs, errorMsg), errorMsg);
    check_single_line_suite(defs);
}

BOOST_AUTO_TEST_CASE(test_parsing_last_line_with_trailing_semicolon) {
    ECF_NAME_THIS_TEST();

    std::string content = two_suites() + ";";

    Defs defs;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(parse_as_file(content, defs, errorMsg), errorMsg);
    check_single_line_suite(defs);
}

BOOST_AUTO_TEST_CASE(test_parsing_definition_made_of_a_single_line) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(parse_as_file(single_line_suite, defs, errorMsg), errorMsg);
    check_single_line_suite(defs);
    BOOST_CHECK_EQUAL(defs.suites().size(), static_cast<size_t>(1));
}

BOOST_AUTO_TEST_CASE(test_parsing_invalid_statement_in_last_line_fails) {
    ECF_NAME_THIS_TEST();

    // The end clock precedes the start clock, which is only detected once the whole line is parsed.
    std::string content = "suite s1; clock real +02:00; endclock +01:00; task t1; endsuite";

    {
        Defs defs;
        std::string errorMsg;
        BOOST_CHECK_MESSAGE(!parse_as_file(content, defs, errorMsg), "Expected parsing of file to fail");
    }
    {
        Defs defs;
        std::string errorMsg;
        BOOST_CHECK_MESSAGE(!parse_as_string(content, defs, errorMsg), "Expected parsing of string to fail");
    }
}

BOOST_AUTO_TEST_CASE(test_defs_string_provides_every_line) {
    ECF_NAME_THIS_TEST();

    // A definition string is traversed line by line, independently of a final newline being present.
    for (const auto& ending : {"", "\n"}) {
        DefsString defs_as_string(std::string("suite s1\n task t1\nendsuite") + ending);

        std::vector<std::string> lines;
        while (defs_as_string.good()) {
            std::string line;
            defs_as_string.getline(line);
            lines.push_back(line);
        }

        std::vector<std::string> expected{"suite s1", " task t1", "endsuite"};
        BOOST_CHECK_EQUAL_COLLECTIONS(lines.begin(), lines.end(), expected.begin(), expected.end());
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
