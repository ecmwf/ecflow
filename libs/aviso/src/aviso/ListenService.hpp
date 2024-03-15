/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_ListenService_HPP
#define aviso_ListenService_HPP

#include <functional>

#include "aviso/ConfiguredListener.hpp"
#include "aviso/ListenerSchema.hpp"
#include "aviso/PeriodicExecutor.hpp"

namespace aviso {

class ListenService {
public:
    using address_t    = aviso::etcd::Address;
    using key_prefix_t = std::string;
    using listener_t   = aviso::ConfiguredListener;
    using revision_t   = int64_t;

    struct Entry
    {
        explicit Entry(listener_t listener) : listener_{std::move(listener)}, latest_revision_{0} {}
        explicit Entry(listener_t listener, revision_t revision)
            : listener_{std::move(listener)},
              latest_revision_{revision} {}

        const etcd::Address address() const { return listener_.address(); }
        std::string_view key_prefix() const { return listener_.resolved_base(); }
        std::string_view path() const { return listener_.path(); }
        listener_t listener() { return listener_; }

        revision_t get_latest_revision() const { return latest_revision_; }
        void update_latest_revision(revision_t revision) {
            latest_revision_ = std::max(static_cast<int64_t>(revision), latest_revision_);
        }

    private:
        listener_t listener_;
        revision_t latest_revision_;
    };

    using storage_t            = std::vector<Entry>;
    using notify_callback_t    = std::function<void(const aviso::ConfiguredListener&, const aviso::Notification&)>;
    using subscribe_callback_t = std::function<std::vector<ListenRequest>()>;

    using schema_t = aviso::ListenerSchema;

    ListenService(schema_t schema, notify_callback_t notify, subscribe_callback_t subscribe)
        : schema_{schema},
          executor_{},
          listeners_{},
          notify_{notify},
          subscribe_{subscribe} {};
    ListenService()                     = delete;
    ListenService(const ListenService&) = delete;
    ~ListenService() { stop(); }

    ListenService& operator=(const ListenService&) = delete;

    void start() { start(std::chrono::seconds{1}); }
    void start(std::chrono::seconds expiry) { executor_.start(*this, expiry); }

    void stop() { executor_.stop(); }
    void terminate() { executor_.stop(); }

    void operator()();

    void register_listener(const ListenRequest& request);
    void register_listener(const listener_t& listener);
    void unregister_listener(const std::string& unlisten_path);

private:
    aviso::ListenerSchema schema_;
    aviso::PeriodicExecutor executor_;
    storage_t listeners_;

    notify_callback_t notify_;
    subscribe_callback_t subscribe_;
};

} // namespace aviso

#endif /* aviso_ListenService_HPP */
