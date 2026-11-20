// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

namespace fs       = std::filesystem;
namespace scaffold = ecf::test::scaffold;
using namespace ecf;

namespace {

///
/// @brief The manuals of one test, in a directory provisioned for it, which also holds the server log that the
/// server's handling of a request needs.
///
class Manuals : public scaffold::TestFiles {
public:
    Manuals()
        : log_(root() + "/server.log") {}

private:
    scaffold::TestLog log_;
};

///
/// @brief A suite /suite/family/task.
///
struct Tree
{
    Tree()
        : suite(defs.add_suite("suite")),
          family(suite->add_family("family")),
          task(family->add_task("task")) {}

    Defs defs;
    suite_ptr suite;
    family_ptr family;
    task_ptr task;
};

///
/// @brief The reply of the server to a request for a manual: the manual, or the error.
///
struct Reply
{
    std::string manual;
    std::string error;
};

///
/// @brief Requests the manual of a node from the server, as `ecflow_client --file=<path> manual` does.
///
/// @param[in] tree The definition held by the server.
/// @param[in] path The path of the node.
/// @return The reply of the server; an exception thrown by the handling of the request, which the server reports to
///         the client as an error, is given as the error.
///
Reply request_manual(Tree& tree, const std::string& path) {
    ClientToServerRequest request;
    request.set_cmd(std::make_shared<CFileCmd>(path, CFileCmd::MANUAL, 10000));
    MockServer server(&tree.defs);
    try {
        STC_Cmd_ptr result = request.handleRequest(&server);
        BOOST_REQUIRE(result);
        return {result->get_string(), result->error()};
    }
    catch (const std::runtime_error& e) {
        return {"", e.what()};
    }
}

///
/// @brief Checks that the server gives the expected manual, as captured before the search was shared.
///
/// @param[in] reply The reply of the server.
/// @param[in] expected The expected manual.
///
void check_manual(const Reply& reply, const std::string& expected) {
    BOOST_CHECK_EQUAL(reply.error, "");
    BOOST_CHECK_EQUAL(reply.manual, expected);
}

///
/// @brief Checks that the server gives the expected error, as captured before the search was shared.
///
/// @param[in] reply The reply of the server.
/// @param[in] expected The expected error.
///
void check_error(const Reply& reply, const std::string& expected) {
    BOOST_CHECK_EQUAL(reply.manual, "");
    BOOST_CHECK_EQUAL(reply.error, expected);
}

///
/// @brief Makes a path a symbolic link to itself, so that any attempt to examine it fails.
///
/// @param[in] path The path of the link.
/// @return The path of the link.
///
std::string make_link_loop(const std::string& path) {
    fs::create_symlink(fs::path(path).filename(), path);
    return path;
}

const std::string family_manual = "Manual of %FAMILY% in %SUITE%\nsecond line\n";

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_FileCmdManual)

BOOST_AUTO_TEST_CASE(test_family_manual_from_ecf_files) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.dir("home")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("files/family.man", family_manual);
    manuals.add("home/suite/family.man", "Manual in ECF_HOME\n");
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply, "Manual of family in suite\nsecond line\n");
}

BOOST_AUTO_TEST_CASE(test_suite_manual_from_ecf_home) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.root()));
    manuals.add("suite.man", "%manual\nOnly this\n%end\nnot this\n");
    const auto reply = request_manual(tree, "/suite");
    check_manual(reply, "Only this\n");
}

BOOST_AUTO_TEST_CASE(test_empty_manual_in_ecf_files_falls_back_to_ecf_home) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.dir("home")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("files/family.man", "");
    manuals.add("home/family.man", "Manual in ECF_HOME\n");
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply, "Manual in ECF_HOME\n");
}

BOOST_AUTO_TEST_CASE(test_manual_missing_from_ecf_files_found_in_ecf_home) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.dir("home")));
    tree.family->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("home/suite/family.man", "%include <head.h>\nin ECF_HOME\n");
    manuals.add("home/head.h", "included\n");
    tree.suite->addVariable(Variable("ECF_INCLUDE", manuals.root() + "/home"));
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply, "included\nin ECF_HOME\n");
}

BOOST_AUTO_TEST_CASE(test_manual_not_found_when_ecf_home_does_not_exist) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.root() + "/missing"));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    const auto reply = request_manual(tree, "/suite/family");
    check_error(reply,
                "Failed to find the manual for Suite/Family  /suite/family ECF_HOME directory does not exist and/or "
                "ECF_FILES not defined or directory does not exist.");
}

BOOST_AUTO_TEST_CASE(test_manual_not_found_in_ecf_home) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.root()));
    const auto reply = request_manual(tree, "/suite/family");
    check_error(reply,
                "EcfFile::manual: For node FAMILY:/suite/family, failed to open file  : "
                "EcfFile::open_script_file: Could not open ecf manual file. Input File/cmd string is empty.");
}

BOOST_AUTO_TEST_CASE(test_manual_found_in_ecf_files_when_ecf_home_does_not_exist) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.root() + "/missing"));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("files/suite.man", "Manual of the suite\n");
    const auto reply = request_manual(tree, "/suite");
    check_manual(reply, "Manual of the suite\n");
}

BOOST_AUTO_TEST_CASE(test_manual_directories_are_read_from_user_variables) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.dir("home")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    tree.family->addRepeat(Repeat(RepeatString("ECF_FILES", {manuals.dir("elsewhere")})));
    manuals.add("elsewhere/family.man", "Manual from a repeat\n");
    manuals.add("files/family.man", "Manual from the user variable\n");
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply, "Manual from the user variable\n");
}

BOOST_AUTO_TEST_CASE(test_empty_manual_in_ecf_files_when_ecf_home_does_not_exist) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.root() + "/missing"));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("files/family.man", "");
    const auto reply = request_manual(tree, "/suite/family");
    check_error(reply,
                "Failed to find the manual for Suite/Family  /suite/family ECF_HOME directory does not exist and/or "
                "ECF_FILES not defined or directory does not exist.");
}

BOOST_AUTO_TEST_CASE(test_manual_in_ecf_home_when_ecf_files_does_not_exist) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", manuals.dir("home")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.root() + "/missing"));
    manuals.add("home/family.man", "Manual in ECF_HOME\n");
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply, "Manual in ECF_HOME\n");
}

BOOST_AUTO_TEST_CASE(test_manual_in_ecf_files_when_ecf_home_cannot_be_examined) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", make_link_loop(manuals.root() + "/loop")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("files/family.man", "Manual in ECF_FILES\n");
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply, "Manual in ECF_FILES\n");
}

BOOST_AUTO_TEST_CASE(test_failing_manual_in_ecf_files_when_ecf_home_cannot_be_examined) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", make_link_loop(manuals.root() + "/loop")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    manuals.add("files/family.man", "%manual\nnever ended\n");
    const auto reply = request_manual(tree, "/suite/family");
    check_error(reply,
                "EcfFile::manual:: Failed preprocessing : FAMILY:/suite/family : path/cmd(" + manuals.root() +
                    "/files/family.man)\n: Unterminated manual, matching 'end' is missing");
}

BOOST_AUTO_TEST_CASE(test_fallback_to_ecf_home_that_cannot_be_examined) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", make_link_loop(manuals.root() + "/loop")));
    tree.suite->addVariable(Variable("ECF_FILES", manuals.dir("files")));
    const auto reply = request_manual(tree, "/suite/family");
    // The text of the error is that of the standard library; it names the path that cannot be examined
    BOOST_CHECK_EQUAL(reply.manual, "");
    BOOST_CHECK_MESSAGE(reply.error.find(manuals.root() + "/loop") != std::string::npos, reply.error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
