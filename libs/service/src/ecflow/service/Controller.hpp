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
        SLOG(D, "Controller: subscribe " << s);

        {
            std::scoped_lock lock(subscribe_);
            subscriptions_.push_back(s);
        }
    }

    notifications_t get_notifications(const std::string& name) {
        SLOG(D, "Controller::get_notifications for " << name);

        notifications_t found;

        {
            std::scoped_lock lock(notify_);
            found = notifications_;
            notifications_.clear();
        }

        return found;
    }

    // Background Thread-facing API

    subscriptions_t get_subscriptions() {
        SLOG(D, "Controller: collect subscriptions");

        subscriptions_t found;
        {
            std::scoped_lock lock(subscribe_);
            found = subscriptions_;
            subscriptions_.clear();
        }

        return found;
    }

    void notify(const notification_t& notification) {
        SLOG(D, "Controller: notify " << notification);

        {
            std::scoped_lock lock(notify_);
            notifications_.push_back(notification);
        }
    }

    void start() { running_.start(); };
    void stop() { running_.stop(); };
    void terminate() { running_.terminate(); };

private:
    std::mutex subscribe_;
    std::mutex notify_;

    std::vector<subscription_t> subscriptions_;
    std::vector<notification_t> notifications_;

    service_t running_;
};

} // namespace ecf::service

#endif /* ecflow_service_Controller_HPP */
