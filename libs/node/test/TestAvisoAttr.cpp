// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/CalendarUpdateParams.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Operations.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace {

///
/// @brief Holds what the fake backend received, and what it is to deliver.
///
struct FakeBackendState
{
    std::optional<ecf::service::aviso::AvisoSubscribe> subscribed;
    int subscriptions = 0;
    std::vector<ecf::service::aviso::AvisoResponse> pending;
};

class FakeBackend : public ecf::service::aviso::BaseAvisoBackend {
public:
    explicit FakeBackend(std::shared_ptr<FakeBackendState> state)
        : state_{std::move(state)} {}

    void subscribe(const ecf::service::aviso::AvisoSubscribe& request) override {
        state_->subscribed = request;
        ++state_->subscriptions;
    }

    std::vector<ecf::service::aviso::AvisoResponse> drain() override {
        auto drained = std::move(state_->pending);
        state_->pending.clear();
        return drained;
    }

private:
    std::shared_ptr<FakeBackendState> state_;
};

///
/// @brief Registers a fake backend for the duration of a test case.
///
struct WithFakeBackend
{
    WithFakeBackend() {
        ecf::service::aviso::register_backend([s = state]() { return std::make_unique<FakeBackend>(s); });
    }
    ~WithFakeBackend() { ecf::service::aviso::register_backend({}); }

    std::shared_ptr<FakeBackendState> state = std::make_shared<FakeBackendState>();
};

const std::string definition = R"(
    suite s1
      family f1
        edit CLASS 'od'
        edit ECF_AVISO_URL 'https://example.com/aviso'
        edit ECF_AVISO_AUTH '/path/to/auth'
        task t1
          aviso --name A --listener '{ "event": "mars", "request": { "class": "%CLASS%" } }'
      endfamily
    endsuite
)";

std::shared_ptr<Task> load_task(Defs& defs) {
    DefsStructureParser parser(&defs, definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_REQUIRE_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);

    return defs.suites()[0]->familyVec()[0]->taskVec()[0];
}

///
/// @brief Loads a suite with an Aviso task `t` and an unrelated task `other`, with the given Aviso variables.
///
defs_ptr load_suite_with_aviso(const std::string& aviso_variables, const std::string& aviso_options = "") {
    std::string definition = "suite s\n" + aviso_variables +
                             "  task other\n"
                             "  task t\n"
                             "    aviso --name A --listener '{ \"event\": \"mars\" }'" +
                             aviso_options +
                             "\n"
                             "endsuite\n";

    defs_ptr defs = Defs::create();
    DefsStructureParser parser(defs.get(), definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_REQUIRE_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);
    return defs;
}

const std::string aviso_variables = "  edit ECF_AVISO_URL 'http://aviso:8000'\n  edit ECF_AVISO_AUTH '/path/to/auth'\n";

ecf::service::aviso::AvisoNotification notification(std::uint64_t sequence) {
    return ecf::service::aviso::AvisoNotification{"mars", sequence, R"({"class": "od"})", "null"};
}

///
/// @brief Simulates the release of the node (the node leaves the queued state), and its later requeue.
///
void release_and_requeue(const ecf::AvisoAttr& aviso) {
    aviso.commit();
    aviso.start();
}

///
/// @brief Returns whether a job generation submits the given task.
///
bool submits(const defs_ptr& defs, const std::string& path) {
    Jobs jobs(defs.get());
    JobsParam jobsParam;
    jobs.generate(jobsParam);
    const auto& submitted = jobsParam.submitted();
    return std::any_of(
        submitted.begin(), submitted.end(), [&path](const auto& task) { return task->absNodePath() == path; });
}

node_ptr find(const defs_ptr& defs, const std::string& path) {
    auto node = defs->findAbsNode(path);
    BOOST_REQUIRE_MESSAGE(node, "Node not found: " << path);
    return node;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_AvisoAttr)

BOOST_AUTO_TEST_CASE(can_start_aviso_attribute_with_variable_substitution) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    Defs defs;
    auto task = load_task(defs);

    const auto& avisos = task->avisos();
    BOOST_REQUIRE_EQUAL(avisos.size(), static_cast<size_t>(1));

    const auto& aviso = avisos[0];
    BOOST_CHECK_EQUAL(aviso.name(), "A");
    BOOST_CHECK_EQUAL(aviso.listener(), R"('{ "event": "mars", "request": { "class": "%CLASS%" } }')");
    BOOST_CHECK_EQUAL(aviso.url(), "%ECF_AVISO_URL%");
    BOOST_CHECK_EQUAL(aviso.auth(), "%ECF_AVISO_AUTH%");
    BOOST_CHECK_EQUAL(aviso.active(), "");
    BOOST_CHECK_EQUAL(aviso.reason(), "''");

    aviso.start();

    // Ensure that the variable substitution is done correctly
    BOOST_CHECK_EQUAL(aviso.active(), R"({ "event": "mars", "request": { "class": "od" } })");
    BOOST_REQUIRE(backend.state->subscribed.has_value());
    BOOST_CHECK_EQUAL(backend.state->subscribed->path(), "/s1/f1/t1:A");
    BOOST_CHECK_EQUAL(backend.state->subscribed->listener(), R"({ "event": "mars", "request": { "class": "od" } })");
    BOOST_CHECK_EQUAL(backend.state->subscribed->url(), "https://example.com/aviso");
    BOOST_CHECK_EQUAL(backend.state->subscribed->revision(), 0u);
    BOOST_CHECK_EQUAL(backend.state->subscribed->auth(), "/path/to/auth");

    aviso.finish();

    // Ensure that, after finishing, the active listener is cleared
    BOOST_CHECK_EQUAL(aviso.active(), R"()");
}

BOOST_AUTO_TEST_CASE(is_free_once_a_notification_is_received) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::service::aviso;

    WithFakeBackend backend;

    Defs defs;
    auto task         = load_task(defs);
    const auto& aviso = task->avisos()[0];

    aviso.start();
    BOOST_CHECK(!aviso.isFree());

    backend.state->pending.emplace_back(AvisoNotification{"mars", 5, R"({"class": "od"})", "null"});
    backend.state->pending.emplace_back(AvisoNotification{"mars", 9, R"({"class": "od"})", "null"});

    BOOST_CHECK(aviso.isFree());
    BOOST_CHECK(!task->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_EQUAL(aviso.reason(), "''");

    // The revision advances when the node is released, to the oldest notification
    aviso.commit();
    BOOST_CHECK_EQUAL(aviso.revision(), 5u);

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(is_not_free_when_an_error_is_received) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::service::aviso;

    WithFakeBackend backend;

    Defs defs;
    auto task         = load_task(defs);
    const auto& aviso = task->avisos()[0];

    aviso.start();

    backend.state->pending.emplace_back(AvisoError{"connection refused"});

    BOOST_CHECK(!aviso.isFree());
    BOOST_CHECK(task->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_EQUAL(aviso.reason(), "'connection refused'");

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(reports_error_when_no_backend_is_available) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    Defs defs;
    auto task         = load_task(defs);
    const auto& aviso = task->avisos()[0];

    aviso.start();

    BOOST_CHECK(!aviso.isFree());
    BOOST_CHECK(task->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_EQUAL(aviso.reason(), "'" + std::string{ecf::service::aviso::no_backend} + "'");

    aviso.finish();
    BOOST_CHECK_EQUAL(aviso.active(), R"()");
}

BOOST_AUTO_TEST_CASE(begin_succeeds_when_aviso_url_cannot_be_resolved) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    // ECF_AVISO_URL is not defined
    auto defs = load_suite_with_aviso("  edit ECF_AVISO_AUTH '/path/to/auth'\n");

    BOOST_REQUIRE_NO_THROW(defs->beginAll());

    // Only the Aviso task is affected: it stays queued, and shows why
    auto t     = find(defs, "/s/t");
    auto other = find(defs, "/s/other");
    BOOST_CHECK_EQUAL(t->state(), NState::QUEUED);
    BOOST_CHECK(t->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_MESSAGE(t->avisos()[0].reason().find("ECF_AVISO_URL") != std::string::npos, t->avisos()[0].reason());
    BOOST_CHECK(!other->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK(!backend.state->subscribed.has_value());
}

BOOST_AUTO_TEST_CASE(begin_succeeds_when_aviso_credentials_cannot_be_resolved) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    // ECF_AVISO_AUTH is not defined
    auto defs = load_suite_with_aviso("  edit ECF_AVISO_URL 'http://aviso:8000'\n");

    BOOST_REQUIRE_NO_THROW(defs->beginAll());

    auto t = find(defs, "/s/t");
    BOOST_CHECK(t->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_MESSAGE(t->avisos()[0].reason().find("ECF_AVISO_AUTH") != std::string::npos, t->avisos()[0].reason());
}

BOOST_AUTO_TEST_CASE(begin_succeeds_when_aviso_url_is_empty) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso("  edit ECF_AVISO_URL ''\n  edit ECF_AVISO_AUTH '/path/to/auth'\n");

    BOOST_REQUIRE_NO_THROW(defs->beginAll());

    auto t = find(defs, "/s/t");
    BOOST_CHECK(t->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_MESSAGE(t->avisos()[0].reason().find("Aviso URL is empty") != std::string::npos,
                        t->avisos()[0].reason());
}

BOOST_AUTO_TEST_CASE(requeue_succeeds_when_aviso_url_cannot_be_resolved) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso("  edit ECF_AVISO_AUTH '/path/to/auth'\n");
    BOOST_REQUIRE_NO_THROW(defs->beginAll());

    BOOST_REQUIRE_NO_THROW(defs->requeue());

    auto t = find(defs, "/s/t");
    BOOST_CHECK_EQUAL(t->state(), NState::QUEUED);
    BOOST_CHECK(t->get_flag().is_set(Flag::REMOTE_ERROR));
}

BOOST_AUTO_TEST_CASE(reload_starts_aviso_once_configuration_is_fixed) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso("  edit ECF_AVISO_AUTH '/path/to/auth'\n");
    BOOST_REQUIRE_NO_THROW(defs->beginAll());

    auto t = find(defs, "/s/t");
    BOOST_REQUIRE(t->get_flag().is_set(Flag::REMOTE_ERROR));

    // Once the configuration is fixed, a reload (i.e. `alter change aviso A reload`) starts the attribute
    find(defs, "/s")->addVariable(Variable("ECF_AVISO_URL", "http://aviso:8000"));
    t->changeAviso("A", AvisoAttr::reload_option_value);

    BOOST_REQUIRE(backend.state->subscribed.has_value());
    BOOST_CHECK_EQUAL(backend.state->subscribed->url(), "http://aviso:8000");
    BOOST_CHECK(!t->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_EQUAL(t->avisos()[0].reason(), "''");

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(releases_once_per_notification) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs         = load_suite_with_aviso(aviso_variables);
    const auto& aviso = find(defs, "/s/t")->avisos()[0];
    aviso.start();

    backend.state->pending = {notification(1), notification(2), notification(3)};

    // Each release consumes exactly one notification, in order
    for (std::uint64_t expected = 1; expected <= 3; ++expected) {
        BOOST_CHECK(aviso.isFree());
        release_and_requeue(aviso);
        BOOST_CHECK_EQUAL(aviso.revision(), expected);
    }
    BOOST_CHECK(!aviso.isFree());

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(releases_once_for_all_notifications_when_collapsing) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs         = load_suite_with_aviso(aviso_variables, " --collapse");
    const auto& aviso = find(defs, "/s/t")->avisos()[0];
    BOOST_REQUIRE(aviso.collapse());
    aviso.start();

    backend.state->pending = {notification(1), notification(2), notification(3)};

    BOOST_CHECK(aviso.isFree());
    release_and_requeue(aviso);
    BOOST_CHECK_EQUAL(aviso.revision(), 3u);
    BOOST_CHECK(!aviso.isFree());

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(ignores_notifications_already_consumed) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs         = load_suite_with_aviso(aviso_variables);
    const auto& aviso = find(defs, "/s/t")->avisos()[0];
    aviso.start();

    backend.state->pending = {notification(1), notification(1)};
    BOOST_CHECK(aviso.isFree());
    release_and_requeue(aviso);

    // A notification redelivered after being consumed does not release the node again
    backend.state->pending = {notification(1)};
    BOOST_CHECK(!aviso.isFree());

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(keeps_notification_when_restarted_before_release) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs         = load_suite_with_aviso(aviso_variables);
    const auto& aviso = find(defs, "/s/t")->avisos()[0];
    aviso.start();

    backend.state->pending = {notification(1)};
    BOOST_CHECK(aviso.isFree());

    // The node is restarted (e.g. requeued while still queued) before being released: nothing is consumed
    aviso.start();
    BOOST_CHECK_EQUAL(aviso.revision(), 0u);
    BOOST_CHECK(aviso.isFree());

    release_and_requeue(aviso);
    BOOST_CHECK_EQUAL(aviso.revision(), 1u);

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(why_does_not_consume_the_notification) {
    ECF_NAME_THIS_TEST();

    // Querying why the node is held must not prevent the notification from releasing it

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");

    backend.state->pending = {notification(1)};

    std::vector<std::string> reasons;
    t->top_down_why(reasons);

    BOOST_CHECK_MESSAGE(submits(defs, "/s/t"), "Expected the notification to release the task, after a why()");
    BOOST_CHECK_EQUAL(t->avisos()[0].revision(), 1u);

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(notification_is_kept_while_another_dependency_holds_the_task) {
    ECF_NAME_THIS_TEST();

    // A notification received while a time dependency holds the task must release it once the time is reached

    using namespace ecf;

    WithFakeBackend backend;

    defs_ptr defs = Defs::create();
    auto suite    = defs->add_suite("s");
    suite->addClock(ClockAttr(
        boost::posix_time::ptime(boost::gregorian::date(2015, 6, 7), boost::posix_time::time_duration(8, 0, 0))));
    suite->addVariable(Variable("ECF_AVISO_URL", "http://aviso:8000"));
    suite->addVariable(Variable("ECF_AVISO_AUTH", "/path/to/auth"));
    auto t = suite->add_task("t");
    t->addTime(TimeAttr(TimeSlot(10, 0)));
    t->addAviso(AvisoAttr{t.get(), "A", R"({ "event": "mars" })", "%ECF_AVISO_URL%", 0, "%ECF_AVISO_AUTH%", ""});
    defs->beginAll();

    backend.state->pending = {notification(1)};

    // At 08:00 and 09:00, the time holds the task; at 10:00, the notification received earlier releases it
    CalendarUpdateParams one_hour(boost::posix_time::hours(1));
    BOOST_CHECK(!submits(defs, "/s/t"));
    defs->updateCalendar(one_hour);
    BOOST_CHECK(!submits(defs, "/s/t"));
    defs->updateCalendar(one_hour);
    BOOST_CHECK_MESSAGE(submits(defs, "/s/t"), "Expected the notification to release the task at 10:00");

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(change_keeps_attribute_attached_to_its_node) {
    ECF_NAME_THIS_TEST();

    // `alter change aviso` must keep the attribute attached to its node, and start the new configuration

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE(backend.state->subscribed.has_value());

    // i.e. `alter change aviso A "--listener '{ "event": "dissemination" }'" /s/t`
    t->changeAviso("A", R"(--listener '{ "event": "dissemination" }')");

    const auto& aviso = t->avisos()[0];
    BOOST_CHECK(aviso.parent() == t.get());
    BOOST_REQUIRE(backend.state->subscribed.has_value());
    BOOST_CHECK_EQUAL(backend.state->subscribed->listener(), R"({ "event": "dissemination" })");

    // Once the task has run (its attribute finished), the changed attribute is started again on requeue
    t->avisos()[0].finish();
    backend.state->subscribed.reset();
    BOOST_REQUIRE_NO_THROW(defs->requeue());
    BOOST_REQUIRE(backend.state->subscribed.has_value());
    BOOST_CHECK_EQUAL(backend.state->subscribed->listener(), R"({ "event": "dissemination" })");

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(invalid_change_keeps_previous_attribute_running) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE_EQUAL(backend.state->subscriptions, 1);

    // i.e. `alter change aviso A "--polling 60 --listener '{ "event": "dissemination" }'" /s/t`, with a v1 option
    BOOST_CHECK_THROW(t->changeAviso("A", R"(--polling 60 --listener '{ "event": "dissemination" }')"),
                      std::runtime_error);

    // The previous attribute keeps watching, and is released by the next notification
    const auto& aviso = t->avisos()[0];
    BOOST_CHECK_EQUAL(aviso.active(), R"({ "event": "mars" })");
    BOOST_CHECK_EQUAL(backend.state->subscriptions, 1);
    backend.state->pending.emplace_back(notification(1));
    BOOST_CHECK(aviso.isFree());

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(change_clears_error_of_replaced_attribute) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso("  edit ECF_AVISO_AUTH '/path/to/auth'\n");
    BOOST_REQUIRE_NO_THROW(defs->beginAll());
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE(t->get_flag().is_set(Flag::REMOTE_ERROR));

    // Once the configuration is fixed, the changed attribute is started, and the node is no longer in error
    find(defs, "/s")->addVariable(Variable("ECF_AVISO_URL", "http://aviso:8000"));
    t->changeAviso("A", R"(--listener '{ "event": "dissemination" }')");

    BOOST_CHECK(!t->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_REQUIRE(backend.state->subscribed.has_value());
    BOOST_CHECK_EQUAL(backend.state->subscribed->listener(), R"({ "event": "dissemination" })");

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(change_clears_error_of_replaced_attribute_when_node_is_not_queued) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso("  edit ECF_AVISO_AUTH '/path/to/auth'\n");
    BOOST_REQUIRE_NO_THROW(defs->beginAll());
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE(t->get_flag().is_set(Flag::REMOTE_ERROR));

    // The changed attribute is not started while the node is not waiting for it, but the node is no longer in error
    t->setStateOnly(NState::COMPLETE);
    t->changeAviso("A", R"(--listener '{ "event": "dissemination" }')");

    BOOST_CHECK(!t->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_EQUAL(backend.state->subscriptions, 0);
}

BOOST_AUTO_TEST_CASE(aborted_task_released_by_aviso_is_resubmitted_within_ecf_tries) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables + "  edit ECF_TRIES 2\n");
    defs->beginAll();
    auto t = find(defs, "/s/t");

    // The notification releases the task, which is submitted and then aborts
    backend.state->pending = {notification(1)};
    BOOST_REQUIRE_MESSAGE(submits(defs, "/s/t"), "Expected the notification to release the task");
    t->set_state(NState::SUBMITTED);
    t->set_state(NState::ABORTED);
    BOOST_REQUIRE_EQUAL(t->avisos()[0].revision(), 1u);

    // As a time attribute whose slot fired, the attribute stays free, so that the task is resubmitted (ECF_TRIES)
    const auto& aviso = t->avisos()[0];
    BOOST_CHECK(aviso.isSetFree());
    BOOST_CHECK(aviso.isFree());
    BOOST_CHECK_MESSAGE(submits(defs, "/s/t"), "Expected the aborted task to be resubmitted");

    // Once the task is queued again, the attribute waits for the next notification
    BOOST_REQUIRE_NO_THROW(defs->requeue());
    BOOST_CHECK(!aviso.isSetFree());
    BOOST_CHECK(!aviso.isFree());
    backend.state->pending = {notification(2)};
    BOOST_CHECK(aviso.isFree());

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(requeue_does_not_start_aviso_of_task_with_complete_default_status) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables, "\n    defstatus complete");
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE_EQUAL(t->state(), NState::COMPLETE);
    BOOST_REQUIRE_EQUAL(backend.state->subscriptions, 0);

    // The requeue leaves the task complete, so that there is no notification to wait for
    BOOST_REQUIRE_NO_THROW(defs->requeue());
    BOOST_CHECK_EQUAL(t->state(), NState::COMPLETE);
    BOOST_CHECK_EQUAL(backend.state->subscriptions, 0);
    BOOST_CHECK(t->avisos()[0].active().empty());
}

BOOST_AUTO_TEST_CASE(forcing_a_finished_task_queued_starts_the_aviso_again) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE_EQUAL(backend.state->subscriptions, 1);

    // The task completes (e.g. after a release), which finishes the attribute
    backend.state->pending.emplace_back(notification(1));
    BOOST_REQUIRE(t->avisos()[0].isFree());
    t->set_state(NState::COMPLETE);
    BOOST_CHECK_EQUAL(t->avisos()[0].revision(), 1u);

    // i.e. `ecflow_client --force=queued /s/t`: the attribute watches again, after the consumed notification
    t->set_state(NState::QUEUED, true);
    BOOST_CHECK_EQUAL(backend.state->subscriptions, 2);
    BOOST_REQUIRE(backend.state->subscribed.has_value());
    BOOST_CHECK_EQUAL(backend.state->subscribed->revision(), 1u);

    BOOST_CHECK(!t->avisos()[0].isFree());
    backend.state->pending.emplace_back(notification(2));
    BOOST_CHECK(t->avisos()[0].isFree());

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(server_bootstrap_starts_and_shutdown_finishes_queued_aviso) {
    ECF_NAME_THIS_TEST();

    // The server (re)start traversal must start the Aviso attribute of a queued node, e.g. after loading a
    // checkpoint, and the halt/shutdown traversal must finish it

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");

    // As after loading a checkpoint: the node is queued, but its attribute is not started
    t->avisos()[0].finish();
    backend.state->subscribed.reset();

    ecf::visit_all(*defs, BootstrapDefs{});
    BOOST_REQUIRE_MESSAGE(backend.state->subscribed.has_value(), "Expected the bootstrap to start the attribute");
    BOOST_CHECK_EQUAL(t->avisos()[0].active(), R"({ "event": "mars" })");

    ecf::visit_all(*defs, ShutdownDefs{});
    BOOST_CHECK_MESSAGE(t->avisos()[0].active().empty(), "Expected the shutdown to finish the attribute");
}

BOOST_AUTO_TEST_CASE(server_bootstrap_starts_and_shutdown_finishes_queued_aviso_of_alias) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE_EQUAL(backend.state->subscriptions, 1);

    // As after loading a checkpoint: the alias is queued, but its attribute is not started
    auto alias = t->isTask()->add_alias_only();
    alias->addAviso(AvisoAttr{
        alias.get(), "B", R"({ "event": "dissemination" })", AvisoAttr::default_url, 0, AvisoAttr::default_auth, ""});
    BOOST_REQUIRE_EQUAL(alias->state(), NState::QUEUED);
    BOOST_REQUIRE(alias->avisos()[0].active().empty());

    ecf::visit_all(*defs, BootstrapDefs{});
    BOOST_CHECK_EQUAL(backend.state->subscriptions, 2);
    BOOST_CHECK_EQUAL(alias->avisos()[0].active(), R"({ "event": "dissemination" })");

    ecf::visit_all(*defs, ShutdownDefs{});
    BOOST_CHECK(alias->avisos()[0].active().empty());
    BOOST_CHECK(t->avisos()[0].active().empty());
}

BOOST_AUTO_TEST_CASE(generated_variables_are_defined_before_any_release) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    auto defs = load_suite_with_aviso(aviso_variables);
    auto t    = find(defs, "/s/t");

    std::vector<Variable> vars;
    t->gen_variables(vars);

    for (const auto* name : {AvisoAttr::genvar_event_type,
                             AvisoAttr::genvar_event_sequence,
                             AvisoAttr::genvar_event_data_identifier,
                             AvisoAttr::genvar_event_data_payload}) {
        BOOST_CHECK_MESSAGE(std::any_of(vars.begin(), vars.end(), [name](const auto& v) { return v.name() == name; }),
                            "Expected generated variable " << name);
        BOOST_CHECK_MESSAGE(!t->findGenVariable(name).empty(), "Expected to find generated variable " << name);
    }
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_type).value(), "");
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "0");

    // A node without an Aviso attribute does not define them
    BOOST_CHECK(find(defs, "/s/other")->findGenVariable(AvisoAttr::genvar_event_type).empty());
}

BOOST_AUTO_TEST_CASE(generated_variables_describe_the_notification_that_released_the_node) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::service::aviso;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");

    backend.state->pending = {
        AvisoNotification{"mars", 7, R"({"class":"od","step":"6"})", R"({"location":"file:///x"})"}};

    BOOST_REQUIRE_MESSAGE(submits(defs, "/s/t"), "Expected the notification to release the task");

    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_type).value(), "mars");
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "7");
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_data_identifier).value(),
                      R"({"class":"od","step":"6"})");
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_data_payload).value(), R"({"location":"file:///x"})");

    // The variables are available to the job, as any other variable
    std::string line = "%ECF_AVISO_EVENT_TYPE%@%ECF_AVISO_EVENT_SEQUENCE% %ECF_AVISO_EVENT_DATA_PAYLOAD%";
    BOOST_REQUIRE(t->variableSubstitution(line));
    BOOST_CHECK_EQUAL(line, R"(mars@7 {"location":"file:///x"})");

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(generated_variables_describe_the_notification_while_the_job_is_generated) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace ecf::service::aviso;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t            = find(defs, "/s/t");
    const auto& aviso = t->avisos()[0];

    backend.state->pending = {AvisoNotification{"mars", 7, R"({"class":"od"})", R"({"location":"file:///x"})"}};

    // The job is generated while the node is still queued, i.e. once the notification releases the node, but
    // before the node leaves the queued state (when the notification is committed)
    BOOST_REQUIRE(aviso.isFree());

    std::string line = "%ECF_AVISO_EVENT_TYPE%@%ECF_AVISO_EVENT_SEQUENCE% %ECF_AVISO_EVENT_DATA_PAYLOAD%";
    BOOST_REQUIRE(t->variableSubstitution(line));
    BOOST_CHECK_EQUAL(line, R"(mars@7 {"location":"file:///x"})");

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(generated_variables_follow_each_release) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs         = load_suite_with_aviso(aviso_variables);
    auto t            = find(defs, "/s/t");
    const auto& aviso = t->avisos()[0];
    aviso.start();

    backend.state->pending = {notification(1), notification(2)};

    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "0");

    // Held to release the node: the variables describe the notification, before and after it is consumed
    BOOST_CHECK(aviso.isFree());
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "1");
    release_and_requeue(aviso);
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "1");

    BOOST_CHECK(aviso.isFree());
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "2");
    release_and_requeue(aviso);
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "2");

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(generated_variables_describe_the_latest_notification_when_collapsing) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs         = load_suite_with_aviso(aviso_variables, " --collapse");
    auto t            = find(defs, "/s/t");
    const auto& aviso = t->avisos()[0];
    aviso.start();

    backend.state->pending = {notification(1), notification(2), notification(3)};

    BOOST_CHECK(aviso.isFree());
    release_and_requeue(aviso);
    BOOST_CHECK_EQUAL(t->findGenVariable(AvisoAttr::genvar_event_sequence).value(), "3");

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(aviso_is_only_allowed_on_tasks_and_aliases) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    Defs defs;
    auto s = defs.add_suite("s");
    auto f = s->add_family("f");
    auto t = f->add_task("t");
    auto a = t->add_alias_only();

    const AvisoAttr aviso{
        nullptr, "A", R"({ "event": "mars" })", AvisoAttr::default_url, 0, AvisoAttr::default_auth, ""};

    BOOST_CHECK_NO_THROW(t->addAviso(aviso));
    BOOST_CHECK_NO_THROW(a->addAviso(aviso));

    // Families and suites have no job, so a notification would never be consumed
    BOOST_CHECK_THROW(f->addAviso(aviso), std::runtime_error);
    BOOST_CHECK_THROW(s->addAviso(aviso), std::runtime_error);
    BOOST_CHECK(f->avisos().empty());
    BOOST_CHECK(s->avisos().empty());
}

BOOST_AUTO_TEST_CASE(copy_of_a_task_keeps_a_detached_aviso) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE_EQUAL(backend.state->subscriptions, 1);

    // The copy holds the attribute, attached to the copy, without the connection of the original (e.g. an archive)
    Task copy{*t->isTask()};
    BOOST_REQUIRE_EQUAL(copy.avisos().size(), 1u);
    BOOST_CHECK_EQUAL(copy.avisos()[0].listener(), t->avisos()[0].listener());
    BOOST_CHECK(copy.avisos()[0].parent() == &copy);
    backend.state->pending.emplace_back(notification(1));
    BOOST_CHECK(!copy.avisos()[0].isFree());
    BOOST_CHECK(t->avisos()[0].isFree());

    t->avisos()[0].finish();
}

BOOST_AUTO_TEST_CASE(aviso_is_not_allowed_on_a_node_with_a_mirror) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    Defs defs;
    auto s  = defs.add_suite("s");
    auto t1 = s->add_task("t1");
    auto t2 = s->add_task("t2");

    const AvisoAttr aviso{
        nullptr, "A", R"({ "event": "mars" })", AvisoAttr::default_url, 0, AvisoAttr::default_auth, ""};
    const MirrorAttr mirror{nullptr,
                            "M",
                            "/s/t",
                            MirrorAttr::default_remote_host,
                            MirrorAttr::default_remote_port,
                            MirrorAttr::default_polling,
                            false,
                            MirrorAttr::default_remote_auth,
                            "",
                            false};

    // A mirrored node takes its state from the remote node, which a release by a notification would contradict
    t1->addMirror(mirror);
    BOOST_CHECK_THROW(t1->addAviso(aviso), std::runtime_error);
    BOOST_CHECK(t1->avisos().empty());

    t2->addAviso(aviso);
    BOOST_CHECK_THROW(t2->addMirror(mirror), std::runtime_error);
    BOOST_CHECK(t2->mirrors().empty());
}

BOOST_AUTO_TEST_CASE(starting_a_running_attribute_keeps_its_configuration) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    auto defs = load_suite_with_aviso(aviso_variables);
    defs->beginAll();
    auto t            = find(defs, "/s/t");
    const auto& aviso = t->avisos()[0];
    BOOST_REQUIRE_EQUAL(backend.state->subscriptions, 1);
    const auto active = aviso.active();

    // The configuration changes while the attribute runs; starting it again (e.g. requeue of the queued node)
    // keeps the running watch and its configuration, without reporting an error on the unused configuration
    defs->findAbsNode("/s")->addVariable(Variable("ECF_AVISO_URL", ""));
    aviso.start();

    BOOST_CHECK_EQUAL(backend.state->subscriptions, 1);
    BOOST_CHECK_EQUAL(aviso.active(), active);
    BOOST_CHECK(!aviso.has_error());
    BOOST_CHECK(!t->get_flag().is_set(Flag::REMOTE_ERROR));

    // A reload applies the new configuration
    t->changeAviso("A", AvisoAttr::reload_option_value);
    BOOST_CHECK(aviso.has_error());
    BOOST_CHECK(t->get_flag().is_set(Flag::REMOTE_ERROR));

    aviso.finish();
}

BOOST_AUTO_TEST_CASE(deleting_an_attribute_in_error_clears_the_error_flag) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;

    WithFakeBackend backend;

    // ECF_AVISO_URL is not defined
    auto defs = load_suite_with_aviso("  edit ECF_AVISO_AUTH '/path/to/auth'\n");
    defs->beginAll();
    auto t = find(defs, "/s/t");
    BOOST_REQUIRE(t->get_flag().is_set(Flag::REMOTE_ERROR));

    // The deleted attribute no longer holds the node in error
    t->deleteAviso("A");
    BOOST_CHECK(t->avisos().empty());
    BOOST_CHECK(!t->get_flag().is_set(Flag::REMOTE_ERROR));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
