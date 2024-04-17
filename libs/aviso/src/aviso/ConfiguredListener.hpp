/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef aviso_ConfiguredListener_HPP
#define aviso_ConfiguredListener_HPP

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "aviso/Listener.hpp"
#include "aviso/ListenerSchema.hpp"
#include "aviso/etcd/Address.hpp"

namespace aviso {

class ListenRequest {
public:
    static ListenRequest make_listen_start(std::string_view path,
                                           std::string_view listener_cfg,
                                           std::string_view address,
                                           std::string_view schema,
                                           uint32_t polling,
                                           uint64_t revision) {
        return ListenRequest{true, path, listener_cfg, address, schema, polling, revision};
    }

    static ListenRequest make_listen_finish(std::string_view path) { return ListenRequest{false, path}; }

    bool is_start() const { return start_; }

    const std::string& path() const { return path_; }
    const std::string& listener_cfg() const { return listener_cfg_; }
    const std::string& address() const { return address_; }
    const std::string& schema() const { return schema_; }
    uint32_t polling() const { return polling_; }
    uint64_t revision() const { return revision_; }

private:
    explicit ListenRequest(bool start, std::string_view path)
        : start_{start},
          path_{path},
          listener_cfg_{},
          address_{},
          schema_{},
          revision_{0} {}
    explicit ListenRequest(bool start,
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

struct UnlistenRequest
{
    std::string path;
    std::string address;
};

class Notification {
public:
    Notification() = default;
    Notification(std::string_view key, std::string_view value, uint64_t revision)
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

std::ostream& operator<<(std::ostream& os, const Notification& notification);

class ConfiguredListener : private Listener {
public:
    ConfiguredListener(etcd::Address address,
                       std::string_view path,
                       std::string_view name,
                       std::string_view base,
                       std::string_view stem,
                       uint32_t polling,
                       uint64_t revision)
        : Listener(name, base, stem),
          path_{path},
          address_(std::move(address)),
          resolved_base_(base),
          polling_{polling},
          revision_{revision} {}

    void with_parameter(const std::string& parameter, const std::string& value);
    void with_parameter(const std::string& parameter, int64_t value);
    void with_parameter(const std::string& parameter, const std::vector<std::string>& value);

    const std::string& path() const { return path_; }
    const etcd::Address& address() const { return address_; }

    uint32_t polling() const { return polling_; }
    uint64_t revision() const { return revision_; }
    void update_revision(uint64_t revision) { revision_ = std::max(revision, revision_); }

    using Listener::base;
    using Listener::name;
    using Listener::stem;

    using Listener::full;

    std::string_view resolved_base() const { return resolved_base_; }

    std::string prefix() const { return resolved_base_ + '/'; }

    std::optional<Notification> accepts(const std::string& key, const std::string& value, uint64_t revision) const;

private:
    std::string path_;
    etcd::Address address_;
    std::string resolved_base_;
    uint32_t polling_;
    uint64_t revision_;

    using parameters_t = std::variant<std::string, std::int64_t, std::vector<std::string>>;
    std::unordered_map<std::string, parameters_t> parameters_ = {};
};

ConfiguredListener create_configured_listener(const ListenRequest& request);

} // namespace aviso

#endif /* aviso_ConfiguredListener_HPP */
