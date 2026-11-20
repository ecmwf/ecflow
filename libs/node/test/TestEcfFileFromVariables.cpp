// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EcfFile.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/ScriptLocator.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/VariableLookup.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace fs       = std::filesystem;
namespace scaffold = ecf::test::scaffold;
using namespace ecf;

namespace {

/// The files of one test, in a directory provisioned for it
using Files = scaffold::TestFiles;

///
/// @brief A suite with scripts, manuals and included files that use every form of include, variables, a user micro,
/// an alias, a script obtained by a command, and files that fail to pre-process.
///
struct Tree
{
    explicit Tree(Files& files) {
        suite   = defs.add_suite("suite");
        family  = suite->add_family("family");
        task    = family->add_task("task");
        alias   = task->add_alias("alias0");
        family2 = suite->add_family("family2");
        other   = family2->add_task("other");
        cmd     = family->add_task("cmd");
        broken  = family->add_task("broken");

        suite->addVariable(Variable("ECF_HOME", files.root() + "/home"));
        suite->addVariable(Variable("INC2", files.root() + "/inc2"));
        suite->addVariable(Variable("ECF_INCLUDE", "$ECF_HOME/../inc1:%INC2%"));
        suite->addVariable(Variable("PART", "part"));
        family2->addVariable(Variable("ECF_MICRO", "^"));
        cmd->addVariable(Variable("ECF_SCRIPT_CMD", "cat " + files.root() + "/scripts/%TASK%.src"));

        files.add("home/suite/family/task.ecf",
                  "%include <head.h>\n%manual\nManual of %TASK% at %ECF_NAME%\n%include <man_part.h>\n%end\n"
                  "%include \"local.h\"\n%include \"./sibling.h\"\n%include <%PART%.h>\n"
                  "%manual\nsecond %SUITE% %UNDEFINED% 100%%\n%end\n%includenopp <raw.h>\n"
                  "%ecfmicro &\n&manual\nmicro &TASK& %TASK%\n&end\n&ecfmicro %\n%include <tail.h>\n");
        files.add("inc1/head.h", "%manual\nfrom head %SUITE%\n%end\n%include <nested.h>\n");
        files.add("inc2/nested.h", "%manual\nnested %FAMILY%\n%end\n");
        files.add("inc1/man_part.h", "part from %ECF_INCLUDE%\n");
        files.add("inc2/part.h", "%manual\nfrom a variable include name\n%end\n");
        files.add("home/suite/family/local.h", "%manual\nlocal %TASK%\n%end\n");
        files.add("home/suite/family/sibling.h", "%manual\nsibling\n%end\n");
        files.add("inc1/raw.h", "%TASK% raw\n");
        files.add("home/tail.h", "%manual\ntail\n%end\n");
        files.add("home/suite/family/task/alias0.usr", "%include <head.h>\n%manual\nalias %TASK% %ECF_NAME%\n%end\n");
        files.add("home/suite/family2/other.ecf", "^include <head.h>\n^manual\nother ^TASK^ %TASK%\n^end\n");
        files.add("scripts/cmd.src", "%manual\nfrom a command %TASK%\n%end\n%include <head.h>\n");
        files.add("home/suite/family/broken.ecf", "%manual\n%include <missing.h>\n%end\n");
        files.add("home/suite/family.man", "Family %FAMILY%\n%include <head.h>\n%include \"local.h\"\n");
        files.add("home/suite.man", "%manual\nonly %SUITE%\n%end\nnot this\n%include \"local.h\"\n");
    }

    Defs defs;
    suite_ptr suite;
    family_ptr family;
    task_ptr task;
    alias_ptr alias;
    family_ptr family2;
    task_ptr other;
    task_ptr cmd;
    task_ptr broken;
};

/// @brief The outcome of producing a file: its content, or the error.
struct Outcome
{
    std::string content;
    std::string error;
};

///
/// @brief Produces the manual, or the script, of a node through its node, as the server does.
///
/// @param[in] node The node.
/// @param[in] script Whether to produce the script rather than the manual.
/// @return The outcome.
///
Outcome through_node(Node& node, bool script) {
    Outcome outcome;
    try {
        if (Submittable* submittable = node.isSubmittable()) {
            EcfFile file = submittable->locatedEcfFile();
            script ? file.script(outcome.content) : file.manual(outcome.content);
        }
        else {
            const NodeVariableLookup lookup(node);
            auto path = locate_manual_in_ecf_files(node.absNodePath(), lookup);
            if (path.empty()) {
                path = locate_manual_in_ecf_home(node.absNodePath(), lookup).value_or("");
            }
            EcfFile file(&node, path);
            file.manual(outcome.content);
        }
    }
    catch (const std::runtime_error& e) {
        outcome.error = e.what();
    }
    return outcome;
}

///
/// @brief Produces the manual, or the script, of a node from a copy of its variables only.
///
/// @param[in] node The node; its generated variables are updated first, then only its variables are used.
/// @param[in] script Whether to produce the script rather than the manual.
/// @return The outcome.
///
Outcome through_variables(const Node& node, bool script) {
    node.update_generated_variables();
    auto lookup     = std::make_shared<MapVariableLookup>(MapVariableLookup::from_node(node));
    const auto path = node.absNodePath();
    Outcome outcome;
    try {
        const auto kind = EcfFile::node_kind(node);
        if (node.isSubmittable()) {
            const auto location = locate_script(path, EcfFile::script_extension(kind, *lookup), *lookup);
            EcfFile file(kind, path, lookup, location.path_or_cmd, location.origin, location.algorithm);
            script ? file.script(outcome.content) : file.manual(outcome.content);
        }
        else {
            auto manual = locate_manual_in_ecf_files(path, *lookup);
            if (manual.empty()) {
                manual = locate_manual_in_ecf_home(path, *lookup).value_or("");
            }
            EcfFile file(kind, path, lookup, manual);
            file.manual(outcome.content);
        }
    }
    catch (const std::runtime_error& e) {
        outcome.error = e.what();
    }
    return outcome;
}

///
/// @brief Checks that both ways give the same, expected, outcome.
///
/// @param[in] node The node.
/// @param[in] script Whether to produce the script rather than the manual.
/// @param[in] expect_error Whether an error is expected rather than a content.
///
void check_same(Node& node, bool script = false, bool expect_error = false) {
    const auto from_node      = through_node(node, script);
    const auto from_variables = through_variables(node, script);
    BOOST_TEST_MESSAGE(node.absNodePath() << ": [" << from_node.content << "] [" << from_node.error << "]");
    BOOST_CHECK_EQUAL(from_variables.content, from_node.content);
    BOOST_CHECK_EQUAL(from_variables.error, from_node.error);
    BOOST_CHECK_EQUAL(!from_node.error.empty(), expect_error);
    BOOST_CHECK_EQUAL(from_node.content.empty(), expect_error);
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_EcfFileFromVariables)

BOOST_AUTO_TEST_CASE(test_task_manual) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    check_same(*tree.task);
}

BOOST_AUTO_TEST_CASE(test_alias_manual) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    check_same(*tree.alias);
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_a_user_micro) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    check_same(*tree.other);
}

BOOST_AUTO_TEST_CASE(test_family_manual) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    check_same(*tree.family);
}

BOOST_AUTO_TEST_CASE(test_suite_manual_with_a_failing_include) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    check_same(*tree.suite, false, true);
}

BOOST_AUTO_TEST_CASE(test_command_origins_are_refused_without_a_node) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    const auto marker = files.root() + "/command_was_run";
    task_ptr fetched  = tree.family->add_task("fetched");
    fetched->addVariable(Variable("ECF_FETCH", "touch " + marker + " ; echo"));
    tree.cmd->addVariable(Variable("ECF_SCRIPT_CMD", "touch " + marker + " ; cat %TASK%"));

    for (const auto* node : {static_cast<const Node*>(tree.cmd.get()), static_cast<const Node*>(fetched.get())}) {
        node->update_generated_variables();
        auto lookup         = std::make_shared<MapVariableLookup>(MapVariableLookup::from_node(*node));
        const auto location = locate_script(node->absNodePath(), ".ecf", *lookup);
        BOOST_CHECK(location.needs_command());
        BOOST_CHECK_THROW(
            EcfFile(EcfFile::NodeKind::TASK, node->absNodePath(), lookup, location.path_or_cmd, location.origin),
            std::invalid_argument);
    }
    BOOST_CHECK(!fs::exists(marker));
}

BOOST_AUTO_TEST_CASE(test_manual_with_a_missing_include) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    check_same(*tree.broken, false, true);
}

BOOST_AUTO_TEST_CASE(test_invalid_micro) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    tree.family2->addVariable(Variable("ECF_MICRO", "^^"));
    check_same(*tree.other, false, true);
}

BOOST_AUTO_TEST_CASE(test_manual_over_a_hand_built_lookup) {
    ECF_NAME_THIS_TEST();

    Files files;
    files.add("home/s/t.ecf", "%include <head.h>\n%manual\n%TASK% of %SUITE% $ %UNKNOWN%\n%end\n");
    files.add("include/head.h", "%manual\nhead in %ECF_HOME%\n%end\n");
    const auto home = files.root() + "/home";
    auto lookup     = std::make_shared<MapVariableLookup>(
        NameValueMap{{"ECF_HOME", home}, {"ECF_INCLUDE", "$ECF_HOME/../include"}, {"TASK", "t"}, {"SUITE", "s"}},
        NameValueMap{{"TASK", "t"}, {"SUITE", "s"}},
        NameValueMap{{"ECF_HOME", home}, {"ECF_INCLUDE", "$ECF_HOME/../include"}, {"ECF_MICRO", "%"}});

    EcfFile file(EcfFile::NodeKind::TASK, "/s/t", lookup, home + "/s/t.ecf");
    std::string manual;
    file.manual(manual);
    BOOST_CHECK_EQUAL(manual, "head in " + home + "\nt of s $ %UNKNOWN%\n");
}

BOOST_AUTO_TEST_CASE(test_operations_that_need_a_node_fail_without_one) {
    ECF_NAME_THIS_TEST();

    Files files;
    files.add("home/s/t.ecf", "echo %TASK%\n");
    const auto home = files.root() + "/home";
    auto lookup     = std::make_shared<MapVariableLookup>(NameValueMap{{"ECF_HOME", home}, {"TASK", "t"}},
                                                      NameValueMap{{"TASK", "t"}},
                                                      NameValueMap{{"ECF_HOME", home}, {"ECF_MICRO", "%"}});

    EcfFile file(EcfFile::NodeKind::TASK, "/s/t", lookup, home + "/s/t.ecf");
    std::string out;
    JobsParam jobs_param;
    const auto needs_node = [](const std::runtime_error& e) {
        return std::string(e.what()).find("needs the node") != std::string::npos;
    };
    BOOST_CHECK_EXCEPTION(file.create_job(jobs_param), std::runtime_error, needs_node);
    BOOST_CHECK_EXCEPTION(file.pre_process(out), std::runtime_error, needs_node);
    BOOST_CHECK_EXCEPTION(file.edit_used_variables(out), std::runtime_error, needs_node);

    std::vector<std::string> user_file{"echo %TASK%"};
    std::string pre_processed = "unchanged";
    BOOST_CHECK_EXCEPTION(file.pre_process_user_file(user_file, pre_processed), std::runtime_error, needs_node);
    BOOST_CHECK(user_file == std::vector<std::string>{"echo %TASK%"});
    BOOST_CHECK_EQUAL(pre_processed, "unchanged");
}

BOOST_AUTO_TEST_CASE(test_script_without_a_node) {
    ECF_NAME_THIS_TEST();

    Files files;
    files.add("home/s/t.ecf", "%include <head.h>\necho %TASK%\n");
    const auto home = files.root() + "/home";
    auto lookup     = std::make_shared<MapVariableLookup>(
        NameValueMap{{"ECF_HOME", home}}, NameValueMap{}, NameValueMap{{"ECF_HOME", home}, {"ECF_MICRO", "%"}});

    { // the script as it is, not pre-processed
        EcfFile file(EcfFile::NodeKind::TASK, "/s/t", lookup, home + "/s/t.ecf");
        std::string script;
        file.script(script);
        BOOST_CHECK_EQUAL(script, "%include <head.h>\necho %TASK%\n");
    }
    { // a missing script, named as the server names it
        EcfFile file(EcfFile::NodeKind::TASK, "/s/missing", lookup, home + "/s/missing.ecf");
        std::string script;
        BOOST_CHECK_EXCEPTION(file.script(script), std::runtime_error, [&](const std::runtime_error& e) {
            return std::string(e.what()) ==
                   "EcfFile::script: Could not open script for task/alias /s/missing at path " + home +
                       "/s/missing.ecf (No such file or directory)";
        });
    }
}

BOOST_AUTO_TEST_CASE(test_null_lookup_is_rejected) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_THROW(EcfFile(EcfFile::NodeKind::TASK, "/s/t", nullptr, "/nowhere/t.ecf"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(test_node_file_takes_the_extension_and_path_when_needed) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    files.add("fetch.sh", "printf '%smanual\\nfetched with %s\\n%send\\n' % \"$*\" %\n");
    task_ptr fetched = tree.family->add_task("fetched");
    fetched->addVariable(Variable("ECF_FETCH", "sh " + files.root() + "/fetch.sh"));
    fetched->addVariable(Variable("ECF_EXTN", ".sms"));

    EcfFile file = fetched->locatedEcfFile();
    fetched->addVariable(Variable("ECF_EXTN", ".new")); // changed after the file was created, as before honoured
    std::string manual;
    file.manual(manual);
    BOOST_CHECK_EQUAL(manual, "fetched with -s fetched.new\n");
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_a_user_extension) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    task_ptr sms = tree.family->add_task("sms");
    sms->addVariable(Variable("ECF_EXTN", ".sms"));
    files.add("home/suite/family/sms.ecf", "%manual\nthe .ecf script\n%end\n");
    files.add("home/suite/family/sms.sms", "%manual\nthe .sms script of %TASK%\n%end\n");
    check_same(*sms);
    BOOST_CHECK_EQUAL(through_variables(*sms, false).content, "the .sms script of sms\n");
}

BOOST_AUTO_TEST_CASE(test_script_extension_and_node_kind) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    MapVariableLookup plain({}, {}, {});
    MapVariableLookup with_extn({{"ECF_EXTN", ".all"}}, {}, {{"ECF_EXTN", ".sms"}});
    BOOST_CHECK_EQUAL(EcfFile::script_extension(EcfFile::NodeKind::TASK, plain), ".ecf");
    BOOST_CHECK_EQUAL(EcfFile::script_extension(EcfFile::NodeKind::TASK, with_extn), ".sms"); // user variable only
    BOOST_CHECK_EQUAL(EcfFile::script_extension(EcfFile::NodeKind::ALIAS, with_extn), ".usr");
    BOOST_CHECK_EQUAL(EcfFile::script_extension(EcfFile::NodeKind::FAMILY, plain), "");
    BOOST_CHECK_EQUAL(EcfFile::script_extension(EcfFile::NodeKind::SUITE, plain), "");

    BOOST_CHECK(EcfFile::node_kind(*tree.task) == EcfFile::NodeKind::TASK);
    BOOST_CHECK(EcfFile::node_kind(*tree.alias) == EcfFile::NodeKind::ALIAS);
    BOOST_CHECK(EcfFile::node_kind(*tree.family) == EcfFile::NodeKind::FAMILY);
    BOOST_CHECK(EcfFile::node_kind(*tree.suite) == EcfFile::NodeKind::SUITE);
}

BOOST_AUTO_TEST_CASE(test_task_manual_with_repeats_named_like_the_variables) {
    ECF_NAME_THIS_TEST();

    Files files;
    Tree tree(files);
    family_ptr family3 = tree.suite->add_family("family3");
    family_ptr sub     = family3->add_family("sub");
    task_ptr task4     = sub->add_task("task4");
    family3->addRepeat(Repeat(RepeatString("ECF_INCLUDE", {files.root() + "/repeat_inc"})));
    sub->addRepeat(Repeat(RepeatString("ECF_MICRO", {"&"})));
    task4->addRepeat(Repeat(RepeatString("ECF_HOME", {files.root() + "/x/y"})));
    files.add("home/suite/family3/sub/task4.ecf",
              "%manual\n%include <tail2.h>\n%include <dollar.h>\n%include \"q.h\"\n&TASK& %TASK%\n%end\n");
    files.add("x/y/tail2.h", "tail2 from the repeat ECF_HOME\n");
    files.add("home/tail2.h", "tail2 from the user ECF_HOME\n");
    files.add("x/inc1/dollar.h", "dollar from the repeat ECF_HOME\n");
    files.add("inc1/dollar.h", "dollar from the user ECF_HOME\n");
    files.add("repeat_inc/tail2.h", "tail2 from the repeat ECF_INCLUDE\n");
    files.add("repeat_inc/dollar.h", "dollar from the repeat ECF_INCLUDE\n");
    files.add("home/suite/family3/sub/q.h", "q from the user ECF_HOME\n");
    files.add("x/y/suite/family3/sub/q.h", "q from the repeat ECF_HOME\n");
    check_same(*task4);
}

BOOST_AUTO_TEST_CASE(test_lookup_without_ecf_micro_is_rejected_as_by_the_server) {
    ECF_NAME_THIS_TEST();

    auto lookup = std::make_shared<MapVariableLookup>(NameValueMap{}, NameValueMap{}, NameValueMap{});
    BOOST_CHECK_EXCEPTION(EcfFile(EcfFile::NodeKind::FAMILY, "/s/f", lookup, "/nowhere/f.man"),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              return std::string(e.what()) ==
                                     "EcfFile::EcfFile: Node /s/f is referencing a invalid ECF_MICRO variable(' ). "
                                     "ECF_MICRO when overridden, must be a single character.";
                          });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
