// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <boost/test/unit_test.hpp>

#include "ecflow/base/stc/SNewsCmd.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Base)

BOOST_AUTO_TEST_SUITE(T_SNewsCmd)

namespace {

/// Sets the server-wide change numbers for the duration of a test, and restores the previous values afterwards,
/// since they are process-wide and shared with every other test in the module.
struct ChangeNumbers
{
    ChangeNumbers(unsigned int state_change_no, unsigned int modify_change_no)
        : saved_state_(Ecf::state_change_no()),
          saved_modify_(Ecf::modify_change_no()) {
        Ecf::set_state_change_no(state_change_no);
        Ecf::set_modify_change_no(modify_change_no);
    }
    ~ChangeNumbers() {
        Ecf::set_state_change_no(saved_state_);
        Ecf::set_modify_change_no(saved_modify_);
    }

    unsigned int saved_state_;
    unsigned int saved_modify_;
};

bool contains(const std::string& text, const std::string& expected) {
    return text.find(expected) != std::string::npos;
}

} // namespace

BOOST_AUTO_TEST_CASE(test_evaluate_news_without_handle) {
    ECF_NAME_THIS_TEST();

    // With client handle 0, the decision only depends on the server-wide change numbers
    const unsigned int state  = 10;
    const unsigned int modify = 5;
    ChangeNumbers numbers(state, modify);

    defs_ptr defs             = Defs::create();
    const ClientSuiteMgr& mgr = defs->client_suite_mgr();

    { // client in step with the server -> NO_NEWS
        auto outcome = evaluate_news(0, state, modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NO_NEWS);
        BOOST_CHECK_EQUAL(outcome.annotation, " [:NO_NEWS]");
    }

    { // client state number ahead of the server (server restarted) -> DO_FULL_SYNC
        auto outcome = evaluate_news(0, state + 1, modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::DO_FULL_SYNC);
        BOOST_CHECK_EQUAL(outcome.annotation, " [server(10,5) : client no > server no ! :DO_FULL_SYNC]");
    }

    { // client modify number ahead of the server -> DO_FULL_SYNC
        auto outcome = evaluate_news(0, state, modify + 1, mgr);
        BOOST_CHECK(outcome.news == ServerReply::DO_FULL_SYNC);
        BOOST_CHECK_EQUAL(outcome.annotation, " [server(10,5) : client no > server no ! :DO_FULL_SYNC]");
    }

    { // client modify number behind the server -> NEWS, large-scale changes
        auto outcome = evaluate_news(0, state, modify - 2, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NEWS);
        BOOST_CHECK_EQUAL(outcome.annotation, " [server(10,5) : *Large* scale changes(2) :NEWS]");
    }

    { // client modify number behind takes precedence over the state number, whatever the latter
        auto outcome = evaluate_news(0, state - 3, modify - 1, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NEWS);
        BOOST_CHECK_EQUAL(outcome.annotation, " [server(10,5) : *Large* scale changes(1) :NEWS]");
    }

    { // client state number behind the server -> NEWS, small-scale changes
        auto outcome = evaluate_news(0, state - 3, modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NEWS);
        BOOST_CHECK_EQUAL(outcome.annotation, " [server(10,5) : *Small* scale changes(3) :NEWS]");
    }
}

BOOST_AUTO_TEST_CASE(test_evaluate_news_with_unknown_handle) {
    ECF_NAME_THIS_TEST();

    ChangeNumbers numbers(10, 5);

    defs_ptr defs             = Defs::create();
    const ClientSuiteMgr& mgr = defs->client_suite_mgr();

    // No handle was ever registered, so any non-zero handle is unknown -> DO_FULL_SYNC
    auto outcome = evaluate_news(42, 0, 0, mgr);
    BOOST_CHECK(outcome.news == ServerReply::DO_FULL_SYNC);
    BOOST_CHECK_EQUAL(outcome.annotation, " [server(10,5) : Cannot find handle(42) :DO_FULL_SYNC]");
}

BOOST_AUTO_TEST_CASE(test_evaluate_news_with_handle) {
    ECF_NAME_THIS_TEST();

    ChangeNumbers numbers(10, 5);

    defs_ptr defs = Defs::create();
    defs->add_suite("s1");
    defs->add_suite("s2");
    ClientSuiteMgr& mgr = defs->client_suite_mgr();

    // Register a handle over a subset of the suites. The registration itself marks the handle as changed.
    const unsigned int handle = mgr.create_client_suite(false, {"s1"}, "user");
    BOOST_REQUIRE(mgr.valid_handle(handle));
    BOOST_REQUIRE(mgr.handle_changed(handle));

    // The numbers the handle currently holds, as the server sees them
    unsigned int handle_state  = 0;
    unsigned int handle_modify = 0;
    mgr.max_change_no(handle, handle_state, handle_modify);

    { // client ahead of the handle (server restarted) -> DO_FULL_SYNC, checked before the handle-changed flag
        auto outcome = evaluate_news(handle, handle_state + 1, handle_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::DO_FULL_SYNC);
        BOOST_CHECK(contains(outcome.annotation, " [server handle("));
        BOOST_CHECK(contains(outcome.annotation, " : client no > server no ! :DO_FULL_SYNC]"));
    }

    { // client in step, but the handle changed (new handle, or suites added or removed) -> NEWS
        auto outcome = evaluate_news(handle, handle_state, handle_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NEWS);
        BOOST_CHECK(
            contains(outcome.annotation, " : *Large* scale changes (new handle or suites added or removed) :NEWS]"));
    }

    // Creating the client definitions clears the handle-changed flag, as the server does on a full sync
    (void)mgr.create_defs(handle, defs);
    BOOST_REQUIRE(!mgr.handle_changed(handle));

    { // client in step with the handle -> NO_NEWS
        auto outcome = evaluate_news(handle, handle_state, handle_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NO_NEWS);
        BOOST_CHECK_EQUAL(outcome.annotation, " [:NO_NEWS]");
    }

    // Changes are made as the server makes them: with the counters active, and within a SuiteChanged guard that
    // records the new counters on the suite when the guard goes out of scope.
    Ecf::set_server(true);

    // A large-scale change to the suite in the handle (modify number) moves the handle ahead of the client.
    // Adding or removing nodes below a suite only advances the state number; the modify number advances on
    // suite-level changes such as a requeue, a reset, or a suite being added, removed or reordered.
    {
        SuiteChanged changed(defs->findSuite("s1"));
        Ecf::incr_modify_change_no();
    }

    unsigned int modified_state  = 0;
    unsigned int modified_modify = 0;
    mgr.max_change_no(handle, modified_state, modified_modify);
    BOOST_REQUIRE(modified_modify > handle_modify);

    { // client modify number behind the handle -> NEWS, large-scale changes
        auto outcome = evaluate_news(handle, modified_state, handle_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NEWS);
        BOOST_CHECK(contains(outcome.annotation, " [server handle("));
        BOOST_CHECK(contains(outcome.annotation, " : *Large* scale changes :NEWS]"));
    }

    { // client in step again -> NO_NEWS
        auto outcome = evaluate_news(handle, modified_state, modified_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NO_NEWS);
    }

    // A state change to the suite in the handle (state number) moves the handle ahead of the client
    {
        SuiteChanged changed(defs->findSuite("s1"));
        defs->findSuite("s1")->set_state(NState::ACTIVE);
    }

    unsigned int changed_state  = 0;
    unsigned int changed_modify = 0;
    mgr.max_change_no(handle, changed_state, changed_modify);
    BOOST_REQUIRE(changed_state > modified_state);
    BOOST_REQUIRE(changed_modify == modified_modify);

    { // client state number behind the handle -> NEWS, small-scale changes
        auto outcome = evaluate_news(handle, modified_state, modified_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NEWS);
        BOOST_CHECK(contains(outcome.annotation, " [server handle("));
        BOOST_CHECK(contains(outcome.annotation, " : *Small* scale changes :NEWS]"));
    }

    { // a change to a suite outside the handle is not news for this handle
        {
            SuiteChanged changed(defs->findSuite("s2"));
            Ecf::incr_modify_change_no();
            defs->findSuite("s2")->set_state(NState::ACTIVE);
        }

        auto outcome = evaluate_news(handle, changed_state, changed_modify, mgr);
        BOOST_CHECK(outcome.news == ServerReply::NO_NEWS);
    }

    Ecf::set_server(false);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
