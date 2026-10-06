// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/CalendarUpdateParams.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Jobs.hpp"
#include "ecflow/node/JobsParam.hpp"
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
    std::vector<ecf::service::aviso::AvisoResponse> pending;
};

class FakeBackend : public ecf::service::aviso::AvisoBackend {
public:
    explicit FakeBackend(std::shared_ptr<FakeBackendState> state)
        : state_{std::move(state)} {}

    void subscribe(const ecf::service::aviso::AvisoSubscribe& request) override { state_->subscribed = request; }

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

    // LD1: querying why the node is held must not prevent the notification from releasing it

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

    // LD2: a notification received while a time dependency holds the task must release it once the time is reached

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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
