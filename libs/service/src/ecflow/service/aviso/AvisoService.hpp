/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_AvisoService_HPP
#define ecflow_service_AvisoService_HPP

#include <mutex>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/service/Controller.hpp"
#include "ecflow/service/Log.hpp"
#include "ecflow/service/aviso/Aviso.hpp"
#include "ecflow/service/executor/PeriodicTaskExecutor.hpp"

namespace ecf::service::aviso {

template <typename Configuration, typename Notification>
struct NotificationPackage
{
    std::string path;
    Configuration configuration;
    Notification notification;
};

template <typename Configuration, typename Notification>
inline std::ostream& operator<<(std::ostream& os, const NotificationPackage<Configuration, Notification>& p) {
    return os << "NotificationPackage{" << p.path << ", " << p.configuration << ", " << p.notification << "}";
}

class AvisoService {
public:
    using address_t     = aviso::etcd::Address;
    using schema_path_t = std::string;
    using key_prefix_t  = std::string;
    using listener_t    = ConfiguredListener;
    using revision_t    = int64_t;

    using notification_t  = NotificationPackage<ConfiguredListener, AvisoNotification>;
    using subscription_t  = AvisoRequest;
    using subscriptions_t = std::vector<subscription_t>;

    struct Entry
    {
        explicit Entry(listener_t listener) : listener_{std::move(listener)} {}
        explicit Entry(listener_t listener, revision_t revision) : listener_{std::move(listener)} {}

        const aviso::etcd::Address address() const { return listener_.address(); }
        std::string prefix() const { return listener_.prefix(); }
        std::string_view path() const { return listener_.path(); }
        const listener_t& listener() const { return listener_; }
        listener_t& listener() { return listener_; }

        std::string auth_token;

    private:
        listener_t listener_;
    };

    using storage_t            = std::vector<Entry>;
    using notify_callback_t    = std::function<void(const AvisoService::notification_t&)>;
    using subscribe_callback_t = std::function<subscriptions_t()>;

    AvisoService(notify_callback_t notify, subscribe_callback_t subscribe)
        : executor_{[this](const std::chrono::system_clock::time_point& now) { this->operator()(now); }},
          listeners_{},
          notify_{notify},
          subscribe_{subscribe} {};
    AvisoService()                    = delete;
    AvisoService(const AvisoService&) = delete;
    ~AvisoService() { stop(); }

    AvisoService& operator=(const AvisoService&) = delete;

    void start();
    void stop() { executor_.stop(); }
    void terminate() { executor_.stop(); }

    void operator()(const std::chrono::system_clock::time_point& now);

private:
    void register_listener(const AvisoRequest& request);
    void unregister_listener(const std::string& unlisten_path);

    executor::PeriodicTaskExecutor<std::function<void(const std::chrono::system_clock::time_point& now)>> executor_;
    storage_t listeners_;

    notify_callback_t notify_;
    subscribe_callback_t subscribe_;
};

class AvisoController : private Controller<AvisoService> {
public:
    using base_t = Controller<AvisoService>;

public:
    AvisoController();

    using base_t::notify;
    using base_t::poll_notifications;
    using base_t::start;
    using base_t::stop;
    using base_t::subscribe;
    using base_t::terminate;
    using base_t::unsubscribe;
};

} // namespace ecf::service::aviso

#endif /* ecflow_service_Registry_HPP */
