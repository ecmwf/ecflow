// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EcfFile.hpp"
#include "ecflow/node/Family.hpp"
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

/// The scripts of one test, in a directory provisioned for it
using Scripts = scaffold::TestFiles;

///
/// @brief A suite /suite/family/task, with the variables set by each test.
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
/// @brief The outcome of one search: the location found, or the error.
///
struct Outcome
{
    bool found{false};
    EcfFile::Origin origin{EcfFile::ECF_SCRIPT};
    std::string path_or_cmd;
    EcfFile::EcfFileSearchAlgorithm algorithm{EcfFile::PRUNE_ROOT};
    std::string error;
};

///
/// @brief Searches the script of a node as the server does, through Submittable::locatedEcfFile.
///
/// @param[in] node The task or the alias.
/// @return The outcome of the search.
///
Outcome through_server(const Submittable& node) {
    Outcome outcome;
    try {
        EcfFile file        = node.locatedEcfFile();
        outcome.found       = true;
        outcome.origin      = file.ecf_file_origin();
        outcome.path_or_cmd = file.script_path_or_cmd();
        outcome.algorithm   = file.ecf_file_search_algorithm();
    }
    catch (const std::runtime_error& e) {
        outcome.error = e.what();
    }
    return outcome;
}

///
/// @brief Searches the script of a node from a copy of its variables, through ecf::locate_script.
///
/// @param[in] node The task or the alias; its generated variables are updated first.
/// @return The outcome of the search.
///
Outcome through_variables(const Submittable& node) {
    node.update_generated_variables();
    const auto lookup = MapVariableLookup::from_node(node);
    Outcome outcome;
    try {
        ScriptLocation location = locate_script(node.absNodePath(), node.script_extension(), lookup);
        BOOST_CHECK_EQUAL(location.needs_command(),
                          location.origin == EcfFile::ECF_FETCH_CMD || location.origin == EcfFile::ECF_SCRIPT_CMD);
        outcome.found       = true;
        outcome.origin      = location.origin;
        outcome.path_or_cmd = location.path_or_cmd;
        outcome.algorithm   = location.algorithm;
    }
    catch (const std::runtime_error& e) {
        outcome.error = e.what();
    }
    return outcome;
}

///
/// @brief Checks that both searches give the expected location.
///
/// @param[in] node The task or the alias.
/// @param[in] origin The expected origin.
/// @param[in] path_or_cmd The expected path, or substituted command.
/// @param[in] algorithm The expected search algorithm.
///
void check(const Submittable& node,
           EcfFile::Origin origin,
           const std::string& path_or_cmd,
           EcfFile::EcfFileSearchAlgorithm algorithm = EcfFile::PRUNE_ROOT) {
    for (const auto& outcome : {through_server(node), through_variables(node)}) {
        BOOST_CHECK_MESSAGE(outcome.found, "not found: " << outcome.error);
        BOOST_CHECK_EQUAL(EcfFile::origin_str(outcome.origin), EcfFile::origin_str(origin));
        BOOST_CHECK_EQUAL(outcome.path_or_cmd, path_or_cmd);
        BOOST_CHECK_EQUAL(EcfFile::search_algorithm_str(outcome.algorithm), EcfFile::search_algorithm_str(algorithm));
    }
}

///
/// @brief Checks that both searches fail with the expected message, written as the server's search wrote it.
///
/// @param[in] node The task or the alias.
/// @param[in] expected The complete expected message.
///
void check_error(const Submittable& node, const std::string& expected) {
    for (const auto& outcome : {through_server(node), through_variables(node)}) {
        BOOST_CHECK(!outcome.found);
        BOOST_CHECK_EQUAL(outcome.error, expected);
    }
}

/// The start of the message of a script not found, up to the ECF_FILES lines.
std::string not_found_header(const std::string& ecf_home) {
    return "   Script for /suite/family/task cannot be found:\n"
           "   ECF_SCRIPT(" +
           ecf_home +
           "/suite/family/task.ecf) does not exist:\n"
           "   Variable ECF_FETCH not defined:\n"
           "   Variable ECF_SCRIPT_CMD not defined:\n";
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_ScriptLocator)

BOOST_AUTO_TEST_CASE(test_script_found_at_ecf_script) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    const auto script = scripts.add("suite/family/task.ecf");
    check(*tree.task, EcfFile::ECF_SCRIPT, script);
}

BOOST_AUTO_TEST_CASE(test_alias_script_found_at_ecf_script_with_usr_extension) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    alias_ptr alias = tree.task->add_alias("alias0");
    scripts.add("suite/family/task/alias0.ecf"); // a task script at the same place is not the alias script
    const auto script = scripts.add("suite/family/task/alias0.usr");
    check(*alias, EcfFile::ECF_SCRIPT, script);
}

BOOST_AUTO_TEST_CASE(test_alias_script_found_in_ecf_files) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
    tree.family->addVariable(Variable("ECF_FILES", scripts.dir("files")));
    alias_ptr alias   = tree.task->add_alias("alias0");
    const auto script = scripts.add("files/task/alias0.usr");
    check(*alias, EcfFile::ECF_FILES, script);
}

BOOST_AUTO_TEST_CASE(test_script_that_needs_ecf_fetch_is_not_run) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    const auto marker = scripts.root() + "/fetch_was_run";
    tree.suite->addVariable(Variable("ECF_FETCH", "touch " + marker + " ; echo %TASK%"));
    check(*tree.task, EcfFile::ECF_FETCH_CMD, "touch " + marker + " ; echo task");
    BOOST_CHECK(!fs::exists(marker));
}

BOOST_AUTO_TEST_CASE(test_script_that_needs_ecf_script_cmd_is_not_run) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    const auto marker = scripts.root() + "/script_cmd_was_run";
    tree.family->addVariable(Variable("ECF_SCRIPT_CMD", "touch " + marker + " ; cat %ECF_NAME%"));
    check(*tree.task, EcfFile::ECF_SCRIPT_CMD, "touch " + marker + " ; cat /suite/family/task");
    BOOST_CHECK(!fs::exists(marker));
}

BOOST_AUTO_TEST_CASE(test_commands_are_substituted_with_the_user_micro) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    tree.suite->addVariable(Variable("ECF_MICRO", "&"));
    tree.family->addVariable(Variable("ECF_FETCH", "fetch &TASK& &ECF_NAME& %TASK%"));
    check(*tree.task, EcfFile::ECF_FETCH_CMD, "fetch task /suite/family/task %TASK%");
}

BOOST_AUTO_TEST_CASE(test_sources_are_tried_in_the_server_order) {
    ECF_NAME_THIS_TEST();

    { // ECF_SCRIPT before ECF_FETCH
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
        tree.suite->addVariable(Variable("ECF_FETCH", "fetch"));
        const auto script = scripts.add("suite/family/task.ecf");
        check(*tree.task, EcfFile::ECF_SCRIPT, script);
    }
    { // ECF_FETCH before ECF_SCRIPT_CMD
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
        tree.suite->addVariable(Variable("ECF_SCRIPT_CMD", "script_cmd"));
        tree.suite->addVariable(Variable("ECF_FETCH", "fetch"));
        check(*tree.task, EcfFile::ECF_FETCH_CMD, "fetch");
    }
    { // ECF_SCRIPT_CMD before ECF_FILES
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
        tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
        tree.suite->addVariable(Variable("ECF_SCRIPT_CMD", "script_cmd"));
        scripts.add("files/task.ecf");
        check(*tree.task, EcfFile::ECF_SCRIPT_CMD, "script_cmd");
    }
    { // ECF_FILES before ECF_HOME
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
        tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
        scripts.add("home/task.ecf");
        const auto script = scripts.add("files/task.ecf");
        check(*tree.task, EcfFile::ECF_FILES, script);
    }
}

BOOST_AUTO_TEST_CASE(test_commands_are_read_from_all_variables_and_directories_from_user_variables) {
    ECF_NAME_THIS_TEST();

    { // ECF_FETCH: a repeat of that name counts
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
        tree.task->addRepeat(Repeat(RepeatString("ECF_FETCH", {"fetch_from_repeat"})));
        check(*tree.task, EcfFile::ECF_FETCH_CMD, "fetch_from_repeat");
    }
    { // ECF_SCRIPT_CMD: a repeat of that name counts
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
        tree.family->addRepeat(Repeat(RepeatString("ECF_SCRIPT_CMD", {"script_cmd_from_repeat"})));
        check(*tree.task, EcfFile::ECF_SCRIPT_CMD, "script_cmd_from_repeat");
    }
    { // ECF_HOME: a repeat of that name on the task does not hide the suite's user variable
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
        tree.task->addRepeat(Repeat(RepeatString("ECF_HOME", {scripts.dir("elsewhere")})));
        scripts.add("elsewhere/suite/family/task.ecf");
        const auto script = scripts.add("suite/family/task.ecf");
        check(*tree.task, EcfFile::ECF_SCRIPT, script);
    }
    { // ECF_FILES: a repeat of that name on the task does not hide the family's user variable
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
        tree.family->addVariable(Variable("ECF_FILES", scripts.dir("files")));
        tree.task->addRepeat(Repeat(RepeatString("ECF_FILES", {scripts.dir("elsewhere")})));
        scripts.add("elsewhere/task.ecf");
        const auto script = scripts.add("files/task.ecf");
        check(*tree.task, EcfFile::ECF_FILES, script);
    }
    { // ECF_FILES_LOOKUP: a repeat of that name does not select pruning the leaf
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
        tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
        tree.task->addRepeat(Repeat(RepeatString("ECF_FILES_LOOKUP", {"prune_leaf"})));
        scripts.add("files/suite/task.ecf"); // found only by pruning the leaf
        const auto script = scripts.add("files/family/task.ecf");
        check(*tree.task, EcfFile::ECF_FILES, script);
    }
    { // ECF_MICRO: a repeat of that name does not change the micro
        Scripts scripts;
        Tree tree;
        tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
        tree.suite->addVariable(Variable("ECF_FETCH", "fetch %TASK% &TASK&"));
        tree.task->addRepeat(Repeat(RepeatString("ECF_MICRO", {"&"})));
        check(*tree.task, EcfFile::ECF_FETCH_CMD, "fetch task &TASK&");
    }
}

BOOST_AUTO_TEST_CASE(test_script_found_in_ecf_files_by_pruning_the_root) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
    tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
    const auto script = scripts.add("files/family/task.ecf");
    check(*tree.task, EcfFile::ECF_FILES, script);
}

BOOST_AUTO_TEST_CASE(test_script_found_in_ecf_files_by_pruning_the_leaf) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
    tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
    tree.family->addVariable(Variable("ECF_FILES_LOOKUP", "prune_leaf"));
    const auto script = scripts.add("files/suite/task.ecf");
    check(*tree.task, EcfFile::ECF_FILES, script, EcfFile::PRUNE_LEAF);
}

BOOST_AUTO_TEST_CASE(test_script_found_in_ecf_files_after_substitution) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.dir("home")));
    tree.suite->addVariable(Variable("FILES_ROOT", scripts.root()));
    tree.family->addVariable(Variable("ECF_FILES", "%FILES_ROOT%/files"));
    const auto script = scripts.add("files/task.ecf");
    check(*tree.task, EcfFile::ECF_FILES, script);
}

BOOST_AUTO_TEST_CASE(test_script_found_in_ecf_home) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
    const auto script = scripts.add("family/task.ecf");
    check(*tree.task, EcfFile::ECF_HOME, script);
}

BOOST_AUTO_TEST_CASE(test_script_not_found_when_ecf_files_does_not_exist) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    tree.suite->addVariable(Variable("ECF_FILES", scripts.root() + "/missing"));
    check_error(*tree.task,
                not_found_header(scripts.root()) + "   Directory ECF_FILES(" + scripts.root() +
                    "/missing) does not exist:\n"
                    "   Search of directory ECF_HOME(" +
                    scripts.root() + ") failed using prune_root:\n");
}

BOOST_AUTO_TEST_CASE(test_script_not_found_when_ecf_files_does_not_exist_after_substitution) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    const auto home = scripts.root() + "/home";
    tree.suite->addVariable(Variable("ECF_HOME", home));
    tree.suite->addVariable(Variable("FILES_ROOT", scripts.root()));
    tree.suite->addVariable(Variable("ECF_FILES", "%FILES_ROOT%/missing"));
    check_error(*tree.task,
                not_found_header(home) +
                    "   Directory ECF_FILES(%FILES_ROOT%/missing) does not exist:\n"
                    "   Directory ECF_FILES(" +
                    scripts.root() +
                    "/missing) after variable substitution does not exist:\n"
                    "   Directory ECF_HOME(" +
                    home + ") does not exist:\n");
}

BOOST_AUTO_TEST_CASE(test_script_not_found_by_searching_ecf_files_and_ecf_home) {
    ECF_NAME_THIS_TEST();

    for (const std::string algorithm : {"prune_root", "prune_leaf"}) {
        Scripts scripts;
        Tree tree;
        const auto home  = scripts.dir("home");
        const auto files = scripts.dir("files");
        tree.suite->addVariable(Variable("ECF_HOME", home));
        tree.suite->addVariable(Variable("ECF_FILES", files));
        if (algorithm == "prune_leaf") {
            tree.suite->addVariable(Variable("ECF_FILES_LOOKUP", "PRUNE_LEAF"));
        }
        check_error(*tree.task,
                    not_found_header(home) + "   Search of directory ECF_FILES(" + files + ") failed using " +
                        algorithm + ":\n   Search of directory ECF_HOME(" + home + ") failed using " + algorithm +
                        ":\n");
    }
}

BOOST_AUTO_TEST_CASE(test_script_not_found_by_searching_ecf_files_after_substitution) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    const auto home  = scripts.root() + "/home";
    const auto files = scripts.dir("files");
    tree.suite->addVariable(Variable("ECF_HOME", home));
    tree.suite->addVariable(Variable("FILES_ROOT", scripts.root()));
    tree.suite->addVariable(Variable("ECF_FILES", "%FILES_ROOT%/files"));
    check_error(*tree.task,
                not_found_header(home) + "   Search of directory ECF_FILES(variable substituted)(" + files +
                    ") failed:\n"
                    "   Directory ECF_HOME(" +
                    home + ") does not exist:\n");
}

BOOST_AUTO_TEST_CASE(test_script_not_found_without_ecf_files) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    check_error(*tree.task,
                not_found_header(scripts.root()) +
                    "   Variable ECF_FILES not defined:\n"
                    "   Search of directory ECF_HOME(" +
                    scripts.root() + ") failed using prune_root:\n");
}

BOOST_AUTO_TEST_CASE(test_failing_substitution_of_ecf_fetch) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    tree.suite->addVariable(Variable("ECF_FETCH", "fetch %UNDEFINED%"));
    tree.suite->addVariable(Variable("ECF_SCRIPT_CMD", "script_cmd"));
    check_error(*tree.task,
                "   ECF_SCRIPT(" + scripts.root() +
                    "/suite/family/task.ecf) does not exist:\n"
                    "   Variable ECF_FETCH(fetch %UNDEFINED%) defined, but variable substitution has failed:\n");
}

BOOST_AUTO_TEST_CASE(test_failing_substitution_of_ecf_script_cmd) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    Tree tree;
    tree.suite->addVariable(Variable("ECF_HOME", scripts.root()));
    tree.suite->addVariable(Variable("ECF_SCRIPT_CMD", "script_cmd %UNDEFINED%"));
    tree.suite->addVariable(Variable("ECF_FILES", scripts.dir("files")));
    scripts.add("files/task.ecf");
    check_error(*tree.task,
                "   ECF_SCRIPT(" + scripts.root() +
                    "/suite/family/task.ecf) does not exist:\n"
                    "   Variable ECF_FETCH not defined:\n"
                    "   Variable ECF_SCRIPT_CMD(script_cmd %UNDEFINED%) defined, but variable substitution has "
                    "failed:\n");
}

BOOST_AUTO_TEST_CASE(test_search_over_a_hand_built_lookup) {
    ECF_NAME_THIS_TEST();

    Scripts scripts;
    const auto script = scripts.add("suite/task.ecf");

    { // ECF_HOME is taken from the user variables, not from all variables
        MapVariableLookup lookup({{"ECF_HOME", "/nowhere"}}, {}, {{"ECF_HOME", scripts.root()}});
        const auto location = locate_script("/suite/task", ".ecf", lookup);
        BOOST_CHECK_EQUAL(EcfFile::origin_str(location.origin), EcfFile::origin_str(EcfFile::ECF_SCRIPT));
        BOOST_CHECK_EQUAL(location.path_or_cmd, script);
        BOOST_CHECK(!location.needs_command());
    }
    { // ECF_FETCH is taken from all variables, and substituted from them
        MapVariableLookup lookup({{"ECF_HOME", "/nowhere"}, {"ECF_FETCH", "fetch %ECF_NAME%"}},
                                 {{"ECF_NAME", "/suite/task"}},
                                 {{"ECF_HOME", "/nowhere"}});
        const auto location = locate_script("/suite/task", ".ecf", lookup);
        BOOST_CHECK_EQUAL(EcfFile::origin_str(location.origin), EcfFile::origin_str(EcfFile::ECF_FETCH_CMD));
        BOOST_CHECK_EQUAL(location.path_or_cmd, "fetch /suite/task");
        BOOST_CHECK(location.needs_command());
    }
    { // ECF_FETCH known only as a user variable is not used: nothing is found
        MapVariableLookup lookup({{"ECF_HOME", "/nowhere"}}, {}, {{"ECF_HOME", "/nowhere"}, {"ECF_FETCH", "fetch"}});
        BOOST_CHECK_THROW(locate_script("/suite/task", ".ecf", lookup), std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
