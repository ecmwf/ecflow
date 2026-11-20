// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/client/ShadowDefs.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std::chrono_literals;

namespace {

///
/// @brief Stands in for an ecFlow server, recording how ShadowDefs uses it.
///
/// Each update returns a new definition, so that a test can tell which copy ShadowDefs holds; with in_place, updates
/// and later fetches change the definition already returned instead, as ClientInvoker does. Updates can be made to
/// fail, or held until the test releases them.
///
class FakeSource : public ecf::ShadowDefsSource {
public:
    std::shared_ptr<Defs> fetch() override {
        ++fetches;
        if (fail_fetch) {
            throw std::runtime_error("no server");
        }
        std::scoped_lock lock(mutex);
        if (in_place && fetched) {
            // Refreshes the definition it returned before, as ClientInvoker::sync_local does
            fetched->addSuite(Suite::create("f" + std::to_string(fetches.load())));
            return fetched;
        }
        fetched = std::make_shared<Defs>();
        return fetched;
    }

    bool has_news(const std::shared_ptr<Defs>& /* defs */) override {
        ++news_questions;
        if (fail_news) {
            throw std::runtime_error("lost contact");
        }
        return news;
    }

    std::shared_ptr<Defs> update(const std::shared_ptr<Defs>& defs) override {
        {
            std::unique_lock lock(mutex);
            ++started;
            changed.notify_all();
            changed.wait(lock, [this] { return !hold; });
        }
        if (fail_update) {
            ++failed_updates;
            throw std::runtime_error("update refused");
        }
        ++updates;
        if (in_place && defs) {
            // Changes the copy itself, as ClientInvoker::sync does
            defs->addSuite(Suite::create("s" + std::to_string(updates.load())));
            std::scoped_lock lock(mutex);
            produced.push_back(defs);
            return defs;
        }
        auto updated = std::make_shared<Defs>();
        std::scoped_lock lock(mutex);
        produced.push_back(updated);
        return updated;
    }

    std::shared_ptr<Defs> last_produced() {
        std::scoped_lock lock(mutex);
        return produced.empty() ? nullptr : produced.back();
    }

    /// @brief Holds every update that starts from now on, until release is called.
    void hold_updates() {
        std::scoped_lock lock(mutex);
        hold = true;
    }

    /// @brief Lets held updates proceed.
    void release() {
        {
            std::scoped_lock lock(mutex);
            hold = false;
        }
        changed.notify_all();
    }

    /// @brief Waits until the given number of updates have started, or a few seconds have passed.
    bool wait_started(int count) {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, 5s, [this, count] { return started >= count; });
    }

    std::atomic<int> fetches{0};
    std::atomic<int> news_questions{0};
    std::atomic<int> updates{0};
    std::atomic<int> failed_updates{0};
    std::atomic<bool> news{false};
    std::atomic<bool> fail_fetch{false};
    std::atomic<bool> fail_news{false};
    std::atomic<bool> fail_update{false};
    std::atomic<bool> in_place{false};

private:
    std::mutex mutex;
    std::condition_variable changed;
    bool hold{false};
    int started{0};
    std::vector<std::shared_ptr<Defs>> produced;
    std::shared_ptr<Defs> fetched;
};

/// @brief Waits until the condition holds, or a few seconds have passed; returns the condition.
bool eventually(const std::function<bool()>& condition) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!condition() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(2ms);
    }
    return condition();
}

///
/// @brief Ends a threaded test cleanly, whatever its assertions do.
///
/// On destruction it lets held updates proceed, stops the polling thread, which releases waiting refreshes, and joins
/// the callers, so that a failed assertion never leaves a thread running or waiting.
///
class Cleanup {
public:
    Cleanup(FakeSource* source, ecf::ShadowDefs* shadow)
        : source_(source),
          shadow_(shadow) {}
    Cleanup(const Cleanup&)            = delete;
    Cleanup& operator=(const Cleanup&) = delete;
    ~Cleanup() {
        source_->fail_update = false;
        source_->release();
        shadow_->stop();
        for (auto& caller : callers) {
            if (caller.joinable()) {
                caller.join();
            }
        }
    }

    std::vector<std::thread> callers;

private:
    FakeSource* source_;
    ecf::ShadowDefs* shadow_;
};

/// @brief Builds a ShadowDefs over a fake source, keeping a pointer to the source for inspection.
std::pair<std::unique_ptr<ecf::ShadowDefs>, FakeSource*> make_shadow(std::chrono::milliseconds interval) {
    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    auto shadow = std::make_unique<ecf::ShadowDefs>(std::move(source), [interval] { return interval; });
    return {std::move(shadow), raw};
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ShadowDefs)

BOOST_AUTO_TEST_CASE(test_shadow_defs_is_empty_before_start) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    BOOST_CHECK(shadow->get() == nullptr);
    BOOST_CHECK_EQUAL(source->fetches.load(), 0);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_start_fetches_the_whole_definition) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    shadow->start();
    BOOST_CHECK(shadow->get() != nullptr);
    BOOST_CHECK_EQUAL(source->fetches.load(), 1);
    shadow->stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_start_reports_an_unreachable_server) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    source->fail_fetch    = true;
    BOOST_CHECK_THROW(shadow->start(), std::runtime_error);
    BOOST_CHECK(shadow->get() == nullptr);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_poll_without_news_keeps_the_copy) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    shadow->start();
    shadow->stop();
    auto before = shadow->get();

    source->news = false;
    shadow->poll_once();

    BOOST_CHECK_EQUAL(source->news_questions.load(), 1);
    BOOST_CHECK_EQUAL(source->updates.load(), 0);
    BOOST_CHECK(shadow->get() == before);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_poll_with_news_updates_the_copy_and_reports_it) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    std::vector<const Defs*> reported;
    ecf::ShadowDefs shadow(
        std::move(source), [] { return 1h; }, {}, 5s, [&reported](const Defs& defs) { reported.push_back(&defs); });
    shadow.start();
    shadow.stop();

    raw->news = true;
    shadow.poll_once();

    BOOST_CHECK_EQUAL(raw->updates.load(), 1);
    BOOST_CHECK(shadow.get() == raw->last_produced());
    BOOST_REQUIRE_EQUAL(reported.size(), 1u);
    BOOST_CHECK(reported.front() == shadow.get().get());
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_poll_propagates_a_lost_contact) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    shadow->start();
    shadow->stop();
    auto before = shadow->get();

    source->fail_news = true;
    BOOST_CHECK_THROW(shadow->poll_once(), std::runtime_error);
    BOOST_CHECK(shadow->get() == before);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_refresh_updates_and_then_runs_the_continuation) {
    ECF_NAME_THIS_TEST();

    auto [shadow_owner, source] = make_shadow(1h);
    auto* shadow                = shadow_owner.get();
    shadow->start();

    std::shared_ptr<Defs> seen;
    shadow->refresh([&seen, shadow] { seen = shadow->get(); });

    BOOST_CHECK_EQUAL(source->updates.load(), 1);
    BOOST_CHECK(seen != nullptr);
    BOOST_CHECK(seen == source->last_produced());
    shadow->stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_refresh_without_polling_thread_updates_in_the_caller) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    bool ran              = false;
    shadow->refresh([&ran] { ran = true; });

    BOOST_CHECK(ran);
    BOOST_CHECK_EQUAL(source->updates.load(), 1);
    BOOST_CHECK(shadow->get() == source->last_produced());
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_refresh_during_an_update_waits_for_a_later_update) {
    ECF_NAME_THIS_TEST();

    auto [shadow_owner, source] = make_shadow(1h);
    auto* shadow                = shadow_owner.get();
    shadow->start();

    std::atomic<bool> second_done{false};
    std::shared_ptr<Defs> second_seen;
    {
        Cleanup cleanup(source, shadow);

        // The first refresh starts an update, which is held
        source->hold_updates();
        cleanup.callers.emplace_back([shadow] { shadow->refresh(); });
        BOOST_CHECK(source->wait_started(1));

        // A second refresh is registered while that update is still running
        cleanup.callers.emplace_back([&second_done, &second_seen, shadow] {
            shadow->refresh([&second_seen, shadow] { second_seen = shadow->get(); });
            second_done = true;
        });
        BOOST_CHECK(eventually([shadow] { return shadow->requests_made() == 2; }));
        BOOST_CHECK(!second_done.load());

        // The second refresh is served only by an update that started after it was made
        source->release();
        BOOST_CHECK(eventually([&second_done] { return second_done.load(); }));
    }
    BOOST_CHECK_EQUAL(source->updates.load(), 2);
    BOOST_CHECK(second_seen == source->last_produced());
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_refresh_keeps_waiting_until_a_failed_update_succeeds) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    ecf::ShadowDefs shadow(std::move(source), [] { return 1h; }, {}, 5ms);
    shadow.start();

    std::atomic<bool> done{false};
    {
        Cleanup cleanup(raw, &shadow);
        raw->fail_update = true;
        cleanup.callers.emplace_back([&done, &shadow] {
            shadow.refresh();
            done = true;
        });

        BOOST_CHECK(eventually([raw] { return raw->failed_updates.load() >= 2; }));
        BOOST_CHECK(!done.load());

        raw->fail_update = false;
        BOOST_CHECK(eventually([&done] { return done.load(); }));
    }
    BOOST_CHECK_EQUAL(raw->updates.load(), 1);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_stop_releases_a_waiting_refresh) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    ecf::ShadowDefs shadow(std::move(source), [] { return 1h; }, {}, 1h);
    shadow.start();

    std::atomic<bool> done{false};
    Cleanup cleanup(raw, &shadow);
    raw->fail_update = true;
    cleanup.callers.emplace_back([&done, &shadow] {
        shadow.refresh();
        done = true;
    });
    BOOST_CHECK(eventually([raw] { return raw->failed_updates.load() >= 1; }));

    // The polling thread now waits for an hour before retrying; stop must end that wait and the caller's
    const auto begin = std::chrono::steady_clock::now();
    shadow.stop();
    BOOST_CHECK(eventually([&done] { return done.load(); }));
    BOOST_CHECK(std::chrono::steady_clock::now() - begin < 5s);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_polls_on_its_own_at_the_interval_given_by_the_policy) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    std::atomic<int> asked{0};
    ecf::ShadowDefs shadow(std::move(source), [&asked] {
        ++asked;
        return 10ms;
    });
    raw->news = true;
    shadow.start();

    BOOST_CHECK(eventually([raw] { return raw->updates.load() >= 3; }));
    shadow.stop();
    BOOST_CHECK_GE(asked.load(), 3);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_keeps_polling_after_a_lost_contact) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    std::atomic<int> reports{0};
    ecf::ShadowDefs shadow(std::move(source), [] { return 10ms; }, [&reports](const std::string&) { ++reports; }, 10ms);

    raw->fail_news = true;
    shadow.start();
    BOOST_CHECK(eventually([&reports] { return reports.load() >= 2; }));

    raw->fail_news = false;
    raw->news      = true;
    BOOST_CHECK(eventually([raw] { return raw->updates.load() >= 1; }));
    shadow.stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_stop_is_prompt_and_idempotent) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    shadow->start();

    const auto begin = std::chrono::steady_clock::now();
    shadow->stop();
    shadow->stop();
    BOOST_CHECK(std::chrono::steady_clock::now() - begin < 5s);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_start_while_running_is_refused) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    shadow->start();
    BOOST_CHECK_THROW(shadow->start(), std::logic_error);
    BOOST_CHECK_THROW(shadow->start_in_background(), std::logic_error);
    BOOST_CHECK_EQUAL(source->fetches.load(), 1);

    // Once stopped, it can be started again
    shadow->stop();
    shadow->start();
    BOOST_CHECK_EQUAL(source->fetches.load(), 2);
    shadow->stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_is_never_synchronised_before_the_first_contact) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::NeverSynchronised);
    BOOST_CHECK(!shadow->age().has_value());
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_becomes_stale_when_the_last_contact_is_too_old) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    auto now              = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    shadow->set_clock([now] { return *now; });
    shadow->set_max_age(30s);

    shadow->start();
    shadow->stop();
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Current);
    BOOST_CHECK(shadow->age() == std::chrono::milliseconds(0));

    *now += 20s;
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Current);
    BOOST_CHECK(shadow->age() == std::chrono::milliseconds(20s));

    *now += 11s;
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Stale);

    // A failed contact does not refresh the age
    source->fail_news = true;
    BOOST_CHECK_THROW(shadow->poll_once(), std::runtime_error);
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Stale);

    // A successful question for news does, even without news
    source->fail_news = false;
    source->news      = false;
    shadow->poll_once();
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Current);
    BOOST_CHECK(shadow->age() == std::chrono::milliseconds(0));
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_without_maximum_age_is_never_stale) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    auto now              = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    shadow->set_clock([now] { return *now; });

    shadow->start();
    shadow->stop();
    *now += 1000h;
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Current);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_started_in_background_waits_for_the_server) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    std::atomic<int> reports{0};
    ecf::ShadowDefs shadow(std::move(source), [] { return 10ms; }, [&reports](const std::string&) { ++reports; }, 10ms);

    raw->fail_fetch = true;
    shadow.start_in_background();
    BOOST_CHECK(eventually([&reports] { return reports.load() >= 2; }));
    BOOST_CHECK(shadow.state() == ecf::ShadowDefsState::NeverSynchronised);
    BOOST_CHECK(shadow.get() == nullptr);

    raw->fail_fetch = false;
    BOOST_CHECK(eventually([&shadow] { return shadow.state() == ecf::ShadowDefsState::Current; }));
    BOOST_CHECK(shadow.get() != nullptr);
    shadow.stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_read_view_shows_the_copy) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    BOOST_CHECK(!shadow->read());

    shadow->start();
    const auto copy = shadow->get();
    {
        auto view = shadow->read();
        BOOST_REQUIRE(view);
        BOOST_CHECK(view.get() == copy);
        BOOST_CHECK(view->suites().empty());
    }
    shadow->stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_read_views_proceed_in_parallel) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    shadow->start();
    const auto copy = shadow->get();

    auto view = shadow->read();
    std::shared_ptr<const Defs> seen;
    std::thread other([&seen, shadow = shadow.get()] {
        auto second = shadow->read();
        seen        = second.get();
    });
    other.join();
    BOOST_CHECK(seen == copy);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_read_view_holds_back_an_update_and_new_readers) {
    ECF_NAME_THIS_TEST();

    auto [shadow_owner, source] = make_shadow(1h);
    auto* shadow                = shadow_owner.get();
    source->in_place            = true;
    shadow->start();

    std::atomic<bool> refreshed{false};
    std::atomic<bool> reader_entered{false};
    std::atomic<bool> reader_done{false};
    std::size_t suites_seen_by_reader = 0;
    {
        Cleanup cleanup(source, shadow);
        auto view = std::make_unique<ecf::ShadowDefs::ReadView>(shadow->read());

        // An update is requested, and waits for the view
        cleanup.callers.emplace_back([&refreshed, shadow] {
            shadow->refresh();
            refreshed = true;
        });
        BOOST_CHECK(eventually([shadow] { return shadow->update_waiting(); }));
        BOOST_CHECK_EQUAL(source->updates.load(), 0);
        BOOST_CHECK((*view)->suites().empty());

        // A new reader arriving while the update waits is held back, so it cannot delay the update
        cleanup.callers.emplace_back([&reader_entered, &reader_done, &suites_seen_by_reader, shadow] {
            reader_entered        = true;
            auto second           = shadow->read();
            suites_seen_by_reader = second->suites().size();
            reader_done           = true;
        });
        BOOST_CHECK(eventually([&reader_entered] { return reader_entered.load(); }));
        // A negative check: the reader must not get through within a generous delay while the update waits
        std::this_thread::sleep_for(100ms);
        BOOST_CHECK(!reader_done.load());

        // Releasing the view lets the update run, then the new reader, which sees the updated copy
        view.reset();
        BOOST_CHECK(eventually([&refreshed] { return refreshed.load(); }));
        BOOST_CHECK(eventually([&reader_done] { return reader_done.load(); }));
    }
    BOOST_CHECK_EQUAL(source->updates.load(), 1);
    BOOST_CHECK_EQUAL(suites_seen_by_reader, 1u);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_restart_waits_for_read_views_before_fetching) {
    ECF_NAME_THIS_TEST();

    auto [shadow_owner, source] = make_shadow(1h);
    auto* shadow                = shadow_owner.get();
    source->in_place            = true;
    shadow->start();
    shadow->stop();

    std::atomic<bool> restarted{false};
    {
        Cleanup cleanup(source, shadow);
        auto view = std::make_unique<ecf::ShadowDefs::ReadView>(shadow->read());

        // The second fetch would change the published copy in place, so it waits for the view
        cleanup.callers.emplace_back([&restarted, shadow] {
            shadow->start();
            restarted = true;
        });
        auto* raw = source; // a structured binding cannot be captured in C++17
        BOOST_CHECK(eventually([shadow, raw] { return shadow->update_waiting() || raw->fetches.load() > 1; }));
        BOOST_CHECK(shadow->update_waiting());
        BOOST_CHECK_EQUAL(source->fetches.load(), 1);
        BOOST_CHECK((*view)->suites().empty());

        view.reset();
        BOOST_CHECK(eventually([&restarted] { return restarted.load(); }));
    }
    BOOST_CHECK_EQUAL(source->fetches.load(), 2);
    BOOST_CHECK_EQUAL(shadow->get()->suites().size(), 1u);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_contact_time_is_when_the_fetch_returned) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    ecf::ShadowDefs shadow(std::move(source), [] { return 1h; });
    auto now         = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    auto clock_mutex = std::make_shared<std::mutex>();
    shadow.set_clock([now, clock_mutex] {
        std::scoped_lock lock(*clock_mutex);
        return *now;
    });

    {
        // A view held before the start delays the publication of the first fetch
        auto view = std::make_unique<ecf::ShadowDefs::ReadView>(shadow.read());
        shadow.start_in_background();
        BOOST_CHECK(eventually([raw, &shadow] { return raw->fetches.load() == 1 && shadow.update_waiting(); }));
        {
            std::scoped_lock lock(*clock_mutex);
            *now += 10s;
        }
        view.reset();
    }
    BOOST_CHECK(eventually([&shadow] { return shadow.state() == ecf::ShadowDefsState::Current; }));
    BOOST_CHECK(shadow.age() == std::chrono::milliseconds(10s));
    shadow.stop();
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_update_refreshes_the_age_only_when_it_succeeds) {
    ECF_NAME_THIS_TEST();

    auto [shadow, source] = make_shadow(1h);
    auto now              = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    shadow->set_clock([now] { return *now; });
    shadow->set_max_age(30s);
    shadow->start();
    shadow->stop();

    *now += 31s;
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Stale);

    // The question for news succeeds, the update fails: the age is that of the question
    source->news        = true;
    source->fail_update = true;
    BOOST_CHECK_THROW(shadow->poll_once(), std::runtime_error);
    BOOST_CHECK(shadow->age() == std::chrono::milliseconds(0));

    *now += 31s;
    BOOST_CHECK_THROW(shadow->refresh(), std::runtime_error);
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Stale);

    // A refresh that succeeds refreshes the age
    source->fail_update = false;
    shadow->refresh();
    BOOST_CHECK(shadow->state() == ecf::ShadowDefsState::Current);
    BOOST_CHECK(shadow->age() == std::chrono::milliseconds(0));
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_stop_is_prompt_while_the_background_start_fails) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    ecf::ShadowDefs shadow(std::move(source), [] { return 1h; }, {}, 1h);

    raw->fail_fetch = true;
    shadow.start_in_background();
    BOOST_CHECK(eventually([raw] { return raw->fetches.load() >= 1; }));

    const auto begin = std::chrono::steady_clock::now();
    shadow.stop();
    BOOST_CHECK(std::chrono::steady_clock::now() - begin < 5s);
    BOOST_CHECK(shadow.state() == ecf::ShadowDefsState::NeverSynchronised);
}

BOOST_AUTO_TEST_CASE(test_shadow_defs_refresh_before_the_first_fetch_waits_for_the_server) {
    ECF_NAME_THIS_TEST();

    auto source = std::make_unique<FakeSource>();
    auto* raw   = source.get();
    ecf::ShadowDefs shadow(std::move(source), [] { return 1h; }, {}, 10ms);

    raw->fail_fetch = true;
    shadow.start_in_background();
    std::atomic<bool> done{false};
    {
        Cleanup cleanup(raw, &shadow);
        cleanup.callers.emplace_back([&done, &shadow] {
            shadow.refresh();
            done = true;
        });
        BOOST_CHECK(eventually([raw] { return raw->fetches.load() >= 2; }));
        BOOST_CHECK(!done.load());

        raw->fail_fetch = false;
        BOOST_CHECK(eventually([&done] { return done.load(); }));
    }
    BOOST_CHECK(shadow.get() != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
