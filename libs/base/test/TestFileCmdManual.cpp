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
/// @brief Requests a file of a node from the server, as `ecflow_client --file=<path> manual` does.
///
/// @param[in] tree The definition held by the server.
/// @param[in] path The path of the node.
/// @param[in] type The kind of file requested, the manual by default.
/// @return The reply of the server; an exception thrown by the handling of the request, which the server reports to
///         the client as an error, is given as the error.
///
Reply request_manual(Tree& tree, const std::string& path, CFileCmd::File_t type = CFileCmd::MANUAL) {
    ClientToServerRequest request;
    request.set_cmd(std::make_shared<CFileCmd>(path, type, 10000));
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

///
/// @brief Prepares the files of the pre-processing cases: a script, a manual for each kind of node and the included
/// files, under ECF_HOME, two ECF_INCLUDE directories and the directory of the script.
///
struct PreprocessingTree : Tree
{
    explicit PreprocessingTree(Manuals& manuals) {
        alias   = task->add_alias("alias0");
        family2 = suite->add_family("family2");
        other   = family2->add_task("other");

        suite->addVariable(Variable("ECF_HOME", manuals.dir("home")));
        suite->addVariable(Variable("INC2", manuals.dir("inc2")));
        suite->addVariable(Variable("ECF_INCLUDE", "$ECF_HOME/../inc1:%INC2%"));
        suite->addVariable(Variable("PART", "part"));

        manuals.add("home/suite/family/task.ecf",
                    "%include <head.h>\n"
                    "%manual\n"
                    "Manual of %TASK% at %ECF_NAME% in %FAMILY%\n"
                    "%include <man_part.h>\n"
                    "%end\n"
                    "%include \"local.h\"\n"
                    "%include \"./sibling.h\"\n"
                    "%include <%PART%.h>\n"
                    "%comment\n"
                    "comment %UNDEFINED_IN_COMMENT%\n"
                    "%end\n"
                    "echo %FAMILY%\n"
                    "%manual\n"
                    "second block %SUITE% %UNDEFINED_IN_MANUAL% 100%%\n"
                    "%end\n"
                    "%includenopp <raw.h>\n"
                    "%nopp\n"
                    "%TASK% is not substituted here\n"
                    "%end\n"
                    "%ecfmicro &\n"
                    "&manual\n"
                    "micro &TASK& %TASK%\n"
                    "&end\n"
                    "&ecfmicro %\n"
                    "%include <tail.h>\n");
        manuals.add("inc1/head.h", "%manual\nfrom head %SUITE%\n%end\n%include <nested.h>\n");
        manuals.add("inc2/nested.h", "%manual\nnested from inc2 %FAMILY%\n%end\n");
        manuals.add("inc1/man_part.h", "part of the manual from %ECF_INCLUDE%\n");
        manuals.add("inc2/part.h", "%manual\nfrom a variable include name\n%end\n");
        manuals.add("home/suite/family/local.h", "%manual\nlocal %TASK%\n%end\n");
        manuals.add("home/suite/family/sibling.h", "%manual\nsibling\n%end\n");
        manuals.add("inc1/raw.h", "%TASK% raw\n");
        manuals.add("home/tail.h", "%manual\ntail from ECF_HOME\n%end\n");

        manuals.add("home/suite/family/task/alias0.usr",
                    "%include <head.h>\n%manual\nalias %TASK% %ECF_NAME%\n%end\necho alias\n");

        family2->addVariable(Variable("ECF_MICRO", "^"));
        manuals.add("home/suite/family2/other.ecf", "^include <head.h>\n^manual\nother ^TASK^ %TASK%\n^end\n");

        manuals.add("home/suite/family.man", "Family %FAMILY%\n%include <head.h>\n%include \"local.h\"\nend\n");
        manuals.add("home/suite.man", "Suite %SUITE%\n%manual\nonly %SUITE%\n%end\nnot this\n%include <nested.h>\n");
    }

    alias_ptr alias;
    family_ptr family2;
    task_ptr other;
};

///
/// @brief Replaces every "<ROOT>" of a text by the provisioned directory.
///
/// @param[in] manuals The provisioned directory.
/// @param[in] text The text.
/// @return The text with the directory.
///
std::string rooted(const Manuals& manuals, std::string text) {
    for (auto at = text.find("<ROOT>"); at != std::string::npos; at = text.find("<ROOT>")) {
        text.replace(at, 6, manuals.root());
    }
    return text;
}

BOOST_AUTO_TEST_SUITE(T_FileCmdPreprocessing)

BOOST_AUTO_TEST_CASE(test_task_manual_with_includes_and_variables) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    const auto reply = request_manual(tree, "/suite/family/task");
    check_manual(reply,
                 rooted(manuals,
                        "from head suite\n"
                        "nested from inc2 family\n"
                        "Manual of task at /suite/family/task in family\n"
                        "part of the manual from $ECF_HOME/../inc1:<ROOT>/inc2\n"
                        "local task\n"
                        "sibling\n"
                        "from a variable include name\n"
                        "second block suite %UNDEFINED_IN_MANUAL% 100%%\n"
                        "micro task %TASK%\n"
                        "tail from ECF_HOME\n"));
}

BOOST_AUTO_TEST_CASE(test_alias_manual) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    const auto reply = request_manual(tree, "/suite/family/task/alias0");
    check_manual(reply,
                 rooted(manuals,
                        "from head suite\n"
                        "nested from inc2 family\n"
                        "alias task /suite/family/task/alias0\n"));
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_a_user_micro) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    const auto reply = request_manual(tree, "/suite/family2/other");
    check_manual(reply, rooted(manuals, "other other %TASK%\n"));
}

BOOST_AUTO_TEST_CASE(test_family_manual_with_includes) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    const auto reply = request_manual(tree, "/suite/family");
    check_manual(reply,
                 rooted(manuals,
                        "from head suite\n"
                        "nested from inc2 family\n"
                        "local %TASK%\n"));
}

BOOST_AUTO_TEST_CASE(test_suite_manual_with_a_manual_section) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    const auto reply = request_manual(tree, "/suite");
    check_manual(reply,
                 rooted(manuals,
                        "only suite\n"
                        "nested from inc2 %FAMILY%\n"));
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_a_missing_include) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    manuals.add("home/suite/family/third.ecf", "%manual\n%include <missing.h>\n%end\n");
    tree.family->add_task("third");
    const auto reply = request_manual(tree, "/suite/family/third");
    check_error(reply,
                rooted(manuals,
                       "EcfFile::manual:: Failed preprocessing : TASK:/suite/family/third : "
                       "path/cmd(<ROOT>/home/suite/family/third.ecf)\n"
                       ": Could not open include file: <ROOT>/home/missing.h (No such file or directory) include file "
                       "cache size:1"));
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_an_include_name_that_cannot_be_substituted) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    manuals.add("home/suite/family/third.ecf", "%include \"%UNDEFINED%.h\"\n");
    tree.family->add_task("third");
    const auto reply = request_manual(tree, "/suite/family/third");
    check_error(reply,
                rooted(manuals,
                       "EcfFile::manual:: Failed preprocessing : TASK:/suite/family/third : "
                       "path/cmd(<ROOT>/home/suite/family/third.ecf)\n"
                       ": Could not open include file: <ROOT>/home/suite/family/%UNDEFINED%.h (No such file or "
                       "directory) include file cache size:1"));
}

BOOST_AUTO_TEST_CASE(test_suite_manual_with_a_quoted_include) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    manuals.add("home/suite.man", "%include \"local.h\"\n");
    const auto reply = request_manual(tree, "/suite");
    check_error(reply,
                rooted(manuals,
                       "EcfFile::manual:: Failed preprocessing : SUITE:/suite : path/cmd(<ROOT>/home/suite.man)\n"
                       ": FAMILY not specified, at : %include \"local.h\""));
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_an_invalid_micro) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    tree.family2->addVariable(Variable("ECF_MICRO", "^^"));
    const auto reply = request_manual(tree, "/suite/family2/other");
    check_error(reply,
                rooted(manuals,
                       "EcfFile::EcfFile: Node /suite/family2/other is referencing a invalid ECF_MICRO variable(' ^^). "
                       "ECF_MICRO when overridden, must be a single character."));
}

BOOST_AUTO_TEST_CASE(test_script_from_a_command) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    manuals.add("scripts/third.src", "%manual\nfrom a command %TASK%\n%end\n%include <head.h>\necho third\n");
    task_ptr third = tree.family->add_task("third");
    third->addVariable(Variable("ECF_SCRIPT_CMD", "cat " + manuals.root() + "/scripts/%TASK%.src"));
    const auto reply = request_manual(tree, "/suite/family/third", CFileCmd::ECF);
    check_manual(reply,
                 rooted(manuals,
                        "# ecf_script_origin : ECF_SCRIPT_CMD : cat <ROOT>/scripts/third.src\n"
                        "%manual\n"
                        "from a command %TASK%\n"
                        "%end\n"
                        "%include <head.h>\n"
                        "echo third\n"));
}

BOOST_AUTO_TEST_CASE(test_manual_from_a_command) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    manuals.add("scripts/third.src", "%manual\nfrom a command %TASK%\n%end\n%include <head.h>\necho third\n");
    task_ptr third = tree.family->add_task("third");
    third->addVariable(Variable("ECF_SCRIPT_CMD", "cat " + manuals.root() + "/scripts/%TASK%.src"));
    const auto reply = request_manual(tree, "/suite/family/third");
    check_manual(reply,
                 rooted(manuals,
                        "from a command third\n"
                        "from head suite\n"
                        "nested from inc2 family\n"));
}

BOOST_AUTO_TEST_CASE(test_manual_from_ecf_fetch_with_a_user_extension) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    manuals.add("fetch.sh", "printf '%smanual\\nfetched with %s\\n%send\\n' % \"$*\" %\n");
    task_ptr fetched = tree.family->add_task("fetched");
    fetched->addVariable(Variable("ECF_FETCH", "sh " + manuals.root() + "/fetch.sh"));
    fetched->addVariable(Variable("ECF_EXTN", ".sms"));
    const auto reply = request_manual(tree, "/suite/family/fetched");
    check_manual(reply, "fetched with -s fetched.sms\n");
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_repeats_named_like_the_variables) {
    ECF_NAME_THIS_TEST();

    Manuals manuals;
    PreprocessingTree tree(manuals);
    family_ptr family3 = tree.suite->add_family("family3");
    family_ptr sub     = family3->add_family("sub");
    task_ptr task4     = sub->add_task("task4");
    family3->addRepeat(Repeat(RepeatString("ECF_INCLUDE", {manuals.root() + "/repeat_inc"})));
    sub->addRepeat(Repeat(RepeatString("ECF_MICRO", {"&"})));
    task4->addRepeat(Repeat(RepeatString("ECF_HOME", {manuals.root() + "/x/y"})));
    manuals.add("home/suite/family3/sub/task4.ecf",
                "%manual\n%include <tail2.h>\n%include <dollar.h>\n%include \"q.h\"\n&TASK& %TASK%\n%end\n");
    manuals.add("x/y/tail2.h", "tail2 from the repeat ECF_HOME\n");
    manuals.add("home/tail2.h", "tail2 from the user ECF_HOME\n");
    manuals.add("x/inc1/dollar.h", "dollar from the repeat ECF_HOME\n");
    manuals.add("inc1/dollar.h", "dollar from the user ECF_HOME\n");
    manuals.add("repeat_inc/tail2.h", "tail2 from the repeat ECF_INCLUDE\n");
    manuals.add("repeat_inc/dollar.h", "dollar from the repeat ECF_INCLUDE\n");
    manuals.add("home/suite/family3/sub/q.h", "q from the user ECF_HOME\n");
    manuals.add("x/y/suite/family3/sub/q.h", "q from the repeat ECF_HOME\n");
    const auto reply = request_manual(tree, "/suite/family3/sub/task4");
    check_manual(reply,
                 "tail2 from the repeat ECF_HOME\n"
                 "dollar from the repeat ECF_HOME\n"
                 "q from the user ECF_HOME\n"
                 "&TASK& task4\n");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
