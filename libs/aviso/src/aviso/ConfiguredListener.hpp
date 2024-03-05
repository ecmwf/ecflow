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
    static ListenRequest
    make_listen_start(std::string_view path, std::string_view address, std::string_view listener_cfg) {
        return ListenRequest{true, path, address, listener_cfg};
    }

    static ListenRequest make_listen_finish(std::string_view path) { return ListenRequest{false, path}; }

    bool is_start() const { return start_; }

    const std::string& path() const { return path_; }
    const std::string& address() const { return address_; }
    const std::string& listener_cfg() const { return listener_cfg_; }

private:
    explicit ListenRequest(bool start, std::string_view path)
        : start_{start},
          path_{path},
          address_{},
          listener_cfg_{} {}
    explicit ListenRequest(bool start, std::string_view path, std::string_view address, std::string_view listener_cfg)
        : start_{start},
          path_{path},
          address_{address},
          listener_cfg_{listener_cfg} {}

    bool start_;
    std::string path_;
    std::string address_;
    std::string listener_cfg_;
};

struct UnlistenRequest
{
    std::string path;
    std::string address;
};

class Notification {
public:
    Notification() = default;
    Notification(std::string_view key, std::string_view value) : key_{key}, value_{value} {}

    std::string_view key() const { return key_; }
    std::string_view value() const { return value_; }

    void add_parameter(const std::string& parameter, const std::string& value) {
        parameters_.emplace_back(parameter, value);
    }
    std::vector<std::pair<std::string, std::string>> parameters() const { return parameters_; }

private:
    std::string key_{};
    std::string value_{};
    std::vector<std::pair<std::string, std::string>> parameters_{};
};

std::ostream& operator<<(std::ostream& os, const Notification& notification);

class ConfiguredListener : private Listener {
public:
    ConfiguredListener(etcd::Address address,
                       std::string_view path,
                       std::string_view name,
                       std::string_view base,
                       std::string_view stem)
        : Listener(name, base, stem),
          path_{path},
          address_(std::move(address)),
          resolved_base_(base) {}

    void with_parameter(const std::string& parameter, const std::string& value);
    void with_parameter(const std::string& parameter, int64_t value);
    void with_parameter(const std::string& parameter, const std::vector<std::string>& value);

    const std::string& path() const { return path_; }
    const etcd::Address& address() const { return address_; }

    using Listener::base;
    using Listener::name;
    using Listener::stem;

    using Listener::full;

    std::string_view resolved_base() const { return resolved_base_; }

    std::string prefix() const { return resolved_base_ + '/'; }

    std::optional<Notification> accepts(const std::string& key, const std::string& value) const;

private:
    std::string path_;
    etcd::Address address_;
    std::string resolved_base_;

    using parameters_t = std::variant<std::string, std::int64_t, std::vector<std::string>>;
    std::unordered_map<std::string, parameters_t> parameters_ = {};
};

ConfiguredListener create_configured_listener(const ListenRequest& request, const ListenerSchema& schema);

} // namespace aviso

#endif /* aviso_ConfiguredListener_HPP */
