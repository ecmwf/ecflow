// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <unistd.h>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/ServerToClientResponse.hpp"
#include "ecflow/base/cts/user/CFileCmd.hpp"
#include "ecflow/base/cts/user/CtsCmd.hpp"
#include "ecflow/base/stc/ErrorCmd.hpp"
#include "ecflow/base/stc/GroupSTCCmd.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/ErrorCode.hpp"
#include "ecflow/core/FileFailure.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/EcfFile.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

BOOST_AUTO_TEST_SUITE(U_Base)
BOOST_AUTO_TEST_SUITE(T_ErrorCode)

///
/// @brief Verifies preservation of a classified error across a wire archive round-trip.
///
BOOST_AUTO_TEST_CASE(test_numeric_error_round_trip) {
    ECF_NAME_THIS_TEST();

    STC_Cmd_ptr reply = std::make_shared<ErrorCmd>("file not found", ecf::ErrorCode::FILE_UNAVAILABLE);
    std::string archive;
    ecf::save_as_string(archive, reply);

    STC_Cmd_ptr restored;
    ecf::restore_from_string(archive, restored);
    auto* error = dynamic_cast<ErrorCmd*>(restored.get());
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->code(), static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
    BOOST_CHECK_EQUAL(error->error(), "file not found");
}

///
/// @brief Reads a version-zero reply captured from the untouched legacy server.
///
BOOST_AUTO_TEST_CASE(test_legacy_error_archive_defaults_to_zero) {
    ECF_NAME_THIS_TEST();

    // Network framing is omitted; this is the archived JSON body from the old peer.
    const std::string archive =
        R"({"22ServerToClientResponse":{"stc_cmd_":{"polymorphic_id":2147483649,"polymorphic_name":"ErrorCmd","ptr_wrapper":{"id":2147483649,"data":{"cereal_class_version":0,"value0":{"cereal_class_version":0},"error_msg_":"Cannot find node at path '/missing' "}}}}})";
    ServerToClientResponse restored;
    ecf::restore_from_string(archive, restored);
    auto error = std::dynamic_pointer_cast<ErrorCmd>(restored.get_cmd());
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->code(), 0);
    BOOST_CHECK_EQUAL(error->error(), "Cannot find node at path '/missing' ");
    BOOST_CHECK(!error->ok());
}

///
/// @brief Does not mislabel a transport-generated reply as a server-classified failure.
///
BOOST_AUTO_TEST_CASE(test_transport_diagnosis_has_no_server_code) {
    ECF_NAME_THIS_TEST();

    ServerReply reply;
    ecf::ConnectionDiagnosis diagnosis;
    diagnosis.failure = ecf::ConnectionFailure::ConnectionRefused;
    reply.set_diagnosis(diagnosis);
    Cmd_ptr command = std::make_shared<CtsCmd>(CtsCmd::STATS);
    ErrorCmd error("transport error", ecf::ErrorCode::FILE_UNAVAILABLE);
    BOOST_CHECK(!error.handle_server_response(reply, command, false));
    BOOST_CHECK(!reply.has_error_code());
    BOOST_CHECK(!reply.group_reply());
    BOOST_CHECK(reply.group_errors().empty());
    BOOST_CHECK_NE(reply.error_msg().find("Cannot communicate with the server"), std::string::npos);
}

///
/// @brief Verifies that reused and locally replaced replies do not retain an earlier code.
///
BOOST_AUTO_TEST_CASE(test_error_code_does_not_leak_between_replies) {
    ECF_NAME_THIS_TEST();

    ErrorCmd error("missing", ecf::ErrorCode::FILE_UNAVAILABLE);
    error.init("unclassified");
    BOOST_CHECK_EQUAL(error.code(), 0);

    ServerReply reply;
    Cmd_ptr command = std::make_shared<CtsCmd>(CtsCmd::STATS);
    BOOST_CHECK(!error.handle_server_response(reply, command, false));
    BOOST_REQUIRE(reply.has_error_code());
    BOOST_CHECK_EQUAL(reply.error_code(), 0);
    reply.clear_for_invoke(false);
    BOOST_CHECK(!reply.has_error_code());
    reply.set_server_error(9001, "future error");
    BOOST_CHECK(reply.has_error_code());
    BOOST_CHECK(!reply.group_reply());
    reply.set_group_reply();
    BOOST_CHECK(reply.group_reply());
    BOOST_CHECK(!reply.has_error_code());
    BOOST_CHECK(reply.raw_error_message().empty());
    BOOST_CHECK(reply.group_errors().empty());
    reply.add_group_error(9001, "child failure");
    reply.set_server_error(0, "single failure");
    BOOST_CHECK(!reply.group_reply());
    BOOST_CHECK(reply.group_errors().empty());
    BOOST_CHECK(reply.has_error_code());
    BOOST_CHECK_EQUAL(reply.raw_error_message(), "single failure");
    reply.set_error_msg("local failure");
    BOOST_CHECK(!reply.has_error_code());
    BOOST_CHECK(reply.raw_error_message().empty());
}

///
/// @brief Verifies independent ordered snapshots of reused preallocated group errors.
///
BOOST_AUTO_TEST_CASE(test_group_snapshots_reused_error_replies) {
    ECF_NAME_THIS_TEST();

    GroupSTCCmd group;
    group.addChild(PreAllocatedReply::error_cmd("first", ecf::ErrorCode::FILE_UNAVAILABLE));
    group.addChild(PreAllocatedReply::error_cmd("second", ecf::ErrorCode::EMPTY_CONTENT));

    ServerReply reply;
    Cmd_ptr command = std::make_shared<CtsCmd>(CtsCmd::STATS);
    BOOST_CHECK(!group.handle_server_response(reply, command, false));
    BOOST_REQUIRE_EQUAL(reply.group_errors().size(), 2);
    BOOST_CHECK_EQUAL(reply.group_errors()[0].first, static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
    BOOST_CHECK_EQUAL(reply.group_errors()[0].second, "first");
    BOOST_CHECK_EQUAL(reply.group_errors()[1].first, static_cast<int>(ecf::ErrorCode::EMPTY_CONTENT));
    BOOST_CHECK_EQUAL(reply.group_errors()[1].second, "second");
    BOOST_CHECK(!reply.has_error_code());
    reply.clear_for_invoke(false);
    BOOST_CHECK(reply.group_errors().empty());
    BOOST_CHECK(!reply.group_reply());
}

///
/// @brief Retains unknown and zero child codes through a grouped wire archive.
///
BOOST_AUTO_TEST_CASE(test_group_archive_preserves_raw_child_codes) {
    ECF_NAME_THIS_TEST();

    auto group = std::make_shared<GroupSTCCmd>();
    group->addChild(std::make_shared<ErrorCmd>("future", 9001));
    group->addChild(std::make_shared<ErrorCmd>("unclassified", 0));
    STC_Cmd_ptr outgoing = group;
    std::string archive;
    ecf::save_as_string(archive, outgoing);

    STC_Cmd_ptr restored;
    ecf::restore_from_string(archive, restored);
    ServerReply reply;
    Cmd_ptr command = std::make_shared<CtsCmd>(CtsCmd::STATS);
    BOOST_CHECK(!restored->handle_server_response(reply, command, false));
    BOOST_REQUIRE_EQUAL(reply.group_errors().size(), 2);
    BOOST_CHECK_EQUAL(reply.group_errors()[0].first, 9001);
    BOOST_CHECK_EQUAL(reply.group_errors()[0].second, "future");
    BOOST_CHECK_EQUAL(reply.group_errors()[1].first, 0);
    BOOST_CHECK_EQUAL(reply.group_errors()[1].second, "unclassified");
    BOOST_CHECK(!reply.has_error_code());
}

///
/// @brief Verifies that unknown numeric codes survive serialization without file classification.
///
BOOST_AUTO_TEST_CASE(test_unknown_code_is_retained_without_file_classification) {
    ECF_NAME_THIS_TEST();

    ErrorCmd error("future error");
    error.init("future error", 9001);
    std::string archive;
    STC_Cmd_ptr outgoing = std::make_shared<ErrorCmd>(error);
    ecf::save_as_string(archive, outgoing);
    STC_Cmd_ptr incoming;
    ecf::restore_from_string(archive, incoming);
    BOOST_CHECK_EQUAL(dynamic_cast<ErrorCmd&>(*incoming).code(), 9001);
    ecf::CodedError coded("formatted", 9001, "future error");
    BOOST_CHECK_EQUAL(coded.code(), 9001);
    BOOST_CHECK_EQUAL(coded.message(), "future error");
}

///
/// @brief Distinguishes missing files from configuration and command failures after fallback.
///
BOOST_AUTO_TEST_CASE(test_file_absence_is_not_a_generic_open_failure) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    auto location = MakeDirectory().create();
    BOOST_CHECK(ecf::file_is_absent((location.path() / "missing.job").string()));
    BOOST_CHECK(!ecf::file_is_absent(location.path().string()));
    BOOST_CHECK(!ecf::valid_file_root((location.path() / "missing").string()));
    BOOST_CHECK(ecf::valid_file_root(location.path().string()));
    std::ofstream(location.path() / "regular-file") << "not a directory\n";
    BOOST_CHECK(ecf::file_is_absent((location.path() / "regular-file" / "child").string()));

    Defs defs;
    auto suite = defs.add_suite("coded");
    suite->add_variable("ECF_HOME", location.path().string());
    auto task = suite->add_task("t");
    task->add_variable("ECF_JOB", (location.path() / "missing.job").string());
    TestLog log((location.path() / "server.log").string());
    MockServer server(&defs);
    auto request_file = [&server](const std::string& path, const std::string& kind) {
        ClientToServerRequest request;
        request.set_cmd(std::make_shared<CFileCmd>(path, kind, ""));
        return request.handleRequest(&server);
    };
    for (const auto& file_type : {"job", "jobout", "kill", "stat", "script", "manual"}) {
        try {
            request_file("/coded/t", file_type);
            BOOST_FAIL("Expected missing file to fail for " << file_type);
        }
        catch (const ecf::FileHandlingError& error) {
            BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
        }
    }
    auto user_jobout = suite->add_task("user_jobout");
    user_jobout->add_variable("ECF_JOBOUT", (location.path() / "missing-user.jobout").string());
    try {
        request_file("/coded/user_jobout", "jobout");
        BOOST_FAIL("Expected user-specified job output to be missing");
    }
    catch (const ecf::FileHandlingError& error) {
        BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
        BOOST_CHECK_NE(std::string(error.what()).find("user specified job-out"), std::string::npos);
    }
    try {
        request_file("/coded", "jobout");
        BOOST_FAIL("Expected non-task jobout request to fail");
    }
    catch (const ecf::CodedError&) {
        BOOST_FAIL("A non-task jobout request is not a classified file failure");
    }
    catch (const std::runtime_error&) {
    }
    try {
        request_file("/coded", "manual");
        BOOST_FAIL("Expected missing suite manual to fail");
    }
    catch (const ecf::FileHandlingError& error) {
        BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
    }

    // An invalid ECF_OUT still permits confirmed absence when ECF_HOME is a valid fallback.
    suite->add_variable("ECF_OUT", (location.path() / "invalid-out").string());
    task->update_generated_variables();
    try {
        request_file("/coded/t", "jobout");
        BOOST_FAIL("Expected missing job output to fail");
    }
    catch (const ecf::FileHandlingError& error) {
        BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
        BOOST_CHECK_NE(std::string(error.what()).find("ECF_JOBOUT=ECF_OUT/ECF_NAME.ECF_TRYNO"), std::string::npos);
    }

    auto invalid_home = defs.add_suite("invalid_home");
    invalid_home->add_variable("ECF_HOME", (location.path() / "invalid-home").string());
    invalid_home->add_task("t");
    try {
        request_file("/invalid_home/t", "jobout");
        BOOST_FAIL("Expected invalid ECF_HOME to fail");
    }
    catch (const ecf::CodedError&) {
        BOOST_FAIL("An invalid ECF_HOME is not confirmed file absence");
    }
    catch (const std::runtime_error&) {
    }

    fs::create_directories(location.path() / "coded");
    {
        std::ofstream fallback(location.path() / "coded" / "t.ecf");
        fallback << "%manual\nfrom fallback\n%end\necho ready\n";
    }
    // A missing generated ECF_SCRIPT must not hide a usable ECF_HOME fallback.
    BOOST_CHECK(request_file("/coded/t", "script")->ok());

    // A candidate removed after location but before open remains a missing file,
    // whereas a failed script-fetch command is a processing failure.
    std::string script_contents;
    EcfFile removed_script(task.get(), (location.path() / "coded" / "gone.ecf").string(), EcfFile::ECF_HOME);
    BOOST_CHECK_THROW(removed_script.script(script_contents), ecf::FileHandlingError);
    EcfFile failed_command(task.get(), "false", EcfFile::ECF_SCRIPT_CMD);
    try {
        failed_command.script(script_contents);
        BOOST_FAIL("Expected command failure");
    }
    catch (const ecf::FileHandlingError&) {
        BOOST_FAIL("A failed script command is not file absence");
    }
    catch (const std::runtime_error&) {
    }

    fs::remove(location.path() / "coded" / "t.ecf");
    suite->add_variable("ECF_FILES", (location.path() / "invalid-root").string());
    try {
        request_file("/coded/t", "script");
        BOOST_FAIL("Expected invalid configured root to fail");
    }
    catch (const ecf::FileHandlingError&) {
        BOOST_FAIL("An invalid configured root is not confirmed file absence");
    }
    catch (const std::runtime_error&) {
    }
}

///
/// @brief Distinguishes missing source, missing include and an empty container manual.
///
BOOST_AUTO_TEST_CASE(test_manual_and_script_negative_controls) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    auto location = MakeDirectory().create();
    Defs defs;
    auto suite = defs.add_suite("manual_cases");
    suite->add_variable("ECF_HOME", location.path().string());
    auto task = suite->add_task("t");
    TestLog log((location.path() / "server.log").string());
    MockServer server(&defs);
    auto request_file = [&server](const std::string& path, const std::string& kind) {
        ClientToServerRequest request;
        request.set_cmd(std::make_shared<CFileCmd>(path, kind, ""));
        return request.handleRequest(&server);
    };

    fs::create_directories(location.path() / "manual_cases");
    auto script = location.path() / "manual_cases" / "t.ecf";
    {
        std::ofstream output(script);
        output << "echo no manual section\n";
    }
    BOOST_CHECK(request_file("/manual_cases/t", "manual")->ok());

    {
        std::ofstream output(script);
        output << "%manual\n%include absent.h\n%end\n";
    }
    try {
        request_file("/manual_cases/t", "manual");
        BOOST_FAIL("Expected missing include processing to fail");
    }
    catch (const ecf::CodedError&) {
        BOOST_FAIL("A missing include is not a missing source script");
    }
    catch (const std::runtime_error&) {
    }

    fs::remove(script);
    try {
        request_file("/manual_cases/t", "script");
        BOOST_FAIL("Expected missing source script to fail");
    }
    catch (const ecf::FileHandlingError& error) {
        BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::FILE_UNAVAILABLE));
    }

    auto fetch_task = suite->add_task("fetch");
    fetch_task->add_variable("ECF_FETCH", "false");
    auto command_task = suite->add_task("command");
    command_task->add_variable("ECF_SCRIPT_CMD", "false");
    for (const auto& path : {"/manual_cases/fetch", "/manual_cases/command"}) {
        try {
            request_file(path, "script");
            BOOST_FAIL("Expected failed script retrieval command");
        }
        catch (const ecf::CodedError&) {
            BOOST_FAIL("A failed script retrieval command is not file absence");
        }
        catch (const std::runtime_error&) {
        }
    }

    auto manual_root = location.path() / "manual_files";
    fs::create_directories(manual_root);
    suite->add_variable("ECF_FILES", manual_root.string());
    std::ofstream(manual_root / "manual_cases.man").close();
    try {
        request_file("/manual_cases", "manual");
        BOOST_FAIL("Expected an empty suite manual to fail");
    }
    catch (const ecf::EmptyContentError& error) {
        BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::EMPTY_CONTENT));
    }
    suite->add_family("family");
    fs::create_directories(manual_root / "manual_cases");
    std::ofstream(manual_root / "manual_cases" / "family.man").close();
    try {
        request_file("/manual_cases/family", "manual");
        BOOST_FAIL("Expected an empty family manual to fail");
    }
    catch (const ecf::EmptyContentError& error) {
        BOOST_CHECK_EQUAL(error.code(), static_cast<int>(ecf::ErrorCode::EMPTY_CONTENT));
    }
    auto home_manual = location.path() / "manual_cases.man";
    std::ofstream(home_manual) << "manual from home\n";
    BOOST_CHECK(request_file("/manual_cases", "manual")->ok());
    fs::remove(home_manual);

    auto invalid_home = defs.add_suite("invalid_home_manual");
    invalid_home->add_variable("ECF_FILES", manual_root.string());
    invalid_home->add_variable("ECF_HOME", (location.path() / "invalid-home").string());
    std::ofstream(manual_root / "invalid_home_manual.man").close();
    try {
        request_file("/invalid_home_manual", "manual");
        BOOST_FAIL("Expected invalid manual fallback root to fail");
    }
    catch (const ecf::CodedError&) {
        BOOST_FAIL("An invalid ECF_HOME takes precedence over an empty ECF_FILES manual");
    }
    catch (const std::runtime_error&) {
    }

    suite->add_variable("ECF_FILES", (location.path() / "invalid-manual-root").string());
    try {
        request_file("/manual_cases", "manual");
        BOOST_FAIL("Expected invalid manual root to fail");
    }
    catch (const ecf::CodedError&) {
        BOOST_FAIL("An invalid ECF_FILES root is not confirmed manual absence");
    }
    catch (const std::runtime_error&) {
    }
}

///
/// @brief An inaccessible file and a symlink loop are not confirmed absence.
///
BOOST_AUTO_TEST_CASE(test_file_io_failures_are_not_absence) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;
    auto location = MakeDirectory().create();
    auto loop     = location.path() / "loop.job";
    fs::create_symlink(loop.filename(), loop);
    BOOST_CHECK(!ecf::file_is_absent(loop.string()));

    auto restricted = location.path() / "restricted";
    fs::create_directory(restricted);
    auto job = restricted / "job";
    std::ofstream(job) << "content\n";

    Defs defs;
    auto suite     = defs.add_suite("io_cases");
    auto loop_task = suite->add_task("loop");
    loop_task->add_variable("ECF_JOB", loop.string());
    auto restricted_task = suite->add_task("restricted");
    restricted_task->add_variable("ECF_JOB", job.string());
    TestLog log((location.path() / "server.log").string());
    MockServer server(&defs);
    auto expect_unclassified = [&server](const std::string& path) {
        ClientToServerRequest request;
        request.set_cmd(std::make_shared<CFileCmd>(path, "job", ""));
        try {
            request.handleRequest(&server);
            BOOST_FAIL("Expected job read failure");
        }
        catch (const ecf::CodedError&) {
            BOOST_FAIL("An I/O failure is not confirmed file absence");
        }
        catch (const std::runtime_error&) {
        }
    };
    expect_unclassified("/io_cases/loop");

    fs::permissions(restricted, fs::perms::none);
    // Container builds may run as root, which bypasses directory permissions.
    if (geteuid() != 0) {
        BOOST_CHECK(!ecf::file_is_absent(job.string()));
        expect_unclassified("/io_cases/restricted");
    }
    fs::permissions(restricted, fs::perms::owner_all);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
