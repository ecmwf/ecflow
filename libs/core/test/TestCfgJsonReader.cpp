// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <variant>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/cfg/Configuration.hpp"
#include "ecflow/core/cfg/JsonReader.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::cfg::ConfigurationError;
using ecf::cfg::CTree;
using ecf::cfg::JsonReader;

namespace scaffold = ecf::test::scaffold;

namespace {

const std::string origin = "host.3141.ecf.cfg";

///
/// @brief Checks that the given callable throws a ConfigurationError whose message contains every given fragment.
///
/// @tparam F Callable taking no arguments
/// @param[in] f The callable expected to throw
/// @param[in] fragments Texts expected in the message of the error
///
template <typename F>
void check_error(F f, const std::vector<std::string>& fragments) {
    BOOST_CHECK_EXCEPTION(f(), ConfigurationError, [&](const ConfigurationError& e) {
        const std::string message = e.what();
        BOOST_TEST_INFO("message: " << message);
        for (const auto& fragment : fragments) {
            if (message.find(fragment) == std::string::npos) {
                return false;
            }
        }
        return true;
    });
}

///
/// @brief Checks that reading the given text throws a ConfigurationError whose message contains every given fragment.
///
/// @param[in] text The document
/// @param[in] fragments Texts expected in the message of the error, besides the origin
///
void check_refused(const std::string& text, std::vector<std::string> fragments) {
    BOOST_TEST_CONTEXT("text: '" << text << "'") {
        fragments.push_back(origin);
        check_error([&] { (void)JsonReader().read(text, origin); }, fragments);
    }
}

///
/// @brief Returns the message of the ConfigurationError thrown when reading the given text.
///
/// @param[in] text The document
/// @return The message, or `(no error)` when the text is read without error.
///
std::string error_of(const std::string& text) {
    try {
        (void)JsonReader().read(text, origin);
    }
    catch (const ConfigurationError& e) {
        return e.what();
    }
    return "(no error)";
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_CfgJsonReader)

//
// Reading documents
//

BOOST_AUTO_TEST_CASE(test_read_scalars) {
    ECF_NAME_THIS_TEST();

    const JsonReader reader;

    BOOST_CHECK(std::holds_alternative<CTree::Null>(reader.read("null", origin).node()));
    BOOST_CHECK_EQUAL(std::get<bool>(reader.read("true", origin).node()), true);
    BOOST_CHECK_EQUAL(std::get<bool>(reader.read("false", origin).node()), false);

    BOOST_CHECK_EQUAL(std::get<std::int64_t>(reader.read("0", origin).node()), 0);
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(reader.read("-5", origin).node()), -5);
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(reader.read("9223372036854775807", origin).node()), INT64_MAX);
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(reader.read("-9223372036854775808", origin).node()), INT64_MIN);

    // A fraction or an exponent makes a floating-point number
    BOOST_CHECK_EQUAL(std::get<double>(reader.read("1.5", origin).node()), 1.5);
    BOOST_CHECK_EQUAL(std::get<double>(reader.read("1e3", origin).node()), 1000.0);
    BOOST_CHECK_EQUAL(std::get<double>(reader.read("3141.0", origin).node()), 3141.0);

    BOOST_CHECK_EQUAL(std::get<std::string>(reader.read(R"("text")", origin).node()), "text");
    BOOST_CHECK_EQUAL(std::get<std::string>(reader.read(R"("")", origin).node()), "");
    BOOST_CHECK_EQUAL(std::get<std::string>(reader.read(R"("é\n")", origin).node()), "\xc3\xa9\n");

    // Surrounding white space is ignored
    BOOST_CHECK_EQUAL(std::get<bool>(reader.read(" \n\ttrue\n", origin).node()), true);

    // The root node has an empty path
    BOOST_CHECK_EQUAL(reader.read("true", origin).path(), "");
}

BOOST_AUTO_TEST_CASE(test_read_objects_and_arrays) {
    ECF_NAME_THIS_TEST();

    const auto tree = JsonReader().read(R"({
        "version": 1,
        "variables": { "ECF_PORT": 3141, "ECF_HOME": "/home" },
        "list": [ true, { "a": null }, [] ],
        "empty": {}
    })",
                                        origin);

    BOOST_REQUIRE(std::holds_alternative<CTree::Mapping>(tree.node()));
    BOOST_CHECK_EQUAL(std::get<CTree::Mapping>(tree.node()).size(), 4U);

    const auto* version = tree.find("version");
    BOOST_REQUIRE(version);
    BOOST_CHECK_EQUAL(version->path(), "/version");
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(version->node()), 1);

    const auto* variables = tree.find("variables");
    BOOST_REQUIRE(variables);
    BOOST_CHECK_EQUAL(variables->path(), "/variables");
    const auto* port = variables->find("ECF_PORT");
    BOOST_REQUIRE(port);
    BOOST_CHECK_EQUAL(port->path(), "/variables/ECF_PORT");
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(port->node()), 3141);
    const auto* home = variables->find("ECF_HOME");
    BOOST_REQUIRE(home);
    BOOST_CHECK_EQUAL(home->path(), "/variables/ECF_HOME");
    BOOST_CHECK_EQUAL(std::get<std::string>(home->node()), "/home");

    const auto* list = tree.find("list");
    BOOST_REQUIRE(list);
    BOOST_REQUIRE(std::holds_alternative<CTree::Sequence>(list->node()));
    const auto& items = std::get<CTree::Sequence>(list->node());
    BOOST_REQUIRE_EQUAL(items.size(), 3U);
    BOOST_CHECK_EQUAL(items[0].path(), "/list/0");
    BOOST_CHECK_EQUAL(std::get<bool>(items[0].node()), true);
    BOOST_CHECK_EQUAL(items[1].path(), "/list/1");
    const auto* a = items[1].find("a");
    BOOST_REQUIRE(a);
    BOOST_CHECK_EQUAL(a->path(), "/list/1/a");
    BOOST_CHECK(std::holds_alternative<CTree::Null>(a->node()));
    BOOST_CHECK_EQUAL(items[2].path(), "/list/2");
    BOOST_CHECK(std::get<CTree::Sequence>(items[2].node()).empty());

    const auto* empty = tree.find("empty");
    BOOST_REQUIRE(empty);
    BOOST_CHECK(std::get<CTree::Mapping>(empty->node()).empty());
}

BOOST_AUTO_TEST_CASE(test_read_keeps_document_order) {
    ECF_NAME_THIS_TEST();

    const auto tree    = JsonReader().read(R"({ "c": 1, "a": 2, "b": 3 })", origin);
    const auto& fields = std::get<CTree::Mapping>(tree.node());

    std::vector<std::string> keys;
    for (const auto& [key, value] : fields) {
        keys.push_back(key);
    }
    const std::vector<std::string> expected{"c", "a", "b"};
    BOOST_CHECK_EQUAL_COLLECTIONS(keys.begin(), keys.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_read_escapes_keys_in_paths) {
    ECF_NAME_THIS_TEST();

    // As per JSON Pointer, '~' is written '~0' and '/' is written '~1'
    const auto tree = JsonReader().read(R"({ "a/b": { "m~n": 1, "": 2 } })", origin);

    const auto* ab = tree.find("a/b");
    BOOST_REQUIRE(ab);
    BOOST_CHECK_EQUAL(ab->path(), "/a~1b");
    const auto* mn = ab->find("m~n");
    BOOST_REQUIRE(mn);
    BOOST_CHECK_EQUAL(mn->path(), "/a~1b/m~0n");
    const auto* empty = ab->find("");
    BOOST_REQUIRE(empty);
    BOOST_CHECK_EQUAL(empty->path(), "/a~1b/");
}

//
// Refusing malformed or ambiguous documents
//

BOOST_AUTO_TEST_CASE(test_refuse_duplicate_keys) {
    ECF_NAME_THIS_TEST();

    check_refused(R"({ "a": 1, "a": 2 })", {"/a", "duplicate"});
    check_refused(R"({ "a": 1, "a": 1 })", {"/a", "duplicate"});
    check_refused(R"({ "v": { "x": 1, "y": 2, "x": {} } })", {"/v/x", "duplicate"});
    check_refused(R"([ {}, { "k": 1, "k": 2 } ])", {"/1/k", "duplicate"});
}

BOOST_AUTO_TEST_CASE(test_refuse_integers_out_of_range) {
    ECF_NAME_THIS_TEST();

    check_refused("9223372036854775808", {"9223372036854775808", "range"});
    check_refused("-9223372036854775809", {"-9223372036854775809", "range"});
    check_refused("18446744073709551616", {"18446744073709551616", "range"});
    check_refused(R"({ "p": 99999999999999999999 })", {"/p", "99999999999999999999", "range"});
    check_refused("[ 1, 9223372036854775808 ]", {"/1", "9223372036854775808", "range"});
}

BOOST_AUTO_TEST_CASE(test_refuse_floating_point_numbers_out_of_range) {
    ECF_NAME_THIS_TEST();

    check_refused("1e400", {"1e400", "overflow"});
    check_refused("-1e400", {"-1e400", "overflow"});
    check_refused("1.8e308", {"1.8e308", "overflow"});
    check_refused("0.00018e312", {"0.00018e312", "overflow"});

    // The largest double is accepted, and an underflow becomes zero
    const JsonReader reader;
    BOOST_CHECK_EQUAL(std::get<double>(reader.read("1.7976931348623157e308", origin).node()),
                      std::numeric_limits<double>::max());
    BOOST_CHECK_EQUAL(std::get<double>(reader.read("1e-400", origin).node()), 0.0);
}

BOOST_AUTO_TEST_CASE(test_refuse_syntax_errors) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"",
                                   "   ",
                                   "{",
                                   "}",
                                   "[1,]",
                                   R"({ "a": 1, })",
                                   R"({ 'a': 1 })",
                                   R"({ "a" 1 })",
                                   R"({ a: 1 })",
                                   "tru",
                                   "True",
                                   "+1",
                                   "01",
                                   ".5",
                                   "NaN",
                                   "Infinity",
                                   R"("unterminated)",
                                   "\"\xff\""}) {
        check_refused(text, {"syntax error"});
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_syntax_errors_with_position) {
    ECF_NAME_THIS_TEST();

    check_refused("{\n  \"a\": 1,\n}", {"line 3, column 1"});
    check_refused("{\n  \"a\": 1\n  \"b\": 2\n}", {"line 3, column 5"});
}

BOOST_AUTO_TEST_CASE(test_error_messages) {
    ECF_NAME_THIS_TEST();

    // The origin, then the path when known, then the reason
    BOOST_CHECK_EQUAL(error_of(R"({ "a": 1, "a": 2 })"), origin + ": /a: duplicate key 'a'");
    BOOST_CHECK_EQUAL(error_of(R"({ "a/b": [ { "~": 1, "~": 2 } ] })"), origin + ": /a~1b/0/~0: duplicate key '~'");
    BOOST_CHECK_EQUAL(error_of(R"({ "p": 9223372036854775808 })"),
                      origin + ": /p: integer 9223372036854775808 is out of the range of a 64-bit signed integer");
    BOOST_CHECK_EQUAL(error_of("9223372036854775808"),
                      origin + ": integer 9223372036854775808 is out of the range of a 64-bit signed integer");

    // The syntax errors keep the position given by the parser, without the identifier of its exception
    const auto syntax = error_of("{ \"a\": }");
    BOOST_TEST_INFO("message: " << syntax);
    BOOST_CHECK(syntax.rfind(origin + ": parse error at line 1, column 8: syntax error", 0) == 0);
    BOOST_CHECK(syntax.find("json.exception") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_refuse_content_after_the_document) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"{} {}", "1 2", "{}x", "[] ,", "true false"}) {
        check_refused(text, {"syntax error"});
    }
}

BOOST_AUTO_TEST_CASE(test_refuse_comments) {
    ECF_NAME_THIS_TEST();

    for (const std::string text : {"// comment\n{}", "{} // comment", "{ /* comment */ }", "# comment\n{}"}) {
        check_refused(text, {"syntax error"});
    }
}

//
// Reading files
//

BOOST_AUTO_TEST_CASE(test_read_file) {
    ECF_NAME_THIS_TEST();

    const scaffold::WithTestFile file(scaffold::AutomaticTestFile{"ecf.cfg"},
                                      R"({ "variables": { "ECF_PORT": 3141 } })");

    const auto tree       = JsonReader().read_file(file.path());
    const auto* variables = tree.find("variables");
    BOOST_REQUIRE(variables);
    const auto* port = variables->find("ECF_PORT");
    BOOST_REQUIRE(port);
    BOOST_CHECK_EQUAL(std::get<std::int64_t>(port->node()), 3141);
}

BOOST_AUTO_TEST_CASE(test_read_file_names_the_file_in_errors) {
    ECF_NAME_THIS_TEST();

    const scaffold::WithTestFile file(scaffold::AutomaticTestFile{"ecf.cfg"}, R"({ "a": 1, "a": 2 })");

    check_error([&] { (void)JsonReader().read_file(file.path()); }, {file.path().string(), "/a", "duplicate"});
}

BOOST_AUTO_TEST_CASE(test_read_file_refuses_missing_file) {
    ECF_NAME_THIS_TEST();

    const std::string missing = "missing." + origin;
    check_error([&] { (void)JsonReader().read_file(missing); }, {missing});
}

BOOST_AUTO_TEST_CASE(test_read_file_refuses_directory) {
    ECF_NAME_THIS_TEST();

    const scaffold::MakeDirectory make;
    const auto directory = make.create();
    check_error([&] { (void)JsonReader().read_file(directory.path()); }, {directory.path().string()});
}

BOOST_AUTO_TEST_CASE(test_read_file_refuses_unreadable_file) {
    ECF_NAME_THIS_TEST();

    const scaffold::WithTestFile file(scaffold::AutomaticTestFile{"ecf.cfg"}, "{}");
    std::filesystem::permissions(file.path(), std::filesystem::perms::none);

    if (std::ifstream(file.path())) {
        // e.g. when running as root, permissions do not prevent reading
        BOOST_TEST_MESSAGE("Skipped: the file remains readable without permissions");
    }
    else {
        check_error([&] { (void)JsonReader().read_file(file.path()); }, {file.path().string(), "cannot be read"});
    }

    std::filesystem::permissions(file.path(), std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
