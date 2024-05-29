/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_Controller_HPP
#define ecflow_service_Controller_HPP

#include <mutex>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ecflow/service/Log.hpp"

namespace ecf::service {

template <typename Service>
class Controller {
public:
    using service_t       = Service;
    using subscription_t  = typename service_t::subscription_t;
    using subscriptions_t = std::vector<subscription_t>;
    using notification_t  = typename Service::notification_t;
    using notifications_t = std::vector<notification_t>;

    template <typename... Args>
    explicit Controller(Args&&... args) : running_{std::forward<Args>(args)...} {}

    // Attribute-facing API

    void subscribe(const subscription_t& s) {
        ALOG(D, "Controller: subscribe " << s);
        std::scoped_lock lock(subscribe_);
        subscriptions_.push_back(s);
    }

    notifications_t poll_notifications(const std::string& name) {
        ALOG(D, "Controller: poll_notifications for " << name);

        std::scoped_lock lock(notify_);

        if (auto found = notifications_.find(name); found != notifications_.end()) {
            notifications_t new_notifications = {found->second};
            notifications_.erase(found);
            return new_notifications;
        }

        // No notifications found
        return {};
    }

    // Background Thread-facing API

    subscriptions_t get_subscriptions() {
        ALOG(D, "Controller: collect subscriptions");

        std::scoped_lock lock(subscribe_);
        auto new_subscriptions = subscriptions_;
        subscriptions_.clear();

        return new_subscriptions;
    }

    void notify(const notification_t& notification) {
        ALOG(D, "Controller: notify " << notification);

        std::scoped_lock lock(notify_);

        if (const auto& key = service_t::key(notification); key.has_value()) {
            if (auto found = notifications_.find(key.value()); found != notifications_.end()) {
                found->second.push_back(notification);
            }
            else {
                notifications_[key.value()] = {notification};
            }
        }
    }

    void start() { running_.start(); };
    void stop() { running_.stop(); };
    void terminate() { running_.terminate(); };

private:
    std::mutex subscribe_;
    std::mutex notify_;

    std::vector<subscription_t> subscriptions_;
    std::unordered_map<std::string, std::vector<notification_t>> notifications_;

    service_t running_;
};

} // namespace ecf::service

#endif /* ecflow_service_Controller_HPP */
