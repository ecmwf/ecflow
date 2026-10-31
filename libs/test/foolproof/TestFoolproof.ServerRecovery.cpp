// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <list>
#include <string>
#include <unistd.h>
#include <vector>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include "ServerStartup.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace {

///
/// @brief Writes a distinctive checkpoint with persisted node states and optional stale server settings.
///
/// @param[in] path Owned checkpoint destination
/// @param[in] marker Primary or backup fixture identifier
/// @param[in] stale_settings Whether to persist deliberately obsolete exported server variables
///
void write_recovery_checkpoint(const fs::path& path, const std::string& marker, bool stale_settings) {
    Defs definition;
    definition.restore_from_string("suite " + marker +
                                   "_suite\n family group\n task finished\n"
                                   " edit RECOVERY_SOURCE " +
                                   marker + "\n task waiting\n endfamily\nendsuite\n");
    auto finished = definition.findAbsNode("/" + marker + "_suite/group/finished");
    auto waiting  = definition.findAbsNode("/" + marker + "_suite/group/waiting");
    BOOST_REQUIRE(finished);
    BOOST_REQUIRE(waiting);
    finished->set_state(marker == "primary" ? NState::COMPLETE : NState::ABORTED, true);
    waiting->set_state(NState::QUEUED, true);
    definition.server_state().set_state(SState::RUNNING);
    definition.server_state().add_or_update_user_variables("RECOVERY_USER_NOTE", "persisted_" + marker);

    if (stale_settings) {
        for (const std::string name :
             {"ECF_HOME",     "ECF_PORT",     "ECF_HOST",          "ECF_LOG",       "ECF_CHECK",
              "ECF_CHECKOLD", "ECF_PASSWD",   "ECF_CUSTOM_PASSWD", "ECF_LISTS",     "ECF_INTERVAL",
              "ECF_JOB_CMD",  "ECF_KILL_CMD", "ECF_STATUS_CMD",    "ECF_CHECK_CMD", "ECF_URL_CMD",
              "ECF_URL_BASE", "ECF_URL",      "ECF_MICRO",         "ECF_PID",       "ECF_VERSION"}) {
            definition.server_state().add_or_update_server_variable(name, "stale_" + name);
        }
    }
    definition.write_to_checkpt_file(path.string());
    if (stale_settings) {
        BOOST_CHECK(ecf::test::foolproof::read_file_bytes(path).find("stale_ECF_HOME") != std::string::npos);
    }
}

///
/// @brief Builds a checkpoint whose parsing fails after a server variable and a suite have been read.
///
/// @param[in] marker Primary or backup fixture identifier, used in the partially parsed names
/// @return The checkpoint text, ending with an unknown keyword.
///
std::string corrupt_checkpoint_text(const std::string& marker) {
    return "edit FAILED_CHECKPOINT_ONLY " + marker + "\nsuite partial_" + marker +
           "\n task not_recovered\nUNKNOWN_CHECKPOINT_KEYWORD\n";
}

///
/// @brief Checks executable checkpoint selection, restored state and preservation of startup inputs.
///
/// Fixture kinds:
/// - missing: No checkpoint path exists.
/// - valid: A distinctive persisted definition.
/// - corrupt: A partial definition followed by an invalid record.
/// - unreadable: A valid checkpoint without read permission.
/// - directory: An owned directory containing a sentinel file.
///
/// @param[in] primary Primary checkpoint fixture kind
/// @param[in] backup Backup checkpoint fixture kind
/// @param[in] from_environment Whether checkpoint filenames override conflicting config paths
/// @param[in] stale_settings Whether valid fixtures contain obsolete server variables
///
void check_server_recovery(const std::string& primary,
                           const std::string& backup,
                           bool from_environment,
                           bool stale_settings = false) {
    using namespace ecf::test::scaffold;

    auto startup    = MakeDirectory{}.create();
    const auto home = startup.path() / "server home";
    fs::create_directories(home);

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = ecf::test::foolproof::reserve_port();
    // The losing port settings are reserved too, so that a precedence regression cannot bind another test's port
    auto config_decoy                          = ecf::test::foolproof::reserve_port();
    auto environment_decoy                     = ecf::test::foolproof::reserve_port();
    const std::array<std::string, 2> kinds     = {primary, backup};
    const std::array<std::string, 2> names     = {"ECF_CHECK", "ECF_CHECKOLD"};
    const std::array<std::string, 2> filenames = {"primary.check", "backup.check"};
    std::list<WithTestFile> files;
    std::vector<std::pair<fs::path, std::string>> snapshots;

    const auto restore_permissions = [&] {
        for (size_t i = 0; i < kinds.size(); ++i) {
            if (kinds[i] == "unreadable" && fs::exists(home / filenames[i])) {
                std::error_code error;
                fs::permissions(home / filenames[i],
                                fs::perms::owner_read | fs::perms::owner_write,
                                fs::perm_options::replace,
                                error);
                BOOST_CHECK_MESSAGE(!error, "Unable to restore checkpoint permissions: " << error.message());
            }
        }
    };
    BOOST_SCOPE_EXIT_ALL(&) {
        restore_permissions();
    };

    for (size_t i = 0; i < kinds.size(); ++i) {
        const auto path          = home / filenames[i];
        const std::string marker = i == 0 ? "primary" : "backup";
        if (kinds[i] == "missing") {
            continue;
        }
        if (kinds[i] == "directory") {
            fs::create_directory(path);
            files.emplace_back(NamedTestFile{(path / "sentinel").string()}, "preserve directory contents\n");
            snapshots.emplace_back(path / "sentinel", ecf::test::foolproof::read_file_bytes(path / "sentinel"));
            continue;
        }

        if (kinds[i] == "corrupt") {
            files.emplace_back(NamedTestFile{path.string()}, corrupt_checkpoint_text(marker));
        }
        else {
            // Created empty for its clean-up, since the definition writes the checkpoint itself
            files.emplace_back(NamedTestFile{path.string()}, "");
            write_recovery_checkpoint(path, marker, stale_settings);
        }
        snapshots.emplace_back(path, ecf::test::foolproof::read_file_bytes(path));
        if (kinds[i] == "unreadable") {
            fs::permissions(path, fs::perms::none, fs::perm_options::replace);
            if (::access(path.c_str(), R_OK) == 0) {
                ecf::test::scaffold::skip_unenforced_permissions("the unreadable checkpoint case");
                return;
            }
        }
    }

    std::string config = "ECF_HOME = " + home.string() + "\nECF_PORT = " + std::to_string(config_decoy.value()) +
                         "\nECF_LOG = server.log\n"
                         "ECF_CHECKMODE = CHECK_NEVER\nECF_CHECKINTERVAL = 137\nECF_INTERVAL = 17\n"
                         "ECF_PASSWD = no.passwd\nECF_CUSTOM_PASSWD = no.custom_passwd\nECF_LISTS = no.lists\n"
                         "ECF_JOB_CMD = configured_submit\nECF_KILL_CMD = configured_kill\n"
                         "ECF_STATUS_CMD = configured_status\nECF_CHECK_CMD = configured_check\n"
                         "ECF_URL_CMD = configured_url_command\nECF_URL_BASE = https://current.invalid\n"
                         "ECF_URL = current/path\nECF_MICRODEF = @\n";
    std::list<WithTestEnvironmentVariable> overrides;
    WithTestEnvironmentVariable environment_port("ECF_PORT", std::to_string(environment_decoy.value()));
    for (size_t i = 0; i < names.size(); ++i) {
        config += names[i] + " = " + (from_environment ? "unused." + filenames[i] : filenames[i]) + "\n";
        if (from_environment) {
            overrides.emplace_back(names[i], filenames[i]);
        }
    }
    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()}, config);

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(port.value()), "--ecfinterval=23", "--dis_job_gen"},
                    startup.path());
    bool stopped = false;
    BOOST_SCOPE_EXIT_ALL(&) {
        if (!stopped) {
            BOOST_CHECK_EQUAL(process.terminate(), 0);
        }
    };

    const bool no_checkpoints = primary == "missing" && backup == "missing";
    const bool recoverable    = primary == "valid" || backup == "valid";
    const bool refused        = !no_checkpoints && !recoverable;
    if (refused) {
        const auto [exited, status, output] = ecf::test::foolproof::wait_for_exit(process);
        stopped                             = true;
        BOOST_REQUIRE_MESSAGE(exited, "Unusable checkpoints did not reject startup: " << output);
        BOOST_CHECK_EQUAL(status, 1);
        BOOST_CHECK_MESSAGE(output.find("no usable checkpoint") != std::string::npos, output);
        for (size_t i = 0; i < kinds.size(); ++i) {
            // Each diagnostic names the setting together with its own file, so that the files cannot be confused
            const auto quoted = names[i] + " '" + (home / filenames[i]).string() + "'";
            if (kinds[i] == "missing") {
                BOOST_CHECK_MESSAGE(output.find(quoted + " is missing.") != std::string::npos, output);
            }
            else {
                BOOST_CHECK_MESSAGE(output.find("Failed to load " + quoted + ": ") != std::string::npos, output);
                const auto cause = kinds[i] == "directory"    ? "not a regular file"
                                   : kinds[i] == "unreadable" ? "Unable to open file"
                                                              : "UNKNOWN_CHECKPOINT_KEYWORD";
                BOOST_CHECK_MESSAGE(output.find(cause) != std::string::npos, output);
            }
        }
    }
    else {
        const auto error = ecf::test::foolproof::wait_for_listener(port.value());
        BOOST_REQUIRE_MESSAGE(!error, "Recoverable server did not start: " << error.message());

        ClientInvoker client;
        ecf::test::foolproof::configure_client(client, host.value(), port.value());
        BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
        BOOST_REQUIRE(client.defs());
        const auto definition = client.defs();
        BOOST_CHECK(definition->server_state().get_state() == SState::HALTED);
        BOOST_CHECK_EQUAL(definition->suites().size(), no_checkpoints ? 0 : 1);
        if (!no_checkpoints) {
            const std::string marker = primary == "valid" ? "primary" : "backup";
            auto finished            = definition->findAbsNode("/" + marker + "_suite/group/finished");
            auto waiting             = definition->findAbsNode("/" + marker + "_suite/group/waiting");
            BOOST_REQUIRE(finished);
            BOOST_REQUIRE(waiting);
            BOOST_CHECK(finished->state() == (marker == "primary" ? NState::COMPLETE : NState::ABORTED));
            BOOST_CHECK(waiting->state() == NState::QUEUED);
            std::string value;
            BOOST_REQUIRE(finished->findVariableValue("RECOVERY_SOURCE", value));
            BOOST_CHECK_EQUAL(value, marker);
            BOOST_CHECK_EQUAL(definition->server_state().find_variable("RECOVERY_USER_NOTE"), "persisted_" + marker);
            BOOST_CHECK(!definition->findSuite(marker == "primary" ? "backup_suite" : "primary_suite"));
            BOOST_CHECK(!definition->findSuite("partial_primary"));
            BOOST_CHECK(!definition->findSuite("partial_backup"));
            BOOST_CHECK(definition->server_state().find_variable("FAILED_CHECKPOINT_ONLY").empty());
        }

        for (const auto& [name, value] :
             std::vector<std::pair<std::string, std::string>>{{"ECF_HOME", home.string()},
                                                              {"ECF_PORT", std::to_string(port.value())},
                                                              {"ECF_HOST", host.value()},
                                                              {"ECF_LOG", (home / "server.log").string()},
                                                              {"ECF_CHECK", (home / filenames[0]).string()},
                                                              {"ECF_CHECKOLD", (home / filenames[1]).string()},
                                                              {"ECF_PASSWD", "no.passwd"},
                                                              {"ECF_CUSTOM_PASSWD", "no.custom_passwd"},
                                                              {"ECF_LISTS", "no.lists"},
                                                              {"ECF_INTERVAL", "23"},
                                                              {"ECF_JOB_CMD", "configured_submit"},
                                                              {"ECF_KILL_CMD", "configured_kill"},
                                                              {"ECF_STATUS_CMD", "configured_status"},
                                                              {"ECF_CHECK_CMD", "configured_check"},
                                                              {"ECF_URL_CMD", "configured_url_command"},
                                                              {"ECF_URL_BASE", "https://current.invalid"},
                                                              {"ECF_URL", "current/path"},
                                                              {"ECF_MICRO", "@"},
                                                              {"ECF_PID", std::to_string(process.pid())},
                                                              {"ECF_VERSION", ecf::Version::full()}}) {
            BOOST_CHECK_EQUAL(definition->server_state().find_variable(name), value);
        }
        BOOST_REQUIRE_EQUAL(process.terminate(), 0);
        stopped = true;
    }

    restore_permissions();
    // With CHECK_NEVER, only the refusal path can alter the inputs, by replacing a checkpoint it could not load
    if (refused) {
        for (const auto& [path, content] : snapshots) {
            BOOST_CHECK_EQUAL(ecf::test::foolproof::read_file_bytes(path), content);
        }
    }
    for (size_t i = 0; i < kinds.size(); ++i) {
        if (kinds[i] == "missing") {
            BOOST_CHECK(!fs::exists(home / filenames[i]));
        }
        else if (kinds[i] == "directory") {
            BOOST_CHECK(fs::is_directory(home / filenames[i]));
        }
    }

    const auto error = ecf::test::foolproof::probe_no_listener(port.value());
    BOOST_CHECK_MESSAGE(error == boost::asio::error::connection_refused,
                        "Recovery fixture left a listener: " << error.message());
}

///
/// @brief Checks that a restore requested from a running server discards the data of a failed primary checkpoint.
///
/// The server starts without checkpoints; a corrupt primary and the given backup are then written, and the
/// restore is requested through the client.
///
/// @param[in] backup Backup checkpoint fixture kind: valid or corrupt
///
void check_runtime_restore(const std::string& backup) {
    using namespace ecf::test::scaffold;

    auto startup    = MakeDirectory{}.create();
    const auto home = startup.path() / "server home";
    fs::create_directories(home);

    ecf::test::foolproof::ClearedServerEnvironment cleared;

    auto host = MakeHost{}.create();
    auto port = ecf::test::foolproof::reserve_port();
    WithTestFile configuration(NamedTestFile{(startup.path() / "server_environment.cfg").string()},
                               "ECF_HOME = " + home.string() +
                                   "\nECF_LOG = server.log\nECF_CHECKMODE = CHECK_NEVER\n"
                                   "ECF_CHECK = primary.check\nECF_CHECKOLD = backup.check\n");

    Process process(ecf::test::foolproof::ecflow_server_executable(),
                    {"--port=" + std::to_string(port.value()), "--dis_job_gen"},
                    startup.path());
    BOOST_SCOPE_EXIT_ALL(&) {
        BOOST_CHECK_EQUAL(process.terminate(), 0);
    };

    const auto error = ecf::test::foolproof::wait_for_listener(port.value());
    BOOST_REQUIRE_MESSAGE(!error, "Server without checkpoints did not start: " << error.message());

    ClientInvoker client;
    ecf::test::foolproof::configure_client(client, host.value(), port.value());

    // Written after start-up, so that the server holds an empty, HALTED definition when the restore is requested
    WithTestFile primary_file(NamedTestFile{(home / "primary.check").string()}, corrupt_checkpoint_text("primary"));
    WithTestFile backup_file(NamedTestFile{(home / "backup.check").string()},
                             backup == "valid" ? "" : corrupt_checkpoint_text("backup"));
    if (backup == "valid") {
        write_recovery_checkpoint(home / "backup.check", "backup", false);
    }

    BOOST_REQUIRE_MESSAGE(client.restoreDefsFromCheckPt() == 0, client.errorMsg());
    BOOST_REQUIRE_MESSAGE(client.getDefs() == 0, client.errorMsg());
    const auto definition = client.defs();
    BOOST_REQUIRE(definition);

    BOOST_CHECK(!definition->findSuite("partial_primary"));
    BOOST_CHECK(!definition->findSuite("partial_backup"));
    BOOST_CHECK_EQUAL(definition->server_state().find_variable("FAILED_CHECKPOINT_ONLY"), "");
    BOOST_CHECK(definition->server_state().get_state() == SState::HALTED);
    BOOST_CHECK_EQUAL(definition->server_state().find_variable("ECF_HOME"), home.string());
    BOOST_CHECK_EQUAL(definition->server_state().find_variable("ECF_CHECK"), (home / "primary.check").string());
    if (backup == "valid") {
        BOOST_CHECK_EQUAL(definition->suites().size(), 1);
        auto finished = definition->findAbsNode("/backup_suite/group/finished");
        BOOST_REQUIRE(finished);
        BOOST_CHECK(finished->state() == NState::ABORTED);
        BOOST_CHECK_EQUAL(definition->server_state().find_variable("RECOVERY_USER_NOTE"), "persisted_backup");
    }
    else {
        BOOST_CHECK_EQUAL(definition->suites().size(), 0);
    }
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_ServerRecovery)

///
/// @brief Verifies an empty HALTED definition when both checkpoint files are absent.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_no_checkpoints) {
    ECF_NAME_THIS_TEST();

    for (const bool source : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << source) {
            check_server_recovery("missing", "missing", source);
        }
    }
}

///
/// @brief Verifies restoration of distinctive nodes and persisted states from a valid primary checkpoint.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_primary) {
    ECF_NAME_THIS_TEST();

    for (const bool source : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << source) {
            check_server_recovery("valid", "missing", source);
        }
    }
}

///
/// @brief Verifies restoration from a valid backup when the primary checkpoint is absent.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_backup_only) {
    ECF_NAME_THIS_TEST();

    for (const bool source : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << source) {
            check_server_recovery("missing", "valid", source);
        }
    }
}

///
/// @brief Verifies fallback to a valid backup after corrupt, unreadable and directory-valued primary paths.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_backup_after_bad_primary) {
    ECF_NAME_THIS_TEST();

    for (const std::string kind : {"corrupt", "unreadable", "directory"}) {
        for (const bool source : {false, true}) {
            BOOST_TEST_CONTEXT("Primary " << kind << ", environment source: " << source) {
                check_server_recovery(kind, "valid", source);
            }
        }
    }
}

///
/// @brief Verifies that a valid primary wins over a different valid backup definition.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_primary_wins) {
    ECF_NAME_THIS_TEST();

    for (const bool source : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << source) {
            check_server_recovery("valid", "valid", source);
        }
    }
}

///
/// @brief Verifies unusable checkpoint combinations fail without replacing either input or leaving a listener.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_no_usable_checkpoint) {
    ECF_NAME_THIS_TEST();

    for (const std::string primary : {"missing", "corrupt", "unreadable", "directory"}) {
        for (const std::string backup : {"missing", "corrupt", "unreadable", "directory"}) {
            if (primary == "missing" && backup == "missing") {
                continue;
            }
            for (const bool source : {false, true}) {
                BOOST_TEST_CONTEXT("Primary " << primary << ", backup " << backup
                                              << ", environment source: " << source) {
                    check_server_recovery(primary, backup, source);
                }
            }
        }
    }
}

///
/// @brief Verifies current startup settings replace stale server variables while recovered workflow data survives.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_current_settings) {
    ECF_NAME_THIS_TEST();

    for (const bool source : {false, true}) {
        BOOST_TEST_CONTEXT("Environment source: " << source) {
            check_server_recovery("valid", "missing", source, true);
            check_server_recovery("missing", "valid", source, true);
        }
    }
}

///
/// @brief Verifies that a restore requested at runtime discards a failed primary before using the backup.
///
BOOST_AUTO_TEST_CASE(test_server_recovery_runtime_restore_after_bad_primary) {
    ECF_NAME_THIS_TEST();

    for (const std::string backup : {"valid", "corrupt"}) {
        BOOST_TEST_CONTEXT("Backup " << backup) {
            check_runtime_restore(backup);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
