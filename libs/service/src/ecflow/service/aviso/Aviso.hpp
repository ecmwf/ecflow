/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_service_aviso_Aviso_HPP
#define ecflow_service_aviso_Aviso_HPP

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ecflow/service/aviso/etcd/Address.hpp"

namespace ecf::service::aviso {

/**
 *
 * An AvisoRequest is a request to start or stop listening for notifications based a given configuration.
 *
 * This class holds information used to fully configure an Aviso Listener, including:
 *  - the listener configuration,
 *  - the address of the ETCD server,
 *  - the schema used to specify the notifications
 * */
class AvisoRequest {
public:
    static AvisoRequest make_listen_start(std::string_view path,
                                          std::string_view listener_cfg,
                                          std::string_view address,
                                          std::string_view schema,
                                          uint32_t polling,
                                          uint64_t revision) {
        return AvisoRequest{true, path, listener_cfg, address, schema, polling, revision};
    }

    static AvisoRequest make_listen_finish(std::string_view path) { return AvisoRequest{false, path}; }

    bool is_start() const { return start_; }

    const std::string& path() const { return path_; }
    const std::string& listener_cfg() const { return listener_cfg_; }
    const std::string& address() const { return address_; }
    const std::string& schema() const { return schema_; }
    uint32_t polling() const { return polling_; }
    uint64_t revision() const { return revision_; }

private:
    explicit AvisoRequest(bool start, std::string_view path)
        : start_{start},
          path_{path},
          listener_cfg_{},
          address_{},
          schema_{},
          revision_{0} {}
    explicit AvisoRequest(bool start,
                          std::string_view path,
                          std::string_view listener_cfg,
                          std::string_view address,
                          std::string_view schema,
                          uint32_t polling,
                          uint64_t revision)
        : start_{start},
          path_{path},
          listener_cfg_{listener_cfg},
          address_{address},
          schema_{schema},
          polling_{polling},
          revision_{revision} {}

    bool start_;
    std::string path_;
    std::string listener_cfg_;
    std::string address_;
    std::string schema_;
    uint32_t polling_;
    uint64_t revision_;
};

std::ostream& operator<<(std::ostream& os, const AvisoRequest& request);

/**
 *
 * A AvisoNotification represents a notification of a change in a key-value pair in ETCD.
 *
 * */
class AvisoNotification {
public:
    AvisoNotification() = default;
    AvisoNotification(std::string_view key, std::string_view value, uint64_t revision)
        : key_{key},
          value_{value},
          revision_{revision} {}

    std::string_view key() const { return key_; }
    std::string_view value() const { return value_; }
    uint64_t revision() const { return revision_; }

    void add_parameter(const std::string& parameter, const std::string& value) {
        parameters_.emplace_back(parameter, value);
    }
    std::vector<std::pair<std::string, std::string>> parameters() const { return parameters_; }

private:
    std::string key_{};
    std::string value_{};
    uint64_t revision_{0};
    std::vector<std::pair<std::string, std::string>> parameters_{};
};

std::ostream& operator<<(std::ostream& os, const AvisoNotification& notification);

/**
 * A Listener represents an Aviso Listener, loaded from the Schema with placeholders as part of the `base` and `stem`.
 */
class Listener {
public:
    Listener() = default;
    Listener(std::string_view name, std::string_view base, std::string_view stem)
        : name_{name},
          base_{base},
          stem_{stem} {}

    std::string_view name() const { return name_; }
    std::string_view base() const { return base_; }
    std::string_view stem() const { return stem_; }

    std::string full() const { return base_ + '/' + stem_; }

private:
    std::string name_{};
    std::string base_{};
    std::string stem_{};
};

/**
 * A ConfiguredListener is an Aviso Listener with complete configuration, including ETCD server connection data and
 * all placeholders instantiated.
 *
 * A ConfiguredListener object is responsible for connecting to the ETCS server, collect all relevant notifications
 * and check if any notification matches the listener's configuration.
 */
class ConfiguredListener : private Listener {
public:
    /**
     * Create a ConfiguredListener from an AvisoRequest.
     *
     * @param request The AvisoRequest to create the ConfiguredListener from.
     * @return The ConfiguredListener.
     */
    static ConfiguredListener make_configured_listener(const AvisoRequest& request);

public:
    ConfiguredListener(ecf::service::aviso::etcd::Address address,
                       std::string_view path,
                       std::string_view name,
                       std::string_view base,
                       std::string_view stem,
                       uint32_t polling,
                       uint64_t revision);

    void with_parameter(const std::string& parameter, const std::string& value);
    void with_parameter(const std::string& parameter, int64_t value);
    void with_parameter(const std::string& parameter, const std::vector<std::string>& value);

    const std::string& path() const { return path_; }
    const ecf::service::aviso::etcd::Address& address() const { return address_; }

    uint32_t polling() const { return polling_; }
    uint64_t revision() const { return revision_; }
    void update_revision(uint64_t revision) { revision_ = std::max(revision, revision_); }

    using Listener::base;
    using Listener::name;
    using Listener::stem;

    using Listener::full;

    std::string_view resolved_base() const { return resolved_base_; }

    std::string prefix() const { return resolved_base_ + '/'; }

    std::optional<AvisoNotification> accepts(const std::string& key, const std::string& value, uint64_t revision) const;

private:
    std::string path_;
    ecf::service::aviso::etcd::Address address_;
    std::string resolved_base_;
    uint32_t polling_;
    uint64_t revision_;

    using parameters_t = std::variant<std::string, std::int64_t, std::vector<std::string>>;
    std::unordered_map<std::string, parameters_t> parameters_ = {};
};

/**
 * A ListenerSchema is the specification of available Listeners.
 *
 * The specification of each Listener, describes the name, the base and the stem (with eventual placeholders).
 * The specification is loaded from a schema file.
 */
class ListenerSchema {
public:
    std::optional<Listener> get_listener(const std::string& name) const;

    static ListenerSchema load(const std::string& schema_path);
    static ListenerSchema load(std::istream& schema_stream);

private:
    void add_listener(const Listener& listener);

    std::unordered_map<std::string, Listener> listeners_{};
};

} // namespace ecf::service::aviso

#endif /* ecflow_service_aviso_Aviso_HPP */
