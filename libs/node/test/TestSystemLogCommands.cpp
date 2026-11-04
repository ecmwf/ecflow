// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <chrono>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Signal.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

using namespace ecf;
using ecf::test::scaffold::TestLog;

namespace {

///
/// @brief Provides a log, and a System configured to log the spawned commands or not, for the duration of a test.
///
class WithSystemLog {
public:
    ///
    /// @brief Creates the log and configures System.
    ///
    /// @param[in] log_commands Whether System logs the spawned commands
    ///
    explicit WithSystemLog(bool log_commands)
        : directory_(ecf::test::scaffold::MakeDirectory{}.create()),
          log_path_((directory_.path() / "system.log").string()),
          log_(log_path_) {
        System::Configuration configuration;
        configuration.log_commands = log_commands;
        System::instance()->set_configuration(configuration);
        // A failed command is reported to the node of its path, which is looked up in the Defs
        System::instance()->setDefs(defs_);
    }

    WithSystemLog(const WithSystemLog&)            = delete;
    WithSystemLog& operator=(const WithSystemLog&) = delete;

    ~WithSystemLog() { System::destroy(); }

    ///
    /// @brief Returns the EXT records written to the log, without their type and time stamp.
    ///
    /// @return The records, in the order written.
    ///
    std::vector<std::string> ext_records() const {
        Log::instance()->flush_only();
        std::string contents;
        BOOST_REQUIRE_MESSAGE(File::open(log_path_, contents), "Could not open the log " << log_path_);

        const std::regex record(R"(^EXT:\[[^\]]+\] (.*)$)");
        std::vector<std::string> records;
        std::istringstream lines(contents);
        for (std::string line; std::getline(lines, line);) {
            if (std::smatch match; std::regex_match(line, match, record)) {
                records.push_back(match[1]);
            }
        }
        return records;
    }

    ///
    /// @brief Returns the whole content of the log.
    ///
    /// @return The content.
    ///
    std::string contents() const {
        Log::instance()->flush_only();
        std::string contents;
        BOOST_REQUIRE_MESSAGE(File::open(log_path_, contents), "Could not open the log " << log_path_);
        return contents;
    }

private:
    ecf::test::scaffold::Directory directory_;
    std::string log_path_;
    TestLog log_;
    defs_ptr defs_ = Defs::create();
};

///
/// @brief Waits for every child spawned by System to terminate, and handles the terminations.
///
void wait_for_children() {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (System::instance()->process() != 0) {
        BOOST_REQUIRE_MESSAGE(std::chrono::steady_clock::now() < deadline, "The spawned commands did not terminate");
        // Unblocks SIGCHLD while in scope, so that the terminations are caught
        Signal unblock_on_destruction_then_reblock;
        System::instance()->processTerminatedChildren();
    }
}

///
/// @brief Spawns a command, at the path `/s/t`.
///
/// @param[in] cmd_type The kind of command
/// @param[in] cmd The command, spawned and logged as is
/// @param[in] context The context of the command
///
void spawn(System::CmdType cmd_type, const std::string& cmd, const System::Context& context = {}) {
    std::string errorMsg;
    BOOST_REQUIRE_MESSAGE(System::instance()->spawn(cmd_type, cmd, cmd, context, "/s/t", errorMsg),
                          "System::spawn() failed: " << errorMsg);
}

///
/// @brief Returns the records, with every pid replaced by `pid(N)`, so that they can be compared.
///
/// @param[in] records The records
/// @return The records without their pids.
///
std::vector<std::string> without_pids(std::vector<std::string> records) {
    const std::regex pid(R"(pid\([0-9]+\))");
    for (auto& record : records) {
        record = std::regex_replace(record, pid, "pid(N)");
    }
    return records;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_SystemLogCommands)

BOOST_AUTO_TEST_CASE(test_system_logs_the_records_of_each_kind_of_command) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    for (const auto& [cmd_type, name] :
         std::vector<std::pair<System::CmdType, std::string>>{{System::ECF_JOB_CMD, "ECF_JOB_CMD"},
                                                              {System::ECF_KILL_CMD, "ECF_KILL_CMD"},
                                                              {System::ECF_STATUS_CMD, "ECF_STATUS_CMD"}}) {
        BOOST_TEST_CONTEXT("Command: " << name) {
            const auto before = log.ext_records().size();
            spawn(cmd_type, "true", {{"QUEUE", "nf"}, {"HOST", "hpc"}});
            wait_for_children();

            const auto records = without_pids(log.ext_records());
            const std::vector<std::string> expected{name + " spawn pid(N) path(/s/t) cmd[ true ]",
                                                    name + R"( context pid(N) path(/s/t) {"QUEUE":"nf","HOST":"hpc"})",
                                                    name + " exit pid(N) path(/s/t) status(0)"};
            BOOST_REQUIRE_EQUAL(records.size(), before + expected.size());
            BOOST_CHECK_EQUAL_COLLECTIONS(records.begin() + before, records.end(), expected.begin(), expected.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(test_system_logs_the_same_pid_in_the_records_of_a_command) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    spawn(System::ECF_JOB_CMD, "true");
    wait_for_children();

    const auto records = log.ext_records();
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    const std::regex pid(R"(pid\(([0-9]+)\))");
    std::smatch first;
    BOOST_REQUIRE(std::regex_search(records[0], first, pid));
    BOOST_CHECK_NE(first[1].str(), "0");
    for (const auto& record : records) {
        std::smatch match;
        BOOST_REQUIRE(std::regex_search(record, match, pid));
        BOOST_CHECK_EQUAL(match[1].str(), first[1].str());
    }
}

BOOST_AUTO_TEST_CASE(test_system_logs_every_termination) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    spawn(System::ECF_JOB_CMD, "exit 3");
    wait_for_children();
    spawn(System::ECF_JOB_CMD, "kill -9 $$");
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 6U);
    BOOST_CHECK_EQUAL(records[2], "ECF_JOB_CMD exit pid(N) path(/s/t) status(3)");
    BOOST_CHECK_EQUAL(records[5], "ECF_JOB_CMD signal pid(N) path(/s/t) signal(9)");
}

BOOST_AUTO_TEST_CASE(test_system_logs_no_record_when_disabled) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(false);
    spawn(System::ECF_JOB_CMD, "true", {{"QUEUE", "nf"}});
    spawn(System::ECF_JOB_CMD, "exit 3");
    wait_for_children();

    BOOST_CHECK(log.ext_records().empty());
}

BOOST_AUTO_TEST_CASE(test_system_logs_each_record_on_one_line) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    spawn(System::ECF_STATUS_CMD, "true\ntrue", {{"SCRIPT", "a\nb\r"}});
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[0], R"(ECF_STATUS_CMD spawn pid(N) path(/s/t) cmd[ true\ntrue ])");
    BOOST_CHECK_EQUAL(records[1], R"(ECF_STATUS_CMD context pid(N) path(/s/t) {"SCRIPT":"a\nb\r"})");
}

BOOST_AUTO_TEST_CASE(test_system_logs_the_command_to_log_with_the_fixed_rules) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    // The caller masks the substituted credentials; System masks those written literally
    std::string errorMsg;
    BOOST_REQUIRE(System::instance()->spawn(System::ECF_JOB_CMD,
                                            "true abc123 --password literal",
                                            "true *** --password literal",
                                            {{"API_TOKEN", "***"}, {"OPTS", "--token literal"}},
                                            "/s/t",
                                            errorMsg));
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[0], "ECF_JOB_CMD spawn pid(N) path(/s/t) cmd[ true *** --password *** ]");
    BOOST_CHECK_EQUAL(records[1], R"(ECF_JOB_CMD context pid(N) path(/s/t) {"API_TOKEN":"***","OPTS":"--token ***"})");
    BOOST_CHECK(log.contents().find("abc123") == std::string::npos);
    BOOST_CHECK(log.contents().find("literal") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_submittable_logs_no_credential_value) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    s->addVariable(Variable::new_variable("API_TOKEN", "abc123"));
    s->addVariable(Variable::new_variable("AUTH", "Bearer %API_TOKEN%"));
    s->addVariable(Variable::new_variable("DB_PASSWORD", "s3cr3t"));
    // The credentials are substituted directly, through another variable, as %NAME:default%, and written literally
    t->addVariable(Variable::new_variable(
        "ECF_KILL_CMD", "true %API_TOKEN% '%AUTH%' %DB_PASSWORD:x% %MY_SECRET:fallback% --token literal %ECF_RID%"));

    t->kill("4242");
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[0],
                      "ECF_KILL_CMD spawn pid(N) path(/s/t) cmd[ true *** 'Bearer ***' *** *** --token *** 4242 ]");
    BOOST_CHECK_EQUAL(records[1],
                      R"(ECF_KILL_CMD context pid(N) path(/s/t) {"API_TOKEN":"***","AUTH":"Bearer %API_TOKEN%",)"
                      R"("DB_PASSWORD":"***","MY_SECRET":"***","ECF_RID":"4242"})");
    for (const auto* secret : {"abc123", "s3cr3t", "fallback", "literal"}) {
        BOOST_CHECK_MESSAGE(log.contents().find(secret) == std::string::npos, "Found '" << secret << "' in the log");
    }
}

BOOST_AUTO_TEST_CASE(test_system_configuration_defaults_and_is_discarded_by_destroy) {
    ECF_NAME_THIS_TEST();

    System::destroy();
    BOOST_CHECK(!System::instance()->configuration().log_commands);

    System::Configuration configuration;
    configuration.log_commands = true;
    System::instance()->set_configuration(configuration);
    BOOST_CHECK(System::instance()->configuration().log_commands);

    System::destroy();
    BOOST_CHECK(!System::instance()->configuration().log_commands);
    System::destroy();
}

BOOST_AUTO_TEST_CASE(test_system_logs_an_empty_context_and_an_empty_path) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    std::string errorMsg;
    BOOST_REQUIRE(System::instance()->spawn(System::ECF_STATUS_CMD, "true", "true", {}, "", errorMsg));
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    const std::vector<std::string> expected{"ECF_STATUS_CMD spawn pid(N) path() cmd[ true ]",
                                            "ECF_STATUS_CMD context pid(N) path() {}",
                                            "ECF_STATUS_CMD exit pid(N) path() status(0)"};
    BOOST_CHECK_EQUAL_COLLECTIONS(records.begin(), records.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(test_system_keeps_the_existing_records_of_a_failed_command) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    spawn(System::ECF_JOB_CMD, "exit 3");
    wait_for_children();

    BOOST_CHECK(log.contents().find("exited with status 3 [ exit 3 ]") != std::string::npos);
    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[2], "ECF_JOB_CMD exit pid(N) path(/s/t) status(3)");
}

BOOST_AUTO_TEST_CASE(test_system_identifies_the_records_of_concurrent_commands) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    // Both commands run at the same time, and terminate in either order
    spawn(System::ECF_JOB_CMD, "sleep 0.2; exit 4");
    spawn(System::ECF_JOB_CMD, "exit 5");
    wait_for_children();

    const auto records = log.ext_records();
    BOOST_REQUIRE_EQUAL(records.size(), 6U);
    const std::regex spawned(R"(ECF_JOB_CMD spawn pid\(([0-9]+)\) path\(/s/t\) cmd\[ (.*) \])");
    std::size_t matched = 0;
    for (const auto& record : records) {
        std::smatch spawn_match;
        if (!std::regex_match(record, spawn_match, spawned)) {
            continue;
        }
        const auto status = spawn_match[2].str() == "exit 5" ? "5" : "4";
        const auto exit   = "ECF_JOB_CMD exit pid(" + spawn_match[1].str() + ") path(/s/t) status(" + status + ")";
        BOOST_CHECK_MESSAGE(std::find(records.begin(), records.end(), exit) != records.end(), "Missing: " << exit);
        ++matched;
    }
    BOOST_CHECK_EQUAL(matched, 2U);
}

BOOST_AUTO_TEST_CASE(test_submittable_logs_the_status_command) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    s->addVariable(Variable::new_variable("API_TOKEN", "abc123"));
    t->addVariable(Variable::new_variable("ECF_STATUS_CMD", "true %API_TOKEN%"));
    t->set_state(NState::SUBMITTED);

    t->status();
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[0], "ECF_STATUS_CMD spawn pid(N) path(/s/t) cmd[ true *** ]");
    BOOST_CHECK_EQUAL(records[1], R"(ECF_STATUS_CMD context pid(N) path(/s/t) {"API_TOKEN":"***"})");
}

BOOST_AUTO_TEST_CASE(test_submittable_records_no_rid_when_the_kill_command_has_none) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    t->addVariable(Variable::new_variable("ECF_KILL_CMD", "true kill"));

    t->kill("4242");
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[0], "ECF_KILL_CMD spawn pid(N) path(/s/t) cmd[ true kill ]");
    BOOST_CHECK_EQUAL(records[1], "ECF_KILL_CMD context pid(N) path(/s/t) {}");
}

BOOST_AUTO_TEST_CASE(test_submittable_logs_no_credential_value_of_an_alias_variable) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    auto output = ecf::test::scaffold::MakeDirectory{}.create();
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    s->addVariable(Variable::new_variable(
        "ECF_KILL_CMD", "echo %API_TOKEN:0% > " + (output.path() / "kill.out").string() + " # %ECF_RID%"));
    // The name of an alias variable holds a ':', which separates the parts of a credential name as well
    alias_ptr a = t->add_alias_only();
    a->add_alias_variable("API_TOKEN:0", "abc123");

    a->kill("4242");
    wait_for_children();

    // The command runs with the value
    std::string run;
    BOOST_REQUIRE(File::open((output.path() / "kill.out").string(), run));
    BOOST_CHECK_EQUAL(run, "abc123\n");

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_MESSAGE(records[0].find("cmd[ echo *** > ") != std::string::npos, records[0]);
    BOOST_CHECK_MESSAGE(records[1].find(R"({"API_TOKEN:0":"***","ECF_RID":"4242"})") != std::string::npos, records[1]);
    BOOST_CHECK_MESSAGE(log.contents().find("abc123") == std::string::npos, "Found 'abc123' in the log");
}

BOOST_AUTO_TEST_CASE(test_submittable_logs_no_record_when_the_substitution_fails) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    t->addVariable(Variable::new_variable("ECF_KILL_CMD", "true %UNDEFINED%"));

    BOOST_CHECK_THROW(t->kill("4242"), std::runtime_error);
    BOOST_CHECK_EQUAL(System::instance()->process(), 0);
    BOOST_CHECK(log.ext_records().empty());
}

BOOST_AUTO_TEST_CASE(test_system_logs_the_context_with_explicit_boundaries) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    // A value holding spaces, '=', quotes and backslashes cannot be mistaken for further variables
    spawn(System::ECF_JOB_CMD, "true", {{"X", "XXX VAR=VALUE"}, {"Q", R"(say "hi" \ bye)"}, {"EMPTY", ""}});
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(
        records[1],
        R"(ECF_JOB_CMD context pid(N) path(/s/t) {"X":"XXX VAR=VALUE","Q":"say \"hi\" \\ bye","EMPTY":""})");
}

BOOST_AUTO_TEST_CASE(test_system_logs_a_context_value_that_is_not_utf8) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    spawn(System::ECF_JOB_CMD,
          "true",
          {{"BYTES",
            std::string("a\xff"
                        "b")}});
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[1],
                      "ECF_JOB_CMD context pid(N) path(/s/t) {\"BYTES\":\"a\xEF\xBF\xBD"
                      "b\"}");
}

BOOST_AUTO_TEST_CASE(test_system_logs_the_last_value_of_a_name_given_twice) {
    ECF_NAME_THIS_TEST();

    WithSystemLog log(true);
    spawn(System::ECF_KILL_CMD, "true", {{"ECF_RID", "1"}, {"QUEUE", "nf"}, {"ECF_RID", "4242"}});
    wait_for_children();

    const auto records = without_pids(log.ext_records());
    BOOST_REQUIRE_EQUAL(records.size(), 3U);
    BOOST_CHECK_EQUAL(records[1], R"(ECF_KILL_CMD context pid(N) path(/s/t) {"ECF_RID":"4242","QUEUE":"nf"})");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
