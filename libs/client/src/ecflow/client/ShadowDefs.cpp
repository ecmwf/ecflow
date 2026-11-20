// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/client/ShadowDefs.hpp"

#include <exception>
#include <stdexcept>
#include <utility>

#include "ecflow/node/Defs.hpp"

namespace ecf {

std::shared_ptr<Defs> ClientInvokerShadowDefsSource::fetch() {
    client_.news_local();
    client_.sync_local();
    return client_.defs();
}

bool ClientInvokerShadowDefsSource::has_news(const std::shared_ptr<Defs>& defs) {
    auto copy = defs;
    client_.news(copy);
    return client_.get_news();
}

std::shared_ptr<Defs> ClientInvokerShadowDefsSource::update(const std::shared_ptr<Defs>& defs) {
    if (defs != nullptr) {
        auto copy = defs;
        client_.sync(copy);
    }
    return client_.defs();
}

ShadowDefs::ShadowDefs(std::unique_ptr<ShadowDefsSource> source,
                       IntervalPolicy next_interval,
                       Report report,
                       std::chrono::milliseconds retry_delay,
                       Updated updated)
    : source_(std::move(source)),
      next_interval_(std::move(next_interval)),
      report_(std::move(report)),
      retry_delay_(retry_delay),
      updated_(std::move(updated)),
      now_([] { return std::chrono::steady_clock::now(); }) {
}

ShadowDefs::ReadView::ReadView(std::shared_lock<std::shared_mutex> lock, std::shared_ptr<const Defs> defs)
    : lock_(std::move(lock)),
      defs_(std::move(defs)) {
}

ShadowDefs::~ShadowDefs() {
    stop();
}

void ShadowDefs::start() {
    if (thread_) {
        throw std::logic_error("ShadowDefs::start: the polling thread already runs");
    }
    fetch_copy();
    start_in_background();
}

void ShadowDefs::start_in_background() {
    if (thread_) {
        throw std::logic_error("ShadowDefs::start_in_background: the polling thread already runs");
    }
    {
        std::scoped_lock lock(wakeup_mutex_);
        stopping_ = false;
    }
    thread_  = std::make_unique<std::thread>([this] { loop(); });
    running_ = true;
}

void ShadowDefs::stop() {
    if (!thread_) {
        return;
    }
    {
        std::scoped_lock lock(wakeup_mutex_);
        stopping_ = true;
    }
    wakeup_.notify_all();
    thread_->join();
    thread_.reset();
    running_ = false;
}

std::shared_lock<std::shared_mutex> ShadowDefs::admit_reader() const {
    std::unique_lock admission(admission_mutex_);
    admission_.wait(admission, [this] { return writers_waiting_ == 0; });
    // Taken while admission is held, so that a writer arriving now waits for this reader rather than overtaking it
    return std::shared_lock(defs_mutex_);
}

std::unique_lock<std::shared_mutex> ShadowDefs::admit_writer() {
    {
        std::scoped_lock admission(admission_mutex_);
        ++writers_waiting_;
    }
    std::unique_lock lock(defs_mutex_);
    {
        std::scoped_lock admission(admission_mutex_);
        --writers_waiting_;
    }
    admission_.notify_all();
    return lock;
}

std::shared_ptr<Defs> ShadowDefs::get() const {
    auto lock = admit_reader();
    return defs_;
}

ShadowDefs::ReadView ShadowDefs::read() const {
    auto lock                        = admit_reader();
    std::shared_ptr<const Defs> defs = defs_;
    return ReadView(std::move(lock), std::move(defs));
}

bool ShadowDefs::update_waiting() const {
    std::scoped_lock admission(admission_mutex_);
    return writers_waiting_ > 0;
}

void ShadowDefs::set_max_age(std::chrono::milliseconds max_age) {
    std::scoped_lock lock(contact_mutex_);
    max_age_ = max_age;
}

void ShadowDefs::set_clock(Clock now) {
    std::scoped_lock lock(contact_mutex_);
    now_ = std::move(now);
}

ShadowDefsState ShadowDefs::state() const {
    std::scoped_lock lock(contact_mutex_);
    if (!last_contact_) {
        return ShadowDefsState::NeverSynchronised;
    }
    if (max_age_.count() > 0 && now_() - *last_contact_ > max_age_) {
        return ShadowDefsState::Stale;
    }
    return ShadowDefsState::Current;
}

std::optional<std::chrono::milliseconds> ShadowDefs::age() const {
    std::scoped_lock lock(contact_mutex_);
    if (!last_contact_) {
        return std::nullopt;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(now_() - *last_contact_);
}

std::chrono::steady_clock::time_point ShadowDefs::now() const {
    std::scoped_lock lock(contact_mutex_);
    return now_();
}

void ShadowDefs::record_contact(std::chrono::steady_clock::time_point when) {
    std::scoped_lock lock(contact_mutex_);
    last_contact_ = when;
}

void ShadowDefs::fetch_copy() {
    std::chrono::steady_clock::time_point contact;
    if (get()) {
        // A copy is published, as after a restart: the source may refresh it in place, so the fetch waits for the
        // read views in progress and holds back new ones, as an update does
        auto lock = admit_writer();
        defs_     = source_->fetch();
        contact   = now();
    }
    else {
        // Nothing is published, so readers are not held back while the server is slow or absent
        auto fetched = source_->fetch();
        // The contact happened when the fetch returned, however long the publication waits for read views
        contact   = now();
        auto lock = admit_writer();
        defs_     = std::move(fetched);
    }
    record_contact(contact);
}

void ShadowDefs::refresh(const std::function<void()>& then) {
    if (!running_) {
        update_copy();
        then();
        return;
    }

    std::unique_lock lock(wakeup_mutex_);
    const auto request = ++requested_;
    wakeup_.notify_all();
    wakeup_.wait(lock, [this, request] { return served_ >= request || stopping_; });
    lock.unlock();

    then();
}

void ShadowDefs::poll_once() {
    const bool news = source_->has_news(get());
    record_contact(now());
    if (news) {
        update_copy();
    }
}

void ShadowDefs::update_copy() {
    std::shared_ptr<Defs> updated;
    std::chrono::steady_clock::time_point contact;
    {
        // The update changes the copy in place, so it waits for the read views in progress and holds back new ones
        auto lock = admit_writer();
        updated   = source_->update(defs_);
        defs_     = updated;
        contact   = now();
    }
    record_contact(contact);
    if (updated_ && updated) {
        updated_(*updated);
    }
}

void ShadowDefs::loop() {
    std::chrono::milliseconds wait{0};
    bool first = true;
    while (true) {
        try {
            if (!get()) {
                // Started without the server: the first fetch is retried until it succeeds
                fetch_copy();
            }
            if (first) {
                // The first wait is given by the policy once the definition has been fetched
                wait  = next_interval_();
                first = false;
            }

            std::uint64_t target = 0;
            {
                std::unique_lock lock(wakeup_mutex_);
                wakeup_.wait_for(lock, wait, [this] { return requested_ > served_ || stopping_; });
                if (stopping_) {
                    return;
                }
                target = requested_;
            }

            if (target > served_requests()) {
                // Every request made before this update started is served by it; a request made while it runs
                // waits for the next one
                update_copy();
                {
                    std::scoped_lock lock(wakeup_mutex_);
                    served_ = target;
                }
                wakeup_.notify_all();
            }
            else {
                poll_once();
            }

            wait = next_interval_();
        }
        catch (const std::exception& e) {
            if (report_) {
                report_(e.what());
            }
            std::unique_lock lock(wakeup_mutex_);
            wakeup_.wait_for(lock, retry_delay_, [this] { return stopping_.load(); });
            if (stopping_) {
                return;
            }
        }
    }
}

std::uint64_t ShadowDefs::requests_made() {
    std::scoped_lock lock(wakeup_mutex_);
    return requested_;
}

std::uint64_t ShadowDefs::served_requests() {
    std::scoped_lock lock(wakeup_mutex_);
    return served_;
}

} // namespace ecf
