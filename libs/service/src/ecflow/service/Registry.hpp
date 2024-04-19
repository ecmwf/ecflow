/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_services_Registry_HPP
#define ecflow_services_Registry_HPP

#include <mutex>
#include <unordered_map>
#include <variant>
#include <vector>

#include "aviso/Aviso.hpp"
#include "aviso/Log.hpp"
#include "ecflow/base/AbstractServer.hpp"

namespace ecf::service {

class TheOneServer {
public:
    static void set_server(AbstractServer& server) { TheOneServer::instance().server_ = &server; }
    static AbstractServer& server() { return *TheOneServer::instance().server_; }

private:
    TheOneServer() = default;

    static TheOneServer& instance() {
        static TheOneServer the_one_server;
        return the_one_server;
    }

    AbstractServer* server_ = nullptr;
};

struct NotificationPackage
{
    std::string path;
    aviso::ConfiguredListener listener;
    aviso::Notification notification;
};

class AvisoController {
public:
    using subscription_t  = aviso::ListenRequest;
    using subscriptions_t = std::vector<subscription_t>;
    using notification_t  = NotificationPackage;
    using notifications_t = std::vector<notification_t>;

    // Attribute-facing API

    void subscribe(const subscription_t& s) {
        ALOG(D,
             "AvisoController::subscribe: " << s.path() << " " << s.address() << " " << s.listener_cfg() << " "
                                            << s.revision());
        std::scoped_lock lock(subscribe_);
        subscriptions_.push_back(s);
    }

    void unsubscribe(const subscription_t& s) {
        ALOG(D,
             "AvisoController::unsubscribe: " << s.path() << " " << s.address() << " " << s.listener_cfg() << " "
                                              << s.revision());
        std::scoped_lock lock(subscribe_);
        subscriptions_.push_back(s);
    }

    notifications_t poll_notifications(const std::string& name) {
        ALOG(D, "AvisoController::poll_notifications: " << name);

        std::scoped_lock lock(notify_);

        if (auto found = notifications_.find(name); found != notifications_.end()) {
            notifications_t new_notifications = {found->second};
            notifications_.erase(found);
            return new_notifications;
        }

        // No notifications found
        return {};
    }

    // Aviso-facing API

    subscriptions_t get_subscriptions() {
        ALOG(D, "AvisoController::get_subscriptions");

        std::scoped_lock lock(subscribe_);
        auto new_subscriptions = subscriptions_;
        subscriptions_.clear();

        return new_subscriptions;
    }

    void notify(const notification_t& notification) {
        ALOG(D, "AvisoController::notify: " << notification.path << " " << notification.notification);

        std::scoped_lock lock(notify_);

        const auto& key = notification.path;
        if (auto found = notifications_.find(key); found != notifications_.end()) {
            found->second.push_back(notification);
        }
        else {
            notifications_[key] = {notification};
        }
    }

private:
    std::mutex subscribe_;
    std::mutex notify_;

    std::vector<subscription_t> subscriptions_;
    std::unordered_map<std::string, std::vector<notification_t>> notifications_;
};

class AvisoRunner {
public:
    AvisoRunner(AvisoController& controller)
        : running_{[this](const aviso::ConfiguredListener& listener, const aviso::Notification& notification) {
                       AvisoRunner::notify(this->controller_, listener, notification);
                       // The following is a hack to force the server to increment the job generation count
                       TheOneServer::server().increment_job_generation_count();
                   },
                   [this]() { return AvisoRunner::subscribe(this->controller_); }},
          controller_{controller} {};

    void start() { running_.start(); };
    void stop() { running_.stop(); };
    void terminate() { running_.terminate(); };

    static void notify(AvisoController& controller,
                       const aviso::ConfiguredListener& listener,
                       const aviso::Notification& notification);
    static std::vector<aviso::ListenRequest> subscribe(AvisoController& controller);

private:
    aviso::ListenService running_;
    AvisoController& controller_;
};

inline void AvisoRunner::notify(AvisoController& controller,
                                const aviso::ConfiguredListener& listener,
                                const aviso::Notification& notification) {
    controller.notify(NotificationPackage{listener.path(), listener, notification});
}

inline std::vector<aviso::ListenRequest> AvisoRunner::subscribe(AvisoController& controller) {
    return controller.get_subscriptions();
}

} // namespace ecf::service

#endif /* ecflow_services_Registry_HPP */
