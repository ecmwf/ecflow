// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "MockServer.hpp"
#include "TestHelper.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/ServerReply.hpp"
#include "ecflow/base/cts/user/AlterCmd.hpp"
#include "ecflow/base/stc/SSyncCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Serialization.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/System.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/formatter/DefsWriter.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/TestLog.hpp"

///
/// \brief Pins the behaviour of `--alter change repeat <value>` (AlterCmd::REPEAT): handling by the server,
///        persistence of the request, and synchronisation of the result with clients.
///

namespace {

///
/// Provides the log for each test case in the suite, since the commands under test report their activity to it.
///
struct LogFixture
{
    ecf::test::scaffold::TestLog test_log{"test_alter_cmd_repeat.log"};
    ~LogFixture() { ecf::System::destroy(); }
};

///
/// @brief Creates a request to change the Repeat of the given nodes.
///
Cmd_ptr change_repeat(const std::vector<std::string>& paths, const std::string& value) {
    return std::make_shared<AlterCmd>(paths, AlterCmd::REPEAT, value, "");
}

Cmd_ptr change_repeat(const std::string& path, const std::string& value) {
    return change_repeat(std::vector<std::string>{path}, value);
}

///
/// @brief Handles a request that is expected to fail, and retrieves the error.
///
/// @return the error message, or an empty string if the request succeeded
///
std::string error_of(Defs& defs, Cmd_ptr cmd) {
    ClientToServerRequest request;
    request.set_cmd(cmd);
    MockServer server(&defs);
    try {
        request.handleRequest(&server);
    }
    catch (const std::runtime_error& e) {
        return e.what();
    }
    return std::string{};
}

///
/// @brief Checks that the error contains the given fragment.
///
void check_error(const std::string& error, const std::string& fragment) {
    BOOST_CHECK_MESSAGE(error.find(fragment) != std::string::npos,
                        "expected '" << fragment << "' in the error, but found: '" << error << "'");
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_FIXTURE_TEST_SUITE(T_AlterCmdRepeat, LogFixture)

/*
 * Test Suite: ::handling
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(handling)

BOOST_AUTO_TEST_CASE(change_repeat_datetime) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->addRepeat(RepeatDateTime("DT", "20240101T000000", "20240102T000000", "06:00:00"));

    TestHelper::invokeRequest(&defs, change_repeat(s->absNodePath(), "20240101T180000"));
    BOOST_CHECK_EQUAL(s->repeat().valueAsString(), "20240101T180000");
}

BOOST_AUTO_TEST_CASE(change_repeat_datetimelist) {
    ECF_NAME_THIS_TEST();

    using ecf::Instant;
    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->addRepeat(RepeatDateTimeList("DTL", {Instant::parse("20240101T000000"), Instant::parse("20240102T120000")}));

    TestHelper::invokeRequest(&defs, change_repeat(s->absNodePath(), "20240102T120000"));
    BOOST_CHECK_EQUAL(s->repeat().valueAsString(), "20240102T120000");
    BOOST_CHECK_EQUAL(s->repeat().index_or_value(), 1);
}

BOOST_AUTO_TEST_CASE(change_repeat_day_is_ignored) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    s->addRepeat(RepeatDay(2));

    TestHelper::invokeRequest(&defs, change_repeat(s->absNodePath(), "5"), false);
    BOOST_CHECK_EQUAL(s->repeat().step(), 2);
    BOOST_CHECK(s->repeat().is_repeat_day());
}

BOOST_AUTO_TEST_CASE(change_repeat_on_several_paths) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s  = defs.add_suite("s");
    task_ptr t1  = s->add_task("t1");
    task_ptr t2  = s->add_task("t2");
    family_ptr f = s->add_family("f");
    t1->addRepeat(RepeatInteger("N", 0, 10, 1));
    t2->addRepeat(RepeatEnumerated("E", {"1", "3", "5", "7"}));
    f->addRepeat(RepeatDateList("DL", {20260101, 20260103}));

    // The value is interpreted by each Repeat in turn: an integer, a member of the enumeration, an index...
    TestHelper::invokeRequest(&defs,
                              change_repeat(std::vector<std::string>{t1->absNodePath(), t2->absNodePath()}, "3"));
    BOOST_CHECK_EQUAL(t1->repeat().value(), 3);
    BOOST_CHECK_EQUAL(t2->repeat().index_or_value(), 1); // the member "3"
}

BOOST_AUTO_TEST_SUITE_END() // handling

/*
 * Test Suite: ::errors
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(errors)

BOOST_AUTO_TEST_CASE(change_repeat_on_node_without_repeat_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");

    auto error = error_of(defs, change_repeat(t->absNodePath(), "1"));
    check_error(error, "Alter (change) failed for /s/t");
    check_error(error, "Could not find repeat on /s/t");
}

BOOST_AUTO_TEST_CASE(change_repeat_on_unknown_node_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    defs.add_suite("s");

    auto error = error_of(defs, change_repeat("/s/unknown", "1"));
    check_error(error, "Could not find node at path /s/unknown");
}

BOOST_AUTO_TEST_CASE(change_repeat_with_invalid_value_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t  = s->add_task("t");
    t->addRepeat(RepeatDate("YMD", 20260101, 20261231, 7));

    struct Case
    {
        std::string value;
        std::string fragment;
    };
    const std::vector<Case> cases{
        {"", "expected 8 characters"},
        {"2026010", "expected 8 characters"},
        {"2026010x", "is not convertible to an long"},
        {"20260230", "is not valid"},
        {"20251225", "should be in the range"},
        {"20260102", "is not in line with the delta/step"},
    };
    for (const auto& c : cases) {
        auto error = error_of(defs, change_repeat(t->absNodePath(), c.value));
        check_error(error, "Alter (change) failed for /s/t");
        check_error(error, c.fragment);
        BOOST_CHECK_EQUAL(t->repeat().value(), 20260101);
    }
}

BOOST_AUTO_TEST_CASE(change_repeat_with_non_member_is_refused) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t1 = s->add_task("t1");
    task_ptr t2 = s->add_task("t2");
    task_ptr t3 = s->add_task("t3");
    t1->addRepeat(RepeatDateList("DL", {20260101, 20260103}));
    t2->addRepeat(RepeatEnumerated("E", {"a", "b"}));
    t3->addRepeat(RepeatString("S", {"a", "b"}));

    check_error(error_of(defs, change_repeat(t1->absNodePath(), "20260102")), "is not a valid member");
    check_error(error_of(defs, change_repeat(t2->absNodePath(), "c")), "is not a valid index or a member");
    check_error(error_of(defs, change_repeat(t2->absNodePath(), "2")), "is not a valid index");
    check_error(error_of(defs, change_repeat(t3->absNodePath(), "c")), "is not a valid index or member");
    check_error(error_of(defs, change_repeat(t3->absNodePath(), "2")), "is not a valid index");

    BOOST_CHECK_EQUAL(t1->repeat().index_or_value(), 0);
    BOOST_CHECK_EQUAL(t2->repeat().index_or_value(), 0);
    BOOST_CHECK_EQUAL(t3->repeat().index_or_value(), 0);
}

BOOST_AUTO_TEST_CASE(change_repeat_on_several_paths_is_applied_per_path) {
    ECF_NAME_THIS_TEST();

    Defs defs;
    suite_ptr s = defs.add_suite("s");
    task_ptr t1 = s->add_task("t1");
    task_ptr t2 = s->add_task("t2");
    task_ptr t3 = s->add_task("t3");
    t1->addRepeat(RepeatInteger("N", 0, 10, 1));
    t2->addRepeat(RepeatInteger("N", 0, 3, 1));
    t3->addRepeat(RepeatInteger("N", 0, 10, 1));

    // The request fails for the path whose Repeat refuses the value, and is still applied to the others
    auto error = error_of(
        defs, change_repeat(std::vector<std::string>{t1->absNodePath(), t2->absNodePath(), t3->absNodePath()}, "5"));
    check_error(error, "Alter (change) failed for /s/t2");
    BOOST_CHECK_MESSAGE(error.find("/s/t1") == std::string::npos, "unexpected error for /s/t1: " << error);
    BOOST_CHECK_MESSAGE(error.find("/s/t3") == std::string::npos, "unexpected error for /s/t3: " << error);

    BOOST_CHECK_EQUAL(t1->repeat().value(), 5);
    BOOST_CHECK_EQUAL(t2->repeat().value(), 0);
    BOOST_CHECK_EQUAL(t3->repeat().value(), 5);
}

BOOST_AUTO_TEST_CASE(change_repeat_value_is_not_validated_by_the_client) {
    ECF_NAME_THIS_TEST();

    // Any value, empty included, is accepted when the request is built; only the server validates it
    const std::vector<std::string> paths{"/s/t"};
    BOOST_CHECK_NO_THROW(AlterCmd(paths, "change", "repeat", "", ""));
    BOOST_CHECK_NO_THROW(AlterCmd(paths, "change", "repeat", "garbage", ""));
    BOOST_CHECK_NO_THROW(AlterCmd(paths, "change", "repeat", "--begin 20260101 --end 20261231", ""));
}

BOOST_AUTO_TEST_SUITE_END() // errors

/*
 * Test Suite: ::persistence
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(persistence)

BOOST_AUTO_TEST_CASE(change_repeat_request_survives_a_round_trip) {
    ECF_NAME_THIS_TEST();

    for (const std::string value : {"20260101", "a value with spaces", "--begin 20260101 --end 20261231"}) {
        ClientToServerRequest request;
        request.set_cmd(change_repeat(std::vector<std::string>{"/s/t1", "/s/t2"}, value));

        std::string text;
        ecf::save_as_string(text, request);

        ClientToServerRequest restored;
        ecf::restore_from_string(text, restored);
        BOOST_CHECK_MESSAGE(restored == request, "expected the request to survive a round trip: " << text);

        auto* cmd = dynamic_cast<AlterCmd*>(restored.get_cmd().get());
        BOOST_REQUIRE(cmd);
        BOOST_CHECK_EQUAL(cmd->name(), value);
        BOOST_CHECK_EQUAL(cmd->value(), "");
        BOOST_CHECK_EQUAL(cmd->change_attr_type(), AlterCmd::REPEAT);
    }
}

BOOST_AUTO_TEST_CASE(change_repeat_requests_differing_by_value_differ) {
    ECF_NAME_THIS_TEST();

    ClientToServerRequest a;
    a.set_cmd(change_repeat("/s/t", "1"));
    ClientToServerRequest b;
    b.set_cmd(change_repeat("/s/t", "2"));
    ClientToServerRequest c;
    c.set_cmd(change_repeat("/s/t", "1"));

    BOOST_CHECK(!(a == b));
    BOOST_CHECK(a == c);
}

BOOST_AUTO_TEST_CASE(change_repeat_request_is_printed_unquoted) {
    ECF_NAME_THIS_TEST();

    {
        AlterCmd cmd(std::vector<std::string>{"/s/t"}, AlterCmd::REPEAT, "20260101", "");
        std::string printed;
        cmd.print_only(printed);
        BOOST_CHECK_EQUAL(printed, "--alter change repeat 20260101 /s/t");
    }
    {
        AlterCmd cmd(std::vector<std::string>{"/s/t"}, AlterCmd::REPEAT, "--begin 20260101 --end 20261231", "");
        std::string printed;
        cmd.print_only(printed);
        BOOST_CHECK_EQUAL(printed, "--alter change repeat --begin 20260101 --end 20261231 /s/t");
    }
    {
        AlterCmd cmd(std::vector<std::string>{"/s/t1", "/s/t2"}, AlterCmd::REPEAT, "20260101", "");
        std::string printed;
        cmd.print(printed);
        const std::string expected = "--alter change repeat 20260101 /s/t1 /s/t2 :";
        BOOST_CHECK_MESSAGE(printed.rfind(expected, 0) == 0, "expected '" << expected << "...' but found " << printed);
    }
}

BOOST_AUTO_TEST_SUITE_END() // persistence

/*
 * Test Suite: ::synchronisation
 * ************************************************************ */

BOOST_AUTO_TEST_SUITE(synchronisation)

BOOST_AUTO_TEST_CASE(change_repeat_reaches_client_through_incremental_sync) {
    ECF_NAME_THIS_TEST();

    auto make_defs = []() {
        auto defs    = Defs::create();
        suite_ptr s  = defs->add_suite("s");
        family_ptr f = s->add_family("f");
        f->addRepeat(RepeatDate("YMD", 20260101, 20261231, 1));
        f->addEvent(Event("e"));
        f->addMeter(Meter("m", 0, 100));
        f->addLabel(Label("l", "value"));
        f->add_task("t")->addRepeat(RepeatInteger("N", 0, 10, 2));
        defs->beginAll();
        return defs;
    };

    defs_ptr server_defs = make_defs();
    defs_ptr client_defs = make_defs();
    server_defs->server_state().set_state(SState::HALTED);
    client_defs->server_state().set_state(SState::HALTED);

    ServerReply server_reply;
    server_reply.set_client_defs(client_defs);
    BOOST_REQUIRE(*server_defs == *client_defs);

    unsigned int client_state_change_no  = Ecf::state_change_no();
    unsigned int client_modify_change_no = Ecf::modify_change_no();

    TestHelper::invokeRequest(server_defs.get(), change_repeat("/s/f", "20260315"));
    TestHelper::invokeRequest(server_defs.get(), change_repeat("/s/f/t", "4"));
    BOOST_REQUIRE(!(*server_defs == *client_defs));

    MockServer mock_server(server_defs);
    SSyncCmd cmd(0, client_state_change_no, client_modify_change_no, &mock_server);
    BOOST_CHECK(cmd.do_sync(server_reply));
    BOOST_CHECK(server_reply.in_sync());
    BOOST_CHECK(!server_reply.full_sync());

    auto client_f = client_defs->findAbsNode("/s/f");
    auto client_t = client_defs->findAbsNode("/s/f/t");
    BOOST_REQUIRE(client_f && client_t);
    BOOST_CHECK_EQUAL(client_f->repeat().value(), 20260315);
    BOOST_CHECK_EQUAL(client_t->repeat().value(), 4);
    BOOST_CHECK_EQUAL(client_f->repeat().start(), 20260101);
    BOOST_CHECK_EQUAL(client_f->repeat().end(), 20261231);
    BOOST_CHECK_EQUAL(client_f->repeat().step(), 1);
    BOOST_CHECK_EQUAL(client_f->events().size(), 1u);
    BOOST_CHECK_EQUAL(client_f->meters().size(), 1u);
    BOOST_CHECK_EQUAL(client_f->labels().size(), 1u);

    DebugEquality debug_equality; // only has effect in DEBUG build
    BOOST_CHECK_MESSAGE(*server_defs == *client_defs, "expected client and server to be the same after sync");
}

BOOST_AUTO_TEST_SUITE_END() // synchronisation

BOOST_AUTO_TEST_SUITE_END() // T_AlterCmdRepeat

BOOST_AUTO_TEST_SUITE_END() // U_Base
