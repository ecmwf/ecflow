// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
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
defs_ptr load_suite_with_aviso(const std::string& aviso_variables) {
    std::string definition = "suite s\n" + aviso_variables +
                             "  task other\n"
                             "  task t\n"
                             "    aviso --name A --listener '{ \"event\": \"mars\" }'\n"
                             "endsuite\n";

    defs_ptr defs = Defs::create();
    DefsStructureParser parser(defs.get(), definition, true);

    std::string errorMsg, warningMsg;
    bool parsedOK = parser.doParse(errorMsg, warningMsg);
    BOOST_REQUIRE_MESSAGE(parsedOK, "Failed to parse definition: " << errorMsg);
    return defs;
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
    BOOST_CHECK_EQUAL(aviso.revision(), 9u);
    BOOST_CHECK(!task->get_flag().is_set(Flag::REMOTE_ERROR));
    BOOST_CHECK_EQUAL(aviso.reason(), "''");

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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
