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

namespace ecf::service {

struct NotificationRequest
{
    std::string path;
    aviso::ConfiguredListener listener;
    aviso::Notification notification;
};

class AvisoController {
public:
    using subscription_t  = aviso::ListenRequest;
    using subscriptions_t = std::vector<subscription_t>;
    using notification_t  = NotificationRequest;
    using notifications_t = std::vector<notification_t>;

    // Attribute-facing API

    void subscribe(const subscription_t& subscription) {
        std::scoped_lock lock(subscribe_);
        subscriptions_.push_back(subscription);
    }

    notifications_t poll_notifications(const std::string& name) {
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
        std::scoped_lock lock(subscribe_);
        auto new_subscriptions = subscriptions_;
        subscriptions_.clear();

        return new_subscriptions;
    }

    void notify(NotificationRequest notification) {
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
    AvisoRunner() : running_{load_listener_schema_default(), AvisoRunner::notify, AvisoRunner::subscribe} {};

    void start() { running_.start(std::chrono::seconds{30}); };
    void stop() { running_.stop(); };
    void terminate() { running_.terminate(); };

    static aviso::ListenerSchema load_listener_schema_default();

    static void notify(const aviso::ConfiguredListener& listener, const aviso::Notification& notification);
    static std::vector<aviso::ListenRequest> subscribe();

private:
    aviso::ListenService running_;
};

class BaseRegistry {
public:
    using name_t = std::string;

    BaseRegistry() : controller_{}, runner_{} {};
    BaseRegistry(const BaseRegistry&)            = delete;
    BaseRegistry(BaseRegistry&&)                 = delete;
    BaseRegistry& operator=(const BaseRegistry&) = delete;
    BaseRegistry& operator=(BaseRegistry&&)      = delete;
    ~BaseRegistry()                              = default;

    template <typename Service>
    Service& register_service(std::string_view name);

    template <>
    AvisoController& register_service<AvisoController>(std::string_view name [[maybe_unused]]) {
        // Nothing to register, actually...
        return controller_;
    }

    template <>
    AvisoRunner& register_service<AvisoRunner>(std::string_view name [[maybe_unused]]) {
        // Nothing to register, actually...
        return runner_;
    }

    template <typename Service>
    Service& get_service(std::string_view name);

    template <>
    AvisoController& get_service<AvisoController>(std::string_view name [[maybe_unused]]) {
        return controller_;
    }

    template <>
    AvisoRunner& get_service<AvisoRunner>(std::string_view name [[maybe_unused]]) {
        return runner_;
    }

private:
    AvisoController controller_;
    AvisoRunner runner_;
};

using Registry = BaseRegistry;

///
/// \brief Global manages a \a service object
///
/// \tparam T the type of the \a service object
///
template <typename T>
struct Global
{
    static T& instance() {
        static T instance;
        return instance;
    }

private:
    Global() = default;
};

using GlobalRegistry = Global<Registry>;

inline aviso::ListenerSchema AvisoRunner::load_listener_schema_default() {
    std::string listener_schema_location =
        "/Users/mamb/w/sandbox.aviso/client/service_configuration/event_listener_schema.json";
    auto listener_schema = aviso::ListenerSchema::load(listener_schema_location);
    return listener_schema;
}

inline void AvisoRunner::notify(const aviso::ConfiguredListener& listener, const aviso::Notification& notification) {
    GlobalRegistry::instance()
        .get_service<AvisoController>("aviso_controller")
        .notify(NotificationRequest{listener.path(), listener, notification});
}

inline std::vector<aviso::ListenRequest> AvisoRunner::subscribe() {
    return GlobalRegistry::instance().get_service<AvisoController>("aviso_controller").get_subscriptions();
}

} // namespace ecf::service

#endif /* ecflow_services_Registry_HPP */