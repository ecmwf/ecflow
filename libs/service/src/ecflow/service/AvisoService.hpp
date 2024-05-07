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

#include "aviso/Aviso.hpp"
#include "aviso/Log.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/service/Registry.hpp"
#include "ecflow/service/executor/PeriodicTaskExecutor.hpp"

namespace ecf::service {

using AvisoController =
    Controller<aviso::ListenRequest, NotificationPackage<aviso::ConfiguredListener, aviso::Notification>>;

class AvisoService {
public:
    using address_t     = aviso::etcd::Address;
    using schema_path_t = std::string;
    using key_prefix_t  = std::string;
    using listener_t    = aviso::ConfiguredListener;
    using revision_t    = int64_t;

    struct Entry
    {
        explicit Entry(listener_t listener) : listener_{std::move(listener)} {}
        explicit Entry(listener_t listener, revision_t revision) : listener_{std::move(listener)} {}

        const aviso::etcd::Address address() const { return listener_.address(); }
        std::string prefix() const { return listener_.prefix(); }
        std::string_view path() const { return listener_.path(); }
        const listener_t& listener() const { return listener_; }
        listener_t& listener() { return listener_; }

    private:
        listener_t listener_;
    };

    using storage_t            = std::vector<Entry>;
    using notify_callback_t    = std::function<void(const aviso::ConfiguredListener&, const aviso::Notification&)>;
    using subscribe_callback_t = std::function<std::vector<aviso::ListenRequest>()>;

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

    void register_listener(const aviso::ListenRequest& request);
    void register_listener(const listener_t& listener);
    void unregister_listener(const std::string& unlisten_path);

private:
    executor::PeriodicTaskExecutor<std::function<void(const std::chrono::system_clock::time_point& now)>> executor_;
    storage_t listeners_;

    notify_callback_t notify_;
    subscribe_callback_t subscribe_;
};

class AvisoRunner : private Runner<AvisoController, AvisoService> {
public:
    using base_t = Runner<AvisoController, AvisoService>;

public:
    AvisoRunner(AvisoController& controller)
        : base_t{controller,
                 [this](const aviso::ConfiguredListener& listener, const aviso::Notification& notification) {
                     AvisoRunner::notify(this->controller_,
                                         AvisoController::notification_t{listener.path(), listener, notification});
                     // The following is a hack to force the server to increment the job generation count
                     TheOneServer::server().increment_job_generation_count();
                 },
                 [this]() { return AvisoRunner::subscribe(this->controller_); }} {};

    using base_t::notify;
    using base_t::start;
    using base_t::stop;
    using base_t::subscribe;
    using base_t::terminate;
};

} // namespace ecf::service

#endif /* ecflow_service_Registry_HPP */
