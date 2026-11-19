// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include "ecflow/client/ShadowDefs.hpp"

#include <exception>
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
      updated_(std::move(updated)) {
}

ShadowDefs::~ShadowDefs() {
    stop();
}

void ShadowDefs::start() {
    auto fetched = source_->fetch();
    {
        std::scoped_lock lock(defs_mutex_);
        defs_ = std::move(fetched);
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

std::shared_ptr<Defs> ShadowDefs::get() const {
    std::scoped_lock lock(defs_mutex_);
    return defs_;
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
    if (source_->has_news(get())) {
        update_copy();
    }
}

void ShadowDefs::update_copy() {
    std::shared_ptr<Defs> updated;
    {
        // The update may change the copy in place, so the copy is not handed out while the update runs
        std::scoped_lock lock(defs_mutex_);
        updated = source_->update(defs_);
        defs_   = updated;
    }
    if (updated_ && updated) {
        updated_(*updated);
    }
}

void ShadowDefs::loop() {
    std::chrono::milliseconds wait{0};
    bool first = true;
    while (true) {
        try {
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
