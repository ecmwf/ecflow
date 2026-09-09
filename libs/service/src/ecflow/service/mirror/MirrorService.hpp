/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_service_mirror_MirrorService_HPP
#define ecflow_service_mirror_MirrorService_HPP

#include <optional>
#include <string>

#include "ecflow/service/Controller.hpp"
#include "ecflow/service/executor/PeriodicTaskExecutor.hpp"
#include "ecflow/service/mirror/Mirror.hpp"
#include "ecflow/service/mirror/MirrorClient.hpp"

namespace ecf::service::mirror {

/**
 *
 * The MirrorService is a service (i.e. an object that executes a background worker thread)
 * that listens for state changes in remote ecFlow servers.
 *
 */
class MirrorService {
public:
    using notification_t  = MirrorResponse;
    using subscription_t  = MirrorRequest;
    using subscriptions_t = std::vector<subscription_t>;

    struct Entry
    {
        MirrorRequest mirror_request_;
        std::string remote_username_;
        std::string remote_password_;
    };

    using storage_t            = std::vector<Entry>;
    using notify_callback_t    = std::function<void(const notification_t& notification)>;
    using subscribe_callback_t = std::function<subscriptions_t()>;

    MirrorService(notify_callback_t notify, subscribe_callback_t subscribe)
        : executor_{[this](const std::chrono::system_clock::time_point& now) { this->operator()(now); }},
          listeners_{},
          notify_{notify},
          subscribe_{subscribe},
          mirror_{} {};

    MirrorService() = delete;

    MirrorService(const MirrorService&)            = delete;
    MirrorService& operator=(const MirrorService&) = delete;
    MirrorService(MirrorService&&)                 = delete;
    MirrorService& operator=(MirrorService&&)      = delete;

    ~MirrorService() { stop(); }

    void start();
    void stop() { executor_.stop(); }
    void terminate() { executor_.stop(); }

    void operator()(const std::chrono::system_clock::time_point& now);

private:
    void register_listener(const MirrorRequest& request);

    executor::PeriodicTaskExecutor<std::function<void(const std::chrono::system_clock::time_point& now)>> executor_;
    storage_t listeners_;

    notify_callback_t notify_;
    subscribe_callback_t subscribe_;

    MirrorClient mirror_;
};

/**
 *
 * The MirrorController is a controller for the MirrorService,
 * and essentially defines the handlers for subscription and notification.
 *
 */
class MirrorController : public Controller<MirrorService> {
public:
    using base_t = Controller<MirrorService>;

    MirrorController();

    using base_t::notify;
    using base_t::start;
    using base_t::stop;
    using base_t::subscribe;
    using base_t::terminate;
};

} // namespace ecf::service::mirror

#endif /* ecflow_service_mirror_MirrorService_HPP */
